/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "settings/settings_type.h"

namespace Settings {

[[nodiscard]] Type GhostGramId();
[[nodiscard]] Type GhostGramOverviewId();
[[nodiscard]] Type GhostGramGeneralId();
[[nodiscard]] Type GhostGramGhostId();
[[nodiscard]] Type GhostGramAccountsId();
[[nodiscard]] Type GhostGramInterfaceId();
[[nodiscard]] Type GhostGramMessagesId();
[[nodiscard]] Type GhostGramPrivacyId();
[[nodiscard]] Type GhostGramMediaId();
[[nodiscard]] Type GhostGramNotificationsId();
[[nodiscard]] Type GhostGramStreamerId();
[[nodiscard]] Type GhostGramToolsId();
[[nodiscard]] Type GhostGramBackupId();
[[nodiscard]] Type GhostGramAdvancedId();
[[nodiscard]] Type GhostGramAboutId();

} // namespace Settings
