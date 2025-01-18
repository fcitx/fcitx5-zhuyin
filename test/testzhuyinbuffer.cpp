/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "testdir.h"
#include "zhuyinbuffer.h"
#include "zhuyincandidate.h"
#include "zhuyinsymbol.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/misc.h>
#include <memory>
#include <zhuyin.h>

using namespace fcitx;

class TestZhuyinProvider : public ZhuyinProviderInterface {
public:
    TestZhuyinProvider() {
        context_.reset(
            zhuyin_init(TESTING_BINARY_DIR "/data", "/Invalid/Path"));
        zhuyin_set_options(context_.get(), USE_TONE | ZHUYIN_CORRECT_ALL |
                                               FORCE_TONE | DYNAMIC_ADJUST);
        zhuyin_set_chewing_scheme(context_.get(), ZHUYIN_STANDARD);
    }

    zhuyin_context_t *context() override { return context_.get(); }

    bool isZhuyin() const override { return true; }
    const ZhuyinSymbol &symbol() const override { return symbol_; }

private:
    ZhuyinSymbol symbol_;
    UniqueCPtr<zhuyin_context_t, zhuyin_fini> context_;
};

void test_basic() {
    TestZhuyinProvider provider;
    ZhuyinBuffer buffer(&provider);
    buffer.type('z');
    FCITX_INFO() << buffer.dump();
    buffer.type('p');
    FCITX_INFO() << buffer.dump();
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();

    buffer.backspace();
    FCITX_INFO() << buffer.dump();

    buffer.reset();

    FCITX_INFO() << buffer.dump();
    buffer.type('z');
    buffer.type('\\');
    buffer.type('p');
    buffer.type('\\');
    buffer.type('p');
    buffer.type('a');
    FCITX_INFO() << buffer.dump();

    buffer.moveCursorLeft();
    buffer.type('b');
    buffer.type('c');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorLeft();
    buffer.moveCursorLeft();
    buffer.moveCursorLeft();
    buffer.moveCursorLeft();
    buffer.type('e');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorRight();
    buffer.type('e');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorLeft();
    buffer.del();
    FCITX_INFO() << buffer.dump();
    buffer.reset();
    buffer.type('z');
    buffer.type(0x263a);
    buffer.type('p');
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    buffer.backspace();
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.reset();
    buffer.type('z');
    buffer.type('p');
    buffer.moveCursorLeft();
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorLeft();
    buffer.del();
    buffer.moveCursorRight();
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.reset();
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorLeft();
    buffer.type('a');
    buffer.type('p');
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.moveCursorRight();
    buffer.type('a');
    buffer.type('p');
    buffer.type(' ');
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    FCITX_INFO() << buffer.dump();
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    buffer.moveCursorToBeginning();
    FCITX_INFO() << buffer.dump();
    buffer.type('.');
    buffer.type('p');
    FCITX_INFO() << buffer.dump();
    buffer.backspace();
    FCITX_INFO() << buffer.dump();
    buffer.reset();
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    buffer.type('n');
    buffer.type('i');
    buffer.type('f');
    buffer.moveCursorToBeginning();
    buffer.moveCursorRight();
    buffer.del();
    FCITX_INFO() << buffer.dump();
}

void test_candidate() {
    TestZhuyinProvider provider;
    ZhuyinBuffer buffer(&provider);
    auto printCandidate = [](std::unique_ptr<ZhuyinCandidate> candidate) {
        FCITX_INFO() << candidate->text().toString();
    };
    buffer.type('z');
    buffer.type('p');
    buffer.type(' ');
    buffer.type('a');
    buffer.type('p');
    buffer.type(' ');
    buffer.showCandidate(printCandidate);
    buffer.moveCursorLeft();
    buffer.showCandidate(printCandidate);
    buffer.moveCursorLeft();
    buffer.showCandidate(printCandidate);
}

int main() {
    test_basic();
    test_candidate();
    return 0;
}
