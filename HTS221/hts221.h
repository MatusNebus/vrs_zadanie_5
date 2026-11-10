#ifndef HTS221_H
#define HTS221_H

#include <stdint.h>

// I2C adresa senzora
#define HTS221_ADDRESS       (0x5F << 1)

// Registre senzora
#define HTS221_WHO_AM_I      0x0F
#define HTS221_ID            0xBC
#define HTS221_CTRL_REG1     0x20
#define HTS221_HUMIDITY_OUT  0x28
#define HTS221_TEMP_OUT      0x2A
#define HTS221_CALIB         0x30
#define HTS221_AUTO_INC      0x80

// Callback funkcie
typedef uint8_t (*hts221_read_cb)(uint8_t, uint8_t,
                                  uint8_t *, uint8_t);
typedef uint8_t (*hts221_write_cb)(uint8_t, uint8_t,
                                   const uint8_t *, uint8_t);

// Funkcie senzora
uint8_t hts221_init(hts221_read_cb read,
                    hts221_write_cb write);

uint8_t hts221_read_temperature(float *temperature);
uint8_t hts221_read_humidity(float *humidity);

#endif
