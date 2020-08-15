/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyinbuffer.h"
#include <cassert>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utf8.h>
#include <sstream>
#include <stdexcept>

namespace fcitx {

ZhuyinSectionCandidate::ZhuyinSectionCandidate(SectionIterator section,
                                               unsigned int i)

    : section_(section), index_(i) {
    lookup_candidate_t *candidate = NULL;
    if (!zhuyin_get_candidate(section->instance(), i, &candidate)) {
        throw std::runtime_error("Failed to get candidate");
    }

    const gchar *word = NULL;
    if (!zhuyin_get_candidate_string(section->instance(), candidate, &word)) {
        throw std::runtime_error("Failed to get string");
    }
    setText(Text(word));
}

void ZhuyinSectionCandidate::select(InputContext *) const {
    lookup_candidate_t *candidate = NULL;
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

void SymbolSectionCandidate::select(InputContext *) const {
    section_->setSymbol(symbol_);
    emit<ZhuyinCandidate::selected>();
}

SymbolZhuyinSectionCandidate::SymbolZhuyinSectionCandidate(
    SectionIterator section, std::string symbol, size_t offset)

    : SymbolSectionCandidate(section, symbol), offset_(offset) {}

void SymbolZhuyinSectionCandidate::select(InputContext *) const {
    section_->buffer()->setZhuyinSymbolTo(section_, offset_, symbol_);
    emit<ZhuyinCandidate::selected>();
}

ZhuyinSection::ZhuyinSection(ZhuyinSectionType type,
                             ZhuyinProviderInterface *provider,
                             ZhuyinBuffer *buffer)
    : InputBuffer(type == ZhuyinSectionType::Zhuyin
                      ? InputBufferOption::AsciiOnly
                      : InputBufferOption::FixedCursor),
      provider_(provider), buffer_(buffer), type_(type),
      instance_(type == ZhuyinSectionType::Zhuyin
                    ? zhuyin_alloc_instance(provider_->context())
                    : nullptr) {}

ZhuyinSection::ZhuyinSection(uint32_t init, ZhuyinSectionType type,
                             ZhuyinProviderInterface *provider,
                             ZhuyinBuffer *buffer)
    : ZhuyinSection(type, provider, buffer) {
    this->type(init);
}

bool ZhuyinSection::typeImpl(const char *s, size_t length) {
    InputBuffer::typeImpl(s, length);
    if (!instance_) {
        const auto &candidates = provider_->symbol().lookup(userInput());
        if (candidates.empty()) {
            currentSymbol_ = userInput();
        } else {
            currentSymbol_ = candidates[0];
        }
        return true;
    }
    if (provider_->isZhuyin()) {
        zhuyin_parse_more_chewings(instance_.get(), userInput().data());
    } else {
        zhuyin_parse_more_full_pinyins(instance_.get(), userInput().data());
    }
    zhuyin_guess_sentence(instance_.get());
    return true;
}

size_t ZhuyinSection::prevChar() const {
    if (cursor() == 0 || !instance_) {
        return 0;
    }

    auto length = parsedZhuyinLength();
    if (cursor() > length) {
        return cursor() - 1;
    }
    size_t offset = 0;
    size_t prevCursor = cursor() - 1;
    zhuyin_get_zhuyin_offset(instance_.get(), prevCursor, &offset);
    return offset;
}

size_t ZhuyinSection::nextChar() const {
    if (cursor() == size()) {
        return cursor();
    }

    auto length = parsedZhuyinLength();
    if (cursor() + 1 >= length) {
        return cursor() + 1;
    }
    size_t offset = 0, right = 0;
    size_t nextCursor = cursor() + 1;
    zhuyin_get_zhuyin_offset(instance_.get(), nextCursor, &offset);
    zhuyin_get_right_zhuyin_offset(instance_.get(), offset, &right);
    return right;
}

void ZhuyinSection::erase(size_t from, size_t to) {
    InputBuffer::erase(from, to);
    if (provider_->isZhuyin()) {
        zhuyin_parse_more_chewings(instance_.get(), userInput().data());
    } else {
        zhuyin_parse_more_full_pinyins(instance_.get(), userInput().data());
    }
    zhuyin_guess_sentence(instance_.get());
}

void ZhuyinSection::setSymbol(std::string symbol) {
    currentSymbol_ = std::move(symbol);
}

std::string ZhuyinSection::preedit() const { return preeditWithCursor().first; }

void ZhuyinSection::learn() {
    if (!instance_) {
        return;
    }
    zhuyin_train(instance_.get());
}

std::pair<std::string, size_t> ZhuyinSection::preeditWithCursor() const {
    std::string result;
    if (!instance_) {
        return {currentSymbol_, currentSymbol_.size()};
    }

    auto length = parsedZhuyinLength();
    char *sentence = nullptr;
    if (length) {
        zhuyin_get_sentence(instance_.get(), &sentence);
    }

    if (sentence) {
        result.append(sentence);
    }

    auto preeditCursor = cursor();
    if (preeditCursor >= length) {
        preeditCursor -= length;
        if (sentence) {
            preeditCursor += strlen(sentence);
        }
    } else {
        size_t offset;
        zhuyin_get_character_offset(instance_.get(), sentence, cursor(),
                                    &offset);
        preeditCursor = utf8::ncharByteLength(sentence, offset);
    }

    free(sentence);
    for (; length < size(); length++) {
        if (provider_->isZhuyin()) {
            gchar **symbols = nullptr;
            zhuyin_in_chewing_keyboard(instance_.get(), charAt(length),
                                       &symbols);
            if (symbols && symbols[0]) {
                result.append(symbols[0]);
            }
            g_strfreev(symbols);
        } else {
            result.push_back(static_cast<char>(charAt(length)));
        }

        if (length + 1 == cursor()) {
            preeditCursor = result.size();
        }
    }
    return {result, preeditCursor};
}

size_t ZhuyinSection::parsedZhuyinLength() const {
    if (!instance_) {
        throw std::runtime_error("Bug in fcitx5-zhuyin");
    }
    return zhuyin_get_parsed_input_length(instance_.get());
}

void ZhuyinSection::showCandidate(
    const std::function<void(std::unique_ptr<ZhuyinCandidate>)> &callback,
    SectionIterator iter, size_t offset) {
    assert(&*iter == this);
    if (!instance_) {
        if (size() == 1) {
            auto c = charAt(offset);
            gchar **symbols = nullptr;
            if (c < std::numeric_limits<signed char>::max() &&
                zhuyin_in_chewing_keyboard(buffer_->instance(), c, &symbols)) {
                // Make sure we have two symbol.
                if (symbols[0] && symbols[1]) {
                    for (size_t i = 0; symbols[i]; i++) {
                        callback(std::make_unique<SymbolSectionCandidate>(
                            iter, symbols[i]));
                    }
                }
                g_strfreev(symbols);
                return;
            }
        }

        auto candidates = provider_->symbol().lookup(userInput());
        if (candidates.empty()) {
            return;
        }
        for (const auto &symbol : candidates) {
            callback(std::make_unique<SymbolSectionCandidate>(iter, symbol));
        }
        return;
    }

    if (offset >= parsedZhuyinLength()) {
        if (!provider_->isZhuyin() || offset >= size()) {
            return;
        }
        auto c = charAt(offset);
        gchar **symbols = nullptr;
        if (c < std::numeric_limits<signed char>::max() &&
            zhuyin_in_chewing_keyboard(instance_.get(), c, &symbols)) {
            // Make sure we have two symbol.
            if (symbols[0] && symbols[1]) {
                for (size_t i = 0; symbols[i]; i++) {
                    callback(std::make_unique<SymbolZhuyinSectionCandidate>(
                        iter, symbols[i], offset));
                }
            }
            g_strfreev(symbols);
        }
        return;
    }

    zhuyin_get_zhuyin_offset(instance_.get(), offset, &offset);
    zhuyin_guess_candidates_after_cursor(instance_.get(), offset);
    guint len = 0;
    zhuyin_get_n_candidate(instance_.get(), &len);
    for (size_t i = 0; i < len; i++) {
        callback(std::make_unique<ZhuyinSectionCandidate>(iter, i));
    }
}

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
          (charutils::islower(c) /* || c == '\'' */)))) {
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
        if (cursor_->size() == 0) {
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
                auto zhuyinCandidate =
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
    newSymbol->setSymbol(symbol);
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
