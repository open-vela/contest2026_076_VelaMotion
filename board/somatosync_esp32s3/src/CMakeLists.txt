# contest2026_076_VelaMotion — board/somatosync_esp32s3/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0

nuttx_add_library(board)

set(SRCS
    somatosync_boot.c
    somatosync_bringup.c
)

if(CONFIG_BOARDCTL)
  list(APPEND SRCS somatosync_appinit.c)
endif()

if(CONFIG_SOMATOSYNC_OLED)
  list(APPEND SRCS somatosync_board_lcd.c)
endif()

if(CONFIG_SOMATOSYNC_W5500)
  list(APPEND SRCS somatosync_board_eth.c)
endif()

if(CONFIG_ESP32S3_SPI)
  list(APPEND SRCS somatosync_board_spi.c)
endif()

target_sources(board PRIVATE ${SRCS})
