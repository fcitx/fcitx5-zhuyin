/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyinsection.h"
#include "zhuyinbuffer.h"
#include "zhuyincandidate.h"
#include <limits>
#include <fcitx-utils/utf8.h>

namespace fcitx {

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

} // namespace fcitx
