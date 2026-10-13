/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/basic_types.h"
#include "mtproto/core_types.h"
#include "data/data_msg_id.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <vector>

namespace GhostGram::History {

struct SavedMessage {
	FullMsgId fullId;
	QString fromName;
	QString chatTitle;
	QString text;
	QString previousText;
	TimeId date = 0;
	TimeId recordedAt = 0;
	bool isDeleted = true;
};

void SaveDeletedMessage(
	FullMsgId fullId,
	const QString &fromName,
	const QString &chatTitle,
	const QString &text,
	TimeId date);

void SaveEditedMessage(
	FullMsgId fullId,
	const QString &fromName,
	const QString &chatTitle,
	const QString &previousText,
	const QString &newText,
	TimeId date);

[[nodiscard]] std::vector<SavedMessage> GetDeletedMessages(PeerId peerId = 0);
[[nodiscard]] std::vector<SavedMessage> GetEditedMessages(FullMsgId fullId = {});
[[nodiscard]] bool HasDeletedVersion(FullMsgId fullId);
void ClearHistory();
[[nodiscard]] int TotalDeletedCount();
[[nodiscard]] int TotalEditedCount();

struct BookmarkItem {
	FullMsgId fullId;
	QString text;
	TimeId recordedAt = 0;
};

void SaveBookmark(FullMsgId fullId, const QString &text);
[[nodiscard]] std::vector<BookmarkItem> GetBookmarks();
[[nodiscard]] int TotalBookmarksCount();

void SaveChatNote(PeerId peerId, const QString &note);
[[nodiscard]] QString GetChatNote(PeerId peerId);

} // namespace GhostGram::History
