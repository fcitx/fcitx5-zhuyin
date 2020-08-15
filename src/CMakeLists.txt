set(RIME_SOURCES
    zhuyinbuffer.cpp
    zhuyinengine.cpp
    zhuyinsymbol.cpp
)
add_library(zhuyin MODULE ${RIME_SOURCES})
target_link_libraries(zhuyin Fcitx5::Core Fcitx5::Config Fcitx5::Module::QuickPhrase PkgConfig::LibZhuyin ${FMT_TARGET})
set_target_properties(zhuyin PROPERTIES PREFIX "")
install(TARGETS zhuyin DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(zhuyin.conf.in zhuyin.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zhuyin.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
fcitx5_translate_desktop_file(zhuyin-addon.conf.in zhuyin-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zhuyin-addon.conf" RENAME zhuyin.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

