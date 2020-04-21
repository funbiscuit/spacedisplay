
add_subdirectory(pfd)

# create library for crc which doesn't have cmake file
add_library(crc crc/crc_16.c)
target_include_directories(crc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/crc)
add_library(CRC::CRC ALIAS crc)
