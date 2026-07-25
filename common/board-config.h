#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifndef STATUS_LED
#error "STATUS_LED not defined. Set board defines in base-boards.ini."
#endif

const unsigned long SERIAL_BAUD = 115200;

#include "status-led.h"

#endif  // BOARD_CONFIG_H
