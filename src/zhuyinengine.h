/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_ZHUYIN_ZHUYINENGINE_H_
#define _FCITX5_ZHUYIN_ZHUYINENGINE_H_

#include "zhuyinbuffer.h"
#include "zhuyinsymbol.h"
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <quickphrase_public.h>
#include <zhuyin.h>

namespace fcitx {

enum class Scheme {
  Standard,
  Hsu,
  IBM,
  GinYieh,
  Eten,
  Eten26,
  Dvorak,
  HsuDvorak,
  DachenCP26,
  Hanyu,
  Luoma,
  SecondaryZhuyin
};

enum class SelectionKey {
  Digit,
  asdfghjkl,
  asdfzxcv89,
  asdfjkl789,
  aoeuhtn789,
  _1234qweras,
  dstnaeo789
};

FCITX_CONFIG_ENUM_NAME(SelectionKey, "1234567890", "asdfghjkl;", "asdfzxcv89",
                       "asdfjkl789", "aoeuhtn789", "1234qweras", "dstnaeo789");

FCITX_CONFIG_ENUM_NAME_WITH_I18N(Scheme, N_("Standard"), N_("Hsu"), N_("IBM"),
                                 N_("GinYieh"), N_("Eten"), N_("Eten26"),
                                 N_("Standard Dvorak"), N_("Hsu Dvorak"),
                                 N_("Dachen CP26"), N_("Hanyu"), N_("Luoma"),
                                 N_("Secondary Zhuyin"));

FCITX_CONFIGURATION(
    FuzzyConfig, Option<bool> fuzzyCCh{this, "FuzzyCCh", "ㄘ <=> ㄔ", false};
    Option<bool> fuzzySSh{this, "FuzzySSh", "ㄙ <=> ㄕ", false};
    Option<bool> fuzzyZZh{this, "FuzzyZZh", "ㄗ <=> ㄓ", false};
    Option<bool> fuzzyFH{this, "FuzzyFH", "ㄈ <=> ㄏ", false};
    Option<bool> fuzzyGK{this, "FuzzyGK", "ㄍ <=> ㄎ", false};
    Option<bool> fuzzyLN{this, "FuzzyLN", "ㄌ <=> ㄋ", false};
    Option<bool> fuzzyLR{this, "FuzzyLR", "ㄌ <=> ㄖ", false};
    Option<bool> fuzzyAnAng{this, "FuzzyAnAng", "ㄢ <=> ㄤ", false};
    Option<bool> fuzzyEnEng{this, "FuzzyEnEng", "ㄧㄣ <=> ㄧㄥ", false};
    Option<bool> fuzzyInIng{this, "FuzzyInIng", "ㄣ <=> ㄥ", false};);

FCITX_CONFIGURATION(
    ZhuyinConfig,
    OptionWithAnnotation<Scheme, SchemeI18NAnnotation> layout{
        this, "Layout", _("Layout"), Scheme::Standard};
    Option<SelectionKey> selectionKey{this, "SelectionKey", _("Selection Key"),
                                      SelectionKey::Digit};
    Option<bool> needTone{this, "NeedTone", _("Require tone in zhuyin"), true};
    Option<bool> commitOnSwitch{
        this, "CommitOnSwitch",
        _("Commit current preedit when switching to other input method"), true};
    Option<int, IntConstrain> pageSize{this, "PageSize", _("Page size"), 10,
                                       IntConstrain(3, 10)};
    Option<bool> useEasySymbol{this, "EasySymbol", _("Use easy symbol"), true};
    Option<Key, KeyConstrain> quickphraseKey{
        this, "QuickPhraseKey", _("QuickPhrase Trigger Key"),
        Key(FcitxKey_grave), KeyConstrain{KeyConstrainFlag::AllowModifierLess}};
    Option<std::string> quickphraseKeySymbol{
        this, "QuickPhraseSymbol", _("QuickPhrase Trigger Key Symbol"), "…"};
    KeyListOption prevPage{
        this,
        "PrevPage",
        _("Prev Page"),
        {Key(FcitxKey_Left), Key(FcitxKey_Page_Up)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nextPage{
        this,
        "NextPage",
        _("Next Page"),
        {Key(FcitxKey_Right), Key(FcitxKey_Page_Down)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption prevCandidate{
        this,
        "PrevCandidate",
        _("Prev Candidate"),
        {Key("Up"), Key("Shift+Tab")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nextCandidate{
        this,
        "NextCandidate",
        _("Next Candidate"),
        {Key("Down"), Key("Tab")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    Option<FuzzyConfig> fuzzy{this, "Fuzzy", _("Fuzzy")};);

class ZhuyinEngine;

class ZhuyinState final : public InputContextProperty {
public:
  ZhuyinState(ZhuyinEngine *engine, InputContext *ic);

  void keyEvent(KeyEvent &keyEvent);
  void reset();
  void commit();

  void updateUI(bool showCandidate = false);

private:
  ZhuyinEngine *engine_;
  ZhuyinBuffer buffer_;
  InputContext *ic_;
};

class ZhuyinEngine : public InputMethodEngine, public ZhuyinProviderInterface {
public:
  explicit ZhuyinEngine(Instance *instance);

  void keyEvent(const fcitx::InputMethodEntry &entry,
                fcitx::KeyEvent &keyEvent) override;
  void activate(const fcitx::InputMethodEntry &,
                fcitx::InputContextEvent &) override;
  void deactivate(const fcitx::InputMethodEntry &entry,
                  fcitx::InputContextEvent &event) override;
  void reset(const fcitx::InputMethodEntry &,
             fcitx::InputContextEvent &) override;
  const fcitx::Configuration *getConfig() const override;
  void setConfig(const fcitx::RawConfig &) override;
  void save() override;
  void reloadConfig() override;

  zhuyin_context_t *context() override { return context_.get(); }
  bool isZhuyin() const override { return isZhuyin_; }
  const auto &config() const { return config_; }
  const ZhuyinSymbol &symbol() const override { return symbol_; }

  const KeyList &selectionKeys() const { return selectionKeys_; }

  FCITX_ADDON_DEPENDENCY_LOADER(fullwidth, instance_->addonManager());
  FCITX_ADDON_DEPENDENCY_LOADER(chttrans, instance_->addonManager());
  FCITX_ADDON_DEPENDENCY_LOADER(quickphrase, instance_->addonManager());

private:
  Instance *instance_;
  UniqueCPtr<zhuyin_context_t, zhuyin_fini> context_;
  FactoryFor<ZhuyinState> factory_;
  ZhuyinSymbol symbol_;
  ZhuyinConfig config_;
  KeyList selectionKeys_;
  bool isZhuyin_ = true;
};

class ZhuyinEngineFactory final : public AddonFactory {
public:
  fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
    registerDomain("fcitx5-zhuiyin", FCITX_INSTALL_LOCALEDIR);
    return new ZhuyinEngine(manager->instance());
  }
};

} // namespace fcitx

#endif // _FCITX5_ZHUYIN_ZHUYINENGINE_H_
