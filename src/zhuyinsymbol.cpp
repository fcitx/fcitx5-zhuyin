/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyinsymbol.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>

namespace fcitx {

namespace {
const std::vector<std::string> empty;
}

ZhuyinSymbol::ZhuyinSymbol() { initBuiltin(); }

const std::vector<std::string> &
ZhuyinSymbol::lookup(const std::string &key) const {
    if (auto iter = symbols_.find(key); iter != symbols_.end()) {
        return iter->second;
    }
    return empty;
}

void ZhuyinSymbol::load(std::FILE *file) {
    symbols_.clear();
    initBuiltin();

    if (!file) {
        for (char c = 'A'; c <= 'Z'; c++) {
            char latin[] = {c, '\0'};
            symbols_.erase(latin);
        }
        return;
    }
    UniqueCPtr<char> line;
    size_t size = 0;
    while (getline(line, &size, file) >= 0) {
        auto trimmed = stringutils::trim(line.get());
        if (trimmed.empty()) {
            continue;
        }

        auto space = trimmed.find(" ");
        auto key = trimmed.substr(0, space);
        auto value = trimmed.substr(space + 1);
        if (key.empty() || value.empty()) {
            continue;
        }

        std::vector<std::string> items;
        items.push_back(std::move(value));
        items.push_back(key);
        if (auto iter = symbolToGroup_.find(items[0]);
            iter != symbolToGroup_.end()) {
            for (const auto &item : groups_[iter->second]) {
                if (item != items[0]) {
                    items.push_back(item);
                }
            }
        }
        symbols_[key] = std::move(items);
    }
}

void ZhuyinSymbol::initBuiltin() {
    symbolToGroup_.clear();
    groups_.clear();
    constexpr const char *const symbolGroup[][50] = {
        {"0", "\xC3\xB8", nullptr},
        /* "ø" */
        {"[", "\xE3\x80\x8C", "\xE3\x80\x8E", "\xE3\x80\x8A", "\xE3\x80\x88",
         "\xE3\x80\x90", "\xE3\x80\x94", nullptr},
        /* "「", "『", "《", "〈", "【", "〔" */
        {"]", "\xE3\x80\x8D", "\xE3\x80\x8F", "\xE3\x80\x8B", "\xE3\x80\x89",
         "\xE3\x80\x91", "\xE3\x80\x95", nullptr},
        /* "」", "』", "》", "〉", "】", "〕" */
        {"{", "\xEF\xBD\x9B", nullptr},
        /* "｛" */
        {"}", "\xEF\xBD\x9D", nullptr},
        /* "｝" */
        {"<", "\xEF\xBC\x8C", "\xE2\x86\x90", nullptr},
        /* "，", "←" */
        {">", "\xE3\x80\x82", "\xE2\x86\x92", "\xEF\xBC\x8E", nullptr},
        /* "。", "→", "．" */
        {"?", "\xEF\xBC\x9F", "\xC2\xBF", nullptr},
        /* "？", "¿" */
        {"!", "\xEF\xBC\x81", "\xE2\x85\xA0", "\xC2\xA1", nullptr},
        /* "！", "Ⅰ","¡" */
        {"@", "\xEF\xBC\xA0", "\xE2\x85\xA1", "\xE2\x8A\x95", "\xE2\x8A\x99",
         "\xE3\x8A\xA3", "\xEF\xB9\xAB", nullptr},
        /* "＠", "Ⅱ", "⊕", "⊙", "㊣", "﹫" */
        {"#", "\xEF\xBC\x83", "\xE2\x85\xA2", "\xEF\xB9\x9F", nullptr},
        /* "＃", "Ⅲ", "﹟" */
        {"$", "\xEF\xBC\x84", "\xE2\x85\xA3", "\xE2\x82\xAC", "\xEF\xB9\xA9",
         "\xEF\xBF\xA0", "\xE2\x88\xAE", "\xEF\xBF\xA1", "\xEF\xBF\xA5",
         nullptr},
        /* "＄", "Ⅳ", "€", "﹩", "￠", "∮","￡", "￥" */
        {"%", "\xEF\xBC\x85", "\xE2\x85\xA4", nullptr},
        /* "％", "Ⅴ" */
        {"^", "\xEF\xB8\xBF", "\xE2\x85\xA5", "\xEF\xB9\x80", "\xEF\xB8\xBD",
         "\xEF\xB8\xBE", nullptr},
        /* "︿", "Ⅵ", "﹀", "︽", "︾" */
        {"&", "\xEF\xBC\x86", "\xE2\x85\xA6", "\xEF\xB9\xA0", nullptr},
        /* "＆", "Ⅶ", "﹠" */
        {"*", "\xEF\xBC\x8A", "\xE2\x85\xA7", "\xC3\x97", "\xE2\x80\xBB",
         "\xE2\x95\xB3", "\xEF\xB9\xA1", "\xE2\x98\xAF", "\xE2\x98\x86",
         "\xE2\x98\x85", nullptr},
        /* "＊", "Ⅷ", "×", "※", "╳", "﹡", "☯", "☆", "★" */
        {"(", "\xEF\xBC\x88", "\xE2\x85\xA8", nullptr},
        /* "（", "Ⅸ" */
        {")", "\xEF\xBC\x89", "\xE2\x85\xA9", nullptr},
        /* "）", "Ⅹ" */
        {"_", "\xEF\xBC\xBF", "\xE2\x80\xA6", "\xE2\x80\xA5", "\xE2\x86\x90",
         "\xE2\x86\x92", "\xEF\xB9\x8D", "\xEF\xB9\x89", "\xCB\x8D",
         "\xEF\xBF\xA3", "\xE2\x80\x93", "\xE2\x80\x94", "\xC2\xAF",
         "\xEF\xB9\x8A", "\xEF\xB9\x8E", "\xEF\xB9\x8F", "\xEF\xB9\xA3",
         "\xEF\xBC\x8D", nullptr},
        /* "＿", "…", "‥", "←", "→", "﹍", "﹉", "ˍ", "￣"
         * "–", "—", "¯", "﹊", "﹎", "﹏", "﹣", "－" */
        {"+", "\xEF\xBC\x8B", "\xC2\xB1", "\xEF\xB9\xA2", nullptr},
        /* "＋", "±", "﹢" */
        {"=", "\xEF\xBC\x9D", "\xE2\x89\x92", "\xE2\x89\xA0", "\xE2\x89\xA1",
         "\xE2\x89\xA6", "\xE2\x89\xA7", "\xEF\xB9\xA6", nullptr},
        /* "＝", "≒", "≠", "≡", "≦", "≧", "﹦" */
        {"`", "\xE3\x80\x8F", "\xE3\x80\x8E", "\xE2\x80\xB2", "\xE2\x80\xB5",
         nullptr},
        /* "』", "『", "′", "‵" */
        {"~", "\xEF\xBD\x9E", nullptr},
        /* "～" */
        {":", "\xEF\xBC\x9A", "\xEF\xBC\x9B", "\xEF\xB8\xB0", "\xEF\xB9\x95",
         nullptr},
        /* "：", "；", "︰", "﹕" */
        {"\"", "\xEF\xBC\x9B", nullptr},
        /* "；" */
        {"\'", "\xE3\x80\x81", "\xE2\x80\xA6", "\xE2\x80\xA5", nullptr},
        /* "、", "…", "‥" */
        {"\\", "\xEF\xBC\xBC", "\xE2\x86\x96", "\xE2\x86\x98", "\xEF\xB9\xA8",
         nullptr},
        /* "＼", "↖", "↘", "﹨" */
        {"-",
         "\xEF\xBC\x8D",
         "\xEF\xBC\xBF",
         "\xEF\xBF\xA3",
         "\xC2\xAF",
         "\xCB\x8D",
         "\xE2\x80\x93",
         "\xE2\x80\x94",
         "\xE2\x80\xA5",
         "\xE2\x80\xA6",
         "\xE2\x86\x90",
         "\xE2\x86\x92",
         "\xE2\x95\xB4",
         "\xEF\xB9\x89",
         "\xEF\xB9\x8A",
         "\xEF\xB9\x8D",
         "\xEF\xB9\x8E",
         "\xEF\xB9\x8F",
         "\xEF\xB9\xA3",
         nullptr},
        /* "－", "＿", "￣", "¯", "ˍ", "–", "—", "‥", "…"
         * "←", "→", "╴", "﹉", "﹊", "﹍", "﹎", "﹏", "﹣" */
        {"/", "\xEF\xBC\x8F", "\xC3\xB7", "\xE2\x86\x97", "\xE2\x86\x99",
         "\xE2\x88\x95", nullptr},
        /* "／","÷","↗","↙","∕" */
        {"|", "\xE2\x86\x91", "\xE2\x86\x93", "\xE2\x88\xA3", "\xE2\x88\xA5",
         "\xEF\xB8\xB1", "\xEF\xB8\xB3", "\xEF\xB8\xB4", 0},
        /* "↑", "↓", "∣", "∥", "︱", "︳", "︴" */
        {"A", "\xC3\x85", "\xCE\x91", "\xCE\xB1", "\xE2\x94\x9C",
         "\xE2\x95\xA0", "\xE2\x95\x9F", "\xE2\x95\x9E", nullptr},
        /* "Å","Α", "α", "├", "╠", "╟", "╞" */
        {"B", "\xCE\x92", "\xCE\xB2", "\xE2\x88\xB5", nullptr},
        /* "Β", "β","∵" */
        {"C", "\xCE\xA7", "\xCF\x87", "\xE2\x94\x98", "\xE2\x95\xAF",
         "\xE2\x95\x9D", "\xE2\x95\x9C", "\xE2\x95\x9B", "\xE3\x8F\x84",
         "\xE2\x84\x83", "\xE3\x8E\x9D", "\xE2\x99\xA3", "\xC2\xA9", nullptr},
        /* "Χ", "χ", "┘", "╯", "╝", "╜", "╛"
         * "㏄", "℃", "㎝", "♣", "©" */
        {"D", "\xCE\x94", "\xCE\xB4", "\xE2\x97\x87", "\xE2\x97\x86",
         "\xE2\x94\xA4", "\xE2\x95\xA3", "\xE2\x95\xA2", "\xE2\x95\xA1",
         "\xE2\x99\xA6", nullptr},
        /* "Δ", "δ", "◇", "◆", "┤", "╣", "╢", "╡","♦" */
        {"E", "\xCE\x95", "\xCE\xB5", "\xE2\x94\x90", "\xE2\x95\xAE",
         "\xE2\x95\x97", "\xE2\x95\x93", "\xE2\x95\x95", nullptr},
        /* "Ε", "ε", "┐", "╮", "╗", "╓", "╕" */
        {"F", "\xCE\xA6", "\xCF\x88", "\xE2\x94\x82", "\xE2\x95\x91",
         "\xE2\x99\x80", nullptr},
        /* "Φ", "ψ", "│", "║", "♀" */
        {"G", "\xCE\x93", "\xCE\xB3", nullptr},
        /* "Γ", "γ" */
        {"H", "\xCE\x97", "\xCE\xB7", "\xE2\x99\xA5", nullptr},
        /* "Η", "η","♥" */
        {"I", "\xCE\x99", "\xCE\xB9", nullptr},
        /* "Ι", "ι" */
        {"J", "\xCF\x86", nullptr},
        /* "φ" */
        {"K", "\xCE\x9A", "\xCE\xBA", "\xE3\x8E\x9E", "\xE3\x8F\x8E", nullptr},
        /* "Κ", "κ","㎞", "㏎" */
        {"L", "\xCE\x9B", "\xCE\xBB", "\xE3\x8F\x92", "\xE3\x8F\x91", nullptr},
        /* "Λ", "λ","㏒", "㏑" */
        {"M", "\xCE\x9C", "\xCE\xBC", "\xE2\x99\x82", "\xE2\x84\x93",
         "\xE3\x8E\x8E", "\xE3\x8F\x95", "\xE3\x8E\x9C", "\xE3\x8E\xA1",
         nullptr},
        /* "Μ", "μ", "♂", "ℓ", "㎎", "㏕", "㎜","㎡" */
        {"N", "\xCE\x9D", "\xCE\xBD", "\xE2\x84\x96", nullptr},
        /* "Ν", "ν","№" */
        {"O", "\xCE\x9F", "\xCE\xBF", nullptr},
        /* "Ο", "ο" */
        {"P", "\xCE\xA0", "\xCF\x80", nullptr},
        /* "Π", "π" */
        {"Q", "\xCE\x98", "\xCE\xB8", "\xD0\x94", "\xE2\x94\x8C",
         "\xE2\x95\xAD", "\xE2\x95\x94", "\xE2\x95\x93", "\xE2\x95\x92",
         nullptr},
        /* "Θ", "θ","Д","┌", "╭", "╔", "╓", "╒" */
        {"R", "\xCE\xA1", "\xCF\x81", "\xE2\x94\x80", "\xE2\x95\x90",
         "\xC2\xAE", nullptr},
        /* "Ρ", "ρ", "─", "═" ,"®" */
        {"S", "\xCE\xA3", "\xCF\x83", "\xE2\x88\xB4", "\xE2\x96\xA1",
         "\xE2\x96\xA0", "\xE2\x94\xBC", "\xE2\x95\xAC", "\xE2\x95\xAA",
         "\xE2\x95\xAB", "\xE2\x88\xAB", "\xC2\xA7", "\xE2\x99\xA0", nullptr},
        /* "Σ", "σ", "∴", "□", "■", "┼", "╬", "╪", "╫"
         * "∫", "§", "♠" */
        {"T", "\xCE\xA4", "\xCF\x84", "\xCE\xB8", "\xE2\x96\xB3",
         "\xE2\x96\xB2", "\xE2\x96\xBD", "\xE2\x96\xBC", "\xE2\x84\xA2",
         "\xE2\x8A\xBF", "\xE2\x84\xA2", nullptr},
        /* "Τ", "τ","θ","△","▲","▽","▼","™","⊿", "™" */
        {"U", "\xCE\xA5", "\xCF\x85", "\xCE\xBC", "\xE2\x88\xAA",
         "\xE2\x88\xA9", nullptr},
        /* "Υ", "υ","μ","∪", "∩" */
        {"V", "\xCE\xBD", nullptr},
        {"W", "\xE2\x84\xA6", "\xCF\x89", "\xE2\x94\xAC", "\xE2\x95\xA6",
         "\xE2\x95\xA4", "\xE2\x95\xA5", nullptr},
        /* "Ω", "ω", "┬", "╦", "╤", "╥" */
        {"X", "\xCE\x9E", "\xCE\xBE", "\xE2\x94\xB4", "\xE2\x95\xA9",
         "\xE2\x95\xA7", "\xE2\x95\xA8", nullptr},
        /* "Ξ", "ξ", "┴", "╩", "╧", "╨" */
        {"Y", "\xCE\xA8", nullptr},
        /* "Ψ" */
        {"Z", "\xCE\x96", "\xCE\xB6", "\xE2\x94\x94", "\xE2\x95\xB0",
         "\xE2\x95\x9A", "\xE2\x95\x99", "\xE2\x95\x98", nullptr},
        /* "Ζ", "ζ", "└", "╰", "╚", "╙", "╘" */
    };

    for (size_t i = 0; i < FCITX_ARRAY_SIZE(symbolGroup); i++) {
        symbolToGroup_[symbolGroup[i][0]] = groups_.size();

        std::vector<std::string> items;
        std::vector<std::string> items2;
        for (size_t j = 1; symbolGroup[i][j]; j++) {
            items.push_back(symbolGroup[i][j]);
            items2.push_back(symbolGroup[i][j]);
            symbolToGroup_[symbolGroup[i][j]] = groups_.size();
            if (j == 1) {
                items2.push_back(symbolGroup[i][0]);
            }
        }
        symbols_[symbolGroup[i][0]] = std::move(items2);
        groups_.push_back(std::move(items));
    }
}

} // namespace fcitx
