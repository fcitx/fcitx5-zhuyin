/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINCANDIDATE_H_
#define _FCITX5_ZHUYIN_ZHUYINCANDIDATE_H_

#include "zhuyinsection.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx/candidatelist.h>

namespace fcitx {

// Base class for Zhuyin candidate.
class ZhuyinCandidate : public CandidateWord, public ConnectableObject {
public:
    virtual bool isZhuyin() const { return false; };
    FCITX_DECLARE_SIGNAL(ZhuyinCandidate, selected, void());

private:
    FCITX_DEFINE_SIGNAL(ZhuyinCandidate, selected);
};

// Candidate for zhuyin section.
class ZhuyinSectionCandidate : public ZhuyinCandidate {
public:
    ZhuyinSectionCandidate(SectionIterator section, unsigned int i);
    bool isZhuyin() const override { return true; }
    void select(InputContext *) const override;
    FCITX_DECLARE_SIGNAL(ZhuyinSectionCandidate, selected,
                         void(SectionIterator));

private:
    FCITX_DEFINE_SIGNAL(ZhuyinSectionCandidate, selected);
    SectionIterator section_;
    unsigned int index_;
};

// Candidate for symbol section.
class SymbolSectionCandidate : public ZhuyinCandidate {
public:
    SymbolSectionCandidate(SectionIterator section, std::string symbol);
    void select(InputContext *) const override;

protected:
    FCITX_DEFINE_SIGNAL(ZhuyinSectionCandidate, selected);
    SectionIterator section_;
    std::string symbol_;
};

// Candidate for symbol section.
class SymbolZhuyinSectionCandidate : public SymbolSectionCandidate {
public:
    SymbolZhuyinSectionCandidate(SectionIterator section, std::string symbol,
                                 size_t offset);
    void select(InputContext *) const override;

private:
    size_t offset_;
};

} // namespace fcitx

#endif // _FCITX5_ZHUYIN_ZHUYINCANDIDATE_H_
