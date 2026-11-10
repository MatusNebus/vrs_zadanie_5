#include "hts221.h"

// Callbacky na I2C komunikaciu
static hts221_read_cb sensor_read;
static hts221_write_cb sensor_write;

// Kalibracne hodnoty
static float h0, h1, t0, t1;
static int16_t h0_out, h1_out, t0_out, t1_out;


// Pomocna funkcia na spojenie dvoch bajtov
static int16_t to_int16(uint8_t low, uint8_t high)
{
    return (int16_t)((uint16_t)high << 8 | low);
}


// Inicializacia senzora
uint8_t hts221_init(hts221_read_cb read,
                    hts221_write_cb write)
{
    uint8_t id;
    uint8_t config = 0x85;
    uint8_t cal[16];

    if (read == 0 || write == 0)
        return 1;

    sensor_read = read;
    sensor_write = write;

    // Kontrola WHO_AM_I
    if (sensor_read(HTS221_ADDRESS, HTS221_WHO_AM_I, &id, 1))
        return 1;

    if (id != HTS221_ID)
        return 1;

    // Nacitanie kalibracnych registrov naraz
    if (sensor_read(HTS221_ADDRESS,
                    HTS221_CALIB | HTS221_AUTO_INC, cal, 16))
        return 1;

    // Kalibracia vlhkosti
    h0 = cal[0] / 2.0f;
    h1 = cal[1] / 2.0f;
    h0_out = to_int16(cal[6], cal[7]);
    h1_out = to_int16(cal[10], cal[11]);

    // Kalibracia teploty
    t0 = (cal[2] | ((cal[5] & 0x03) << 8)) / 8.0f;
    t1 = (cal[3] | ((cal[5] & 0x0C) << 6)) / 8.0f;
    t0_out = to_int16(cal[12], cal[13]);
    t1_out = to_int16(cal[14], cal[15]);

    if (h1_out == h0_out || t1_out == t0_out)
        return 1;

    // Zapnutie senzora, 1 Hz, BDU
    return sensor_write(HTS221_ADDRESS,
                        HTS221_CTRL_REG1, &config, 1);
}


// Precitanie teploty
uint8_t hts221_read_temperature(float *temperature)
{
    uint8_t data[2];

    if (temperature == 0 || sensor_read == 0)
        return 1;

    if (sensor_read(HTS221_ADDRESS,
                    HTS221_TEMP_OUT | HTS221_AUTO_INC,
                    data, 2))
        return 1;

    int16_t raw = to_int16(data[0], data[1]);

    *temperature = t0 + (raw - t0_out) *
                   (t1 - t0) / (t1_out - t0_out);

    return 0;
}


// Precitanie vlhkosti
uint8_t hts221_read_humidity(float *humidity)
{
    uint8_t data[2];

    if (humidity == 0 || sensor_read == 0)
        return 1;

    if (sensor_read(HTS221_ADDRESS,
                    HTS221_HUMIDITY_OUT | HTS221_AUTO_INC,
                    data, 2))
        return 1;

    int16_t raw = to_int16(data[0], data[1]);

    *humidity = h0 + (raw - h0_out) *
                (h1 - h0) / (h1_out - h0_out);

    // Relativna vlhkost je v rozsahu 0 az 100 %
    if (*humidity < 0) *humidity = 0;
    if (*humidity > 100) *humidity = 100;

    return 0;
}
