/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyinbuffer.h"
#include "zhuyincandidate.h"
#include <cassert>
#include <fcitx-utils/charutils.h>
#include <sstream>

namespace fcitx {

ZhuyinBuffer::ZhuyinBuffer(ZhuyinProviderInterface *provider)
    : provider_(provider), context_(provider->context()),
      instance_(zhuyin_alloc_instance(context_)) {
    // Put a place holder.
    sections_.emplace_back(ZhuyinSectionType::Symbol, provider_, this);
    cursor_ = sections_.begin();
}

void ZhuyinBuffer::moveCursorToBeginning() { cursor_ = sections_.begin(); }

void ZhuyinBuffer::moveCursorToEnd() {
    cursor_ = std::prev(sections_.end());
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        cursor_->setCursor(cursor_->size());
    }
}

bool ZhuyinBuffer::moveCursorLeft() {
    if (cursor_ == sections_.begin()) {
        return false;
    }
    // Move within the section.
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        if (auto prevChar = cursor_->prevChar(); prevChar > 0) {
            cursor_->setCursor(prevChar);
            return true;
        }
    }

    // Move across the section.
    cursor_ = std::prev(cursor_);
    // Fix the cursor within the section.
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        cursor_->setCursor(cursor_->size());
    }
    return true;
}

bool ZhuyinBuffer::moveCursorRight() {
    if (isCursorAtTheEnd()) {
        return false;
    }
    // Move within the section.
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin &&
        cursor_->cursor() < cursor_->size()) {
        cursor_->setCursor(cursor_->nextChar());
        return true;
    }

    cursor_ = std::next(cursor_);
    // Fix the cursor within the section.
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        cursor_->setCursor(0);
        cursor_->setCursor(cursor_->nextChar());
    }
    return true;
}

bool ZhuyinBuffer::isCursorAtTheEnd() const {
    return std::next(cursor_) == sections_.end() && isCursorOnEdge(cursor_);
}

bool ZhuyinBuffer::isCursorOnEdge(SectionIterator cursor) {
    return cursor->size() == cursor->cursor();
}

void ZhuyinBuffer::type(uint32_t c) {
    gchar **symbols = nullptr;
    if (c <= std::numeric_limits<unsigned char>::max() &&
        ((provider_->isZhuyin() &&
          zhuyin_in_chewing_keyboard(instance_.get(), c, &symbols)) ||
         (!provider_->isZhuyin() &&
          (charutils::islower(c) || (c >= '1' && c <= '5'))))) {
        g_strfreev(symbols);
        if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
            cursor_->type(c);
            if (isCursorOnEdge(cursor_) && c == ' ' &&
                cursor_->parsedZhuyinLength() != cursor_->size()) {
                backspace();
                auto next = std::next(cursor_);
                cursor_ = sections_.emplace(next, c, ZhuyinSectionType::Symbol,
                                            provider_, this);
            }
        } else if (isCursorOnEdge(cursor_)) {
            auto next = std::next(cursor_);
            if (c == ' ') {
                cursor_ = sections_.emplace(next, c, ZhuyinSectionType::Symbol,
                                            provider_, this);
            } else if (next == sections_.end() ||
                       next->sectionType() != ZhuyinSectionType::Zhuyin) {
                cursor_ = sections_.emplace(next, c, ZhuyinSectionType::Zhuyin,
                                            provider_, this);
            } else {
                next->setCursor(0);
                next->type(c);
                cursor_ = next;
            }
        }
    } else {
        if (isCursorOnEdge(cursor_)) {
            cursor_ =
                sections_.emplace(std::next(cursor_), c,
                                  ZhuyinSectionType::Symbol, provider_, this);
        } else {
            assert((cursor_)->sectionType() == ZhuyinSectionType::Zhuyin);
            assert(cursor_->cursor() != 0);

            auto next = std::next(cursor_);
            auto offset = cursor_->cursorByChar();
            auto subText = cursor_->userInput().substr(offset);

            cursor_->erase(offset, cursor_->size());

            // Add new symbol.
            cursor_ = sections_.emplace(next, c, ZhuyinSectionType::Symbol,
                                        provider_, this);
            // Add split section.
            auto subZhuyin = sections_.emplace(next, ZhuyinSectionType::Zhuyin,
                                               provider_, this);
            subZhuyin->type(subText);
        }
    }
}

void ZhuyinBuffer::backspace() {
    if (cursor_ == sections_.begin()) {
        return;
    }

    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        assert(cursor_->cursor() != 0);
        auto prevChar = cursor_->prevChar();
        cursor_->erase(prevChar, cursor_->cursor());
        // Remove this section.
        if (cursor_->empty()) {
            auto newCursor = std::prev(cursor_);
            sections_.erase(cursor_);
            cursor_ = newCursor;
        } else if (cursor_->cursor() == 0) {
            cursor_ = std::prev(cursor_);
        } else {
            return;
        }

        // Fix the cursor within the section.
        if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
            cursor_->setCursor(cursor_->size());
        }
        return;
    }

    auto newCursor = std::prev(cursor_);
    sections_.erase(cursor_);
    cursor_ = newCursor;
    if (cursor_->sectionType() == ZhuyinSectionType::Zhuyin) {
        cursor_->setCursor(cursor_->size());
        auto next = std::next(cursor_);
        if (next != sections_.end() &&
            next->sectionType() == ZhuyinSectionType::Zhuyin) {
            // Merge cursor_ and next.
            auto currentSize = cursor_->size();
            cursor_->type(next->userInput());
            cursor_->setCursor(currentSize);
            sections_.erase(next);
        }
    }
}

void ZhuyinBuffer::del() {
    if (moveCursorRight()) {
        backspace();
    }
}

Text ZhuyinBuffer::preedit() const {
    Text text;
    text.setCursor(0);
    for (auto iter = std::next(sections_.begin()), end = sections_.end();
         iter != end; ++iter) {
        auto [preedit, cursor] = iter->preeditWithCursor();
        if (cursor_ == iter) {
            text.setCursor(text.textLength() + cursor);
        }
        text.append(preedit, TextFormatFlag::Underline);
    }
    return text;
}

std::string ZhuyinBuffer::rawText() const {
    std::string result;
    for (auto iter = std::next(sections_.begin()), end = sections_.end();
         iter != end; ++iter) {
        result.append(iter->userInput());
    }
    return result;
}

void ZhuyinBuffer::learn() {
    for (auto iter = std::next(sections_.begin()), end = sections_.end();
         iter != end; ++iter) {
        iter->learn();
    }
}

void ZhuyinBuffer::reset() {
    sections_.erase(std::next(sections_.begin()), sections_.end());
    cursor_ = sections_.begin();
}

void ZhuyinBuffer::showCandidate(
    const std::function<void(std::unique_ptr<ZhuyinCandidate>)> &callback) {
    auto callbackWrapper =
        [this, &callback](std::unique_ptr<ZhuyinCandidate> candidate) {
            if (candidate->isZhuyin()) {
                auto *zhuyinCandidate =
                    static_cast<ZhuyinSectionCandidate *>(candidate.get());
                zhuyinCandidate->connect<ZhuyinSectionCandidate::selected>(
                    [this](SectionIterator iter) {
                        cursor_ = iter;
                        if (cursor_->cursor() == 0 &&
                            cursor_ != sections_.begin()) {
                            cursor_ = std::prev(cursor_);
                            cursor_->setCursor(cursor_->size());
                        }
                    });
            }
            callback(std::move(candidate));
        };
    if (isCursorAtTheEnd()) {
        cursor_->showCandidate(callbackWrapper, cursor_, cursor_->prevChar());
    } else if (isCursorOnEdge(cursor_)) {
        auto section = std::next(cursor_);
        section->showCandidate(callbackWrapper, section, 0);
    } else {
        cursor_->showCandidate(callbackWrapper, cursor_, cursor_->cursor());
    }
}

void ZhuyinBuffer::setZhuyinSymbolTo(SectionIterator iter, size_t offset,
                                     std::string symbol) {
    assert(iter->sectionType() == ZhuyinSectionType::Zhuyin);
    if (offset >= iter->size()) {
        return;
    }
    auto next = std::next(iter);
    auto beforeSize = offset;
    auto chr = iter->charAt(offset);
    auto after = iter->userInput().substr(offset + 1);
    if (beforeSize == 0) {
        sections_.erase(iter);
    } else {
        iter->erase(offset, iter->size());
    }
    auto newSymbol = sections_.emplace(next, chr, ZhuyinSectionType::Symbol,
                                       provider_, this);
    newSymbol->setSymbol(std::move(symbol));
    if (!after.empty()) {
        auto newSection =
            sections_.emplace(next, ZhuyinSectionType::Zhuyin, provider_, this);
        newSection->type(after);
    }
    cursor_ = newSymbol;
}

std::string ZhuyinBuffer::dump() const {
    bool first = true;
    std::stringstream sstream;
    sstream << "ZhuyinBuffer(";
    for (const auto &section : sections_) {
        if (first) {
            first = false;
            continue;
        }
        sstream << "<Preedit:\"" << section.preedit() << "\",Raw:\""
                << section.userInput() << "\",";
        switch (section.sectionType()) {
        case ZhuyinSectionType::Zhuyin:
            sstream << "Zhuyin";
            break;
        case ZhuyinSectionType::Symbol:
            sstream << "Symbol";
            break;
        }
        sstream << ">";
    }
    auto preeditText = preedit();
    sstream << "Preedit:" << preeditText.toString() << ","
            << preeditText.cursor() << ")";

    return sstream.str();
}

} // namespace fcitx
