/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_ghostgram.h"

#include "settings/settings_common_session.h"

#include "lang/lang_keys.h"
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

void BuildEmptyCategory(SectionBuilder &builder) {
	builder.addSkip();
	builder.addDividerText(tr::lng_settings_ghostgram_empty());
}

class GhostGramGeneral : public Section<GhostGramGeneral> {
public:
	GhostGramGeneral(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

class GhostGramInterface : public Section<GhostGramInterface> {
public:
	GhostGramInterface(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

class GhostGramPrivacy : public Section<GhostGramPrivacy> {
public:
	GhostGramPrivacy(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

class GhostGramTools : public Section<GhostGramTools> {
public:
	GhostGramTools(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

class GhostGram : public Section<GhostGram> {
public:
	GhostGram(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kGeneralMeta = BuildHelper({
	.id = GhostGramGeneral::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_general,
	.icon = &st::menuIconSettings,
}, BuildEmptyCategory);

const auto kInterfaceMeta = BuildHelper({
	.id = GhostGramInterface::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_interface,
	.icon = &st::menuIconCustomize,
}, BuildEmptyCategory);

const auto kPrivacyMeta = BuildHelper({
	.id = GhostGramPrivacy::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_privacy,
	.icon = &st::menuIconLock,
}, BuildEmptyCategory);

const auto kToolsMeta = BuildHelper({
	.id = GhostGramTools::Id(),
	.parentId = GhostGramId(),
	.title = &tr::lng_settings_ghostgram_tools,
	.icon = &st::menuIconExperimental,
}, BuildEmptyCategory);

const SectionBuildMethod kGeneralSection = kGeneralMeta.build;
const SectionBuildMethod kInterfaceSection = kInterfaceMeta.build;
const SectionBuildMethod kPrivacySection = kPrivacyMeta.build;
const SectionBuildMethod kToolsSection = kToolsMeta.build;

const auto kMeta = BuildHelper({
	.id = GhostGram::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_ghostgram,
	.icon = &st::menuIconStealth,
}, [](SectionBuilder &builder) {
	builder.addDividerText(tr::lng_settings_ghostgram_about());

	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_general(),
		.targetSection = GhostGramGeneral::Id(),
		.icon = { &st::menuIconSettings },
		.keywords = { u"general"_q, u"ghostgram"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_interface(),
		.targetSection = GhostGramInterface::Id(),
		.icon = { &st::menuIconCustomize },
		.keywords = { u"interface"_q, u"appearance"_q, u"ui"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_privacy(),
		.targetSection = GhostGramPrivacy::Id(),
		.icon = { &st::menuIconLock },
		.keywords = { u"privacy"_q, u"security"_q },
	});
	builder.addSectionButton({
		.title = tr::lng_settings_ghostgram_tools(),
		.targetSection = GhostGramTools::Id(),
		.icon = { &st::menuIconExperimental },
		.keywords = { u"tools"_q, u"experimental"_q },
	});
});

const SectionBuildMethod kGhostGramSection = kMeta.build;

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

GhostGram::GhostGram(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> GhostGram::title() {
	return tr::lng_settings_ghostgram();
}

void GhostGram::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	build(content, kGhostGramSection);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type GhostGramId() {
	return GhostGram::Id();
}

} // namespace Settings
