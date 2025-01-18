add_library(zhuyin-lib OBJECT
    zhuyinbuffer.cpp
    zhuyincandidate.cpp
    zhuyinsection.cpp
    zhuyinsymbol.cpp
)
target_link_libraries(zhuyin-lib Fcitx5::Core PkgConfig::LibZhuyin)
set_property(TARGET zhuyin-lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_fcitx5_addon(zhuyin zhuyinengine.cpp)
target_link_libraries(zhuyin Fcitx5::Core Fcitx5::Config Fcitx5::Module::QuickPhrase PkgConfig::LibZhuyin ${FMT_TARGET} zhuyin-lib)
set_target_properties(zhuyin PROPERTIES PREFIX "")
install(TARGETS zhuyin DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(zhuyin.conf.in zhuyin.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zhuyin.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
configure_file(zhuyin-addon.conf.in.in zhuyin-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/zhuyin-addon.conf.in" zhuyin-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zhuyin-addon.conf" RENAME zhuyin.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

