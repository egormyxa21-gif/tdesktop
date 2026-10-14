#pragma once

#include <QByteArray>
#include <QColor>
#include <QPair>
#include <QString>
#include <QVector>

#include <optional>

namespace GhostGram::Settings {

enum class ThemePreset : int;

} // namespace GhostGram::Settings

namespace GhostGram::Theme {

struct Spec {
	QString id;
	bool dark = true;
	QColor bg;
	QColor accent;
	QColor msgIn;
	QColor msgOut;
	QColor text;
	QColor textSubtle;
	QColor danger;
	QColor success;
};

struct FieldOverride {
	QString field;
	QColor color;
};

[[nodiscard]] std::optional<Spec> PresetSpec(Settings::ThemePreset preset);
[[nodiscard]] Spec SpecWithOverrides(
	const Spec &base,
	const QVector<FieldOverride> &overrides);
[[nodiscard]] QByteArray BuildPalette(Spec spec, bool ensureReadable = false);
[[nodiscard]] QByteArray BuildPresetPalette(Settings::ThemePreset preset);
[[nodiscard]] std::optional<Spec> ParseSpec(const QByteArray &json);
[[nodiscard]] QByteArray SerializeSpec(const Spec &spec);
[[nodiscard]] QString ColorHex(const QColor &color);
[[nodiscard]] double ContrastRatio(const QColor &a, const QColor &b);
[[nodiscard]] QColor EnsureReadable(const QColor &bg, const QColor &fg);
[[nodiscard]] QColor EnsureReadableBackground(
	const QColor &windowBg,
	const QColor &fg,
	const QColor &bg);
[[nodiscard]] Spec EnsureReadableSpec(Spec spec);

} // namespace GhostGram::Theme
