configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)

add_executable(testzhuyinbuffer testzhuyinbuffer.cpp)
target_link_libraries(testzhuyinbuffer Fcitx5::Core PkgConfig::LibZhuyin zhuyin-lib)
target_include_directories(testzhuyinbuffer PRIVATE ../src ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME testzhuyinbuffer COMMAND testzhuyinbuffer)
