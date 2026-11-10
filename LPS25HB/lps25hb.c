#include "lps25hb.h"

// odovzdane funkcie na prenos dat; static ich ponecha iba v tomto subore
static lps25hb_read_cb sensor_read;
static lps25hb_write_cb sensor_write;


// overi pripojeny senzor a nastavi meranie; 0 = uspech, 1 = chyba
uint8_t lps25hb_init(lps25hb_read_cb read,
                     lps25hb_write_cb write)
{
    uint8_t id;
    uint8_t config = 0x94; // aktivny rezim, meranie 1 hz a bdu

    // bez funkcii na citanie a zapis nie je mozne komunikovat
    if (read == 0 || write == 0)
        return 1;

    // ulozenie funkcii odovzdanych z main.c, aby ich driver mohol volat neskor
    sensor_read = read;
    sensor_write = write;

    // precita jeden bajt z registra 0x0f do premennej id
    if (sensor_read(LPS25HB_ADDRESS,
                    LPS25HB_WHO_AM_I, &id, 1))
        return 1;

    // register 0x0f musi obsahovat 0xbd, identifikacne cislo tohto typu senzora
    if (id != LPS25HB_ID)
        return 1;

    // zapis nastavenia; bdu brani pomiesaniu casti hodnoty pri jej citani
    return sensor_write(LPS25HB_ADDRESS,
                        LPS25HB_CTRL_REG1, &config, 1);
}


// precita tlak v hpa a ulozi ho cez ukazovatel; 0 = uspech, 1 = chyba
uint8_t lps25hb_read_pressure(float *pressure)
{
    uint8_t data[3]; // miesto pre tri prijate bajty tlaku

    // overenie vystupnej adresy a dostupnosti citacieho callbacku
    if (pressure == 0 || sensor_read == 0)
        return 1;

    // tri bajty jednym prenosom; auto_inc posuva register z 0x28 na 0x29 a 0x2a
    if (sensor_read(LPS25HB_ADDRESS,
                    LPS25HB_PRESS_OUT_XL | LPS25HB_AUTO_INC,
                    data, 3))
        return 1;

    // spojenie bajtov: data[0] je najnizsi, data[2] je najvyssi bajt cisla
    uint32_t raw = ((uint32_t)data[2] << 16) |
                   ((uint32_t)data[1] << 8) |
                   data[0];

    // delenie 4096 prevedie surove cislo na hpa; *pressure zapise vysledok volajucemu
    *pressure = raw / 4096.0f;

    return 0;
}


// vypocet pribliznej vysky oproti polohe, v ktorej bol zmerany referencny tlak
float lps25hb_relative_height(float pressure,
                              float reference_pressure)
{
    // pokles tlaku o 1 hpa znamena pri zemi priblizne stupnutie o 8.3 metra
    return (reference_pressure - pressure) * 8.3f;
}
