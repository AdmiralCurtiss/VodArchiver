#include "filename_util.h"

#include <cstddef>
#include <format>
#include <string>
#include <string_view>

#include "util/text.h"

namespace VodArchiver {
std::string MakeStringFileSystemSafeBaseName(std::string_view s) {
    std::string fn;
    fn.reserve(s.size());
    for (char c : s) {
        if ((c >= '\0' && c <= '\x1f') // control chars
            || c == '<' || c == '>' || c == ':' || c == '"' || c == '|' || c == '*'
            || c == '?'              // illegal on windows
            || c == '\\' || c == '/' // path separators
            || c == ' '              // although spaces are technically valid we don't want them
            || c == '.'              // no dots either, just in case
        ) {
            fn.push_back('_');
        } else {
            fn.push_back(c);
        }
    }
    return fn;
}

static std::string GetByteAsHexString(char byte) {
    return std::format("{:02x}", static_cast<int>(static_cast<unsigned char>(byte)));
}

static std::string ToUpperFirstChar(std::string_view s) {
    if (s.empty()) {
        return std::string();
    }

    const bool maybeRomanNumeral = [&]() -> bool {
        for (char c : s) {
            if (!(c == 'I' || c == 'V' || c == 'X')) {
                return false;
            }
        }
        return true;
    }();
    if (maybeRomanNumeral) {
        return std::string(s);
    }

    // TODO: Unicode support?
    return std::format("{}{}",
                       HyoutaUtils::TextUtils::ToUpper(s.substr(0, 1)),
                       HyoutaUtils::TextUtils::ToLower(s.substr(1)));
}

std::string FileSystemEscapeName(std::string_view s) {
    std::string fn;
    fn.reserve(s.size());
    for (char c : s) {
        if ((c >= '\0' && c <= '\x1f') // control chars
            || c == '<' || c == '>' || c == ':' || c == '"' || c == '|' || c == '*'
            || c == '?'              // illegal on windows
            || c == '\\' || c == '/' // path separators
            || c == ' '              // although spaces are technically valid we don't want them
            || c == '.'              // no dots either, just in case
            || c == '-'              // the escape char itself must be escaped too
        ) {
            fn.push_back('-');
            fn.push_back('-');
            fn.append(GetByteAsHexString(c));
            fn.push_back('-');
            fn.push_back('-');
        } else {
            fn.push_back(c);
        }
    }
    return fn;
}

std::string MakeIntercapsFilename(std::string_view gamename) {
    const std::string safename =
        HyoutaUtils::TextUtils::Replace(MakeStringFileSystemSafeBaseName(gamename), "-", "_");
    const std::vector<std::string_view> split = HyoutaUtils::TextUtils::Split(safename, "_");
    std::string result;
    for (std::string_view s : split) {
        if (!s.empty()) {
            result.append(ToUpperFirstChar(s));
        }
    }
    return result;
}

std::string Crop(std::string_view s, size_t length) {
    return s.size() <= length ? std::string(s) : std::string(s.substr(0, length));
}
} // namespace VodArchiver
