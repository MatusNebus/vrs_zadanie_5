#include "lps25hb.h"

// Ulozene callback funkcie
static lps25hb_read_cb sensor_read;
static lps25hb_write_cb sensor_write;


// Inicializacia senzora
uint8_t lps25hb_init(lps25hb_read_cb read,
                     lps25hb_write_cb write)
{
    uint8_t id;
    uint8_t config = 0x94;

    if (read == 0 || write == 0)
        return 1;

    // Registracia callbackov
    sensor_read = read;
    sensor_write = write;

    // Overenie identity senzora
    if (sensor_read(LPS25HB_ADDRESS,
                    LPS25HB_WHO_AM_I, &id, 1))
        return 1;

    if (id != LPS25HB_ID)
        return 1;

    // Zapnutie senzora, 1 Hz, BDU
    return sensor_write(LPS25HB_ADDRESS,
                        LPS25HB_CTRL_REG1, &config, 1);
}


// Precitanie tlaku
uint8_t lps25hb_read_pressure(float *pressure)
{
    uint8_t data[3];

    if (pressure == 0 || sensor_read == 0)
        return 1;

    // Tri bajty jednym I2C prenosom
    if (sensor_read(LPS25HB_ADDRESS,
                    LPS25HB_PRESS_OUT_XL | LPS25HB_AUTO_INC,
                    data, 3))
        return 1;

    // Spojenie troch bajtov do jedneho cisla
    uint32_t raw = ((uint32_t)data[2] << 16) |
                   ((uint32_t)data[1] << 8) |
                   data[0];

    // Prevod na tlak v hPa
    *pressure = raw / 4096.0f;

    return 0;
}


// Vypocet relativnej vysky
float lps25hb_relative_height(float pressure,
                              float reference_pressure)
{
    // Priblizne 8.3 metra na 1 hPa pri zemi
    return (reference_pressure - pressure) * 8.3f;
}
