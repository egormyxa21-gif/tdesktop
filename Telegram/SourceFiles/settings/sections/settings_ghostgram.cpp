/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_ghostgram.h"

#include "ghostgram/ghostgram_settings.h"
#include "ghostgram/ghostgram_history.h"
#include "ghostgram/ghostgram_theme.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "settings/sections/settings_privacy_security.h"
#include "settings/sections/settings_notifications.h"
#include "settings/settings_builder.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/color_editor.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QStandardPaths>

#include <array>

namespace Settings {
namespace {

using namespace Builder;
namespace Theme = GhostGram::Theme;

void AddSettingToggle(
		SectionBuilder &builder,
		QString id,
		rpl::producer<QString> title,
		rpl::producer<bool> value,
		Fn<void(bool)> setter,
		QStringList keywords = {}) {
	const auto button = builder.addButton({
		.id = std::move(id),
		.title = std::move(title),
		.st = &st::settingsButtonNoIcon,
		.toggled = std::move(value),
		.keywords = std::move(keywords),
	});
	if (button) {
		button->toggledValue()
		| rpl::on_next(setter, button->lifetime());
	}
}

Theme::Spec PaletteSpec() {
	auto danger = st::botKbDangerBg->c;
	danger.setAlpha(255);
	auto success = st::botKbSuccessBg->c;
	success.setAlpha(255);
	return Theme::Spec{
		.id = u"palette"_q,
		.dark = (st::windowBg->c.lightness() < 128),
		.bg = st::windowBg->c,
		.accent = st::windowBgActive->c,
		.msgIn = st::msgInBg->c,
		.msgOut = st::msgOutBg->c,
		.text = st::windowFg->c,
		.textSubtle = st::windowSubTextFg->c,
		.danger = danger,
		.success = success,
	};
}

Theme::Spec CurrentSpec() {
	auto spec = Theme::ParseSpec(
		GhostGram::Settings::CurrentCustomTheme());
	if (!spec) {
		spec = Theme::PresetSpec(
			GhostGram::Settings::CurrentThemePreset());
	}
	if (!spec) {
		spec = PaletteSpec();
	}
	return GhostGram::Settings::AutoContrast()
		? Theme::EnsureReadableSpec(*spec)
		: *spec;
}

rpl::producer<Theme::Spec> ActiveSpec() {
	return rpl::combine(
			GhostGram::Settings::ThemePresetValue(),
			GhostGram::Settings::CustomThemeValue(),
			GhostGram::Settings::AutoContrastValue())
	| rpl::map([](
			GhostGram::Settings::ThemePreset preset,
			const QByteArray &custom,
			bool autoContrast) {
		auto spec = Theme::ParseSpec(custom);
		if (!spec) {
			spec = Theme::PresetSpec(preset);
		}
		if (!spec) {
			spec = PaletteSpec();
		}
		return autoContrast
			? Theme::EnsureReadableSpec(*spec)
			: *spec;
	});
}

QColor SpecColor(const Theme::Spec &spec, const QString &field) {
	if (field == u"accent"_q) {
		return spec.accent;
	} else if (field == u"msgIn"_q) {
		return spec.msgIn;
	} else if (field == u"msgOut"_q) {
		return spec.msgOut;
	} else if (field == u"text"_q) {
		return spec.text;
	} else if (field == u"textSubtle"_q) {
		return spec.textSubtle;
	} else if (field == u"danger"_q) {
		return spec.danger;
	} else if (field == u"success"_q) {
		return spec.success;
	}
	return spec.bg;
}

bool SavedSlotExists(const QByteArray &json, int index) {
	const auto array = QJsonDocument::fromJson(json).array();
	return (index >= 0 && index < array.size());
}

QByteArray SavedSlotAt(const QByteArray &json, int index) {
	const auto array = QJsonDocument::fromJson(json).array();
	return QJsonDocument(
		array.at(index).toObject()).toJson(QJsonDocument::Compact);
}

constexpr auto kListedItems = 100;

void ShowBookmarksBox(not_null<Window::SessionController*> controller) {
	const auto items = GhostGram::History::GetBookmarks();
	const auto exportData = GhostGram::History::ExportJson();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_ghostgram_ui_zakladki_ghostgram());
		if (items.empty()) {
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				rpl::single(
					tr::lng_ghostgram_ui_zakladok_poka_net_dobavlyayte_so(tr::now)),
				st::boxLabel));
		}
		auto index = 0;
		for (const auto &bm : items) {
			if (index++ == kListedItems) {
				box->addRow(object_ptr<Ui::FlatLabel>(
					box,
					rpl::single(
						tr::lng_ghostgram_ui_i_esche_1(tr::now)
						+ QString::number(items.size() - kListedItems)),
					st::boxLabel));
				break;
			}
			const auto button = Ui::CreateChild<Ui::SettingsButton>(
				box,
				rpl::single(bm.text),
				st::settingsButton);
			button->setClickedCallback([=] {
				box->closeBox();
				controller->showPeerHistory(
					bm.fullId.peer,
					SectionShow::Way::ClearStack,
					bm.fullId.msg);
			});
			box->addRow(object_ptr<Ui::SettingsButton>(button));
		}
		if (!items.empty()) {
			box->addButton(tr::lng_ghostgram_ui_udalit_vse(), [=] {
				controller->show(Ui::MakeConfirmBox({
					.text = tr::lng_ghostgram_ui_udalit_vse_zakladki_ghostgram(tr::now),
					.confirmed = [=](Fn<void()> close) {
						GhostGram::History::RemoveAllBookmarks();
						box->closeBox();
						close();
					},
					.confirmText = tr::lng_ghostgram_ui_udalit(tr::now),
				}));
			});
		}
		box->addButton(tr::lng_ghostgram_ui_eksport_v_json(), [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_history.json"_q;
			auto file = QFile(path);
			if (file.open(QIODevice::WriteOnly)) {
				file.write(exportData);
				file.close();
			}
			box->closeBox();
			controller->show(Ui::MakeInformBox(
				tr::lng_ghostgram_ui_file_saved_to(tr::now) + path));
		});
		box->addButton(tr::lng_ghostgram_ui_zakryt(), [=] {
			box->closeBox();
		});
	}));
}

void ShowSavedMessagesBox(not_null<Window::SessionController*> controller) {
	const auto deleted = GhostGram::History::GetDeletedMessages();
	const auto edited = GhostGram::History::GetEditedMessages();
	const auto exportData = GhostGram::History::ExportJson();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_ghostgram_ui_sokhranennaya_istoriya_ghostgram());
		if (deleted.empty() && edited.empty()) {
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				rpl::single(
					tr::lng_ghostgram_ui_sokhranennykh_soobscheniy_poka_n(tr::now)),
				st::boxLabel));
		}
		auto listed = 0;
		for (const auto &msg : deleted) {
			if (listed == kListedItems) {
				break;
			}
			++listed;
			const auto title = tr::lng_ghostgram_ui_udaleno_1_2(tr::now)
				.arg(msg.chatTitle)
				.arg(msg.text.left(80));
			const auto button = Ui::CreateChild<Ui::SettingsButton>(
				box,
				rpl::single(title),
				st::settingsButton);
			const auto fullId = msg.fullId;
			button->setClickedCallback([=] {
				box->closeBox();
				controller->showPeerHistory(
					fullId.peer,
					SectionShow::Way::ClearStack,
					fullId.msg);
			});
			box->addRow(object_ptr<Ui::SettingsButton>(button));
		}
		for (const auto &msg : edited) {
			if (listed == kListedItems) {
				break;
			}
			++listed;
			const auto title = tr::lng_ghostgram_ui_pravka_1_2(tr::now)
				.arg(msg.chatTitle)
				.arg(msg.previousText.left(40)
					+ u" → %1"_q.arg(msg.text.left(40)));
			const auto button = Ui::CreateChild<Ui::SettingsButton>(
				box,
				rpl::single(title),
				st::settingsButton);
			const auto fullId = msg.fullId;
			button->setClickedCallback([=] {
				box->closeBox();
				controller->showPeerHistory(
					fullId.peer,
					SectionShow::Way::ClearStack,
					fullId.msg);
			});
			box->addRow(object_ptr<Ui::SettingsButton>(button));
		}
		box->addButton(tr::lng_ghostgram_ui_eksport_v_json(), [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_history.json"_q;
			auto file = QFile(path);
			if (file.open(QIODevice::WriteOnly)) {
				file.write(exportData);
				file.close();
			}
			box->closeBox();
			controller->show(Ui::MakeInformBox(
				tr::lng_ghostgram_ui_file_saved_to(tr::now) + path));
		});
		box->addButton(tr::lng_ghostgram_ui_zakryt(), [=] {
			box->closeBox();
		});
	}));
}

class ThemePreview : public Ui::RpWidget {
public:
	explicit ThemePreview(not_null<Ui::VerticalLayout*> parent)
	: Ui::RpWidget(parent) {
		setFixedHeight(st::settingsButton.height);
	}

	void setSpec(const Theme::Spec &spec) {
		_colors = {
			spec.bg,
			spec.accent,
			spec.msgIn,
			spec.msgOut,
			spec.text,
			spec.textSubtle,
			spec.danger,
			spec.success,
		};
		update();
	}

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	std::array<QColor, 8> _colors;

};

void ThemePreview::paintEvent(QPaintEvent *e) {
	Painter p(this);
	const auto gap = st::settingsButton.padding.left();
	const auto count = int(_colors.size());
	const auto cell = (width() - gap * (count - 1)) / count;
	for (auto i = 0; i != count; ++i) {
		const auto rect = QRect(
			i * (cell + gap),
			0,
			cell,
			height());
		p.setPen(st::windowSubTextFg);
		p.setBrush(_colors[i]);
		p.drawRoundedRect(rect, st::boxRadius, st::boxRadius);
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

// 10. Streamer
class GhostGramStreamer : public Section<GhostGramStreamer> {
public:
	GhostGramStreamer(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 11. Tools
class GhostGramTools : public Section<GhostGramTools> {
public:
	GhostGramTools(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 12. Backup
class GhostGramBackup : public Section<GhostGramBackup> {
public:
	GhostGramBackup(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 13. Advanced
class GhostGramAdvanced : public Section<GhostGramAdvanced> {
public:
	GhostGramAdvanced(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

// 14. About
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

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_bystryy_status_moduley());

	AddSettingToggle(
		builder,
		u"overview/ghost_toggle"_q,
		tr::lng_ghostgram_ghost_read_receipts(),
		GhostGram::Settings::GhostReadReceiptsValue(),
		GhostGram::Settings::SetGhostReadReceipts,
		{ u"ghost"_q, u"stealth"_q });

	AddSettingToggle(
		builder,
		u"overview/streamer_toggle"_q,
		tr::lng_ghostgram_streamer_mode(),
		GhostGram::Settings::StreamerModeValue(),
		GhostGram::Settings::SetStreamerMode,
		{ u"streamer"_q });

	AddSettingToggle(
		builder,
		u"overview/anti_recall_toggle"_q,
		tr::lng_ghostgram_anti_recall(),
		GhostGram::Settings::AntiRecallEnabledValue(),
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

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_interfeys_dialogov());
	AddSettingToggle(
		builder,
		u"general/compact_chats"_q,
		tr::lng_ghostgram_compact_chats(),
		GhostGram::Settings::CompactChatListValue(),
		GhostGram::Settings::SetCompactChatList,
		{ u"compact"_q, u"chats"_q });
	builder.addDividerText(tr::lng_ghostgram_compact_chats_about());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_filtratsiya_kontenta());
	AddSettingToggle(
		builder,
		u"general/hide_sponsored"_q,
		tr::lng_ghostgram_hide_sponsored(),
		GhostGram::Settings::HideSponsoredValue(),
		GhostGram::Settings::SetHideSponsored,
		{ u"ads"_q, u"sponsored"_q });
	builder.addDividerText(tr::lng_ghostgram_hide_sponsored_about());
}

void BuildGhostCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_ghost_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_privatnost_soobscheniy());
	AddSettingToggle(
		builder,
		u"ghost/read_receipts"_q,
		tr::lng_ghostgram_ghost_read_receipts(),
		GhostGram::Settings::GhostReadReceiptsValue(),
		GhostGram::Settings::SetGhostReadReceipts,
		{ u"read"_q, u"receipts"_q, u"stealth"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_read_receipts_about());

	AddSettingToggle(
		builder,
		u"ghost/typing"_q,
		tr::lng_ghostgram_ghost_typing(),
		GhostGram::Settings::GhostTypingValue(),
		GhostGram::Settings::SetGhostTyping,
		{ u"typing"_q, u"status"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_typing_about());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_privatnost_istoriy_i_statusa());
	AddSettingToggle(
		builder,
		u"ghost/stories"_q,
		tr::lng_ghostgram_ghost_stories(),
		GhostGram::Settings::GhostStoriesValue(),
		GhostGram::Settings::SetGhostStories,
		{ u"stories"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_stories_about());

	AddSettingToggle(
		builder,
		u"ghost/offline"_q,
		tr::lng_ghostgram_ghost_offline(),
		GhostGram::Settings::GhostOfflineValue(),
		GhostGram::Settings::SetGhostOffline,
		{ u"offline"_q, u"status"_q });
	builder.addDividerText(tr::lng_ghostgram_ghost_offline_about());
}

void BuildAccountsCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_accounts_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_upravlenie_akkauntami());

	AddSettingToggle(
		builder,
		u"accounts/fast_switch"_q,
		tr::lng_ghostgram_accounts_fast_switch(),
		GhostGram::Settings::FastAccountSwitchValue(),
		GhostGram::Settings::SetFastAccountSwitch,
		{ u"switch"_q, u"accounts"_q });
	builder.addDividerText(tr::lng_ghostgram_accounts_fast_switch_about());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_limit_aktivnykh_podklyucheniy());

	const auto limits = { 6, 10, 15, 20 };
	for (const auto limit : limits) {
		builder.addButton({
			.id = u"accounts/limit_"_q + QString::number(limit),
			.title = rpl::single(QString::number(limit) + tr::lng_ghostgram_ui_akkauntov(tr::now)),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::MaxAccountsValue()
				| rpl::map([=](int cur) {
					return (cur == limit) ? tr::lng_ghostgram_ui_aktivno(tr::now) : QString();
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
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_interface_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_tsvetovye_temy_ghostgram());

	struct ThemeOption {
		GhostGram::Settings::ThemePreset preset;
		rpl::producer<QString> title;
	};

	const auto options = std::vector<ThemeOption>{
		{ GhostGram::Settings::ThemePreset::Default,
			tr::lng_ghostgram_theme_default() },
		{ GhostGram::Settings::ThemePreset::Obsidian,
			tr::lng_ghostgram_theme_obsidian() },
		{ GhostGram::Settings::ThemePreset::Phantom,
			tr::lng_ghostgram_theme_phantom() },
		{ GhostGram::Settings::ThemePreset::Midnight,
			tr::lng_ghostgram_theme_midnight() },
		{ GhostGram::Settings::ThemePreset::Violet,
			tr::lng_ghostgram_theme_violet() },
		{ GhostGram::Settings::ThemePreset::Arctic,
			tr::lng_ghostgram_theme_arctic() },
		{ GhostGram::Settings::ThemePreset::Emerald,
			tr::lng_ghostgram_theme_emerald() },
		{ GhostGram::Settings::ThemePreset::Crimson,
			tr::lng_ghostgram_theme_crimson() },
		{ GhostGram::Settings::ThemePreset::Sunset,
			tr::lng_ghostgram_theme_sunset() },
		{ GhostGram::Settings::ThemePreset::Graphite,
			tr::lng_ghostgram_theme_graphite() },
		{ GhostGram::Settings::ThemePreset::Ocean,
			tr::lng_ghostgram_theme_ocean() },
		{ GhostGram::Settings::ThemePreset::Nord,
			tr::lng_ghostgram_theme_nord() },
		{ GhostGram::Settings::ThemePreset::Cyberpunk,
			tr::lng_ghostgram_theme_cyberpunk() },
		{ GhostGram::Settings::ThemePreset::Amoled,
			tr::lng_ghostgram_theme_amoled() },
		{ GhostGram::Settings::ThemePreset::Solarized,
			tr::lng_ghostgram_theme_solarized() },
		{ GhostGram::Settings::ThemePreset::Daylight,
			tr::lng_ghostgram_theme_daylight() },
		{ GhostGram::Settings::ThemePreset::Lavender,
			tr::lng_ghostgram_theme_lavender() },
	};

	for (auto &opt : options) {
		const auto preset = opt.preset;
		builder.addButton({
			.id = u"theme/preset_"_q + QString::number(static_cast<int>(preset)),
			.title = std::move(opt.title),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::ThemePresetValue()
				| rpl::map([=](GhostGram::Settings::ThemePreset cur) {
					return (cur == preset) ? tr::lng_ghostgram_ui_vybrana(tr::now) : QString();
				}),
			.onClick = [=] {
				GhostGram::Settings::SetThemePreset(preset);
			},
			.keywords = { u"theme"_q, u"color"_q },
		});
	}

	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_redaktor_temy());

	builder.add([](const WidgetContext &ctx) {
		const auto preview = new ThemePreview(ctx.container);
		ActiveSpec()
		| rpl::on_next([=](const Theme::Spec &spec) {
			preview->setSpec(spec);
		}, preview->lifetime());
		return WidgetToAdd{ .widget = preview };
	});

	struct ColorFieldRow {
		QString field;
		rpl::producer<QString> title;
	};
	const auto colorRows = std::vector<ColorFieldRow>{
		{ u"bg"_q, tr::lng_ghostgram_ui_fon_prilozheniya() },
		{ u"accent"_q, tr::lng_ghostgram_ui_aktsent() },
		{ u"msgIn"_q, tr::lng_ghostgram_ui_vkhodyaschie_soobscheniya() },
		{ u"msgOut"_q, tr::lng_ghostgram_ui_iskhodyaschie_soobscheniya() },
		{ u"text"_q, tr::lng_ghostgram_ui_osnovnoy_tekst() },
		{ u"textSubtle"_q, tr::lng_ghostgram_ui_dopolnitelnyy_tekst() },
		{ u"danger"_q, tr::lng_ghostgram_ui_oshibki_i_opasnye_deystviya() },
		{ u"success"_q, tr::lng_ghostgram_ui_uspekh_i_onlayn() },
	};
	for (auto &row : colorRows) {
		const auto field = row.field;
		builder.addButton({
			.id = u"theme/color_"_q + field,
			.title = std::move(row.title),
			.st = &st::settingsButtonNoIcon,
			.label = ActiveSpec()
				| rpl::map([=](const Theme::Spec &spec) {
					return Theme::ColorHex(SpecColor(spec, field));
				}),
			.onClick = [=] {
				const auto spec = CurrentSpec();
				const auto color = SpecColor(spec, field);
				controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					const auto editor = box->addRow(
						object_ptr<ColorEditor>(
							box,
							ColorEditor::Mode::RGBA,
							color));
					box->setWidth(editor->width());
					box->addButton(tr::lng_box_done(), [=] {
						const auto updated = Theme::SpecWithOverrides(
							spec,
							{ { field, editor->color() } });
						GhostGram::Settings::SetCustomTheme(
							Theme::SerializeSpec(updated));
						box->closeBox();
					});
					box->addButton(tr::lng_cancel(), [=] {
						box->closeBox();
					});
					box->setFocusCallback([=] {
						editor->setInnerFocus();
					});
				});
			},
			.keywords = { u"theme"_q, u"color"_q },
		});
	}

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_chitaemost());

	AddSettingToggle(
		builder,
		u"theme/auto_contrast"_q,
		tr::lng_ghostgram_theme_auto_contrast(),
		GhostGram::Settings::AutoContrastValue(),
		GhostGram::Settings::SetAutoContrast,
		{ u"contrast"_q, u"readable"_q });
	builder.addDividerText(tr::lng_ghostgram_theme_auto_contrast_about());

	builder.addDividerText(
		ActiveSpec()
		| rpl::map([](const Theme::Spec &spec) {
			const auto ratio = Theme::ContrastRatio(
				spec.bg,
				spec.text);
			const auto value = QString::number(ratio, 'f', 1);
			return (ratio >= 4.5)
				? tr::lng_ghostgram_ui_kontrast_teksta_i_fona_1_norma(tr::now).arg(value)
				: tr::lng_ghostgram_ui_kontrast_teksta_i_fona_1_slishko(tr::now).arg(value);
		}));

	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_obmen_temoy());

	builder.addButton({
		.id = u"theme/export"_q,
		.title = tr::lng_ghostgram_ui_eksport_temy_v_json(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_theme.json"_q;
			auto file = QFile(path);
			if (file.open(QIODevice::WriteOnly)) {
				file.write(Theme::SerializeSpec(CurrentSpec()));
				file.close();
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_theme_exported_to(tr::now) + path));
			} else {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_ne_udalos_zapisat_fayl_temy(tr::now)));
			}
		},
		.keywords = { u"export"_q, u"theme"_q },
	});

	builder.addButton({
		.id = u"theme/import"_q,
		.title = tr::lng_ghostgram_ui_import_temy_iz_json(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			const auto path = QStandardPaths::writableLocation(
				QStandardPaths::DocumentsLocation)
				+ u"/ghostgram_theme.json"_q;
			auto file = QFile(path);
			if (!file.open(QIODevice::ReadOnly)) {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_file_not_found_at(tr::now) + path));
				return;
			}
			const auto data = file.readAll();
			file.close();
			if (!Theme::ParseSpec(data)) {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_oshibka_formata_json_v_fayle_tem(tr::now)));
				return;
			}
			GhostGram::Settings::SetCustomTheme(data);
			controller->show(Ui::MakeInformBox(
				tr::lng_ghostgram_ui_tema_uspeshno_importirovana(tr::now)));
		},
		.keywords = { u"import"_q, u"theme"_q },
	});

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_sokhranennye_palitry());

	builder.addButton({
		.id = u"theme/save_current"_q,
		.title = tr::lng_ghostgram_ui_sokhranit_tekuschuyu_temu(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			if (!GhostGram::Settings::AddSavedTheme(
					Theme::SerializeSpec(CurrentSpec()))) {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_net_svobodnykh_slotov_dlya_palit(tr::now)));
			}
		},
		.keywords = { u"save"_q, u"theme"_q },
	});

	constexpr auto kSavedSlots = 8;
	for (auto i = 0; i != kSavedSlots; ++i) {
		const auto index = i;
		builder.addButton({
			.id = u"theme/slot_"_q + QString::number(index),
			.title = rpl::single(tr::lng_ghostgram_ui_palitra_1(tr::now).arg(index + 1)),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::SavedThemesValue()
				| rpl::map([=](const QByteArray &json) {
					return SavedSlotExists(json, index)
						? tr::lng_ghostgram_ui_zapolnena(tr::now)
						: tr::lng_ghostgram_ui_pusto(tr::now);
				}),
			.onClick = [=] {
				const auto json = GhostGram::Settings::SavedThemesJson();
				if (!SavedSlotExists(json, index)) {
					return;
				}
				controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					box->setTitle(tr::lng_ghostgram_ui_sokhranennaya_palitra(tr::now));
					box->addButton(tr::lng_ghostgram_ui_primenit(), [=] {
						GhostGram::Settings::SetCustomTheme(
							SavedSlotAt(json, index));
						box->closeBox();
					});
					box->addButton(tr::lng_ghostgram_ui_udalit(), [=] {
						GhostGram::Settings::RemoveSavedTheme(index);
						box->closeBox();
					});
					box->addButton(tr::lng_cancel(), [=] {
						box->closeBox();
					});
				});
			},
			.keywords = { u"theme"_q, u"saved"_q },
		});
	}

	builder.addButton({
		.id = u"theme/clear_saved"_q,
		.title = tr::lng_ghostgram_ui_udalit_vse_sokhranennye_palitry(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::lng_ghostgram_ui_udalit_vse_sokhranennye_palitry_(tr::now),
				.confirmed = [=](Fn<void()> close) {
					GhostGram::Settings::ClearSavedThemes();
					close();
				},
				.confirmText = tr::lng_ghostgram_ui_udalit(tr::now),
			}));
		},
		.keywords = { u"clear"_q, u"saved"_q },
	});
}

void BuildMessagesCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_messages_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_lokalnaya_istoriya_i_anti_recall());

	AddSettingToggle(
		builder,
		u"messages/anti_recall"_q,
		tr::lng_ghostgram_anti_recall(),
		GhostGram::Settings::AntiRecallEnabledValue(),
		GhostGram::Settings::SetAntiRecallEnabled,
		{ u"anti-recall"_q, u"deleted"_q });
	builder.addDividerText(tr::lng_ghostgram_anti_recall_about());

	AddSettingToggle(
		builder,
		u"messages/anti_recall_mark"_q,
		tr::lng_ghostgram_anti_recall_mark(),
		GhostGram::Settings::AntiRecallShowMarkValue(),
		GhostGram::Settings::SetAntiRecallShowMark,
		{ u"mark"_q, u"deleted"_q });
	builder.addDividerText(tr::lng_ghostgram_anti_recall_mark_about());

	AddSettingToggle(
		builder,
		u"messages/edit_history"_q,
		tr::lng_ghostgram_edit_history(),
		GhostGram::Settings::EditHistoryEnabledValue(),
		GhostGram::Settings::SetEditHistoryEnabled,
		{ u"edit"_q, u"history"_q });
	builder.addDividerText(tr::lng_ghostgram_edit_history_about());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_srok_khraneniya_lokalnoy_istorii());
	const auto retentionOptions = { 7, 30, 90, 0 };
	for (const auto days : retentionOptions) {
		const auto title = (days == 0)
			? tr::lng_ghostgram_ui_bez_ogranicheniy(tr::now)
			: (QString::number(days) + tr::lng_ghostgram_ui_dney(tr::now));

		builder.addButton({
			.id = u"messages/retention_"_q + QString::number(days),
			.title = rpl::single(title),
			.st = &st::settingsButtonNoIcon,
			.label = GhostGram::Settings::HistoryRetentionDaysValue()
				| rpl::map([=](int cur) {
					return (cur == days) ? tr::lng_ghostgram_ui_aktivno(tr::now) : QString();
				}),
			.onClick = [=] {
				GhostGram::Settings::SetHistoryRetentionDays(days);
			},
			.keywords = { u"retention"_q, u"days"_q },
		});
	}

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_baza_sokhranennykh_soobscheniy());

	builder.addButton({
		.id = u"messages/stats"_q,
		.title = tr::lng_ghostgram_ui_statistika_lokalnoy_bazy_soobsch(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			const auto deleted = GhostGram::History::TotalDeletedCount();
			const auto edited = GhostGram::History::TotalEditedCount();
			controller->show(Ui::MakeInformBox(
				tr::lng_ghostgram_ui_stats_header(tr::now)
				+ tr::lng_ghostgram_ui_stats_deleted(tr::now)
				+ QString::number(deleted)
				+ tr::lng_ghostgram_ui_stats_edited(tr::now)
				+ QString::number(edited)
				+ tr::lng_ghostgram_ui_stats_file(tr::now)));
		},
		.keywords = { u"stats"_q, u"deleted"_q, u"history"_q },
	});

	builder.addButton({
		.id = u"messages/view_saved"_q,
		.title = tr::lng_ghostgram_ui_prosmotr_sokhranennoy_istorii(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			ShowSavedMessagesBox(controller);
		},
		.keywords = { u"deleted"_q, u"edited"_q, u"view"_q },
	});

	builder.addButton({
		.id = u"messages/clear"_q,
		.title = tr::lng_ghostgram_ui_ochistit_lokalnuyu_istoriyu_soob(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::lng_ghostgram_ui_udalit_vsyu_sokhranennuyu_lokaln(tr::now),
				.confirmed = [=](Fn<void()> close) {
					GhostGram::History::ClearHistory();
					close();
					controller->show(Ui::MakeInformBox(
						tr::lng_ghostgram_ui_lokalnaya_istoriya_soobscheniy_u(tr::now)));
				},
				.confirmText = tr::lng_ghostgram_ui_ochistit(tr::now),
			}));
		},
		.keywords = { u"clear"_q, u"delete"_q, u"history"_q },
	});
}

void BuildPrivacyCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_privacy_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_privatnost_pri_obschenii());

	AddSettingToggle(
		builder,
		u"privacy/forward_clean"_q,
		tr::lng_ghostgram_privacy_forward_clean(),
		GhostGram::Settings::ForwardWithoutQuoteValue(),
		GhostGram::Settings::SetForwardWithoutQuote,
		{ u"forward"_q, u"quote"_q });
	builder.addDividerText(tr::lng_ghostgram_privacy_forward_clean_about());

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

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_sokhranenie_mediafaylov());

	AddSettingToggle(
		builder,
		u"media/preserve_deleted"_q,
		tr::lng_ghostgram_media_preserve(),
		GhostGram::Settings::PreserveDeletedMediaValue(),
		GhostGram::Settings::SetPreserveDeletedMedia,
		{ u"media"_q, u"cache"_q });
	builder.addDividerText(tr::lng_ghostgram_media_preserve_about());
}

void BuildNotificationsCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_notifications_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_sistemnye_uvedomleniya());

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

void BuildStreamerCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_streamer_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_skrytie_informatsii());

	AddSettingToggle(
		builder,
		u"streamer/mode"_q,
		tr::lng_ghostgram_streamer_mode(),
		GhostGram::Settings::StreamerModeValue(),
		GhostGram::Settings::SetStreamerMode,
		{ u"streamer"_q, u"broadcast"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_mode_about());

	AddSettingToggle(
		builder,
		u"streamer/hide_names"_q,
		tr::lng_ghostgram_streamer_hide_names(),
		GhostGram::Settings::StreamerHideNamesValue(),
		GhostGram::Settings::SetStreamerHideNames,
		{ u"streamer"_q, u"names"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_names_about());

	AddSettingToggle(
		builder,
		u"streamer/hide_avatars"_q,
		tr::lng_ghostgram_streamer_hide_avatars(),
		GhostGram::Settings::StreamerHideAvatarsValue(),
		GhostGram::Settings::SetStreamerHideAvatars,
		{ u"streamer"_q, u"avatars"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_avatars_about());

	AddSettingToggle(
		builder,
		u"streamer/hide_previews"_q,
		tr::lng_ghostgram_streamer_hide_previews(),
		GhostGram::Settings::StreamerHidePreviewsValue(),
		GhostGram::Settings::SetStreamerHidePreviews,
		{ u"streamer"_q, u"previews"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_hide_previews_about());

	AddSettingToggle(
		builder,
		u"streamer/censor_phones"_q,
		tr::lng_ghostgram_streamer_censor_phones(),
		GhostGram::Settings::StreamerCensorPhonesValue(),
		GhostGram::Settings::SetStreamerCensorPhones,
		{ u"streamer"_q, u"phones"_q });
	builder.addDividerText(tr::lng_ghostgram_streamer_censor_phones_about());
}

void BuildToolsCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_tools_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_lokalnye_rasshireniya());

	AddSettingToggle(
		builder,
		u"tools/bookmarks"_q,
		tr::lng_ghostgram_tools_bookmarks(),
		GhostGram::Settings::BookmarksEnabledValue(),
		GhostGram::Settings::SetBookmarksEnabled,
		{ u"bookmarks"_q });
	builder.addDividerText(tr::lng_ghostgram_tools_bookmarks_about());

	builder.addButton({
		.id = u"tools/bookmarks_stats"_q,
		.title = tr::lng_ghostgram_ui_prosmotr_zakladok_ghostgram(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			ShowBookmarksBox(controller);
		},
		.keywords = { u"bookmarks"_q, u"stats"_q },
	});

	AddSettingToggle(
		builder,
		u"tools/chat_notes"_q,
		tr::lng_ghostgram_tools_chat_notes(),
		GhostGram::Settings::ChatNotesEnabledValue(),
		GhostGram::Settings::SetChatNotesEnabled,
		{ u"notes"_q });
	builder.addDividerText(tr::lng_ghostgram_tools_chat_notes_about());
}

void BuildBackupCategory(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDividerText(tr::lng_settings_ghostgram_backup_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_upravlenie_konfiguratsiey());

	builder.addButton({
		.id = u"backup/export"_q,
		.title = tr::lng_ghostgram_ui_eksport_nastroek_ghostgram(),
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
					tr::lng_ghostgram_ui_nastroyki_uspeshno_eksportirovan(tr::now) + path));
			} else {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_ne_udalos_zapisat_fayl_nastroek(tr::now)));
			}
		},
		.keywords = { u"export"_q, u"backup"_q },
	});

	builder.addButton({
		.id = u"backup/import"_q,
		.title = tr::lng_ghostgram_ui_import_nastroek_ghostgram(),
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
						tr::lng_ghostgram_ui_nastroyki_ghostgram_uspeshno_imp(tr::now)));
				} else {
					controller->show(Ui::MakeInformBox(
						tr::lng_ghostgram_ui_oshibka_formata_json_v_fayle_nas(tr::now)));
				}
			} else {
				controller->show(Ui::MakeInformBox(
					tr::lng_ghostgram_ui_file_not_found_at(tr::now) + path));
			}
		},
		.keywords = { u"import"_q, u"restore"_q },
	});

	builder.addButton({
		.id = u"backup/reset"_q,
		.title = tr::lng_ghostgram_ui_sbrosit_nastroyki_ghostgram_po_u(),
		.icon = { &st::menuIconClear },
		.onClick = [=] {
			GhostGram::Settings::ResetToDefaults();
			controller->show(Ui::MakeInformBox(
				tr::lng_ghostgram_ui_nastroyki_ghostgram_sbrosheny_na(tr::now)));
		},
		.keywords = { u"reset"_q, u"defaults"_q },
	});

	builder.addDivider();
	builder.addSkip();
	builder.addDividerText(tr::lng_ghostgram_ui_data_folder());
}

void BuildAdvancedCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_advanced_desc());

	builder.addSubsectionTitle(tr::lng_ghostgram_ui_set_i_otladka());

	AddSettingToggle(
		builder,
		u"advanced/block_telemetry"_q,
		tr::lng_ghostgram_privacy_block_telemetry(),
		GhostGram::Settings::BlockTelemetryValue(),
		GhostGram::Settings::SetBlockTelemetry,
		{ u"telemetry"_q, u"network"_q });
	builder.addDividerText(tr::lng_ghostgram_privacy_block_telemetry_about());

	AddSettingToggle(
		builder,
		u"advanced/show_peer_id"_q,
		tr::lng_ghostgram_show_peer_id(),
		GhostGram::Settings::ShowPeerIdValue(),
		GhostGram::Settings::SetShowPeerId,
		{ u"peer"_q, u"id"_q, u"debug"_q });
	builder.addDividerText(tr::lng_ghostgram_show_peer_id_about());
}

void BuildAboutCategory(SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_about_page_desc());

	builder.addSubsectionTitle(rpl::single(u"GhostGram Desktop"_q));
	builder.addDividerText(tr::lng_ghostgram_ui_about_info(tr::now));
}

// Meta definitions for all 14 categories
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

const auto kStreamerMeta = BuildHelper({
	.id = GhostGramStreamer::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_streamer,
	.icon = &st::menuIconExperimental,
}, BuildStreamerCategory);

const auto kToolsMeta = BuildHelper({
	.id = GhostGramTools::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_tools,
	.icon = &st::menuIconSavedMessages,
}, BuildToolsCategory);

const auto kBackupMeta = BuildHelper({
	.id = GhostGramBackup::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_backup,
	.icon = &st::menuIconExport,
}, BuildBackupCategory);

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
		.title = tr::lng_settings_ghostgram_streamer(),
		.targetSection = GhostGramStreamer::Id(),
		.icon = { &st::menuIconExperimental },
		.keywords = { u"streamer"_q, u"broadcast"_q, u"hide"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_tools(),
		.targetSection = GhostGramTools::Id(),
		.icon = { &st::menuIconSavedMessages },
		.keywords = { u"tools"_q, u"bookmarks"_q, u"notes"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_backup(),
		.targetSection = GhostGramBackup::Id(),
		.icon = { &st::menuIconExport },
		.keywords = { u"export"_q, u"import"_q, u"backup"_q, u"reset"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_advanced(),
		.targetSection = GhostGramAdvanced::Id(),
		.icon = { &st::menuIconManage },
		.keywords = { u"advanced"_q, u"telemetry"_q, u"debug"_q },
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
const SectionBuildMethod kStreamerSection = kStreamerMeta.build;
const SectionBuildMethod kToolsSection = kToolsMeta.build;
const SectionBuildMethod kBackupSection = kBackupMeta.build;
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

GhostGramStreamer::GhostGramStreamer(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramStreamer::title() {
	return tr::lng_settings_ghostgram_streamer();
}

void GhostGramStreamer::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kStreamerSection);
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

GhostGramBackup::GhostGramBackup(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGramBackup::title() {
	return tr::lng_settings_ghostgram_backup();
}

void GhostGramBackup::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kBackupSection);
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

Type GhostGramStreamerId() {
	return GhostGramStreamer::Id();
}

Type GhostGramToolsId() {
	return GhostGramTools::Id();
}

Type GhostGramBackupId() {
	return GhostGramBackup::Id();
}

Type GhostGramAdvancedId() {
	return GhostGramAdvanced::Id();
}

Type GhostGramAboutId() {
	return GhostGramAbout::Id();
}

} // namespace Settings
