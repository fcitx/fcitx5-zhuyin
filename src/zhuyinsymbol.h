/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINSYMBOL_H_
#define _FCITX5_ZHUYIN_ZHUYINSYMBOL_H_

#include <cstdio>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fcitx {

class ZhuyinSymbol {
public:
    ZhuyinSymbol();
    void load(std::istream &in);
    const std::vector<std::string> &lookup(const std::string &key) const;
    void reset();
    void initBuiltin();
    void clear();

private:
    std::unordered_map<std::string, std::vector<std::string>> symbols_;
    std::unordered_map<std::string, size_t> symbolToGroup_;
    std::vector<std::vector<std::string>> groups_;
};

} // namespace fcitx

#endif // _FCITX5_ZHUYIN_ZHUYINSYMBOL_H_
