/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_peer_suspicious_name.h"

#include "base/options.h"

#include <QtCore/QTextBoundaryFinder>

namespace Data {
namespace {

enum class Alphabet : uchar {
	Latin,
	Greek,
	Cyrillic,
	Hebrew,
	Arabic,
	Devanagari,
	Thai,
	Hangul,
	Cjk,
};

struct Letters {
	Alphabet alphabet = Alphabet::Latin;
	char32_t from = 0;
	char32_t till = 0;
};

constexpr auto kLetters = std::array{
	Letters{ Alphabet::Latin, 'A', 'Z' },
	Letters{ Alphabet::Latin, 'a', 'z' },
	Letters{ Alphabet::Latin, 0xC0, 0xD6 },
	Letters{ Alphabet::Latin, 0xD8, 0xF6 },
	Letters{ Alphabet::Latin, 0xF8, 0x17E },
	Letters{ Alphabet::Greek, 0x370, 0x3FF },
	Letters{ Alphabet::Cyrillic, 0x400, 0x4FF },
	Letters{ Alphabet::Hebrew, 0x5D0, 0x5EA },
	Letters{ Alphabet::Arabic, 0x620, 0x65F },
	Letters{ Alphabet::Arabic, 0x66E, 0x6D3 },
	Letters{ Alphabet::Devanagari, 0x900, 0x97F },
	Letters{ Alphabet::Thai, 0xE01, 0xE5B },
	Letters{ Alphabet::Hangul, 0x3131, 0x3163 },
	Letters{ Alphabet::Hangul, 0x3165, 0x318E },
	Letters{ Alphabet::Hangul, 0xAC00, 0xD7A3 },
	Letters{ Alphabet::Cjk, 0x3001, 0x302F },
	Letters{ Alphabet::Cjk, 0x3031, 0x303C },
	Letters{ Alphabet::Cjk, 0x3041, 0x30FF },
	Letters{ Alphabet::Cjk, 0x3400, 0x4DBF },
	Letters{ Alphabet::Cjk, 0x4E00, 0x9FFF },
	Letters{ Alphabet::Cjk, 0xFF01, 0xFF0F },
	Letters{ Alphabet::Cjk, 0xFF1A, 0xFF20 },
};

constexpr auto kSymbols = std::array<char32_t, 18>{
	0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x2026,
	0x20AC, 0x20BF, 0xB7, 0x22C6, 0x2664, 0x2765, 0x27C1, 0x27E0,
	0x13080, 0x1F71B,
};

constexpr auto kLetterLikeEmoji = std::array<char32_t, 8>{
	0x1F170, 0x1F171, 0x1F17E, 0x1F17F, 0x24C2, 0x2139, 0x1F51F, 0x20E3,
};

base::options::toggle FlagSuspiciousNamesOption({
	.id = kOptionFlagSuspiciousNames,
	.name = "Flag suspicious names",
	.description = "Mark users and chats whose name mixes alphabets"
		" or uses lookalike characters, like a Cyrillic letter"
		" in a Latin name or letters from mathematical fonts.",
	.restartRequired = true,
});

[[nodiscard]] std::optional<Alphabet> AlphabetOf(char32_t code) {
	for (const auto &letters : kLetters) {
		if (code >= letters.from && code <= letters.till) {
			return letters.alphabet;
		}
	}
	return std::nullopt;
}

[[nodiscard]] bool IsAsciiSpace(char32_t code) {
	return (code == ' ')
		|| (code == '\t')
		|| (code == '\n')
		|| (code == '\f')
		|| (code == '\r');
}

[[nodiscard]] bool IsNeutral(char32_t code) {
	const auto asciiLetter = (code >= 'A' && code <= 'Z')
		|| (code >= 'a' && code <= 'z');
	return (code >= 0x21 && code <= 0x7E && !asciiLetter)
		|| ranges::contains(kSymbols, code);
}

[[nodiscard]] bool IsLetterLikeEmoji(char32_t code) {
	return ranges::contains(kLetterLikeEmoji, code);
}

[[nodiscard]] bool IsAllowed(
		QStringView grapheme,
		std::optional<Alphabet> &first,
		const Fn<bool(QStringView)> &isEmoji) {
	const auto codes = grapheme.toUcs4();
	if (ranges::all_of(codes, IsAsciiSpace)) {
		return true;
	}
	const auto alphabet = AlphabetOf(codes.front());
	const auto sameAlphabet = [&](char32_t code) {
		return AlphabetOf(code) == alphabet;
	};
	if (alphabet && ranges::all_of(codes, sameAlphabet)) {
		if (!first) {
			first = alphabet;
		}
		return (alphabet == first);
	}
	return ranges::all_of(codes, IsNeutral)
		|| (ranges::none_of(codes, IsLetterLikeEmoji) && isEmoji(grapheme));
}

} // namespace

const char kOptionFlagSuspiciousNames[] = "flag-suspicious-names";

bool FlagSuspiciousNames() {
	return FlagSuspiciousNamesOption.value();
}

std::vector<QString> SuspiciousNameCharacters(
		const QString &name,
		Fn<bool(QStringView)> isEmoji) {
	auto result = std::vector<QString>();
	auto first = std::optional<Alphabet>();
	auto finder = QTextBoundaryFinder(QTextBoundaryFinder::Grapheme, name);
	auto from = 0;
	for (auto till = finder.toNextBoundary()
		; till >= 0
		; till = finder.toNextBoundary()) {
		const auto grapheme = QStringView(name).mid(from, till - from);
		from = till;
		if (!IsAllowed(grapheme, first, isEmoji)) {
			result.push_back(grapheme.toString());
		}
	}
	return result;
}

} // namespace Data
