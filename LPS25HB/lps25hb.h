#ifndef LPS25HB_H
#define LPS25HB_H

#include <stdint.h>

// Adresa senzora (7-bitova adresa posunuta o 1 bit)
#define LPS25HB_ADDRESS       (0x5D << 1)

// Registre senzora
#define LPS25HB_WHO_AM_I     0x0F
#define LPS25HB_ID           0xBD
#define LPS25HB_CTRL_REG1    0x20
#define LPS25HB_PRESS_OUT_XL 0x28
#define LPS25HB_AUTO_INC     0x80

// Typy callback funkcii
typedef uint8_t (*lps25hb_read_cb)(uint8_t, uint8_t,
                                    uint8_t *, uint8_t);
typedef uint8_t (*lps25hb_write_cb)(uint8_t, uint8_t,
                                     const uint8_t *, uint8_t);

// Funkcie kniznice senzora
uint8_t lps25hb_init(lps25hb_read_cb read,
                     lps25hb_write_cb write);

uint8_t lps25hb_read_pressure(float *pressure);

float lps25hb_relative_height(float pressure,
                              float reference_pressure);

#endif
