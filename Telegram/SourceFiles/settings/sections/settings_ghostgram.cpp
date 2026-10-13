/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_ghostgram.h"

#include "ghostgram/ghostgram_settings.h"
#include "ghostgram/ghostgram_history.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "settings/sections/settings_privacy_security.h"
#include "settings/sections/settings_notifications.h"
#include "settings/settings_builder.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QFile>
#include <QtCore/QStandardPaths>

namespace Settings {
namespace {

using namespace Builder;

void AddSettingToggle(
		SectionBuilder &builder,
		QString id,
		rpl::producer<QString> title,
		Fn<bool()> getter,
		Fn<void(bool)> setter,
		QStringList keywords = {}) {
	const auto button = builder.addButton({
		.id = std::move(id),
		.title = std::move(title),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(getter()),
		.keywords = std::move(keywords),
	});
	if (button) {
		button->toggledValue()
		| rpl::filter([=](bool checked) {
			return checked != getter();
		})
		| rpl::on_next([=](bool checked) {
			setter(checked);
		}, button->lifetime());
	}
}

// 1. Overview
class GhostGramOverview : public Section<GhostGramOverview> {
public:
	GhostGramOverview(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 2. General
class GhostGramGeneral : public Section<GhostGramGeneral> {
public:
	GhostGramGeneral(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 3. Ghost Mode
class GhostGramGhost : public Section<GhostGramGhost> {
public:
	GhostGramGhost(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 4. Accounts
class GhostGramAccounts : public Section<GhostGramAccounts> {
public:
	GhostGramAccounts(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 5. Interface
class GhostGramInterface : public Section<GhostGramInterface> {
public:
	GhostGramInterface(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 6. Messages
class GhostGramMessages : public Section<GhostGramMessages> {
public:
	GhostGramMessages(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 7. Privacy
class GhostGramPrivacy : public Section<GhostGramPrivacy> {
public:
	GhostGramPrivacy(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 8. Media
class GhostGramMedia : public Section<GhostGramMedia> {
public:
	GhostGramMedia(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 9. Notifications
class GhostGramNotifications : public Section<GhostGramNotifications> {
public:
	GhostGramNotifications(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 10. Tools
class GhostGramTools : public Section<GhostGramTools> {
public:
	GhostGramTools(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 11. Advanced
class GhostGramAdvanced : public Section<GhostGramAdvanced> {
public:
	GhostGramAdvanced(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 12. About
class GhostGramAbout : public Section<GhostGramAbout> {
public:
	GhostGramAbout(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// Root GhostGram section
class GhostGramRoot : public Section<GhostGramRoot> {
public:
	GhostGramRoot(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

void BuildOverviewCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_overview_desc());

	builder.addSubsectionTitle(rpl::single(u"Быстрый статус модулей"_q));

	AddSettingToggle(
		builder,
		u"overview/ghost_toggle"_q,
		tr::lng_ghostgram_ghost_read_receipts(),
		GhostGram::Settings::GhostReadReceipts,
		GhostGram::Settings::SetGhostReadReceipts,
		{ u"ghost"_q, u"stealth"_q });

	AddSettingToggle(
		builder,
		u"overview/streamer_toggle"_q,
		tr::lng_ghostgram_streamer_mode(),
		GhostGram::Settings::StreamerMode,
		GhostGram::Settings::SetStreamerMode,
		{ u"streamer"_q });

	AddSettingToggle(
		builder,
		u"overview/anti_recall_toggle"_q,
		tr::lng_ghostgram_anti_recall(),
		GhostGram::Settings::AntiRecallEnabled,
		GhostGram::Settings::SetAntiRecallEnabled,
		{ u"anti-recall"_q, u"messages"_q });

	builder.addDivider();
	builder.addSkip();

	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_ghost(),
		.targetSection = GhostGramGhost::Id(),
		.icon = { &st::menuIconStealth },
		.keywords = { u"ghost"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_accounts(),
		.targetSection = GhostGramAccounts::Id(),
		.icon = { &st::menuIconAddAccount },
		.keywords = { u"accounts"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_interface(),
		.targetSection = GhostGramInterface::Id(),
		.icon = { &st::menuIconCustomize },
		.keywords = { u"themes"_q },
	});
}

void BuildGeneralCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_general_desc());

	builder.addSubsectionTitle(rpl::single(u"Интерфейс диалогов"_q));
	AddSettingToggle(
		builder,
		u"general/compact_chats"_q,
		tr::lng_ghostgram_compact_chats(),
		GhostGram::Settings::CompactChatList,
		GhostGram::Settings::SetCompactChatList,
		{ u"compact"_q, u"chats"_q });
	builder.addDividerText(tr::lng_ghostgram_compact_chats_about());

	builder.addSubsectionTitle(rpl::single(u"Фильтрация контента"_q));
	AddSettingToggle(
		builder,
		u"general/hide_sponsored"_q,
		tr::lng_ghostgram_hide_sponsored(),
		GhostGram::Settings::HideSponsored,
		GhostGram::Settings::SetHideSponsored,
		{ u"ads"_q, u"sponsored"_q });
	builder.addDividerText(tr::lng_ghostgram_hide_sponsored_about());

	AddSettingToggle(
		builder,
		u"general/block_telemetry"_q,
		tr::lng_ghostgram_privacy_block_telemetry(),
		GhostGram::Settings::BlockTelemetry,
		GhostGram::Settings::SetBlockTelemetry,
		{ u"telemetry"_q, u"privacy"_q });
	builder.addDividerText(tr::lng_ghostgram_privacy_block_telemetry_about());
}

void BuildGhostCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_ghost_desc());

	builder.addSubsectionTitle(rpl::single(u"Приватность сообщений"_q));
	AddSettingToggle(
		builder,
		u"ghost/read_receipts"_q,
		tr::lng_ghostgram_ghost_read_receipts(),
		GhostGram::Settings::GhostReadReceipts,
		GhostGram::Settings::SetGhostReadReceipts,
		{ u"read"_q, u"receipts"_q, u"stealth"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_read_receipts_about());

	AddSettingToggle(
		builder,
		u"ghost/typing"_q,
		tr::lng_ghostgram_ghost_typing(),
		GhostGram::Settings::GhostTyping,
		GhostGram::Settings::SetGhostTyping,
		{ u"typing"_q, u"status"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_typing_about());

	builder.addSubsectionTitle(rpl::single(u"Приватность историй и статуса"_q));
	AddSettingToggle(
		builder,
		u"ghost/stories"_q,
		tr::lng_ghostgram_ghost_stories(),
		GhostGram::Settings::GhostStories,
		GhostGram::Settings::SetGhostStories,
		{ u"stories"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_stories_about());

	AddSettingToggle(
		builder,
		u"ghost/offline"_q,
		tr::lng_ghostgram_ghost_offline(),
		GhostGram::Settings::GhostOffline,
		GhostGram::Settings::SetGhostOffline,
		{ u"offline"_q, u"status"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_offline_about());
}

void BuildAccountsCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_accounts_desc());

	builder.addSubsectionTitle(rpl::single(u"Управление аккаунтами"_q));

	AddSettingToggle(
		builder,
		u"accounts/fast_switch"_q,
		tr::lng_ghostgram_accounts_fast_switch(),
		GhostGram::Settings::FastAccountSwitch,
		GhostGram::Settings::SetFastAccountSwitch,
		{ u"switch"_q, u"accounts"_q });
	builder.addDividerText(tr::lng_ghostgram_accounts_fast_switch_about());

	builder.addSubsectionTitle(rpl::single(u"Лимит активных подключений"_q));

	const auto limits = { 6, 10, 15, 20 };
	for (const auto limit : limits) {
		builder.addButton({
			.id = u"accounts/limit_"_q + QString::number(limit),
			.title = rpl::single(QString::number(limit) + u" аккаунтов"_q),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::MaxAccountsValue()
				| rpl::map([=](int cur) {
					return (cur == limit) ? u"(Активно)"_q : QString();
				}),
			.onClick = [=] {
				GhostGram::Settings::SetMaxAccounts(limit);
			},
			.keywords = { u"limit"_q, u"accounts"_q },
		});
	}
	builder.addDividerText(tr::lng_ghostgram_accounts_limit_about());
}

void BuildInterfaceCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_interface_desc());

	builder.addSubsectionTitle(rpl::single(u"Цветовые темы GhostGram"_q));

	struct ThemeOption {
		GhostGram::Settings::ThemePreset preset;
		rpl::producer<QString> title;
	};

	const auto options = std::vector<ThemeOption>{
		{ GhostGram::Settings::ThemePreset::Default, rpl::single(u"По умолчанию"_q) },
		{ GhostGram::Settings::ThemePreset::Obsidian, tr::lng_ghostgram_theme_obsidian() },
		{ GhostGram::Settings::ThemePreset::Phantom, tr::lng_ghostgram_theme_phantom() },
		{ GhostGram::Settings::ThemePreset::Midnight, tr::lng_ghostgram_theme_midnight() },
		{ GhostGram::Settings::ThemePreset::Violet, tr::lng_ghostgram_theme_violet() },
		{ GhostGram::Settings::ThemePreset::Arctic, tr::lng_ghostgram_theme_arctic() },
	};

	for (auto &opt : options) {
		const auto preset = opt.preset;
		builder.addButton({
			.id = u"theme/preset_"_q + QString::number(static_cast<int>(preset)),
			.title = std::move(opt.title),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::ThemePresetValue()
				| rpl::map([=](GhostGram::Settings::ThemePreset cur) {
					return (cur == preset) ? u"(Выбрана)"_q : QString();
				}),
			.onClick = [=] {
				GhostGram::Settings::SetThemePreset(preset);
			},
			.keywords = { u"theme"_q, u"color"_q },
		});
	}

	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(rpl::single(u"Дополнительное отображение"_q));
	AddSettingToggle(
		builder,
		u"interface/show_peer_id"_q,
		tr::lng_ghostgram_show_peer_id(),
		GhostGram::Settings::ShowPeerId,
		GhostGram::Settings::SetShowPeerId,
		{ u"peer_id"_q, u"id"_q });
	builder.addDividerText(tr::lng_ghostgram_show_peer_id_about());
}

void BuildMessagesCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_messages_desc());

	builder.addSubsectionTitle(rpl::single(u"Локальная история и Anti-recall"_q));

	AddSettingToggle(
		builder,
		u"messages/anti_recall"_q,
		tr::lng_ghostgram_anti_recall(),
		GhostGram::Settings::AntiRecallEnabled,
		GhostGram::Settings::SetAntiRecallEnabled,
		{ u"anti-recall"_q, u"deleted"_q });
	builder.addDividerText(tr::lng_ghostgram_anti_recall_about());

	AddSettingToggle(
		builder,
		u"messages/anti_recall_mark"_q,
		tr::lng_ghostgram_anti_recall_mark(),
		GhostGram::Settings::AntiRecallShowMark,
		GhostGram::Settings::SetAntiRecallShowMark,
		{ u"mark"_q, u"deleted"_q });
	builder.addDividerText(tr::lng_ghostgram_anti_recall_mark_about());

	AddSettingToggle(
		builder,
		u"messages/edit_history"_q,
		tr::lng_ghostgram_edit_history(),
		GhostGram::Settings::EditHistoryEnabled,
		GhostGram::Settings::SetEditHistoryEnabled,
		{ u"edit"_q, u"history"_q });
	builder.addDividerText(tr::lng_ghostgram_edit_history_about());

	builder.addSubsectionTitle(rpl::single(u"Срок хранения локальной истории"_q));
	const auto retentionOptions = { 7, 30, 90, 0 };
	for (const auto days : retentionOptions) {
		const auto title = (days == 0)
			? u"Без ограничений"_q
			: (QString::number(days) + u" дней"_q);

		builder.addButton({
			.id = u"messages/retention_"_q + QString::number(days),
			.title = rpl::single(title),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::HistoryRetentionDaysValue()
				| rpl::map([=](int cur) {
					return (cur == days) ? u"(Активно)"_q : QString();
				}),
			.onClick = [=] {
				GhostGram::Settings::SetHistoryRetentionDays(days);
			},
			.keywords = { u"retention"_q, u"days"_q },
		});
	}

	builder.addSubsectionTitle(rpl::single(u"База сохранённых сообщений"_q));

	builder.addButton({
		.id = u"messages/stats"_q,
		.title = rpl::single(u"Статистика локальной базы сообщений"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			const auto deleted = GhostGram::History::TotalDeletedCount();
			const auto edited = GhostGram::History::TotalEditedCount();
			controller->show(Ui::MakeInformBox(
				u"Статистика Anti-recall GhostGram:\n\n"
				u"• Сохранено удалённых сообщений: "_q
				+ QString::number(deleted)
				+ u"\n• Сохранено отредактированных версий: "_q
				+ QString::number(edited)
				+ u"\n\nФайл локальной базы: ghostgram_history.json"_q));
		},
		.keywords = { u"stats"_q, u"deleted"_q, u"history"_q },
	});

	builder.addButton({
		.id = u"messages/clear"_q,
		.title = rpl::single(u"Очистить локальную историю сообщений"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = u"Удалить всю сохранённую локальную историю Anti-recall? Это действие необратимо."_q,
				.confirmed = [=](Fn<void()> close) {
					GhostGram::History::ClearHistory();
					close();
					controller->show(Ui::MakeInformBox(
						u"Локальная история сообщений успешно очищена."_q));
				},
				.confirmText = u"Очистить"_q,
			}));
		},
		.keywords = { u"clear"_q, u"delete"_q, u"history"_q },
	});
}

void BuildPrivacyCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_privacy_desc());

	builder.addSubsectionTitle(rpl::single(u"Приватность при общении"_q));

	AddSettingToggle(
		builder,
		u"privacy/forward_clean"_q,
		tr::lng_ghostgram_privacy_forward_clean(),
		GhostGram::Settings::ForwardWithoutQuote,
		GhostGram::Settings::SetForwardWithoutQuote,
		{ u"forward"_q, u"quote"_q });
	builder.addDividerText(tr::lng_ghostgram_privacy_forward_clean_about());

	AddSettingToggle(
		builder,
		u"privacy/block_telemetry"_q,
		tr::lng_ghostgram_privacy_block_telemetry(),
		GhostGram::Settings::BlockTelemetry,
		GhostGram::Settings::SetBlockTelemetry,
		{ u"telemetry"_q });
	builder.addDividerText(tr::lng_ghostgram_privacy_block_telemetry_about());

	builder.addDivider();
	builder.addSkip();

	builder.addButton({
		.id = u"privacy/telegram_native"_q,
		.title = tr::lng_settings_section_privacy(),
		.icon = { &st::menuIconLock },
		.onClick = [=] {
			controller->showSettings(PrivacySecurityId());
		},
		.keywords = { u"privacy"_q, u"passcode"_q },
	});
}

void BuildMediaCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_media_desc());

	builder.addSubsectionTitle(rpl::single(u"Сохранение медиафайлов"_q));

	AddSettingToggle(
		builder,
		u"media/preserve_deleted"_q,
		tr::lng_ghostgram_media_preserve(),
		GhostGram::Settings::PreserveDeletedMedia,
		GhostGram::Settings::SetPreserveDeletedMedia,
		{ u"media"_q, u"cache"_q });
	builder.addDividerText(tr::lng_ghostgram_media_preserve_about());
}

void BuildNotificationsCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_notifications_desc());

	builder.addSubsectionTitle(rpl::single(u"Специальные уведомления"_q));

	AddSettingToggle(
		builder,
		u"notifications/streamer_hide"_q,
		tr::lng_ghostgram_streamer_hide_previews(),
		GhostGram::Settings::StreamerHidePreviews,
		GhostGram::Settings::SetStreamerHidePreviews,
		{ u"notifications"_q, u"streamer"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_previews_about());

	builder.addDivider();
	builder.addSkip();

	builder.addButton({
		.id = u"notifications/telegram_native"_q,
		.title = tr::lng_settings_section_notify(),
		.icon = { &st::menuIconNotifications },
		.onClick = [=] {
			controller->showSettings(NotificationsId());
		},
		.keywords = { u"alerts"_q, u"sounds"_q },
	});
}

void BuildToolsCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_tools_desc());

	builder.addSubsectionTitle(rpl::single(u"Режим стримера"_q));

	AddSettingToggle(
		builder,
		u"tools/streamer_mode"_q,
		tr::lng_ghostgram_streamer_mode(),
		GhostGram::Settings::StreamerMode,
		GhostGram::Settings::SetStreamerMode,
		{ u"streamer"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_mode_about());

	AddSettingToggle(
		builder,
		u"tools/streamer_hide_names"_q,
		tr::lng_ghostgram_streamer_hide_names(),
		GhostGram::Settings::StreamerHideNames,
		GhostGram::Settings::SetStreamerHideNames,
		{ u"streamer"_q, u"names"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_names_about());

	AddSettingToggle(
		builder,
		u"tools/streamer_hide_avatars"_q,
		tr::lng_ghostgram_streamer_hide_avatars(),
		GhostGram::Settings::StreamerHideAvatars,
		GhostGram::Settings::SetStreamerHideAvatars,
		{ u"streamer"_q, u"avatars"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_avatars_about());

	AddSettingToggle(
		builder,
		u"tools/streamer_censor_phones"_q,
		tr::lng_ghostgram_streamer_censor_phones(),
		GhostGram::Settings::StreamerCensorPhones,
		GhostGram::Settings::SetStreamerCensorPhones,
		{ u"streamer"_q, u"phones"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_censor_phones_about());

	builder.addSubsectionTitle(rpl::single(u"Локальные расширения"_q));

	AddSettingToggle(
		builder,
		u"tools/bookmarks"_q,
		tr::lng_ghostgram_tools_bookmarks(),
		GhostGram::Settings::BookmarksEnabled,
		GhostGram::Settings::SetBookmarksEnabled,
		{ u"bookmarks"_q });
	builder.addDividerText(tr::lng_ghostgram_tools_bookmarks_about());

	builder.addButton({
		.id = u"tools/bookmarks_stats"_q,
		.title = rpl::single(u"Сохранённые закладки GhostGram"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			const auto count = GhostGram::History::TotalBookmarksCount();
			controller->show(Ui::MakeInformBox(
				u"Локальные закладки GhostGram:\n\n"
				u"• Сохранено закладок: "_q
				+ QString::number(count)
				+ u"\n\nВы можете добавлять сообщения в закладки через контекстное меню любого сообщения."_q));
		},
		.keywords = { u"bookmarks"_q, u"stats"_q },
	});

	AddSettingToggle(
		builder,
		u"tools/chat_notes"_q,
		tr::lng_ghostgram_tools_chat_notes(),
		GhostGram::Settings::ChatNotesEnabled,
		GhostGram::Settings::SetChatNotesEnabled,
		{ u"notes"_q });
	builder.addDividerText(tr::lng_ghostgram_tools_chat_notes_about());
}

void BuildAdvancedCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_advanced_desc());

	builder.addSubsectionTitle(rpl::single(u"Управление конфигурацией"_q));

	builder.addButton({
		.id = u"advanced/export"_q,
		.title = rpl::single(u"Экспорт настроек GhostGram"_q),
		.icon = { &st::menuIconExport },
		.onClick = [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_settings.json"_q;
			auto file = QFile(path);
			if (file.open(QIODevice::WriteOnly)) {
				file.write(GhostGram::Settings::ExportSettingsJson());
				file.close();
				controller->show(Ui::MakeInformBox(
					u"Настройки успешно экспортированы в:\n"_q + path));
			} else {
				controller->show(Ui::MakeInformBox(
					u"Не удалось записать файл настроек."_q));
			}
		},
		.keywords = { u"export"_q, u"backup"_q },
	});

	builder.addButton({
		.id = u"advanced/import"_q,
		.title = rpl::single(u"Импорт настроек GhostGram"_q),
		.icon = { &st::menuIconRestore },
		.onClick = [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_settings.json"_q;
			auto file = QFile(path);
			if (file.open(QIODevice::ReadOnly)) {
				const auto ok = GhostGram::Settings::ImportSettingsJson(
					file.readAll());
				file.close();
				if (ok) {
					controller->show(Ui::MakeInformBox(
						u"Настройки GhostGram успешно импортированы."_q));
				} else {
					controller->show(Ui::MakeInformBox(
						u"Ошибка формата JSON в файле настроек."_q));
				}
			} else {
				controller->show(Ui::MakeInformBox(
					u"Файл не найден по пути:\n"_q + path));
			}
		},
		.keywords = { u"import"_q, u"restore"_q },
	});

	builder.addButton({
		.id = u"advanced/reset"_q,
		.title = rpl::single(u"Сбросить настройки GhostGram по умолчанию"_q),
		.icon = { &st::menuIconClear },
		.onClick = [=] {
			GhostGram::Settings::ResetToDefaults();
			controller->show(Ui::MakeInformBox(
				u"Настройки GhostGram сброшены на начальные."_q));
		},
		.keywords = { u"reset"_q, u"defaults"_q },
	});

	builder.addDivider();
	builder.addSkip();
	builder.addDividerText(rpl::single(
		u"Каталог данных клиента: %APPDATA%\\GhostGram Desktop"_q));
}

void BuildAboutCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_about_page_desc());

	builder.addSubsectionTitle(rpl::single(u"GhostGram Desktop"_q));
	builder.addDividerText(rpl::single(
		u"Версия: 7.3.1-gg (Windows x64)\n"
		u"Ветка: ghostgram\n"
		u"Форк: https://github.com/egormyxa21-gif/tdesktop\n"
		u"Базовая основа: Telegram Desktop 7.3.1\n"
		u"Лицензия: GNU GPLv3 с исключением OpenSSL\n\n"
		u"GhostGram является модифицированным клиентом для опытных пользователей. "
		u"Все настройки и данные хранятся изолированно в профиле GhostGram Desktop."_q));
}

// Meta definitions for all 12 categories
const auto kOverviewMeta = BuildHelper({
	.id = GhostGramOverview::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_overview,
	.icon = &st::menuIconInfo,
}, BuildOverviewCategory);

const auto kGeneralMeta = BuildHelper({
	.id = GhostGramGeneral::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_general,
	.icon = &st::menuIconSettings,
}, BuildGeneralCategory);

const auto kGhostMeta = BuildHelper({
	.id = GhostGramGhost::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_ghost,
	.icon = &st::menuIconStealth,
}, BuildGhostCategory);

const auto kAccountsMeta = BuildHelper({
	.id = GhostGramAccounts::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_accounts,
	.icon = &st::menuIconAddAccount,
}, BuildAccountsCategory);

const auto kInterfaceMeta = BuildHelper({
	.id = GhostGramInterface::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_interface,
	.icon = &st::menuIconCustomize,
}, BuildInterfaceCategory);

const auto kMessagesMeta = BuildHelper({
	.id = GhostGramMessages::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_messages,
	.icon = &st::menuIconSavedMessages,
}, BuildMessagesCategory);

const auto kPrivacyMeta = BuildHelper({
	.id = GhostGramPrivacy::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_privacy,
	.icon = &st::menuIconLock,
}, BuildPrivacyCategory);

const auto kMediaMeta = BuildHelper({
	.id = GhostGramMedia::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_media,
	.icon = &st::menuIconDownload,
}, BuildMediaCategory);

const auto kNotificationsMeta = BuildHelper({
	.id = GhostGramNotifications::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_notifications,
	.icon = &st::menuIconNotifications,
}, BuildNotificationsCategory);

const auto kToolsMeta = BuildHelper({
	.id = GhostGramTools::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_tools,
	.icon = &st::menuIconExperimental,
}, BuildToolsCategory);

const auto kAdvancedMeta = BuildHelper({
	.id = GhostGramAdvanced::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_advanced,
	.icon = &st::menuIconManage,
}, BuildAdvancedCategory);

const auto kAboutMeta = BuildHelper({
	.id = GhostGramAbout::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_about_page,
	.icon = &st::menuIconStealth,
}, BuildAboutCategory);

const auto kMeta = BuildHelper({
	.id = GhostGramRoot::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_ghostgram,
	.icon = &st::menuIconStealth,
}, [](SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_about());

	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_overview(),
		.targetSection = GhostGramOverview::Id(),
		.icon = { &st::menuIconInfo },
		.keywords = { u"overview"_q, u"status"_q, u"ghostgram"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_general(),
		.targetSection = GhostGramGeneral::Id(),
		.icon = { &st::menuIconSettings },
		.keywords = { u"general"_q, u"main"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_ghost(),
		.targetSection = GhostGramGhost::Id(),
		.icon = { &st::menuIconStealth },
		.keywords = { u"ghost"_q, u"stealth"_q, u"read"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_accounts(),
		.targetSection = GhostGramAccounts::Id(),
		.icon = { &st::menuIconAddAccount },
		.keywords = { u"accounts"_q, u"multi"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_interface(),
		.targetSection = GhostGramInterface::Id(),
		.icon = { &st::menuIconCustomize },
		.keywords = { u"interface"_q, u"themes"_q, u"ui"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_messages(),
		.targetSection = GhostGramMessages::Id(),
		.icon = { &st::menuIconSavedMessages },
		.keywords = { u"messages"_q, u"history"_q, u"anti-recall"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_privacy(),
		.targetSection = GhostGramPrivacy::Id(),
		.icon = { &st::menuIconLock },
		.keywords = { u"privacy"_q, u"security"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_media(),
		.targetSection = GhostGramMedia::Id(),
		.icon = { &st::menuIconDownload },
		.keywords = { u"media"_q, u"downloads"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_notifications(),
		.targetSection = GhostGramNotifications::Id(),
		.icon = { &st::menuIconNotifications },
		.keywords = { u"notifications"_q, u"alerts"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_tools(),
		.targetSection = GhostGramTools::Id(),
		.icon = { &st::menuIconExperimental },
		.keywords = { u"tools"_q, u"streamer"_q, u"bookmarks"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_advanced(),
		.targetSection = GhostGramAdvanced::Id(),
		.icon = { &st::menuIconManage },
		.keywords = { u"advanced"_q, u"export"_q, u"import"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_about_page(),
		.targetSection = GhostGramAbout::Id(),
		.icon = { &st::menuIconStealth },
		.keywords = { u"about"_q, u"version"_q },
	});
});

const SectionBuildMethod kOverviewSection = kOverviewMeta.build;
const SectionBuildMethod kGeneralSection = kGeneralMeta.build;
const SectionBuildMethod kGhostSection = kGhostMeta.build;
const SectionBuildMethod kAccountsSection = kAccountsMeta.build;
const SectionBuildMethod kInterfaceSection = kInterfaceMeta.build;
const SectionBuildMethod kMessagesSection = kMessagesMeta.build;
const SectionBuildMethod kPrivacySection = kPrivacyMeta.build;
const SectionBuildMethod kMediaSection = kMediaMeta.build;
const SectionBuildMethod kNotificationsSection = kNotificationsMeta.build;
const SectionBuildMethod kToolsSection = kToolsMeta.build;
const SectionBuildMethod kAdvancedSection = kAdvancedMeta.build;
const SectionBuildMethod kAboutSection = kAboutMeta.build;
const SectionBuildMethod kGhostGramSection = kMeta.build;

// Section constructors and setupContent
GhostGramOverview::GhostGramOverview(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramOverview::title() {
	return tr::lng_settings_ghostgram_overview();
}

void GhostGramOverview::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kOverviewSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramGeneral::GhostGramGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramGeneral::title() {
	return tr::lng_settings_ghostgram_general();
}

void GhostGramGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kGeneralSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramGhost::GhostGramGhost(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramGhost::title() {
	return tr::lng_settings_ghostgram_ghost();
}

void GhostGramGhost::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kGhostSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramAccounts::GhostGramAccounts(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramAccounts::title() {
	return tr::lng_settings_ghostgram_accounts();
}

void GhostGramAccounts::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAccountsSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramInterface::GhostGramInterface(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramInterface::title() {
	return tr::lng_settings_ghostgram_interface();
}

void GhostGramInterface::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kInterfaceSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramMessages::GhostGramMessages(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramMessages::title() {
	return tr::lng_settings_ghostgram_messages();
}

void GhostGramMessages::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMessagesSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramPrivacy::GhostGramPrivacy(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramPrivacy::title() {
	return tr::lng_settings_ghostgram_privacy();
}

void GhostGramPrivacy::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kPrivacySection);
	Ui::ResizeFitChild(this, content);
}

GhostGramMedia::GhostGramMedia(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramMedia::title() {
	return tr::lng_settings_ghostgram_media();
}

void GhostGramMedia::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMediaSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramNotifications::GhostGramNotifications(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramNotifications::title() {
	return tr::lng_settings_ghostgram_notifications();
}

void GhostGramNotifications::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kNotificationsSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramTools::GhostGramTools(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramTools::title() {
	return tr::lng_settings_ghostgram_tools();
}

void GhostGramTools::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kToolsSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramAdvanced::GhostGramAdvanced(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramAdvanced::title() {
	return tr::lng_settings_ghostgram_advanced();
}

void GhostGramAdvanced::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAdvancedSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramAbout::GhostGramAbout(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramAbout::title() {
	return tr::lng_settings_ghostgram_about_page();
}

void GhostGramAbout::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAboutSection);
	Ui::ResizeFitChild(this, content);
}

GhostGramRoot::GhostGramRoot(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramRoot::title() {
	return tr::lng_settings_ghostgram();
}

void GhostGramRoot::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kGhostGramSection);
	Ui::ResizeFitChild(this, content);
}

} // namespace

Type GhostGramId() {
	return GhostGramRoot::Id();
}

Type GhostGramOverviewId() {
	return GhostGramOverview::Id();
}

Type GhostGramGeneralId() {
	return GhostGramGeneral::Id();
}

Type GhostGramGhostId() {
	return GhostGramGhost::Id();
}

Type GhostGramAccountsId() {
	return GhostGramAccounts::Id();
}

Type GhostGramInterfaceId() {
	return GhostGramInterface::Id();
}

Type GhostGramMessagesId() {
	return GhostGramMessages::Id();
}

Type GhostGramPrivacyId() {
	return GhostGramPrivacy::Id();
}

Type GhostGramMediaId() {
	return GhostGramMedia::Id();
}

Type GhostGramNotificationsId() {
	return GhostGramNotifications::Id();
}

Type GhostGramToolsId() {
	return GhostGramTools::Id();
}

Type GhostGramAdvancedId() {
	return GhostGramAdvanced::Id();
}

Type GhostGramAboutId() {
	return GhostGramAbout::Id();
}

} // namespace Settings
