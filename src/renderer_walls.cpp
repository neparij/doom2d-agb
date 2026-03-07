#include "renderer.h"
#include "enc.h"
#include <cstring>

/** Список имён текстур стен, которые считаются прозрачными для неба (сетки, решётки, трапы и т.д.). */
static const char* transparent_wall_names[] = {
    "SW04_0",
    "SW04_1",
    "SW05_0",
    "SW05_1",
    "SW06_0",
    "SW06_1",
    "SW07_0",
    "SW07_1",
    "VTRAP01",
    "HTRAP01",
    "СЕТКА01",
    "ПРОЛОМ1Г",
    "Б_ЗАБОР2",
    "Б_ЗАБОР1",
    "ПАРК_ЗАБ",
    "РЕЛЬС01Г",
    "РЕШЁТКА1",
    "К_СТЕНА2",
    "К_СТЕНА3",
    "Д_ЗАБОР1",
    "РЕШЁТКА2",
    "ЦЕПЬ1",
    "ЦЕПЬ1Л",
    "ЦЕПЬ1П",
    "W111_2",
    "W112_1",
    "W112_2",
    "W112_3",
    "W106_1",
    "W107_1",
    "RW10_4",
    "M1_1",
    "RW47_1",
    "_WATER_0",
    "_WATER_1",
    "_WATER_2"
};
static const int transparent_wall_names_count = sizeof(transparent_wall_names) / sizeof(transparent_wall_names[0]);

/** Сравнивает имя стены из карты (8 байт CP866) со списком прозрачных. Сравнение по uppercase; в карте хвост часто заполнен нулями, у нас — пробелами, нормализуем к пробелам. */
int is_wallname_transparent(const char* map_name_8_cp866) {
    unsigned char map_upper[8], buf[8];
    cp866_upper_8(reinterpret_cast<const unsigned char*>(map_name_8_cp866), map_upper);
    for (int j = 0; j < 8; ++j)
        if (map_upper[j] == 0) map_upper[j] = 0x20;
    for (int i = 0; i < transparent_wall_names_count; ++i) {
        int n = utf_to_cp866(transparent_wall_names[i], buf, 8);
        for (int j = n; j < 8; ++j) buf[j] = 0x20;
        cp866_upper_8(buf, buf);
        if (memcmp(map_upper, buf, 8) == 0)
            return 1;
    }
    return 0;
}
