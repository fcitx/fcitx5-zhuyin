/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINBUFFER_H_
#define _FCITX5_ZHUYIN_ZHUYINBUFFER_H_

#include "zhuyinsection.h"
#include "zhuyinsymbol.h"
#include <fcitx-utils/misc.h>
#include <fcitx/text.h>
#include <list>
#include <variant>
#include <zhuyin.h>

namespace fcitx {

class ZhuyinBuffer;
class ZhuyinSection;

// Helper class to separate the data needed from engine.
class ZhuyinProviderInterface {
public:
  virtual zhuyin_context_t *context() = 0;
  virtual bool isZhuyin() const = 0;
  virtual const ZhuyinSymbol &symbol() const = 0;
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
