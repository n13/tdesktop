/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_peer_suspicious_name.h"

#include <cstdio>

namespace {

struct Check {
	const char *name = nullptr;
	QString (*run)() = nullptr;
};

// Stands in for Ui::Emoji::Find, which needs the generated emoji tables.
[[nodiscard]] bool IsKnownEmoji(QStringView text) {
	static const auto known = std::array{
		QString::fromUtf8("\xF0\x9F\x94\xA5"), // fire
		QString::fromUtf8("\xE2\x9D\xA4\xEF\xB8\x8F"), // red heart
		QString::fromUtf8("\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"), // thumbs up
		QString::fromUtf8("1\xEF\xB8\x8F\xE2\x83\xA3"), // keycap 1
		QString::fromUtf8("\xF0\x9F\x85\xB0"), // A button
	};
	return ranges::contains(known, text.toString());
}

[[nodiscard]] QString Joined(const std::vector<QString> &list) {
	return QStringList(list.begin(), list.end()).join(u", "_q);
}

[[nodiscard]] QString Expect(
		const char *utf8,
		std::vector<const char*> expected) {
	const auto name = QString::fromUtf8(utf8);
	const auto actual = Data::SuspiciousNameCharacters(name, IsKnownEmoji);
	const auto wanted = expected
		| ranges::views::transform([](const char *text) {
			return QString::fromUtf8(text);
		})
		| ranges::to_vector;
	return (actual == wanted)
		? QString()
		: u"\"%1\" flags [%2], expected [%3]"_q
			.arg(name, Joined(actual), Joined(wanted));
}

[[nodiscard]] QString Failures(std::initializer_list<QString> results) {
	auto joined = QString();
	for (const auto &result : results) {
		if (!result.isEmpty()) {
			joined += (joined.isEmpty() ? QString() : u"; "_q) + result;
		}
	}
	return joined;
}

QString CheckOneAlphabetPasses() {
	return Failures({
		Expect("Paypal", {}),
		Expect("Caf\xC3\xA9 M\xC3\xBCller", {}),
		Expect("Jos\xC3\xA9-Luis O'Brien (CEO), #1", {}),
		Expect("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82", {}),
		Expect("\xCE\x95\xCE\xBB\xCE\xBB\xCE\xAC\xCE\xB4\xCE\xB1", {}),
		Expect("\xE4\xBD\xA0\xE5\xA5\xBD", {}),
		Expect("\xEC\x95\x88\xEB\x85\x95", {}),
		Expect("", {}),
	});
}

QString CheckTypographicSymbolsPass() {
	return Failures({
		Expect("Anna \xE2\x80\x9C" "Ann\xE2\x80\x9D Lee", {}),
		Expect("Mike \xE2\x80\x93 CEO \xE2\x80\xA2 \xE2\x82\xAC", {}),
	});
}

QString CheckForeignLetterInWord() {
	return Expect("P\xD0\xB0ypal", { "\xD0\xB0" });
}

QString CheckForeignWordInName() {
	return Failures({
		Expect("Alex \xCE\xA9", { "\xCE\xA9" }),
		Expect("Gao \xE9\xAB\x98", { "\xE9\xAB\x98" }),
	});
}

QString CheckFirstLetterDecidesTheName() {
	return Expect(
		"\xD0\x90lice Smith",
		{ "l", "i", "c", "e", "S", "m", "i", "t", "h" });
}

QString CheckFontLettersAreSuspicious() {
	return Failures({
		Expect(
			"\xE2\x9C\x8E...\xF0\x9D\x94\xBB\xF0\x9D\x95\x92"
			" \xF0\x9D\x94\xBD",
			{
				"\xE2\x9C\x8E",
				"\xF0\x9D\x94\xBB",
				"\xF0\x9D\x95\x92",
				"\xF0\x9D\x94\xBD",
			}),
		Expect(
			"\xEF\xBC\xAE\xEF\xBD\x89",
			{ "\xEF\xBC\xAE", "\xEF\xBD\x89" }),
	});
}

QString CheckInvisibleAndCombiningAreSuspicious() {
	return Failures({
		Expect("Bob\xE2\x80\x8B", { "\xE2\x80\x8B" }),
		Expect("Ann\xE2\x80\xAE", { "\xE2\x80\xAE" }),
		Expect("Jose\xCC\x81", { "e\xCC\x81" }),
		Expect("A\xCC\xB5n", { "A\xCC\xB5" }),
	});
}

QString CheckEmojiPass() {
	return Failures({
		Expect("Weekend hiking \xF0\x9F\x94\xA5", {}),
		Expect("Anna \xE2\x9D\xA4\xEF\xB8\x8F", {}),
		Expect("Bo \xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD", {}),
	});
}

QString CheckLetterLikeEmojiAreSuspicious() {
	return Failures({
		Expect("Room 1\xEF\xB8\x8F\xE2\x83\xA3", { "1\xEF\xB8\x8F\xE2\x83\xA3" }),
		Expect("\xF0\x9F\x85\xB0lice", { "\xF0\x9F\x85\xB0" }),
	});
}

[[nodiscard]] std::vector<Check> Checks() {
	return {
		{ "one-alphabet-passes", CheckOneAlphabetPasses },
		{ "typographic-symbols-pass", CheckTypographicSymbolsPass },
		{ "foreign-letter-in-word", CheckForeignLetterInWord },
		{ "foreign-word-in-name", CheckForeignWordInName },
		{ "first-letter-decides-the-name", CheckFirstLetterDecidesTheName },
		{ "font-letters-are-suspicious", CheckFontLettersAreSuspicious },
		{
			"invisible-and-combining-are-suspicious",
			CheckInvisibleAndCombiningAreSuspicious,
		},
		{ "emoji-pass", CheckEmojiPass },
		{
			"letter-like-emoji-are-suspicious",
			CheckLetterLikeEmojiAreSuspicious,
		},
	};
}

[[nodiscard]] bool Passes(const Check &check) {
	const auto failure = check.run();
	if (!failure.isEmpty()) {
		std::printf("FAIL %s: %s\n", check.name, failure.toUtf8().constData());
		return false;
	}
	std::printf("PASS %s\n", check.name);
	return true;
}

} // namespace

int main() {
	const auto checks = Checks();
	const auto total = int(checks.size());
	auto passed = 0;
	for (const auto &check : checks) {
		if (Passes(check)) {
			++passed;
		}
	}
	std::printf("%d of %d checks passed.\n", passed, total);
	return (passed == total) ? 0 : 1;
}
