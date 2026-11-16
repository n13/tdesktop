/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Data {

extern const char kOptionFlagSuspiciousNames[];

[[nodiscard]] bool FlagSuspiciousNames();

[[nodiscard]] std::vector<QString> SuspiciousNameCharacters(
	const QString &name,
	Fn<bool(QStringView)> isEmoji);

} // namespace Data
