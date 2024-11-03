install(DIRECTORY 16x16 22x22 24x24 48x48 DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor"
        PATTERN .* EXCLUDE
        PATTERN *~ EXCLUDE)


find_program(LIBPINYIN_GEN_BINARY_FILES gen_binary_files HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_GEN_UNIGRAM gen_unigram HINTS "${LIBPINYIN_EXECPREFIX}/bin")
find_program(LIBPINYIN_IMPORT_INTERPOLATION import_interpolation HINTS "${LIBPINYIN_EXECPREFIX}/bin")

set(_LIBPINYIN_TABLE gb_char gbk_char merged)

foreach(f ${_LIBPINYIN_TABLE})
    set(_LIBPINYIN_TABLE_FILES ${_LIBPINYIN_TABLE_FILES} "${f}.table")
    set(_LIBPINYIN_BIN_FILES ${_LIBPINYIN_BIN_FILES} "${f}.bin")
    set(_LIBPINYIN_GEN_BIN_FILES ${_LIBPINYIN_GEN_BIN_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${f}.bin")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/table.conf.in ${CMAKE_CURRENT_BINARY_DIR}/table.conf @ONLY)

set(ZHUYIN_DATA_FILE_NAME model.text.20241103.tar.zst)
set(ZHUYIN_URL "https://download.fcitx-im.org/data/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_ORGDATA ${_LIBPINYIN_TABLE_FILES} interpolation2.text)
set(ZHUYIN_DATA bigram.db ${_LIBPINYIN_BIN_FILES}  phrase_index.bin
                pinyin_index.bin addon_phrase_index.bin addon_pinyin_index.bin)

fcitx5_download(zhuyin_data_tar "${ZHUYIN_URL}" "${ZHUYIN_DATA_FILE_NAME}" bb4a9789b6e0fde1c6547f659ce591de5e691ee3a8345d3c1c74eaf92b31ea48)
fcitx5_extract(zhuyin-data-extract "${ZHUYIN_DATA_FILE_NAME}"
  DEPENDS zhuyin_data_tar OUTPUT ${ZHUYIN_ORGDATA})

add_custom_command(OUTPUT ${ZHUYIN_DATA}
  DEPENDS ${ZHUYIN_ORGDATA} zhuyin-data-extract
  COMMAND "${CMAKE_COMMAND}" -E remove ${ZHUYIN_DATA}
  COMMAND "${LIBPINYIN_GEN_BINARY_FILES}"
  --table-dir "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "${LIBPINYIN_IMPORT_INTERPOLATION}"
  < "${CMAKE_CURRENT_BINARY_DIR}/interpolation2.text"
  COMMAND "${LIBPINYIN_GEN_UNIGRAM}")

add_custom_target(zhuyin_data ALL DEPENDS ${ZHUYIN_DATA})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bigram.db
              ${_LIBPINYIN_GEN_BIN_FILES}
              ${CMAKE_CURRENT_BINARY_DIR}/phrase_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/pinyin_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/addon_phrase_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/addon_pinyin_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/table.conf
              easysymbols.txt
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/zhuyin")

install(FILES zhuyin.lua
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/lua/imeapi/extensions")
