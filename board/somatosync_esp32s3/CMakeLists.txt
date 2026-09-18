# contest2026_076_VelaMotion — board/somatosync_esp32s3/CMakeLists.txt
#
# Board-level CMake entry, mirroring vendor/espressif board layout.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARCH_BOARD_CUSTOM)
  add_subdirectory(src)
endif()
