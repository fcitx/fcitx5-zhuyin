cmake_minimum_required(VERSION 3.12)

project(fcitx5-zhuyin VERSION 5.1.4)

set(REQUIRED_FCITX_VERSION 5.1.13)
find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(ECMUninstallTarget)

find_package(Gettext REQUIRED)
find_package(Fcitx5Core ${REQUIRED_FCITX_VERSION} REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS Notifications QuickPhrase)
find_package(PkgConfig REQUIRED)

set(LIBZHUYIN_DATABASE_FORMAT "KyotoCabinet" CACHE STRING "When can't detect dbformat fallback to this option")

pkg_check_modules(LibZhuyin REQUIRED IMPORTED_TARGET "libzhuyin>=2.3.0")

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-zhuyin\")
fcitx5_add_i18n_definition()
if (EXISTS ${LibZhuyin_LIBDIR}/libpinyin/data/table.conf)
    file(STRINGS ${LibZhuyin_LIBDIR}/libpinyin/data/table.conf LIBPINYIN_DATABASE_FORMAT REGEX "database format")
    set(DATABASE_FORMAT "${LIBPINYIN_DATABASE_FORMAT}")
else()
    message(WARNING "Could not detect database format for libpinyin, fallback to ${LIBZHUYIN_DATABASE_FORMAT}")
    set(DATABASE_FORMAT "database format:${LIBZHUYIN_DATABASE_FORMAT}")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)
enable_testing()
add_subdirectory(test)

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Zhuyin.metainfo.xml.in
                              org.fcitx.Fcitx5.Addon.Zhuyin.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Zhuyin.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

