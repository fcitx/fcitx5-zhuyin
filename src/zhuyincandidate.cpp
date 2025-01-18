/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyincandidate.h"
#include "zhuyinbuffer.h"
#include "zhuyinsection.h"
#include <cstddef>
#include <fcitx/candidatelist.h>
#include <fcitx/text.h>
#include <glib.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <zhuyin.h>

namespace fcitx {

ZhuyinSectionCandidate::ZhuyinSectionCandidate(SectionIterator section,
                                               unsigned int i)

    : section_(section), index_(i) {
    lookup_candidate_t *candidate = nullptr;
    if (!zhuyin_get_candidate(section->instance(), i, &candidate)) {
        throw std::runtime_error("Failed to get candidate");
    }

    const gchar *word = nullptr;
    if (!zhuyin_get_candidate_string(section->instance(), candidate, &word)) {
        throw std::runtime_error("Failed to get string");
    }
    setText(Text(word));
}

void ZhuyinSectionCandidate::select(InputContext * /*inputContext*/) const {
    lookup_candidate_t *candidate = nullptr;
    if (!zhuyin_get_candidate(section_->instance(), index_, &candidate)) {
        return;
    }
    auto newOffset = zhuyin_choose_candidate(section_->instance(),
                                             section_->prevChar(), candidate);
    zhuyin_guess_sentence(section_->instance());
    section_->setCursor(newOffset);
    emit<ZhuyinSectionCandidate::selected>(section_);
    emit<ZhuyinCandidate::selected>();
}

SymbolSectionCandidate::SymbolSectionCandidate(SectionIterator section,
                                               std::string symbol)

    : section_(section), symbol_(std::move(symbol)) {
    setText(Text(symbol_));
}

void SymbolSectionCandidate::select(InputContext * /*inputContext*/) const {
    section_->setSymbol(symbol_);
    emit<ZhuyinCandidate::selected>();
}

SymbolZhuyinSectionCandidate::SymbolZhuyinSectionCandidate(
    SectionIterator section, std::string symbol, size_t offset)

    : SymbolSectionCandidate(section, std::move(symbol)), offset_(offset) {}

void SymbolZhuyinSectionCandidate::select(InputContext * /*unused*/) const {
    section_->buffer()->setZhuyinSymbolTo(section_, offset_, symbol_);
    emit<ZhuyinCandidate::selected>();
}

} // namespace fcitx
