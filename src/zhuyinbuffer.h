/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINBUFFER_H_
#define _FCITX5_ZHUYIN_ZHUYINBUFFER_H_

#include "zhuyinsymbol.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx-utils/misc.h>
#include <fcitx/candidatelist.h>
#include <fcitx/text.h>
#include <list>
#include <variant>
#include <zhuyin.h>

namespace fcitx {

class ZhuyinBuffer;
class ZhuyinSection;
using SectionIterator = std::list<ZhuyinSection>::iterator;

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

// Helper class to separate the data needed from engine.
class ZhuyinProviderInterface {
public:
    virtual zhuyin_context_t *context() = 0;
    virtual bool isZhuyin() const = 0;
    virtual const ZhuyinSymbol &symbol() const = 0;
};

enum class ZhuyinSectionType {
    Zhuyin,
    Symbol,
};

// A section of preedit, it can be either a single symbol, or a series of
// Zhuyin.
class ZhuyinSection : public InputBuffer {
public:
    ZhuyinSection(ZhuyinSectionType type, ZhuyinProviderInterface *provider,
                  ZhuyinBuffer *buffer);
    ZhuyinSection(uint32_t init, ZhuyinSectionType type,
                  ZhuyinProviderInterface *provider, ZhuyinBuffer *buffer);

    ZhuyinSectionType sectionType() const { return type_; }

    size_t parsedZhuyinLength() const;

    std::string preedit() const;
    std::pair<std::string, size_t> preeditWithCursor() const;
    size_t prevChar() const;
    size_t nextChar() const;

    void erase(size_t from, size_t to) override;
    void setSymbol(std::string symbol);

    void showCandidate(
        const std::function<void(std::unique_ptr<ZhuyinCandidate>)> &callback,
        SectionIterator iter, size_t offset);
    void learn();
    auto instance() const { return instance_.get(); }
    auto buffer() const { return buffer_; }

protected:
    bool typeImpl(const char *s, size_t length) override;

private:
    ZhuyinProviderInterface *provider_;
    ZhuyinBuffer *buffer_;
    const ZhuyinSectionType type_;
    std::string currentSymbol_;
    UniqueCPtr<zhuyin_instance_t, zhuyin_free_instance> instance_;
};

// Class that manages a list of ZhuyinSection.
// There is no zhuyin section that close to anotehr zhuyin section.
// An empty place holder symbol section is places at the beginning,
// to simplify the code need to handle the section logic.
class ZhuyinBuffer {
public:
    ZhuyinBuffer(ZhuyinProviderInterface *provider);
    // Type a single character into the buffer.
    void type(uint32_t c);
    bool empty() const { return sections_.size() == 1; }
    // Clear the buffer.
    void reset();

    std::string text() const { return preedit().toStringForCommit(); }
    std::string rawText() const;
    bool isCursorAtTheEnd() const;

    Text preedit() const;

    bool moveCursorLeft();
    bool moveCursorRight();
    void moveCursorToBeginning();
    void moveCursorToEnd();
    void del();
    void backspace();
    void learn();

    void showCandidate(
        const std::function<void(std::unique_ptr<ZhuyinCandidate>)> &callback);
    void setZhuyinSymbolTo(SectionIterator iter, size_t offset,
                           std::string symbol);

    std::string dump() const;

    auto instance() const { return instance_.get(); }

private:
    static bool isCursorOnEdge(SectionIterator cursor);

    ZhuyinProviderInterface *provider_;
    zhuyin_context_t *context_;
    // Dummy instance for query.
    UniqueCPtr<zhuyin_instance_t, zhuyin_free_instance> instance_;
    SectionIterator cursor_;
    std::list<ZhuyinSection> sections_;
};

} // namespace fcitx

#endif // _FCITX5_ZHUYIN_ZHUYINBUFFER_H_
