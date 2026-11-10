#ifndef LPS25HB_H
#define LPS25HB_H

#include <stdint.h>

// adresa zariadenia na i2c, posunuta o jeden bit pre stm32
#define LPS25HB_ADDRESS       (0x5D << 1)

// adresy registrov a hodnoty podla dokumentacie senzora
// who_am_i je adresa registra; id je hodnota, ktoru z neho ocakavame
#define LPS25HB_WHO_AM_I     0x0F
#define LPS25HB_ID           0xBD
#define LPS25HB_CTRL_REG1    0x20 // register nastavenia senzora
#define LPS25HB_PRESS_OUT_XL 0x28 // prvy z troch bajtov tlaku
#define LPS25HB_AUTO_INC     0x80 // pri prenose pokracuj na dalsie registre

// typy odovzdanych funkcii: adresa senzora, register, data, pocet bajtov
// vracaju 0 pri uspechu a 1 pri chybe; zapis nemeni odovzdane data
typedef uint8_t (*lps25hb_read_cb)(uint8_t, uint8_t,
                                    uint8_t *, uint8_t);
typedef uint8_t (*lps25hb_write_cb)(uint8_t, uint8_t,
                                     const uint8_t *, uint8_t);

// overi identitu, ulozi callbacky a nastavi senzor; 0 = uspech, 1 = chyba
uint8_t lps25hb_init(lps25hb_read_cb read,
                     lps25hb_write_cb write);

// tlak ulozi na odovzdanu adresu; navratova hodnota oznamuje uspech alebo chybu
uint8_t lps25hb_read_pressure(float *pressure);

// vrati pribliznu vysku v metroch oproti referencnemu tlaku
float lps25hb_relative_height(float pressure,
                              float reference_pressure);

#endif
