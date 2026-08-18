/*
* MIT License
* Copyright (c) 2025 Littleclone
* Permission is granted to use, copy, modify, and distribute this software,
* provided that the copyright notice and this permission notice are included.
* The software is provided "as is", without warranty of any kind.
*/
#include "../header/VRAM.h"
#include <stdio.h>
#include <stdlib.h>

struct vram* VRAM = NULL;

struct vram* initVRAM() {
    struct vram* VRAM_init = malloc(sizeof(struct vram));
    if (VRAM_init == NULL) {
        printf("Error: VRAM konnte nicht allokiert werden.\n"); // TODO Auswechseln mit Log System sobald vorhanden.
        return NULL;
    }
    VRAM_init->Cell = malloc(MAX_VRAM / 4 * sizeof(Color));
    if (VRAM_init->Cell == NULL) {
        printf("Error: Die VRAM Zellen konnten nicht Allokiert werden.\n"); // TODO Auswechseln mit Log System sobald vorhanden.
        free(VRAM_init);
        VRAM_init = NULL;
        return NULL;
    }
    for (int i = 0; i < (MAX_VRAM / 4); ++i) {
        *(VRAM_init->Cell + i) = BLACK;
    }
    VRAM = VRAM_init;
    return VRAM_init;
}

void StoreVRAM64(unsigned const int _adress, const Color color) {
    if (_adress < (MAX_VRAM / 4)) {
        *(VRAM->Cell + _adress) = color;
    }
}


void ClearVRAM() {
    for (int i = 0; i < (MAX_VRAM / 4); ++i) {
        *(VRAM->Cell + i) = BLACK;
    }
}
