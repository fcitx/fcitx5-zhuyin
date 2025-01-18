/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINSECTION_H_
#define _FCITX5_ZHUYIN_ZHUYINSECTION_H_

#include <cstddef>
#include <cstdint>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx-utils/misc.h>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <zhuyin.h>

namespace fcitx {

class ZhuyinBuffer;
class ZhuyinSection;
class ZhuyinCandidate;
class ZhuyinProviderInterface;
using SectionIterator = std::list<ZhuyinSection>::iterator;

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

} // namespace fcitx

#endif // _FCITX5_ZHUYIN_ZHUYINSECTION_H_
