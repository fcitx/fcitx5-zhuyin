/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "zhuyinengine.h"
#include "zhuyincandidate.h"
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/instance.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <zhuyin.h>

FCITX_DEFINE_LOG_CATEGORY(zhuyin, "zhuyin");
#define ZHUYIN_DEBUG() FCITX_LOGC(zhuyin, Debug)

namespace fcitx {

constexpr size_t MAX_INPUT_LENGTH = 30;

ZhuyinState::ZhuyinState(ZhuyinEngine *engine, InputContext *ic)
    : engine_(engine), buffer_(engine), ic_(ic) {}

void ZhuyinState::reset() {
    buffer_.reset();
    updateUI();
}

void ZhuyinState::commit() {
    ic_->commitString(buffer_.text());
    buffer_.learn();
    reset();
}

void ZhuyinState::keyEvent(KeyEvent &keyEvent) {
    auto *ic = keyEvent.inputContext();
    auto key = keyEvent.key();
    if (keyEvent.isRelease()) {
        return;
    }

    if (auto candidateList = ic->inputPanel().candidateList();
        candidateList && candidateList->size()) {
        auto idx = key.keyListIndex(engine_->selectionKeys());
        if (idx >= 0) {
            candidateList->candidate(idx).select(ic);
            return keyEvent.filterAndAccept();
        }

        if (candidateList->cursorIndex() >= 0 &&
            (key.check(FcitxKey_space) || key.check(FcitxKey_Return))) {
            candidateList->candidate(candidateList->cursorIndex()).select(ic);
            return keyEvent.filterAndAccept();
        }

        if (key.checkKeyList(*engine_->config().prevPage)) {
            candidateList->toPageable()->prev();
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }

        if (key.checkKeyList(*engine_->config().nextPage)) {
            candidateList->toPageable()->next();
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }

        if (key.checkKeyList(*engine_->config().prevCandidate)) {
            candidateList->toCursorMovable()->prevCandidate();
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }

        if (key.checkKeyList(*engine_->config().nextCandidate)) {
            candidateList->toCursorMovable()->nextCandidate();
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }

        if (key.check(FcitxKey_Escape)) {
            ic->inputPanel().setCandidateList(nullptr);
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Home)) {
            candidateList->toPageable()->setPage(0);
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_End)) {
            candidateList->toPageable()->setPage(
                candidateList->toPageable()->totalPages() - 1);
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return keyEvent.filterAndAccept();
        }

        return keyEvent.filterAndAccept();
    }

    if (!buffer_.empty()) {
        if (key.check(FcitxKey_Home)) {
            buffer_.moveCursorToBeginning();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_End)) {
            buffer_.moveCursorToEnd();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Escape)) {
            reset();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_BackSpace)) {
            buffer_.backspace();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Delete)) {
            buffer_.del();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Return)) {
            commit();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Left)) {
            buffer_.moveCursorLeft();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Right)) {
            buffer_.moveCursorRight();
            updateUI();
            return keyEvent.filterAndAccept();
        }
        if (key.check(FcitxKey_Return, KeyState::Shift)) {
            ic->commitString(buffer_.rawText());
            reset();
            return keyEvent.filterAndAccept();
        }

        if (key.check(FcitxKey_Down)) {
            updateUI(true);
        }

        if (key.isCursorMove()) {
            return keyEvent.filterAndAccept();
        }
    }

    auto c = Key::keySymToUnicode(key.sym());
    if (buffer_.empty()) {
        if (key.check(*engine_->config().quickphraseKey) &&
            engine_->quickphrase()) {
            std::string keyString;
            std::string output, altOutput;
            if (c) {
                keyString = utf8::UCS4ToUTF8(c);
                altOutput = keyString;
            } else {
                keyString = engine_->config().quickphraseKey->toString(
                    KeyStringFormat::Localized);
            }
            output = *engine_->config().quickphraseKeySymbol;
            if (output.empty()) {
                output = altOutput;
                altOutput.clear();
            }

            if (!output.empty() && !altOutput.empty()) {
                std::string text =
                    fmt::format(_("Press {} for {} and Return for {}"),
                                keyString, output, altOutput);
                engine_->quickphrase()->call<IQuickPhrase::trigger>(
                    ic, text, "", output, altOutput,
                    *engine_->config().quickphraseKey);
            } else if (!output.empty()) {
                std::string text =
                    fmt::format(_("Press {} for {}"), keyString, output);
                engine_->quickphrase()->call<IQuickPhrase::trigger>(
                    ic, text, "", output, altOutput,
                    *engine_->config().quickphraseKey);
            } else {
                engine_->quickphrase()->call<IQuickPhrase::trigger>(
                    ic, "", "", "", "", Key());
            }

            keyEvent.filterAndAccept();
            return;
        }
    }

    if (key.hasModifier()) {
        return;
    }

    if (c <= std::numeric_limits<signed char>::max() &&
        !charutils::isprint(c)) {
        if (!buffer_.empty()) {
            keyEvent.filterAndAccept();
        }
        return;
    }

    if (c) {
        buffer_.type(c);
        if (utf8::length(buffer_.preedit().toStringForCommit()) >
            MAX_INPUT_LENGTH) {
            ic->commitString(buffer_.text());
            buffer_.learn();
            reset();
        } else {
            updateUI();
        }
        keyEvent.filterAndAccept();
    }
}

void ZhuyinState::updateUI(bool showCandidate) {
    ic_->inputPanel().reset();
    Text preedit(buffer_.preedit());
    if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
        ic_->inputPanel().setClientPreedit(preedit);
        ic_->updatePreedit();
    } else {
        ic_->inputPanel().setPreedit(preedit);
    }

    if (showCandidate) {
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setCursorPositionAfterPaging(
            CursorPositionAfterPaging::SameAsLast);
        candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
        candidateList->setPageSize(*engine_->config().pageSize);
        candidateList->setSelectionKey(engine_->selectionKeys());
        buffer_.showCandidate(
            [this, &candidateList](std::unique_ptr<ZhuyinCandidate> candidate) {
                candidate->connect<ZhuyinCandidate::selected>(
                    [this]() { updateUI(); });
                candidateList->append(std::move(candidate));
            });
        if (candidateList->size()) {
            candidateList->setGlobalCursorIndex(0);
            ic_->inputPanel().setCandidateList(std::move(candidateList));
        }
    }

    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

ZhuyinEngine::ZhuyinEngine(Instance *instance)
    : instance_(instance), factory_([this](InputContext &ic) {
          return new ZhuyinState(this, &ic);
      }) {
    auto userDir = stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        "zhuyin");
    if (!fs::makePath(userDir)) {
        if (fs::isdir(userDir)) {
            ZHUYIN_DEBUG() << "Failed to create user directory: " << userDir;
        }
    }
    context_.reset(
        zhuyin_init(StandardPath::fcitxPath("pkgdatadir", "zhuyin").data(),
                    userDir.data()));

    instance->inputContextManager().registerProperty("zhuyinState", &factory_);
    reloadConfig();
}

void ZhuyinEngine::activate(const InputMethodEntry &,
                            InputContextEvent &event) {
    auto *inputContext = event.inputContext();
    // Request full width.
    fullwidth();
    chttrans();
    for (const auto *actionName : {"chttrans", "punctuation", "fullwidth"}) {
        if (auto *action =
                instance_->userInterfaceManager().lookupAction(actionName)) {
            inputContext->statusArea().addAction(StatusGroup::InputMethod,
                                                 action);
        }
    }
}

void ZhuyinEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
    auto *inputContext = event.inputContext();
    if (event.type() == EventType::InputContextSwitchInputMethod) {
        if (*config_.commitOnSwitch) {
            auto *state = event.inputContext()->propertyFor(&factory_);
            state->commit();
        }
    }
    reset(entry, event);
    inputContext->statusArea().clearGroup(StatusGroup::InputMethod);
}

void ZhuyinEngine::keyEvent(const InputMethodEntry &, KeyEvent &keyEvent) {
    auto *state = keyEvent.inputContext()->propertyFor(&factory_);
    state->keyEvent(keyEvent);
}

void ZhuyinEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto *state = event.inputContext()->propertyFor(&factory_);
    state->reset();
}

void ZhuyinEngine::save() { zhuyin_save(context_.get()); }
void ZhuyinEngine::setConfig(const RawConfig &rawConfig) {
    config_.load(rawConfig, true);
    safeSaveAsIni(config_, "conf/zhuyin.conf");
    reloadConfig();
}
const Configuration *ZhuyinEngine::getConfig() const { return &config_; }

void ZhuyinEngine::reloadConfig() {
    readAsIni(config_, "conf/zhuyin.conf");
    // Keep fd live before fclose.
    StandardPathFile fd;
    UniqueFilePtr file;
    if (*config_.useEasySymbol) {
        fd = StandardPath::global().open(StandardPath::Type::PkgData,
                                         "zhuyin/easysymbols.txt", O_RDONLY);
        if (fd.fd() >= 0) {
            file.reset(fdopen(fd.fd(), "r"));
        }
    }
    symbol_.load(file.get());

    isZhuyin_ = true;
    ZhuyinScheme scheme = ZHUYIN_STANDARD;
    FullPinyinScheme pyScheme = FULL_PINYIN_HANYU;
    switch (*config_.layout) {
    case Scheme::Standard:
        scheme = ZHUYIN_STANDARD;
        break;
    case Scheme::Hsu:
        scheme = ZHUYIN_HSU;
        break;
    case Scheme::IBM:
        scheme = ZHUYIN_IBM;
        break;
    case Scheme::GinYieh:
        scheme = ZHUYIN_GINYIEH;
        break;
    case Scheme::Eten:
        scheme = ZHUYIN_ETEN;
        break;
    case Scheme::Eten26:
        scheme = ZHUYIN_ETEN26;
        break;
    case Scheme::Dvorak:
        scheme = ZHUYIN_STANDARD_DVORAK;
        break;
    case Scheme::HsuDvorak:
        scheme = ZHUYIN_HSU_DVORAK;
        break;
    case Scheme::DachenCP26:
        scheme = ZHUYIN_DACHEN_CP26;
        break;
    case Scheme::Hanyu:
        pyScheme = FULL_PINYIN_HANYU;
        isZhuyin_ = false;
        break;
    case Scheme::Luoma:
        pyScheme = FULL_PINYIN_LUOMA;
        isZhuyin_ = false;
        break;
    case Scheme::SecondaryZhuyin:
        pyScheme = FULL_PINYIN_SECONDARY_ZHUYIN;
        isZhuyin_ = false;
        break;
    }
    if (isZhuyin_) {
        zhuyin_set_chewing_scheme(context_.get(), scheme);
    } else {
        zhuyin_set_full_pinyin_scheme(context_.get(), pyScheme);
    }

    constexpr KeySym syms[][10] = {
        {
            FcitxKey_1,
            FcitxKey_2,
            FcitxKey_3,
            FcitxKey_4,
            FcitxKey_5,
            FcitxKey_6,
            FcitxKey_7,
            FcitxKey_8,
            FcitxKey_9,
            FcitxKey_0,
        },

        {
            FcitxKey_a,
            FcitxKey_s,
            FcitxKey_d,
            FcitxKey_f,
            FcitxKey_g,
            FcitxKey_h,
            FcitxKey_j,
            FcitxKey_k,
            FcitxKey_l,
            FcitxKey_semicolon,
        },

        {
            FcitxKey_a,
            FcitxKey_s,
            FcitxKey_d,
            FcitxKey_f,
            FcitxKey_z,
            FcitxKey_x,
            FcitxKey_c,
            FcitxKey_v,
            FcitxKey_8,
            FcitxKey_9,
        },

        {
            FcitxKey_a,
            FcitxKey_s,
            FcitxKey_d,
            FcitxKey_f,
            FcitxKey_j,
            FcitxKey_k,
            FcitxKey_l,
            FcitxKey_7,
            FcitxKey_8,
            FcitxKey_9,
        },

        {
            FcitxKey_1,
            FcitxKey_2,
            FcitxKey_3,
            FcitxKey_4,
            FcitxKey_q,
            FcitxKey_w,
            FcitxKey_e,
            FcitxKey_r,
            FcitxKey_a,
            FcitxKey_s,
        },
        {
            FcitxKey_d,
            FcitxKey_s,
            FcitxKey_t,
            FcitxKey_n,
            FcitxKey_a,
            FcitxKey_e,
            FcitxKey_o,
            FcitxKey_7,
            FcitxKey_8,
            FcitxKey_9,
        },
    };

    KeyStates states = KeyState::NoState;

    for (auto sym : syms[static_cast<int>(*config_.selectionKey)]) {
        selectionKeys_.emplace_back(sym, states);
    }

    pinyin_option_t options = USE_TONE | ZHUYIN_CORRECT_ALL;
    if (isZhuyin_ && *config_.needTone) {
        options |= FORCE_TONE;
    }
    if (*config_.fuzzy->fuzzyCCh) {
        options |= PINYIN_AMB_C_CH;
    }
    if (*config_.fuzzy->fuzzySSh) {
        options |= PINYIN_AMB_S_SH;
    }
    if (*config_.fuzzy->fuzzyZZh) {
        options |= PINYIN_AMB_Z_ZH;
    }
    if (*config_.fuzzy->fuzzyFH) {
        options |= PINYIN_AMB_F_H;
    }
    if (*config_.fuzzy->fuzzyGK) {
        options |= PINYIN_AMB_G_K;
    }
    if (*config_.fuzzy->fuzzyLN) {
        options |= PINYIN_AMB_L_N;
    }
    if (*config_.fuzzy->fuzzyLR) {
        options |= PINYIN_AMB_L_R;
    }
    if (*config_.fuzzy->fuzzyAnAng) {
        options |= PINYIN_AMB_AN_ANG;
    }
    if (*config_.fuzzy->fuzzyEnEng) {
        options |= PINYIN_AMB_EN_ENG;
    }
    if (*config_.fuzzy->fuzzyInIng) {
        options |= PINYIN_AMB_IN_ING;
    }

    zhuyin_set_options(context_.get(), options);

    instance_->inputContextManager().foreach([this](InputContext *ic) {
        auto *state = ic->propertyFor(&factory_);
        state->reset();
        return true;
    });

    zhuyin_load_phrase_library(context_.get(), USER_DICTIONARY);
}

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::ZhuyinEngineFactory);
