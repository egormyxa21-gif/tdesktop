/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ghostgram/ghostgram_history.h"
#include "ghostgram/ghostgram_settings.h"

#include "base/flat_map.h"
#include "base/flat_set.h"
#include "base/unixtime.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <atomic>
#include <mutex>
#include <deque>

namespace GhostGram::History {
namespace {

constexpr auto kMaxStoredItems = 50000;

std::atomic<int> g_bulkDestroyDepth = 0;

[[nodiscard]] int64 ToInt64(const QJsonValue &value) {
	if (value.isDouble()) {
		return static_cast<int64>(value.toDouble());
	}
	if (value.isString()) {
		auto ok = false;
		const auto result = value.toString().toLongLong(&ok);
		if (ok) {
			return result;
		}
	}
	return 0;
}

struct State {
	std::mutex mutex;
	std::deque<SavedMessage> deletedMessages;
	std::deque<SavedMessage> editedMessages;
	base::flat_set<FullMsgId> deletedIds;
	std::deque<BookmarkItem> bookmarks;
	base::flat_map<PeerId, QString> chatNotes;
	bool loaded = false;
};

State &GetState() {
	static State state;
	return state;
}

bool PruneExpired(State &state);

QString StoragePath() {
	const auto base = QStandardPaths::writableLocation(
		QStandardPaths::AppLocalDataLocation);
	const auto dir = base.isEmpty() ? u"."_q : base;
	QDir().mkpath(dir);
	return dir + u"/ghostgram_history.json"_q;
}

void LoadIfNeeded(State &state) {
	if (state.loaded) {
		return;
	}
	state.loaded = true;
	auto file = QFile(StoragePath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(file.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto root = doc.object();
	const auto deletedArray = root["deleted"].toArray();
	for (const auto &val : deletedArray) {
		const auto obj = val.toObject();
		const auto fullId = FullMsgId(
			PeerId(uint64(ToInt64(obj["peer"]))),
			MsgId(ToInt64(obj["msg"])));
		state.deletedMessages.push_back(SavedMessage{
			.fullId = fullId,
			.fromName = obj["from"].toString(),
			.chatTitle = obj["chat"].toString(),
			.text = obj["text"].toString(),
			.date = TimeId(obj["date"].toInt()),
			.recordedAt = TimeId(obj["recorded"].toInt()),
			.isDeleted = true,
		});
		state.deletedIds.emplace(fullId);
	}

	const auto editedArray = root["edited"].toArray();
	for (const auto &val : editedArray) {
		const auto obj = val.toObject();
		state.editedMessages.push_back(SavedMessage{
			.fullId = FullMsgId(
				PeerId(uint64(ToInt64(obj["peer"]))),
				MsgId(ToInt64(obj["msg"]))),
			.fromName = obj["from"].toString(),
			.chatTitle = obj["chat"].toString(),
			.text = obj["text"].toString(),
			.previousText = obj["prev"].toString(),
			.date = TimeId(obj["date"].toInt()),
			.recordedAt = TimeId(obj["recorded"].toInt()),
			.isDeleted = false,
		});
	}

	const auto bmArray = root["bookmarks"].toArray();
	for (const auto &val : bmArray) {
		const auto obj = val.toObject();
		state.bookmarks.push_back(BookmarkItem{
			.fullId = FullMsgId(
				PeerId(uint64(ToInt64(obj["peer"]))),
				MsgId(ToInt64(obj["msg"]))),
			.text = obj["text"].toString(),
			.recordedAt = TimeId(obj["recorded"].toInt()),
		});
	}

	const auto notesObj = root["notes"].toObject();
	for (auto it = notesObj.begin(); it != notesObj.end(); ++it) {
		state.chatNotes.emplace(PeerId(it.key().toLongLong()), it.value().toString());
	}

	if (PruneExpired(state)) {
		SaveToDisk(state);
	}
}

QByteArray BuildJson(const State &state) {
	auto root = QJsonObject();
	auto deletedArray = QJsonArray();
	for (const auto &msg : state.deletedMessages) {
		auto obj = QJsonObject();
		obj["peer"] = qint64(msg.fullId.peer.value);
		obj["msg"] = qint64(msg.fullId.msg.bare);
		obj["from"] = msg.fromName;
		obj["chat"] = msg.chatTitle;
		obj["text"] = msg.text;
		obj["date"] = int(msg.date);
		obj["recorded"] = int(msg.recordedAt);
		deletedArray.append(obj);
	}
	root["deleted"] = deletedArray;

	auto editedArray = QJsonArray();
	for (const auto &msg : state.editedMessages) {
		auto obj = QJsonObject();
		obj["peer"] = qint64(msg.fullId.peer.value);
		obj["msg"] = qint64(msg.fullId.msg.bare);
		obj["from"] = msg.fromName;
		obj["chat"] = msg.chatTitle;
		obj["text"] = msg.text;
		obj["prev"] = msg.previousText;
		obj["date"] = int(msg.date);
		obj["recorded"] = int(msg.recordedAt);
		editedArray.append(obj);
	}
	root["edited"] = editedArray;

	auto bmArray = QJsonArray();
	for (const auto &bm : state.bookmarks) {
		auto obj = QJsonObject();
		obj["peer"] = qint64(bm.fullId.peer.value);
		obj["msg"] = qint64(bm.fullId.msg.bare);
		obj["text"] = bm.text;
		obj["recorded"] = int(bm.recordedAt);
		bmArray.append(obj);
	}
	root["bookmarks"] = bmArray;

	auto notesObj = QJsonObject();
	for (const auto &[peer, text] : state.chatNotes) {
		notesObj[QString::number(peer.value)] = text;
	}
	root["notes"] = notesObj;

	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void SaveToDisk(const State &state) {
	auto file = QFile(StoragePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(BuildJson(state));
	}
}

bool PruneExpired(State &state) {
	const auto days = GhostGram::Settings::HistoryRetentionDays();
	if (days <= 0) {
		return false;
	}
	const auto cutoff = base::unixtime::now() - TimeId(days) * 86400;
	auto changed = false;
	while (!state.deletedMessages.empty()
		&& state.deletedMessages.back().recordedAt < cutoff) {
		state.deletedIds.remove(state.deletedMessages.back().fullId);
		state.deletedMessages.pop_back();
		changed = true;
	}
	while (!state.editedMessages.empty()
		&& state.editedMessages.back().recordedAt < cutoff) {
		state.editedMessages.pop_back();
		changed = true;
	}
	return changed;
}

} // namespace

void SaveDeletedMessage(
		FullMsgId fullId,
		const QString &fromName,
		const QString &chatTitle,
		const QString &text,
		TimeId date) {
	if (!GhostGram::Settings::AntiRecallEnabled()
		|| text.isEmpty()
		|| g_bulkDestroyDepth.load(std::memory_order_relaxed) > 0) {
		return;
	}
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	if (state.deletedIds.contains(fullId)) {
		return;
	}

	state.deletedMessages.push_front(SavedMessage{
		.fullId = fullId,
		.fromName = fromName,
		.chatTitle = chatTitle,
		.text = text,
		.date = date,
		.recordedAt = base::unixtime::now(),
		.isDeleted = true,
	});
	state.deletedIds.emplace(fullId);

	PruneExpired(state);

	while (state.deletedMessages.size() > kMaxStoredItems) {
		state.deletedIds.remove(state.deletedMessages.back().fullId);
		state.deletedMessages.pop_back();
	}

	SaveToDisk(state);
}

void SaveEditedMessage(
		FullMsgId fullId,
		const QString &fromName,
		const QString &chatTitle,
		const QString &previousText,
		const QString &newText,
		TimeId date) {
	if (!GhostGram::Settings::EditHistoryEnabled()
		|| previousText.isEmpty()
		|| g_bulkDestroyDepth.load(std::memory_order_relaxed) > 0) {
		return;
	}
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	state.editedMessages.push_front(SavedMessage{
		.fullId = fullId,
		.fromName = fromName,
		.chatTitle = chatTitle,
		.text = newText,
		.previousText = previousText,
		.date = date,
		.recordedAt = base::unixtime::now(),
		.isDeleted = false,
	});

	PruneExpired(state);

	while (state.editedMessages.size() > kMaxStoredItems) {
		state.editedMessages.pop_back();
	}

	SaveToDisk(state);
}

std::vector<SavedMessage> GetDeletedMessages(PeerId peerId) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	auto result = std::vector<SavedMessage>();
	result.reserve(state.deletedMessages.size());
	for (const auto &msg : state.deletedMessages) {
		if (!peerId || msg.fullId.peer == peerId) {
			result.push_back(msg);
		}
	}
	return result;
}

std::vector<SavedMessage> GetEditedMessages(FullMsgId fullId) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	auto result = std::vector<SavedMessage>();
	for (const auto &msg : state.editedMessages) {
		if (!fullId || msg.fullId == fullId) {
			result.push_back(msg);
		}
	}
	return result;
}

bool HasDeletedVersion(FullMsgId fullId) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return state.deletedIds.contains(fullId);
}

void ClearHistory() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	state.deletedMessages.clear();
	state.editedMessages.clear();
	state.deletedIds.clear();
	SaveToDisk(state);
}

int TotalDeletedCount() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return int(state.deletedMessages.size());
}

int TotalEditedCount() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return int(state.editedMessages.size());
}

void SaveBookmark(FullMsgId fullId, const QString &text) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	for (const auto &bm : state.bookmarks) {
		if (bm.fullId == fullId) {
			return;
		}
	}

	state.bookmarks.push_front(BookmarkItem{
		.fullId = fullId,
		.text = text,
		.recordedAt = base::unixtime::now(),
	});

	while (state.bookmarks.size() > 5000) {
		state.bookmarks.pop_back();
	}

	SaveToDisk(state);
}

std::vector<BookmarkItem> GetBookmarks() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return std::vector<BookmarkItem>(state.bookmarks.begin(), state.bookmarks.end());
}

void RemoveBookmark(FullMsgId fullId) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	for (auto i = state.bookmarks.begin(); i != state.bookmarks.end(); ++i) {
		if (i->fullId == fullId) {
			state.bookmarks.erase(i);
			SaveToDisk(state);
			return;
		}
	}
}

void RemoveAllBookmarks() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	if (state.bookmarks.empty()) {
		return;
	}
	state.bookmarks.clear();
	SaveToDisk(state);
}

int TotalBookmarksCount() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return int(state.bookmarks.size());
}

QByteArray ExportJson() {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);
	return BuildJson(state);
}

void SaveChatNote(PeerId peerId, const QString &note) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	if (note.trimmed().isEmpty()) {
		state.chatNotes.remove(peerId);
	} else {
		state.chatNotes[peerId] = note;
	}

	SaveToDisk(state);
}

QString GetChatNote(PeerId peerId) {
	auto &state = GetState();
	std::lock_guard<std::mutex> lock(state.mutex);
	LoadIfNeeded(state);

	const auto it = state.chatNotes.find(peerId);
	return (it != state.chatNotes.end()) ? it->second : QString();
}

BulkDestroyGuard::BulkDestroyGuard() {
	g_bulkDestroyDepth.fetch_add(1, std::memory_order_relaxed);
}

BulkDestroyGuard::~BulkDestroyGuard() {
	g_bulkDestroyDepth.fetch_sub(1, std::memory_order_relaxed);
}

} // namespace GhostGram::History
