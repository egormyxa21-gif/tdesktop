/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <rpl/producer.h>

namespace GhostGram::Settings {

enum class ThemePreset : int {
	Default = 0,
	Obsidian = 1,
	Phantom = 2,
	Midnight = 3,
	Violet = 4,
	Arctic = 5,
	Emerald = 6,
	Crimson = 7,
	Sunset = 8,
	Graphite = 9,
	Ocean = 10,
	Nord = 11,
	Cyberpunk = 12,
	Amoled = 13,
	Solarized = 14,
	Daylight = 15,
	Lavender = 16,
};

// Ghost Mode
[[nodiscard]] bool GhostReadReceipts();
void SetGhostReadReceipts(bool value);
[[nodiscard]] rpl::producer<bool> GhostReadReceiptsValue();

[[nodiscard]] bool GhostTyping();
void SetGhostTyping(bool value);
[[nodiscard]] rpl::producer<bool> GhostTypingValue();

[[nodiscard]] bool GhostStories();
void SetGhostStories(bool value);
[[nodiscard]] rpl::producer<bool> GhostStoriesValue();

[[nodiscard]] bool GhostOffline();
void SetGhostOffline(bool value);
[[nodiscard]] rpl::producer<bool> GhostOfflineValue();

// Accounts
[[nodiscard]] int MaxAccounts();
void SetMaxAccounts(int value);
[[nodiscard]] rpl::producer<int> MaxAccountsValue();

[[nodiscard]] bool FastAccountSwitch();
void SetFastAccountSwitch(bool value);
[[nodiscard]] rpl::producer<bool> FastAccountSwitchValue();

// Streamer Mode
[[nodiscard]] bool StreamerMode();
void SetStreamerMode(bool value);
[[nodiscard]] rpl::producer<bool> StreamerModeValue();

[[nodiscard]] bool StreamerHideNames();
void SetStreamerHideNames(bool value);
[[nodiscard]] rpl::producer<bool> StreamerHideNamesValue();

[[nodiscard]] bool StreamerHideAvatars();
void SetStreamerHideAvatars(bool value);
[[nodiscard]] rpl::producer<bool> StreamerHideAvatarsValue();

[[nodiscard]] bool StreamerHidePreviews();
void SetStreamerHidePreviews(bool value);
[[nodiscard]] rpl::producer<bool> StreamerHidePreviewsValue();

[[nodiscard]] bool StreamerCensorPhones();
void SetStreamerCensorPhones(bool value);
[[nodiscard]] rpl::producer<bool> StreamerCensorPhonesValue();

// Messages and History
[[nodiscard]] bool AntiRecallEnabled();
void SetAntiRecallEnabled(bool value);
[[nodiscard]] rpl::producer<bool> AntiRecallEnabledValue();

[[nodiscard]] bool AntiRecallShowMark();
void SetAntiRecallShowMark(bool value);
[[nodiscard]] rpl::producer<bool> AntiRecallShowMarkValue();

[[nodiscard]] bool EditHistoryEnabled();
void SetEditHistoryEnabled(bool value);
[[nodiscard]] rpl::producer<bool> EditHistoryEnabledValue();

[[nodiscard]] int HistoryRetentionDays();
void SetHistoryRetentionDays(int value);
[[nodiscard]] rpl::producer<int> HistoryRetentionDaysValue();

// Interface and Themes
[[nodiscard]] ThemePreset CurrentThemePreset();
void SetThemePreset(ThemePreset value);
[[nodiscard]] rpl::producer<ThemePreset> ThemePresetValue();

[[nodiscard]] QByteArray CurrentCustomTheme();
void SetCustomTheme(const QByteArray &value);
[[nodiscard]] rpl::producer<QByteArray> CustomThemeValue();

[[nodiscard]] bool AutoContrast();
void SetAutoContrast(bool value);
[[nodiscard]] rpl::producer<bool> AutoContrastValue();

[[nodiscard]] QByteArray SavedThemesJson();
[[nodiscard]] rpl::producer<QByteArray> SavedThemesValue();
[[nodiscard]] bool AddSavedTheme(const QByteArray &specJson);
void RemoveSavedTheme(int index);
void ClearSavedThemes();

// WHY: the first GetState() happens while Core::Settings is still empty
// (Main::Domain is built before Local::start), so prefs must be re-read
// once they are actually loaded.
void Reload();

[[nodiscard]] bool CompactChatList();
void SetCompactChatList(bool value);
[[nodiscard]] rpl::producer<bool> CompactChatListValue();

[[nodiscard]] bool ShowPeerId();
void SetShowPeerId(bool value);
[[nodiscard]] rpl::producer<bool> ShowPeerIdValue();

[[nodiscard]] bool HideSponsored();
void SetHideSponsored(bool value);
[[nodiscard]] rpl::producer<bool> HideSponsoredValue();

// Privacy
[[nodiscard]] bool ForwardWithoutQuote();
void SetForwardWithoutQuote(bool value);
[[nodiscard]] rpl::producer<bool> ForwardWithoutQuoteValue();

[[nodiscard]] bool BlockTelemetry();
void SetBlockTelemetry(bool value);
[[nodiscard]] rpl::producer<bool> BlockTelemetryValue();

// Media
[[nodiscard]] bool PreserveDeletedMedia();
void SetPreserveDeletedMedia(bool value);
[[nodiscard]] rpl::producer<bool> PreserveDeletedMediaValue();

// Tools
[[nodiscard]] bool BookmarksEnabled();
void SetBookmarksEnabled(bool value);
[[nodiscard]] rpl::producer<bool> BookmarksEnabledValue();

[[nodiscard]] bool ChatNotesEnabled();
void SetChatNotesEnabled(bool value);
[[nodiscard]] rpl::producer<bool> ChatNotesEnabledValue();

// Management
void ResetToDefaults();
[[nodiscard]] QByteArray ExportSettingsJson();
[[nodiscard]] bool ImportSettingsJson(const QByteArray &data);

} // namespace GhostGram::Settings
