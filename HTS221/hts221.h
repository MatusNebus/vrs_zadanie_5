#ifndef HTS221_H
#define HTS221_H

#include <stdint.h>

// adresa zariadenia na i2c, posunuta o jeden bit pre stm32
#define HTS221_ADDRESS       (0x5F << 1)

// who_am_i je adresa registra; id je hodnota, ktoru z neho ocakavame
#define HTS221_WHO_AM_I      0x0F
#define HTS221_ID            0xBC
#define HTS221_CTRL_REG1     0x20 // register nastavenia senzora
#define HTS221_HUMIDITY_OUT  0x28 // prvy z dvoch bajtov vlhkosti
#define HTS221_TEMP_OUT      0x2A // prvy z dvoch bajtov teploty
#define HTS221_CALIB         0x30 // zaciatok vyrobnych kalibracnych udajov
#define HTS221_AUTO_INC      0x80 // pri prenose pokracuj na dalsie registre

// typy odovzdanych funkcii: adresa senzora, register, data, pocet bajtov
// vracaju 0 pri uspechu a 1 pri chybe; zapis nemeni odovzdane data
typedef uint8_t (*hts221_read_cb)(uint8_t, uint8_t,
                                  uint8_t *, uint8_t);
typedef uint8_t (*hts221_write_cb)(uint8_t, uint8_t,
                                   const uint8_t *, uint8_t);

// overi identitu, nacita kalibraciu a nastavi senzor; 0 = uspech, 1 = chyba
uint8_t hts221_init(hts221_read_cb read,
                    hts221_write_cb write);

// vysledok v stupnoch celzia ulozi na odovzdanu adresu; 0 = uspech, 1 = chyba
uint8_t hts221_read_temperature(float *temperature);
// vysledok v percentach ulozi na odovzdanu adresu; 0 = uspech, 1 = chyba
uint8_t hts221_read_humidity(float *humidity);

#endif
