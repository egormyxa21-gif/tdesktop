/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ghostgram/ghostgram_theme.h"

#include "ghostgram/ghostgram_settings.h"
#include "base/zlib_help.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>
#include <cmath>
#include <vector>

namespace GhostGram::Theme {
namespace {

constexpr auto kNightThemeResource = ":/gui/night.tdesktop-theme";
constexpr auto kDayThemeResource = ":/gui/day-custom-base.tdesktop-theme";
constexpr auto kPaletteLimit = 1024 * 1024;
constexpr auto kMinimumReadableContrast = 4.5;
constexpr auto kMinimumOnAccentDarkContrast = 7.;

using Colors = QVector<QPair<QString, QString>>;

double Linearize(double channel) {
	return (channel <= 0.03928)
		? channel / 12.92
		: std::pow((channel + 0.055) / 1.055, 2.4);
}

double Luminance(const QColor &color) {
	return (0.2126 * Linearize(color.redF())
		+ 0.7152 * Linearize(color.greenF())
		+ 0.0722 * Linearize(color.blueF()));
}

QColor Mix(const QColor &a, const QColor &b, double ratio) {
	const auto value = std::clamp(ratio, 0., 1.);
	return QColor::fromRgbF(
		a.redF() + (b.redF() - a.redF()) * value,
		a.greenF() + (b.greenF() - a.greenF()) * value,
		a.blueF() + (b.blueF() - a.blueF()) * value,
		a.alphaF() + (b.alphaF() - a.alphaF()) * value);
}

QColor WithAlpha(const QColor &color, int alpha) {
	auto result = color;
	result.setAlpha(alpha);
	return result;
}

QColor OnAccent(const QColor &accent) {
	const auto dark = QColor(0x14, 0x14, 0x14);
	const auto white = QColor(0xff, 0xff, 0xff);
	const auto darkRatio = ContrastRatio(accent, dark);
	return (darkRatio >= kMinimumOnAccentDarkContrast
		&& darkRatio > ContrastRatio(accent, white))
		? dark
		: white;
}

QString Hex(const QColor &color) {
	return u"#%1%2%3%4"_q
		.arg(color.red(), 2, 16, QLatin1Char('0'))
		.arg(color.green(), 2, 16, QLatin1Char('0'))
		.arg(color.blue(), 2, 16, QLatin1Char('0'))
		.arg(color.alpha(), 2, 16, QLatin1Char('0'))
		.toLower();
}

void Put(Colors &out, const char *key, const QColor &color) {
	out.append({ QString::fromLatin1(key), Hex(color) });
}

Colors Generate(const Spec &spec) {
	// WHY: hover/ripple flip direction on light backgrounds, matching the
	// official day palette where hover darkens instead of lightening.
	const auto shade = spec.dark ? QColor(0xff, 0xff, 0xff) : QColor(0x00, 0x00, 0x00);
	const auto hoverT = spec.dark ? 0.07 : 0.06;
	constexpr auto rippleT = 0.11;
	const auto bg = spec.bg;
	const auto text = spec.text;
	const auto subtle = spec.textSubtle;
	const auto accent = spec.accent;
	const auto onAccent = OnAccent(accent);
	const auto accentBright = spec.dark ? Mix(accent, shade, 0.20) : Mix(accent, shade, 0.10);
	const auto dim = Mix(bg, text, 0.40);
	const auto hover = Mix(bg, shade, hoverT);
	const auto ripple = Mix(bg, shade, rippleT);
	const auto sidebar = Mix(bg, QColor(0x00, 0x00, 0x00), spec.dark ? 0.35 : 0.08);
	const auto activeDialog = Mix(bg, accent, 0.45);
	const auto bold = Mix(text, bg, 0.05);
	const auto success = spec.success;
	const auto danger = spec.danger;
	const auto msgIn = spec.msgIn;
	const auto msgOut = spec.msgOut;

	auto out = Colors();
	out.reserve(220);

	Put(out, "windowBg", bg);
	Put(out, "windowFg", text);
	Put(out, "windowBgOver", hover);
	Put(out, "windowBgRipple", ripple);
	Put(out, "windowFgOver", bold);
	Put(out, "windowSubTextFg", subtle);
	Put(out, "windowSubTextFgOver", Mix(subtle, text, 0.10));
	Put(out, "windowBoldFg", bold);
	Put(out, "windowBoldFgOver", bold);
	Put(out, "windowBgActive", accent);
	Put(out, "windowFgActive", onAccent);
	Put(out, "windowActiveTextFg", accentBright);
	Put(out, "windowShadowFgFallback", bg);

	Put(out, "activeButtonBg", accent);
	Put(out, "activeButtonBgOver", Mix(accent, shade, hoverT));
	Put(out, "activeButtonBgRipple", Mix(accent, shade, rippleT));
	Put(out, "activeButtonFg", onAccent);
	Put(out, "activeButtonFgOver", onAccent);
	Put(out, "activeButtonSecondaryFg", Mix(accent, onAccent, 0.45));
	Put(out, "activeLineFg", accentBright);
	Put(out, "activeLineFgError", danger);
	Put(out, "lightButtonBg", bg);
	Put(out, "lightButtonBgOver", hover);
	Put(out, "lightButtonBgRipple", ripple);
	Put(out, "lightButtonFg", accentBright);
	Put(out, "attentionButtonFg", danger);
	Put(out, "attentionButtonFgOver", danger);
	Put(out, "attentionButtonBgOver", WithAlpha(Mix(bg, danger, 0.35), 0x64));
	Put(out, "attentionButtonBgRipple", WithAlpha(Mix(bg, danger, 0.45), 0x64));
	Put(out, "outlineButtonBgOver", hover);
	Put(out, "outlineButtonOutlineFg", accent);
	Put(out, "outlineButtonBgRipple", ripple);

	Put(out, "menuBg", bg);
	Put(out, "menuBgOver", hover);
	Put(out, "menuBgRipple", ripple);
	Put(out, "menuIconFg", dim);
	Put(out, "menuIconFgOver", text);
	Put(out, "menuSubmenuArrowFg", Mix(bg, text, 0.35));
	Put(out, "menuFgDisabled", Mix(bg, text, 0.20));
	Put(out, "menuSeparatorFg", Mix(bg, text, 0.09));

	Put(out, "placeholderFg", Mix(bg, text, 0.39));
	Put(out, "placeholderFgActive", Mix(bg, text, 0.27));
	Put(out, "inputBorderFg", Mix(bg, text, 0.14));
	Put(out, "filterInputBorderFg", Mix(bg, text, 0.08));
	Put(out, "filterInputInactiveBg", Mix(bg, shade, 0.05));
	Put(out, "filterInputActiveBg", Mix(bg, shade, 0.05));
	Put(out, "checkboxFg", Mix(bg, accent, 0.55));
	Put(out, "sliderBgInactive", Mix(bg, text, 0.16));
	Put(out, "sliderBgActive", accent);

	Put(out, "tooltipBg", bg);
	Put(out, "tooltipFg", text);
	Put(out, "tooltipBorderFg", bg);
	Put(out, "titleBg", hover);
	Put(out, "titleBgActive", ripple);
	Put(out, "titleFg", subtle);
	Put(out, "titleFgActive", Mix(text, subtle, 0.60));

	Put(out, "boxBg", bg);
	Put(out, "boxSearchBg", bg);
	Put(out, "boxTitleFg", text);
	Put(out, "boxTitleAdditionalFg", subtle);
	Put(out, "boxTextFgGood", success);
	Put(out, "boxTextFgError", danger);
	Put(out, "membersAboutLimitFg", subtle);
	Put(out, "contactsBg", bg);
	Put(out, "contactsBgOver", hover);
	Put(out, "contactsStatusFg", subtle);
	Put(out, "contactsStatusFgOnline", success);

	Put(out, "cancelIconFg", dim);
	Put(out, "cancelIconFgOver", text);
	Put(out, "smallCloseIconFg", Mix(bg, text, 0.30));
	Put(out, "smallCloseIconFgOver", Mix(bg, text, 0.64));

	const auto callBgSolid = Mix(bg, QColor(0x00, 0x00, 0x00), spec.dark ? 0.45 : 0.20);
	Put(out, "callBg", WithAlpha(callBgSolid, 0xf5));
	Put(out, "callArrowFg", accentBright);
	Put(out, "callArrowMissedFg", danger);
	Put(out, "callStatusFg", Mix(callBgSolid, text, 0.65));
	Put(out, "callAnswerBg", accent);
	Put(out, "callAnswerRipple", Mix(accent, shade, hoverT));
	Put(out, "callAnswerBgOuter", WithAlpha(accent, 0x26));
	Put(out, "callHangupBg", danger);
	Put(out, "callHangupRipple", Mix(danger, shade, hoverT));
	Put(out, "callBarBg", Mix(bg, accent, 0.62));
	Put(out, "callBarBgMuted", Mix(bg, accent, 0.40));
	Put(out, "callBarMuteRipple", Mix(Mix(bg, accent, 0.62), shade, hoverT));
	Put(out, "callBarUnmuteRipple", Mix(bg, accent, 0.48));

	Put(out, "introTitleFg", text);
	Put(out, "introDescriptionFg", subtle);
	Put(out, "introErrorFg", danger);
	Put(out, "introCoverTopBg", Mix(bg, accent, 0.55));
	Put(out, "introCoverBottomBg", Mix(bg, accent, 0.75));
	Put(out, "introCoverIconsFg", Mix(accent, bg, 0.20));
	Put(out, "introCoverPlaneTrace", Mix(accent, bg, 0.35));

	Put(out, "dialogsBgOver", hover);
	Put(out, "dialogsRippleBg", ripple);
	Put(out, "dialogsBgActive", activeDialog);
	Put(out, "dialogsRippleBgActive", Mix(activeDialog, shade, hoverT));
	Put(out, "dialogsNameFg", text);
	Put(out, "dialogsDateFg", Mix(bg, text, 0.50));
	Put(out, "dialogsTextFg", Mix(bg, text, 0.45));
	Put(out, "dialogsTextFgService", accentBright);
	Put(out, "dialogsDraftFg", danger);
	Put(out, "dialogsDraftFgActive", spec.dark ? Mix(danger, shade, 0.35) : Mix(danger, bg, 0.20));
	Put(out, "dialogsSendingIconFg", Mix(bg, text, 0.35));
	Put(out, "dialogsSentIconFg", accentBright);
	Put(out, "dialogsUnreadBg", Mix(accent, bg, 0.22));
	Put(out, "dialogsUnreadBgMuted", Mix(bg, accent, 0.45));
	Put(out, "dialogsUnreadFg", onAccent);
	Put(out, "dialogsOnlineBadgeFg", accentBright);
	Put(out, "dialogsVerifiedIconBg", Mix(accent, shade, 0.20));
	Put(out, "dialogsVerifiedIconFg", onAccent);
	Put(out, "dialogsDateFgOver", Mix(bg, text, 0.53));
	Put(out, "dialogsTextFgOver", Mix(bg, text, 0.58));
	Put(out, "dialogsTextFgServiceOver", accentBright);
	Put(out, "dialogsSentIconFgOver", accentBright);
	Put(out, "dialogsUnreadBgOver", Mix(accent, bg, 0.22));
	Put(out, "dialogsUnreadBgMutedOver", Mix(Mix(bg, accent, 0.45), shade, hoverT));
	Put(out, "dialogsUnreadFgOver", onAccent);
	Put(out, "dialogsVerifiedIconBgOver", Mix(accent, shade, 0.20));
	Put(out, "dialogsUnreadBgMutedActive", Mix(activeDialog, onAccent, 0.35));
	Put(out, "dialogsUnreadFgActive", Mix(bg, accent, 0.50));

	Put(out, "searchedBarBg", hover);
	Put(out, "searchedBarFg", Mix(bg, text, 0.53));
	Put(out, "topBarBg", bg);

	Put(out, "emojiPanCategories", bg);
	Put(out, "emojiPanHeaderFg", Mix(bg, text, 0.46));
	Put(out, "emojiIconFg", dim);
	Put(out, "emojiIconFgActive", accentBright);

	Put(out, "historyTextOutFg", Mix(text, bg, 0.06));
	Put(out, "historyLinkInFg", accentBright);
	Put(out, "historyLinkInFgSelected", Mix(accent, onAccent, 0.55));
	Put(out, "historyLinkOutFg", Mix(accent, onAccent, 0.40));
	Put(out, "historyLinkOutFgSelected", Mix(accent, onAccent, 0.55));
	Put(out, "historyOutIconFg", accentBright);
	Put(out, "historySendingOutIconFg", accentBright);
	Put(out, "historySendingInIconFg", Mix(bg, text, 0.43));
	Put(out, "historyCallArrowInFg", accentBright);
	Put(out, "historyUnreadBarBg", Mix(bg, shade, 0.045));
	Put(out, "historyComposeAreaBg", bg);
	Put(out, "historyComposeButtonBgOver", hover);
	Put(out, "historyComposeButtonBgRipple", ripple);
	Put(out, "historyPinnedBg", Mix(bg, shade, 0.045));
	Put(out, "historyToDownBg", Mix(bg, shade, 0.05));
	Put(out, "historyToDownBgOver", hover);
	Put(out, "historyToDownBgRipple", ripple);
	Put(out, "historyToDownFg", Mix(bg, text, 0.65));

	Put(out, "msgInBg", msgIn);
	Put(out, "msgInBgSelected", Mix(msgIn, accent, 0.50));
	Put(out, "msgOutBg", msgOut);
	Put(out, "msgOutBgSelected", Mix(msgOut, accent, 0.50));
	Put(out, "msgSelectOverlay", WithAlpha(accent, 0x4c));
	Put(out, "msgStickerOverlay", WithAlpha(accent, 0x7f));
	Put(out, "msgInServiceFg", accentBright);
	Put(out, "msgOutServiceFg", Mix(accentBright, onAccent, 0.30));
	Put(out, "msgInDateFg", Mix(msgIn, text, 0.42));
	Put(out, "msgOutDateFg", Mix(msgOut, text, 0.45));
	Put(out, "msgServiceBg", WithAlpha(Mix(bg, shade, 0.06), 0xd5));
	Put(out, "msgServiceBgSelected", Mix(bg, accent, 0.75));
	Put(out, "msgInReplyBarColor", accent);
	Put(out, "msgOutReplyBarColor", accentBright);
	Put(out, "msgInMonoFg", Mix(msgIn, accentBright, 0.72));
	Put(out, "msgOutMonoFg", Mix(msgOut, onAccent, 0.62));
	Put(out, "msgInMonoFgSelected", Mix(Mix(msgIn, accentBright, 0.72), onAccent, 0.45));
	Put(out, "msgOutMonoFgSelected", Mix(Mix(msgOut, onAccent, 0.62), onAccent, 0.25));
	Put(out, "msgFileInBg", Mix(accent, onAccent, 0.10));
	Put(out, "msgFileInBgOver", Mix(accent, onAccent, 0.16));
	Put(out, "msgFileInBgSelected", Mix(accent, onAccent, 0.35));
	Put(out, "msgFileOutBg", Mix(accent, onAccent, 0.15));
	Put(out, "msgFileOutBgOver", Mix(accent, onAccent, 0.20));
	Put(out, "msgFileOutBgSelected", Mix(accent, onAccent, 0.40));
	Put(out, "msgFileThumbLinkOutFg", Mix(accent, onAccent, 0.15));
	Put(out, "msgWaveformInActive", Mix(accent, onAccent, 0.08));
	Put(out, "msgWaveformInInactive", Mix(msgIn, text, 0.18));
	Put(out, "msgWaveformInInactiveSelected", Mix(accent, onAccent, 0.20));
	Put(out, "msgWaveformOutActive", Mix(accent, onAccent, 0.30));
	Put(out, "msgWaveformOutInactive", Mix(msgOut, accentBright, 0.50));
	Put(out, "msgWaveformOutInactiveSelected", Mix(accent, onAccent, 0.20));
	Put(out, "msgBotKbOverBgAdd", WithAlpha(Mix(accent, onAccent, 0.25), 0x0f));
	Put(out, "msgBotKbRippleBg", WithAlpha(Mix(accent, onAccent, 0.25), 0x0b));

	Put(out, "overviewCheckBorder", Mix(text, shade, 0.25));
	Put(out, "overviewPhotoSelectOverlay", WithAlpha(accent, 0x33));
	Put(out, "profileStatusFgOver", subtle);
	Put(out, "profileAdminStartFg", accentBright);
	Put(out, "reportSpamBg", Mix(bg, shade, 0.10));

	Put(out, "mainMenuCoverBg", Mix(bg, accent, 0.70));
	Put(out, "mainMenuCloudBg", Mix(bg, accent, 0.55));
	Put(out, "mediaPlayerDisabledFg", Mix(accent, onAccent, 0.45));
	Put(out, "mediaviewTextLinkFg", accentBright);
	Put(out, "botKbBg", Mix(bg, shade, 0.05));
	Put(out, "botKbDownBg", Mix(bg, shade, 0.10));

	Put(out, "sideBarBg", sidebar);
	Put(out, "sideBarBgActive", Mix(sidebar, shade, 0.14));
	Put(out, "sideBarBgRipple", Mix(sidebar, shade, 0.09));
	Put(out, "sideBarTextFg", Mix(sidebar, text, 0.48));
	Put(out, "sideBarTextFgActive", accentBright);
	Put(out, "sideBarIconFg", Mix(sidebar, text, 0.48));
	Put(out, "sideBarIconFgActive", accentBright);
	Put(out, "sideBarBadgeBg", accentBright);
	Put(out, "sideBarBadgeBgMuted", Mix(sidebar, text, 0.45));
	return out;
}

QByteArray LoadThemeFile(const char *path) {
	auto file = QFile(QLatin1String(path));
	if (!file.open(QIODevice::ReadOnly)) {
		return QByteArray();
	}
	const auto content = file.readAll();
	if (content.isEmpty()) {
		return QByteArray();
	}
	zlib::FileToRead reader(content);
	return reader.readFileContent(
		"colors.tdesktop-theme",
		zlib::kCaseInsensitive,
		kPaletteLimit);
}

const QByteArray &Template(bool dark) {
	static const auto night = LoadThemeFile(kNightThemeResource);
	static const auto day = LoadThemeFile(kDayThemeResource);
	return dark ? night : day;
}

QByteArray Merge(const QByteArray &base, const Colors &overrides) {
	auto lines = QHash<QString, QString>();
	for (const auto &[key, value] : overrides) {
		lines.insert(key, u"%1: %2;"_q.arg(key, value));
	}
	auto result = QByteArray();
	result.reserve(base.size() + overrides.size() * 32);
	auto start = 0;
	while (start < base.size()) {
		const auto end = base.indexOf('\n', start);
		const auto next = (end >= 0) ? end + 1 : base.size();
		auto line = base.mid(start, next - start);
		const auto colon = line.indexOf(':');
		if (colon > 0) {
			const auto key = QString::fromLatin1(line.left(colon)).trimmed();
			if (const auto it = lines.find(key); it != lines.end()) {
				line = it.value().toUtf8() + '\n';
				lines.remove(key);
			}
		}
		result.append(line);
		start = next;
	}
	for (const auto &entry : overrides) {
		if (lines.contains(entry.first)) {
			result.append(lines.value(entry.first).toUtf8());
			result.append('\n');
		}
	}
	return result;
}

QColor ParseColor(const QString &value) {
	if (!value.startsWith(u"#"_q)) {
		return QColor();
	}
	const auto hex = value.mid(1);
	const auto size = hex.size();
	if (size != 6 && size != 8) {
		return QColor();
	}
	bool ok = false;
	auto parts = std::vector<int>();
	for (auto i = 0; i < size; i += 2) {
		parts.push_back(hex.mid(i, 2).toInt(&ok, 16));
		if (!ok) {
			return QColor();
		}
	}
	return (parts.size() == 3)
		? QColor(parts[0], parts[1], parts[2])
		: QColor(parts[0], parts[1], parts[2], parts[3]);
}

QColor ReadColor(const QJsonObject &object, const char *key, const QColor &fallback) {
	const auto value = object.value(QString::fromLatin1(key)).toString();
	const auto parsed = ParseColor(value);
	return parsed.isValid() ? parsed : fallback;
}

} // namespace

double ContrastRatio(const QColor &a, const QColor &b) {
	const auto lighter = std::max(Luminance(a), Luminance(b));
	const auto darker = std::min(Luminance(a), Luminance(b));
	return (lighter + 0.05) / (darker + 0.05);
}

QColor EnsureReadable(const QColor &bg, const QColor &fg) {
	if (ContrastRatio(bg, fg) >= kMinimumReadableContrast) {
		return fg;
	}
	const auto toward = (Luminance(bg) < 0.35)
		? QColor(0xff, 0xff, 0xff)
		: QColor(0x00, 0x00, 0x00);
	auto low = 0.;
	auto high = 1.;
	for (auto i = 0; i < 24; ++i) {
		const auto middle = (low + high) / 2.;
		if (ContrastRatio(bg, Mix(fg, toward, middle)) >= kMinimumReadableContrast) {
			high = middle;
		} else {
			low = middle;
		}
	}
	return Mix(fg, toward, high);
}

Spec EnsureReadableSpec(Spec spec) {
	spec.text = EnsureReadable(spec.bg, spec.text);
	spec.textSubtle = EnsureReadable(spec.bg, spec.textSubtle);
	spec.danger = EnsureReadable(spec.bg, spec.danger);
	spec.success = EnsureReadable(spec.bg, spec.success);
	return spec;
}

std::optional<Spec> PresetSpec(Settings::ThemePreset preset) {
	using P = Settings::ThemePreset;
	switch (preset) {
	case P::Obsidian:
		return Spec{
			u"obsidian"_q, true,
			QColor(u"#101014"_q), QColor(u"#2563a8"_q),
			QColor(u"#181820"_q), QColor(u"#222530"_q),
			QColor(u"#f0f0f2"_q), QColor(u"#8a8a94"_q),
			QColor(u"#e05252"_q), QColor(u"#4fbf6c"_q) };
	case P::Phantom:
		return Spec{
			u"phantom"_q, true,
			QColor(u"#13111c"_q), QColor(u"#6d4aa8"_q),
			QColor(u"#1c1828"_q), QColor(u"#382654"_q),
			QColor(u"#ece9f4"_q), QColor(u"#8f86ad"_q),
			QColor(u"#e05a6a"_q), QColor(u"#56c98a"_q) };
	case P::Midnight:
		return Spec{
			u"midnight"_q, true,
			QColor(u"#080d14"_q), QColor(u"#3a7bd5"_q),
			QColor(u"#0e1724"_q), QColor(u"#1a3352"_q),
			QColor(u"#e8eef5"_q), QColor(u"#7d92ab"_q),
			QColor(u"#e55560"_q), QColor(u"#4cbb7e"_q) };
	case P::Violet:
		return Spec{
			u"violet"_q, true,
			QColor(u"#150d1f"_q), QColor(u"#7b2cbf"_q),
			QColor(u"#221433"_q), QColor(u"#4d1975"_q),
			QColor(u"#f1e9f9"_q), QColor(u"#9d8bb5"_q),
			QColor(u"#ef5a78"_q), QColor(u"#5ed493"_q) };
	case P::Arctic:
		return Spec{
			u"arctic"_q, true,
			QColor(u"#0a1317"_q), QColor(u"#1d9baa"_q),
			QColor(u"#0f1e24"_q), QColor(u"#18424d"_q),
			QColor(u"#eaf4f5"_q), QColor(u"#7f9ba3"_q),
			QColor(u"#e6606a"_q), QColor(u"#4ecfae"_q) };
	case P::Emerald:
		return Spec{
			u"emerald"_q, true,
			QColor(u"#0b1412"_q), QColor(u"#1f9d63"_q),
			QColor(u"#10201a"_q), QColor(u"#164a34"_q),
			QColor(u"#e9f5ef"_q), QColor(u"#7f9c8f"_q),
			QColor(u"#e05b5b"_q), QColor(u"#46d18f"_q) };
	case P::Crimson:
		return Spec{
			u"crimson"_q, true,
			QColor(u"#160d0f"_q), QColor(u"#c2334b"_q),
			QColor(u"#211417"_q), QColor(u"#5a1e2a"_q),
			QColor(u"#f6ecee"_q), QColor(u"#a58489"_q),
			QColor(u"#ff5b64"_q), QColor(u"#55c98a"_q) };
	case P::Sunset:
		return Spec{
			u"sunset"_q, true,
			QColor(u"#17100c"_q), QColor(u"#e2703a"_q),
			QColor(u"#221812"_q), QColor(u"#5c3a20"_q),
			QColor(u"#f7efe6"_q), QColor(u"#a89284"_q),
			QColor(u"#ee5b52"_q), QColor(u"#63c98a"_q) };
	case P::Graphite:
		return Spec{
			u"graphite"_q, true,
			QColor(u"#17181a"_q), QColor(u"#7a828d"_q),
			QColor(u"#1e2023"_q), QColor(u"#2b2f34"_q),
			QColor(u"#f2f3f4"_q), QColor(u"#9298a0"_q),
			QColor(u"#e05b5b"_q), QColor(u"#58b97f"_q) };
	case P::Ocean:
		return Spec{
			u"ocean"_q, true,
			QColor(u"#0a1120"_q), QColor(u"#2b6fd6"_q),
			QColor(u"#0f1a30"_q), QColor(u"#1d3f70"_q),
			QColor(u"#e9eef7"_q), QColor(u"#7f8fa8"_q),
			QColor(u"#e2606a"_q), QColor(u"#4bc9a0"_q) };
	case P::Nord:
		return Spec{
			u"nord"_q, true,
			QColor(u"#2e3440"_q), QColor(u"#88c0d0"_q),
			QColor(u"#3b4252"_q), QColor(u"#434c5e"_q),
			QColor(u"#eceff4"_q), QColor(u"#7b8698"_q),
			QColor(u"#bf616a"_q), QColor(u"#a3be8c"_q) };
	case P::Cyberpunk:
		return Spec{
			u"cyberpunk"_q, true,
			QColor(u"#0a0a0f"_q), QColor(u"#fcee0a"_q),
			QColor(u"#131318"_q), QColor(u"#47134f"_q),
			QColor(u"#f2f0e6"_q), QColor(u"#8f8d80"_q),
			QColor(u"#ff2a6d"_q), QColor(u"#05ffa1"_q) };
	case P::Amoled:
		return Spec{
			u"amoled"_q, true,
			QColor(u"#000000"_q), QColor(u"#f2f2f2"_q),
			QColor(u"#101010"_q), QColor(u"#1c1c1c"_q),
			QColor(u"#e6e6e6"_q), QColor(u"#828282"_q),
			QColor(u"#ff4d4d"_q), QColor(u"#4cd964"_q) };
	case P::Solarized:
		return Spec{
			u"solarized"_q, true,
			QColor(u"#002b36"_q), QColor(u"#268bd2"_q),
			QColor(u"#073642"_q), QColor(u"#0b4a58"_q),
			QColor(u"#93a1a1"_q), QColor(u"#657b83"_q),
			QColor(u"#dc322f"_q), QColor(u"#859900"_q) };
	case P::Daylight:
		return Spec{
			u"daylight"_q, false,
			QColor(u"#ffffff"_q), QColor(u"#3390ec"_q),
			QColor(u"#ffffff"_q), QColor(u"#eeffde"_q),
			QColor(u"#000000"_q), QColor(u"#707579"_q),
			QColor(u"#e53935"_q), QColor(u"#31b545"_q) };
	case P::Lavender:
		return Spec{
			u"lavender"_q, false,
			QColor(u"#f8f6fc"_q), QColor(u"#7b2cbf"_q),
			QColor(u"#ffffff"_q), QColor(u"#f0e6ff"_q),
			QColor(u"#17141f"_q), QColor(u"#6e6780"_q),
			QColor(u"#d6336c"_q), QColor(u"#2fb673"_q) };
	case P::Default:
	default: return std::nullopt;
	}
}

Spec SpecWithOverrides(
		const Spec &base,
		const QVector<FieldOverride> &overrides) {
	auto result = base;
	for (const auto &entry : overrides) {
		if (!entry.color.isValid()) {
			continue;
		}
		if (entry.field == u"bg"_q) {
			result.bg = entry.color;
		} else if (entry.field == u"accent"_q) {
			result.accent = entry.color;
		} else if (entry.field == u"msgIn"_q) {
			result.msgIn = entry.color;
		} else if (entry.field == u"msgOut"_q) {
			result.msgOut = entry.color;
		} else if (entry.field == u"text"_q) {
			result.text = entry.color;
		} else if (entry.field == u"textSubtle"_q) {
			result.textSubtle = entry.color;
		} else if (entry.field == u"danger"_q) {
			result.danger = entry.color;
		} else if (entry.field == u"success"_q) {
			result.success = entry.color;
		}
	}
	return result;
}

QByteArray BuildPalette(const Spec &spec) {
	return Merge(Template(spec.dark), Generate(spec));
}

QByteArray BuildPresetPalette(Settings::ThemePreset preset) {
	const auto spec = PresetSpec(preset);
	return spec ? BuildPalette(*spec) : QByteArray();
}

std::optional<Spec> ParseSpec(const QByteArray &json) {
	if (json.isEmpty()) {
		return std::nullopt;
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(json, &error);
	if (error.error != QJsonParseError::NoError
		|| !document.isObject()) {
		return std::nullopt;
	}
	const auto object = document.object();
	const auto version = object.value(u"version"_q).toInt();
	if (version != 1) {
		return std::nullopt;
	}
	const auto id = object.value(u"id"_q).toString();
	if (id.isEmpty() || id.size() > 64) {
		return std::nullopt;
	}
	auto spec = Spec();
	spec.id = id;
	spec.dark = object.value(u"dark"_q).toBool(true);
	spec.bg = ReadColor(object, "bg", QColor());
	spec.accent = ReadColor(object, "accent", QColor());
	spec.msgIn = ReadColor(object, "msgIn", QColor());
	spec.msgOut = ReadColor(object, "msgOut", QColor());
	spec.text = ReadColor(object, "text", QColor());
	spec.textSubtle = ReadColor(object, "textSubtle", QColor());
	spec.danger = ReadColor(object, "danger", QColor());
	spec.success = ReadColor(object, "success", QColor());
	const auto valid = spec.bg.isValid()
		&& spec.accent.isValid()
		&& spec.msgIn.isValid()
		&& spec.msgOut.isValid()
		&& spec.text.isValid()
		&& spec.textSubtle.isValid()
		&& spec.danger.isValid()
		&& spec.success.isValid();
	return valid ? std::optional<Spec>(spec) : std::nullopt;
}

QByteArray SerializeSpec(const Spec &spec) {
	auto object = QJsonObject();
	object.insert(u"version"_q, 1);
	object.insert(u"id"_q, spec.id.isEmpty() ? u"custom"_q : spec.id);
	object.insert(u"dark"_q, spec.dark);
	object.insert(u"bg"_q, Hex(spec.bg));
	object.insert(u"accent"_q, Hex(spec.accent));
	object.insert(u"msgIn"_q, Hex(spec.msgIn));
	object.insert(u"msgOut"_q, Hex(spec.msgOut));
	object.insert(u"text"_q, Hex(spec.text));
	object.insert(u"textSubtle"_q, Hex(spec.textSubtle));
	object.insert(u"danger"_q, Hex(spec.danger));
	object.insert(u"success"_q, Hex(spec.success));
	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QString ColorHex(const QColor &color) {
	return Hex(color);
}

} // namespace GhostGram::Theme
