/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ghostgram/ghostgram_settings.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "window/themes/window_theme.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <rpl/variable.h>

#include <algorithm>

namespace GhostGram::Settings {
namespace {

constexpr auto kGhostReadReceiptsKey = "gg_ghost_read_receipts";
constexpr auto kGhostTypingKey = "gg_ghost_typing";
constexpr auto kGhostStoriesKey = "gg_ghost_stories";
constexpr auto kGhostOfflineKey = "gg_ghost_offline";
constexpr auto kMaxAccountsKey = "gg_max_accounts";
constexpr auto kFastAccountSwitchKey = "gg_fast_account_switch";
constexpr auto kStreamerModeKey = "gg_streamer_mode";
constexpr auto kStreamerHideNamesKey = "gg_streamer_hide_names";
constexpr auto kStreamerHideAvatarsKey = "gg_streamer_hide_avatars";
constexpr auto kStreamerHidePreviewsKey = "gg_streamer_hide_previews";
constexpr auto kStreamerCensorPhonesKey = "gg_streamer_censor_phones";
constexpr auto kAntiRecallEnabledKey = "gg_anti_recall_enabled";
constexpr auto kAntiRecallShowMarkKey = "gg_anti_recall_show_mark";
constexpr auto kEditHistoryEnabledKey = "gg_edit_history_enabled";
constexpr auto kHistoryRetentionDaysKey = "gg_history_retention_days";
constexpr auto kThemePresetKey = "gg_theme_preset";
constexpr auto kCompactChatListKey = "gg_compact_chat_list";
constexpr auto kShowPeerIdKey = "gg_show_peer_id";
constexpr auto kHideSponsoredKey = "gg_hide_sponsored";
constexpr auto kForwardWithoutQuoteKey = "gg_forward_without_quote";
constexpr auto kBlockTelemetryKey = "gg_block_telemetry";
constexpr auto kPreserveDeletedMediaKey = "gg_preserve_deleted_media";
constexpr auto kBookmarksEnabledKey = "gg_bookmarks_enabled";
constexpr auto kChatNotesEnabledKey = "gg_chat_notes_enabled";

struct State {
	rpl::variable<bool> ghostReadReceipts = false;
	rpl::variable<bool> ghostTyping = false;
	rpl::variable<bool> ghostStories = false;
	rpl::variable<bool> ghostOffline = false;
	rpl::variable<int> maxAccounts = 10;
	rpl::variable<bool> fastAccountSwitch = true;
	rpl::variable<bool> streamerMode = false;
	rpl::variable<bool> streamerHideNames = true;
	rpl::variable<bool> streamerHideAvatars = true;
	rpl::variable<bool> streamerHidePreviews = true;
	rpl::variable<bool> streamerCensorPhones = true;
	rpl::variable<bool> antiRecallEnabled = false;
	rpl::variable<bool> antiRecallShowMark = true;
	rpl::variable<bool> editHistoryEnabled = false;
	rpl::variable<int> historyRetentionDays = 30;
	rpl::variable<ThemePreset> themePreset = ThemePreset::Default;
	rpl::variable<bool> compactChatList = false;
	rpl::variable<bool> showPeerId = false;
	rpl::variable<bool> hideSponsored = true;
	rpl::variable<bool> forwardWithoutQuote = false;
	rpl::variable<bool> blockTelemetry = true;
	rpl::variable<bool> preserveDeletedMedia = false;
	rpl::variable<bool> bookmarksEnabled = true;
	rpl::variable<bool> chatNotesEnabled = true;
	bool initialized = false;
};

void ApplyThemePreset(ThemePreset preset);

State &GetState() {
	static State state;
	if (!state.initialized) {
		state.initialized = true;
		auto &core = Core::App().settings();

		state.ghostReadReceipts = core.readPref<bool>(kGhostReadReceiptsKey, false);
		state.ghostTyping = core.readPref<bool>(kGhostTypingKey, false);
		state.ghostStories = core.readPref<bool>(kGhostStoriesKey, false);
		state.ghostOffline = core.readPref<bool>(kGhostOfflineKey, false);

		const auto maxAccountsRaw = core.readPref<QByteArray>(kMaxAccountsKey);
		if (!maxAccountsRaw.isEmpty()) {
			state.maxAccounts = std::clamp(maxAccountsRaw.toInt(), 3, 20);
		} else {
			state.maxAccounts = 10;
		}

		state.fastAccountSwitch = core.readPref<bool>(kFastAccountSwitchKey, true);
		state.streamerMode = core.readPref<bool>(kStreamerModeKey, false);
		state.streamerHideNames = core.readPref<bool>(kStreamerHideNamesKey, true);
		state.streamerHideAvatars = core.readPref<bool>(kStreamerHideAvatarsKey, true);
		state.streamerHidePreviews = core.readPref<bool>(kStreamerHidePreviewsKey, true);
		state.streamerCensorPhones = core.readPref<bool>(kStreamerCensorPhonesKey, true);

		state.antiRecallEnabled = core.readPref<bool>(kAntiRecallEnabledKey, false);
		state.antiRecallShowMark = core.readPref<bool>(kAntiRecallShowMarkKey, true);
		state.editHistoryEnabled = core.readPref<bool>(kEditHistoryEnabledKey, false);

		const auto retentionRaw = core.readPref<QByteArray>(kHistoryRetentionDaysKey);
		if (!retentionRaw.isEmpty()) {
			state.historyRetentionDays = retentionRaw.toInt();
		} else {
			state.historyRetentionDays = 30;
		}

		const auto themeRaw = core.readPref<QByteArray>(kThemePresetKey);
		if (!themeRaw.isEmpty()) {
			state.themePreset = static_cast<ThemePreset>(themeRaw.toInt());
			if (state.themePreset.current() != ThemePreset::Default) {
				ApplyThemePreset(state.themePreset.current());
			}
		} else {
			state.themePreset = ThemePreset::Default;
		}

		state.compactChatList = core.readPref<bool>(kCompactChatListKey, false);
		state.showPeerId = core.readPref<bool>(kShowPeerIdKey, false);
		state.hideSponsored = core.readPref<bool>(kHideSponsoredKey, true);
		state.forwardWithoutQuote = core.readPref<bool>(kForwardWithoutQuoteKey, false);
		state.blockTelemetry = core.readPref<bool>(kBlockTelemetryKey, true);
		state.preserveDeletedMedia = core.readPref<bool>(kPreserveDeletedMediaKey, false);
		state.bookmarksEnabled = core.readPref<bool>(kBookmarksEnabledKey, true);
		state.chatNotesEnabled = core.readPref<bool>(kChatNotesEnabledKey, true);
	}
	return state;
}

void SaveBool(std::string_view key, bool value) {
	Core::App().settings().writePref<bool>(key, value);
	Core::App().saveSettingsDelayed();
}

void SaveInt(std::string_view key, int value) {
	Core::App().settings().writePref<QByteArray>(key, QByteArray::number(value));
	Core::App().saveSettingsDelayed();
}

} // namespace

bool GhostReadReceipts() {
	return GetState().ghostReadReceipts.current();
}

void SetGhostReadReceipts(bool value) {
	auto &state = GetState();
	if (state.ghostReadReceipts.current() != value) {
		state.ghostReadReceipts = value;
		SaveBool(kGhostReadReceiptsKey, value);
	}
}

rpl::producer<bool> GhostReadReceiptsValue() {
	return GetState().ghostReadReceipts.value();
}

bool GhostTyping() {
	return GetState().ghostTyping.current();
}

void SetGhostTyping(bool value) {
	auto &state = GetState();
	if (state.ghostTyping.current() != value) {
		state.ghostTyping = value;
		SaveBool(kGhostTypingKey, value);
	}
}

rpl::producer<bool> GhostTypingValue() {
	return GetState().ghostTyping.value();
}

bool GhostStories() {
	return GetState().ghostStories.current();
}

void SetGhostStories(bool value) {
	auto &state = GetState();
	if (state.ghostStories.current() != value) {
		state.ghostStories = value;
		SaveBool(kGhostStoriesKey, value);
	}
}

rpl::producer<bool> GhostStoriesValue() {
	return GetState().ghostStories.value();
}

bool GhostOffline() {
	return GetState().ghostOffline.current();
}

void SetGhostOffline(bool value) {
	auto &state = GetState();
	if (state.ghostOffline.current() != value) {
		state.ghostOffline = value;
		SaveBool(kGhostOfflineKey, value);
	}
}

rpl::producer<bool> GhostOfflineValue() {
	return GetState().ghostOffline.value();
}

int MaxAccounts() {
	return GetState().maxAccounts.current();
}

void SetMaxAccounts(int value) {
	const auto clamped = std::clamp(value, 3, 20);
	auto &state = GetState();
	if (state.maxAccounts.current() != clamped) {
		state.maxAccounts = clamped;
		SaveInt(kMaxAccountsKey, clamped);
	}
}

rpl::producer<int> MaxAccountsValue() {
	return GetState().maxAccounts.value();
}

bool FastAccountSwitch() {
	return GetState().fastAccountSwitch.current();
}

void SetFastAccountSwitch(bool value) {
	auto &state = GetState();
	if (state.fastAccountSwitch.current() != value) {
		state.fastAccountSwitch = value;
		SaveBool(kFastAccountSwitchKey, value);
	}
}

rpl::producer<bool> FastAccountSwitchValue() {
	return GetState().fastAccountSwitch.value();
}

bool StreamerMode() {
	return GetState().streamerMode.current();
}

void SetStreamerMode(bool value) {
	auto &state = GetState();
	if (state.streamerMode.current() != value) {
		state.streamerMode = value;
		SaveBool(kStreamerModeKey, value);
	}
}

rpl::producer<bool> StreamerModeValue() {
	return GetState().streamerMode.value();
}

bool StreamerHideNames() {
	return GetState().streamerHideNames.current();
}

void SetStreamerHideNames(bool value) {
	auto &state = GetState();
	if (state.streamerHideNames.current() != value) {
		state.streamerHideNames = value;
		SaveBool(kStreamerHideNamesKey, value);
	}
}

rpl::producer<bool> StreamerHideNamesValue() {
	return GetState().streamerHideNames.value();
}

bool StreamerHideAvatars() {
	return GetState().streamerHideAvatars.current();
}

void SetStreamerHideAvatars(bool value) {
	auto &state = GetState();
	if (state.streamerHideAvatars.current() != value) {
		state.streamerHideAvatars = value;
		SaveBool(kStreamerHideAvatarsKey, value);
	}
}

rpl::producer<bool> StreamerHideAvatarsValue() {
	return GetState().streamerHideAvatars.value();
}

bool StreamerHidePreviews() {
	return GetState().streamerHidePreviews.current();
}

void SetStreamerHidePreviews(bool value) {
	auto &state = GetState();
	if (state.streamerHidePreviews.current() != value) {
		state.streamerHidePreviews = value;
		SaveBool(kStreamerHidePreviewsKey, value);
	}
}

rpl::producer<bool> StreamerHidePreviewsValue() {
	return GetState().streamerHidePreviews.value();
}

bool StreamerCensorPhones() {
	return GetState().streamerCensorPhones.current();
}

void SetStreamerCensorPhones(bool value) {
	auto &state = GetState();
	if (state.streamerCensorPhones.current() != value) {
		state.streamerCensorPhones = value;
		SaveBool(kStreamerCensorPhonesKey, value);
	}
}

rpl::producer<bool> StreamerCensorPhonesValue() {
	return GetState().streamerCensorPhones.value();
}

bool AntiRecallEnabled() {
	return GetState().antiRecallEnabled.current();
}

void SetAntiRecallEnabled(bool value) {
	auto &state = GetState();
	if (state.antiRecallEnabled.current() != value) {
		state.antiRecallEnabled = value;
		SaveBool(kAntiRecallEnabledKey, value);
	}
}

rpl::producer<bool> AntiRecallEnabledValue() {
	return GetState().antiRecallEnabled.value();
}

bool AntiRecallShowMark() {
	return GetState().antiRecallShowMark.current();
}

void SetAntiRecallShowMark(bool value) {
	auto &state = GetState();
	if (state.antiRecallShowMark.current() != value) {
		state.antiRecallShowMark = value;
		SaveBool(kAntiRecallShowMarkKey, value);
	}
}

rpl::producer<bool> AntiRecallShowMarkValue() {
	return GetState().antiRecallShowMark.value();
}

bool EditHistoryEnabled() {
	return GetState().editHistoryEnabled.current();
}

void SetEditHistoryEnabled(bool value) {
	auto &state = GetState();
	if (state.editHistoryEnabled.current() != value) {
		state.editHistoryEnabled = value;
		SaveBool(kEditHistoryEnabledKey, value);
	}
}

rpl::producer<bool> EditHistoryEnabledValue() {
	return GetState().editHistoryEnabled.value();
}

int HistoryRetentionDays() {
	return GetState().historyRetentionDays.current();
}

void SetHistoryRetentionDays(int value) {
	auto &state = GetState();
	if (state.historyRetentionDays.current() != value) {
		state.historyRetentionDays = value;
		SaveInt(kHistoryRetentionDaysKey, value);
	}
}

rpl::producer<int> HistoryRetentionDaysValue() {
	return GetState().historyRetentionDays.value();
}

ThemePreset CurrentThemePreset() {
	return GetState().themePreset.current();
}

void ApplyThemePreset(ThemePreset preset) {
	if (preset == ThemePreset::Default) {
		Window::Theme::ResetToSomeDefault();
		return;
	}
	QByteArray palette;
	switch (preset) {
	case ThemePreset::Obsidian:
		palette =
			"windowBg: #0e0e11;\n"
			"windowBgOver: #16161c;\n"
			"windowBgRipple: #22222a;\n"
			"windowBgActive: #252530;\n"
			"topBarBg: #0e0e11;\n"
			"dialogsBg: #0e0e11;\n"
			"dialogsBgOver: #16161c;\n"
			"dialogsBgActive: #22222a;\n"
			"historyComposeAreaBg: #0e0e11;\n"
			"msgInBg: #181820;\n"
			"msgInBgSelected: #262633;\n"
			"msgOutBg: #222530;\n"
			"msgOutBgSelected: #303545;\n"
			"sideBarBg: #0a0a0d;\n"
			"menuBg: #141419;\n";
		break;
	case ThemePreset::Phantom:
		palette =
			"windowBg: #13111c;\n"
			"windowBgOver: #1c1829;\n"
			"windowBgRipple: #272138;\n"
			"windowBgActive: #5a3e85;\n"
			"topBarBg: #13111c;\n"
			"dialogsBg: #13111c;\n"
			"dialogsBgOver: #1c1829;\n"
			"dialogsBgActive: #272138;\n"
			"historyComposeAreaBg: #13111c;\n"
			"msgInBg: #1c1828;\n"
			"msgInBgSelected: #2b243e;\n"
			"msgOutBg: #382654;\n"
			"msgOutBgSelected: #4a3370;\n"
			"sideBarBg: #0e0d14;\n"
			"menuBg: #181524;\n";
		break;
	case ThemePreset::Midnight:
		palette =
			"windowBg: #080d14;\n"
			"windowBgOver: #0f1824;\n"
			"windowBgRipple: #162233;\n"
			"windowBgActive: #1c3654;\n"
			"topBarBg: #080d14;\n"
			"dialogsBg: #080d14;\n"
			"dialogsBgOver: #0f1824;\n"
			"dialogsBgActive: #162233;\n"
			"historyComposeAreaBg: #080d14;\n"
			"msgInBg: #0e1724;\n"
			"msgInBgSelected: #17253b;\n"
			"msgOutBg: #1a3352;\n"
			"msgOutBgSelected: #254975;\n"
			"sideBarBg: #06090e;\n"
			"menuBg: #0c131c;\n";
		break;
	case ThemePreset::Violet:
		palette =
			"windowBg: #150d1f;\n"
			"windowBgOver: #201330;\n"
			"windowBgRipple: #2d1b45;\n"
			"windowBgActive: #7b2cbf;\n"
			"topBarBg: #150d1f;\n"
			"dialogsBg: #150d1f;\n"
			"dialogsBgOver: #201330;\n"
			"dialogsBgActive: #2d1b45;\n"
			"historyComposeAreaBg: #150d1f;\n"
			"msgInBg: #221433;\n"
			"msgInBgSelected: #331e4d;\n"
			"msgOutBg: #4d1975;\n"
			"msgOutBgSelected: #68229e;\n"
			"sideBarBg: #0f0917;\n"
			"menuBg: #1a1026;\n";
		break;
	case ThemePreset::Arctic:
		palette =
			"windowBg: #0a1317;\n"
			"windowBgOver: #101e24;\n"
			"windowBgRipple: #172a33;\n"
			"windowBgActive: #1d5563;\n"
			"topBarBg: #0a1317;\n"
			"dialogsBg: #0a1317;\n"
			"dialogsBgOver: #101e24;\n"
			"dialogsBgActive: #172a33;\n"
			"historyComposeAreaBg: #0a1317;\n"
			"msgInBg: #0f1e24;\n"
			"msgInBgSelected: #182e38;\n"
			"msgOutBg: #18424d;\n"
			"msgOutBgSelected: #235c6b;\n"
			"sideBarBg: #070e12;\n"
			"menuBg: #0e1a20;\n";
		break;
	default:
		return;
	}
	if (Window::Theme::ApplyEditedPalette(palette)) {
		Window::Theme::KeepApplied();
	}
}

void SetThemePreset(ThemePreset value) {
	auto &state = GetState();
	if (state.themePreset.current() != value) {
		state.themePreset = value;
		SaveInt(kThemePresetKey, static_cast<int>(value));
		ApplyThemePreset(value);
	}
}

rpl::producer<ThemePreset> ThemePresetValue() {
	return GetState().themePreset.value();
}

bool CompactChatList() {
	return GetState().compactChatList.current();
}

void SetCompactChatList(bool value) {
	auto &state = GetState();
	if (state.compactChatList.current() != value) {
		state.compactChatList = value;
		SaveBool(kCompactChatListKey, value);
	}
}

rpl::producer<bool> CompactChatListValue() {
	return GetState().compactChatList.value();
}

bool ShowPeerId() {
	return GetState().showPeerId.current();
}

void SetShowPeerId(bool value) {
	auto &state = GetState();
	if (state.showPeerId.current() != value) {
		state.showPeerId = value;
		SaveBool(kShowPeerIdKey, value);
	}
}

rpl::producer<bool> ShowPeerIdValue() {
	return GetState().showPeerId.value();
}

bool HideSponsored() {
	return GetState().hideSponsored.current();
}

void SetHideSponsored(bool value) {
	auto &state = GetState();
	if (state.hideSponsored.current() != value) {
		state.hideSponsored = value;
		SaveBool(kHideSponsoredKey, value);
	}
}

rpl::producer<bool> HideSponsoredValue() {
	return GetState().hideSponsored.value();
}

bool ForwardWithoutQuote() {
	return GetState().forwardWithoutQuote.current();
}

void SetForwardWithoutQuote(bool value) {
	auto &state = GetState();
	if (state.forwardWithoutQuote.current() != value) {
		state.forwardWithoutQuote = value;
		SaveBool(kForwardWithoutQuoteKey, value);
	}
}

rpl::producer<bool> ForwardWithoutQuoteValue() {
	return GetState().forwardWithoutQuote.value();
}

bool BlockTelemetry() {
	return GetState().blockTelemetry.current();
}

void SetBlockTelemetry(bool value) {
	auto &state = GetState();
	if (state.blockTelemetry.current() != value) {
		state.blockTelemetry = value;
		SaveBool(kBlockTelemetryKey, value);
	}
}

rpl::producer<bool> BlockTelemetryValue() {
	return GetState().blockTelemetry.value();
}

bool PreserveDeletedMedia() {
	return GetState().preserveDeletedMedia.current();
}

void SetPreserveDeletedMedia(bool value) {
	auto &state = GetState();
	if (state.preserveDeletedMedia.current() != value) {
		state.preserveDeletedMedia = value;
		SaveBool(kPreserveDeletedMediaKey, value);
	}
}

rpl::producer<bool> PreserveDeletedMediaValue() {
	return GetState().preserveDeletedMedia.value();
}

bool BookmarksEnabled() {
	return GetState().bookmarksEnabled.current();
}

void SetBookmarksEnabled(bool value) {
	auto &state = GetState();
	if (state.bookmarksEnabled.current() != value) {
		state.bookmarksEnabled = value;
		SaveBool(kBookmarksEnabledKey, value);
	}
}

rpl::producer<bool> BookmarksEnabledValue() {
	return GetState().bookmarksEnabled.value();
}

bool ChatNotesEnabled() {
	return GetState().chatNotesEnabled.current();
}

void SetChatNotesEnabled(bool value) {
	auto &state = GetState();
	if (state.chatNotesEnabled.current() != value) {
		state.chatNotesEnabled = value;
		SaveBool(kChatNotesEnabledKey, value);
	}
}

rpl::producer<bool> ChatNotesEnabledValue() {
	return GetState().chatNotesEnabled.value();
}

void ResetToDefaults() {
	SetGhostReadReceipts(false);
	SetGhostTyping(false);
	SetGhostStories(false);
	SetGhostOffline(false);
	SetMaxAccounts(10);
	SetFastAccountSwitch(true);
	SetStreamerMode(false);
	SetStreamerHideNames(true);
	SetStreamerHideAvatars(true);
	SetStreamerHidePreviews(true);
	SetStreamerCensorPhones(true);
	SetAntiRecallEnabled(false);
	SetAntiRecallShowMark(true);
	SetEditHistoryEnabled(false);
	SetHistoryRetentionDays(30);
	SetThemePreset(ThemePreset::Default);
	SetCompactChatList(false);
	SetShowPeerId(false);
	SetHideSponsored(true);
	SetForwardWithoutQuote(false);
	SetBlockTelemetry(true);
	SetPreserveDeletedMedia(false);
	SetBookmarksEnabled(true);
	SetChatNotesEnabled(true);
}

QByteArray ExportSettingsJson() {
	auto obj = QJsonObject();
	obj["ghostReadReceipts"] = GhostReadReceipts();
	obj["ghostTyping"] = GhostTyping();
	obj["ghostStories"] = GhostStories();
	obj["ghostOffline"] = GhostOffline();
	obj["maxAccounts"] = MaxAccounts();
	obj["fastAccountSwitch"] = FastAccountSwitch();
	obj["streamerMode"] = StreamerMode();
	obj["streamerHideNames"] = StreamerHideNames();
	obj["streamerHideAvatars"] = StreamerHideAvatars();
	obj["streamerHidePreviews"] = StreamerHidePreviews();
	obj["streamerCensorPhones"] = StreamerCensorPhones();
	obj["antiRecallEnabled"] = AntiRecallEnabled();
	obj["antiRecallShowMark"] = AntiRecallShowMark();
	obj["editHistoryEnabled"] = EditHistoryEnabled();
	obj["historyRetentionDays"] = HistoryRetentionDays();
	obj["themePreset"] = static_cast<int>(CurrentThemePreset());
	obj["compactChatList"] = CompactChatList();
	obj["showPeerId"] = ShowPeerId();
	obj["hideSponsored"] = HideSponsored();
	obj["forwardWithoutQuote"] = ForwardWithoutQuote();
	obj["blockTelemetry"] = BlockTelemetry();
	obj["preserveDeletedMedia"] = PreserveDeletedMedia();
	obj["bookmarksEnabled"] = BookmarksEnabled();
	obj["chatNotesEnabled"] = ChatNotesEnabled();

	return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

bool ImportSettingsJson(const QByteArray &data) {
	auto error = QJsonParseError();
	const auto doc = QJsonDocument::fromJson(data, &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject()) {
		return false;
	}
	const auto obj = doc.object();

	if (obj.contains("ghostReadReceipts")) {
		SetGhostReadReceipts(obj["ghostReadReceipts"].toBool());
	}
	if (obj.contains("ghostTyping")) {
		SetGhostTyping(obj["ghostTyping"].toBool());
	}
	if (obj.contains("ghostStories")) {
		SetGhostStories(obj["ghostStories"].toBool());
	}
	if (obj.contains("ghostOffline")) {
		SetGhostOffline(obj["ghostOffline"].toBool());
	}
	if (obj.contains("maxAccounts")) {
		SetMaxAccounts(obj["maxAccounts"].toInt(10));
	}
	if (obj.contains("fastAccountSwitch")) {
		SetFastAccountSwitch(obj["fastAccountSwitch"].toBool(true));
	}
	if (obj.contains("streamerMode")) {
		SetStreamerMode(obj["streamerMode"].toBool());
	}
	if (obj.contains("streamerHideNames")) {
		SetStreamerHideNames(obj["streamerHideNames"].toBool(true));
	}
	if (obj.contains("streamerHideAvatars")) {
		SetStreamerHideAvatars(obj["streamerHideAvatars"].toBool(true));
	}
	if (obj.contains("streamerHidePreviews")) {
		SetStreamerHidePreviews(obj["streamerHidePreviews"].toBool(true));
	}
	if (obj.contains("streamerCensorPhones")) {
		SetStreamerCensorPhones(obj["streamerCensorPhones"].toBool(true));
	}
	if (obj.contains("antiRecallEnabled")) {
		SetAntiRecallEnabled(obj["antiRecallEnabled"].toBool());
	}
	if (obj.contains("antiRecallShowMark")) {
		SetAntiRecallShowMark(obj["antiRecallShowMark"].toBool(true));
	}
	if (obj.contains("editHistoryEnabled")) {
		SetEditHistoryEnabled(obj["editHistoryEnabled"].toBool());
	}
	if (obj.contains("historyRetentionDays")) {
		SetHistoryRetentionDays(obj["historyRetentionDays"].toInt(30));
	}
	if (obj.contains("themePreset")) {
		SetThemePreset(static_cast<ThemePreset>(obj["themePreset"].toInt(0)));
	}
	if (obj.contains("compactChatList")) {
		SetCompactChatList(obj["compactChatList"].toBool());
	}
	if (obj.contains("showPeerId")) {
		SetShowPeerId(obj["showPeerId"].toBool());
	}
	if (obj.contains("hideSponsored")) {
		SetHideSponsored(obj["hideSponsored"].toBool(true));
	}
	if (obj.contains("forwardWithoutQuote")) {
		SetForwardWithoutQuote(obj["forwardWithoutQuote"].toBool());
	}
	if (obj.contains("blockTelemetry")) {
		SetBlockTelemetry(obj["blockTelemetry"].toBool(true));
	}
	if (obj.contains("preserveDeletedMedia")) {
		SetPreserveDeletedMedia(obj["preserveDeletedMedia"].toBool());
	}
	if (obj.contains("bookmarksEnabled")) {
		SetBookmarksEnabled(obj["bookmarksEnabled"].toBool(true));
	}
	if (obj.contains("chatNotesEnabled")) {
		SetChatNotesEnabled(obj["chatNotesEnabled"].toBool(true));
	}
	return true;
}

} // namespace GhostGram::Settings
