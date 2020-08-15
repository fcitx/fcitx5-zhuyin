cmake_minimum_required(VERSION 3.4.0)

project(fcitx5-zhuyin VERSION 0.1.0)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(ECMUninstallTarget)

find_package(Gettext REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS Notifications QuickPhrase)
find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)

if (TARGET fmt::fmt-header-only)
  set(FMT_TARGET fmt::fmt-header-only)
else()
  set(FMT_TARGET fmt::fmt)
endif ()
pkg_check_modules(LibZhuyin REQUIRED IMPORTED_TARGET "libzhuyin>=2.3.0")

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-zhuyin\")

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)
enable_testing()
add_subdirectory(test)
