#include "hts221.h"

// odovzdane funkcie na prenos dat, dostupne iba v tomto subore
static hts221_read_cb sensor_read;
static hts221_write_cb sensor_write;

// dva referencne body pre vlhkost a teplotu, ulozene na dalsie merania
static float h0, h1, t0, t1;
// surove hodnoty zodpovedajuce tymto bodom; mozu byt aj zaporne
static int16_t h0_out, h1_out, t0_out, t1_out;


// posunie vyssi bajt o 8 bitov a pripoji nizsi; vysledok je cislo so znamienkom
static int16_t to_int16(uint8_t low, uint8_t high)
{
    return (int16_t)((uint16_t)high << 8 | low);
}


// overi senzor, nacita vyrobnu kalibraciu a nastavi meranie
uint8_t hts221_init(hts221_read_cb read,
                    hts221_write_cb write)
{
    uint8_t id;
    uint8_t config = 0x85; // aktivny rezim, meranie 1 hz a bdu
    uint8_t cal[16]; // miesto pre registre 0x30 az 0x3f

    // bez funkcii na citanie a zapis nie je mozne komunikovat
    if (read == 0 || write == 0)
        return 1;

    // ulozenie callbackov; driver potom nemusi poznat konkretnu periferiu stm32
    sensor_read = read;
    sensor_write = write;

    // precita register 0x0f do id; ocakavana identifikacna hodnota je 0xbc
    if (sensor_read(HTS221_ADDRESS, HTS221_WHO_AM_I, &id, 1))
        return 1;

    if (id != HTS221_ID)
        return 1;

    // jeden prenos nacita 16 bajtov; auto_inc zvysuje adresu registra
    if (sensor_read(HTS221_ADDRESS,
                    HTS221_CALIB | HTS221_AUTO_INC, cal, 16))
        return 1;

    // referencne vlhkosti su ulozene ako dvojnasobok percent, preto delime dvoma
    h0 = cal[0] / 2.0f;
    h1 = cal[1] / 2.0f;
    // spojenie surovych hodnot, ktore zodpovedaju obom referencnym vlhkostiam
    h0_out = to_int16(cal[6], cal[7]);
    h1_out = to_int16(cal[10], cal[11]);

    // masky vyberu horne 2 bity teploty z cal[5] a posuny ich daju na bity 8 a 9
    // pripojenim spodnych 8 bitov vznikne teplota krat 8, preto nakoniec delime osmimi
    t0 = (cal[2] | ((cal[5] & 0x03) << 8)) / 8.0f;
    t1 = (cal[3] | ((cal[5] & 0x0C) << 6)) / 8.0f;
    // surove hodnoty zodpovedajuce obom referencnym teplotam
    t0_out = to_int16(cal[12], cal[13]);
    t1_out = to_int16(cal[14], cal[15]);

    // rovnake surove body by pri vypocte sposobili delenie nulou
    if (h1_out == h0_out || t1_out == t0_out)
        return 1;

    // zapis nastavenia; bdu brani pomiesaniu casti hodnoty pri jej citani
    return sensor_write(HTS221_ADDRESS,
                        HTS221_CTRL_REG1, &config, 1);
}


// precita teplotu v stupnoch celzia; 0 = uspech, 1 = chyba
uint8_t hts221_read_temperature(float *temperature)
{
    uint8_t data[2];

    // overenie vystupnej adresy a dostupnosti citacieho callbacku
    if (temperature == 0 || sensor_read == 0)
        return 1;

    // oba bajty teploty sa precitaju naraz od registra 0x2a
    if (sensor_read(HTS221_ADDRESS,
                    HTS221_TEMP_OUT | HTS221_AUTO_INC,
                    data, 2))
        return 1;

    // spojenie nizsieho a vyssieho bajtu do surovej hodnoty so znamienkom
    int16_t raw = to_int16(data[0], data[1]);

    // prevod pomocou priamky medzi dvoma kalibracnymi bodmi; vysledok sa zapise volajucemu
    *temperature = t0 + (raw - t0_out) *
                   (t1 - t0) / (t1_out - t0_out);

    return 0;
}


// precita relativnu vlhkost v percentach; 0 = uspech, 1 = chyba
uint8_t hts221_read_humidity(float *humidity)
{
    uint8_t data[2];

    // overenie vystupnej adresy a dostupnosti citacieho callbacku
    if (humidity == 0 || sensor_read == 0)
        return 1;

    // oba bajty vlhkosti sa precitaju naraz od registra 0x28
    if (sensor_read(HTS221_ADDRESS,
                    HTS221_HUMIDITY_OUT | HTS221_AUTO_INC,
                    data, 2))
        return 1;

    // spojenie nizsieho a vyssieho bajtu do surovej hodnoty so znamienkom
    int16_t raw = to_int16(data[0], data[1]);

    // prevod pomocou priamky medzi dvoma kalibracnymi bodmi; vysledok sa zapise volajucemu
    *humidity = h0 + (raw - h0_out) *
                (h1 - h0) / (h1_out - h0_out);

    // orezanie vysledku na rozsah relativnej vlhkosti 0 az 100 percent
    if (*humidity < 0) *humidity = 0;
    if (*humidity > 100) *humidity = 100;

    return 0;
}
