/*
* MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/RAM.h"
#include <stdio.h>
#include <stdlib.h>

struct ram* RAM = NULL;

bool initRAM() {
    RAM = (struct ram*)malloc(sizeof(struct ram));
    if (RAM == NULL) {
        printf("Error: RAM konnte nicht allokiert werden.\n"); // TODO Auswechseln mit Log System sobald vorhanden.
        return false;
    }
    RAM->cell = (byte*)malloc(MAX_RAM);
    if (RAM->cell == NULL) {
        printf("Error: Die RAM Zellen konnten nicht Allokiert werden.\n"); // TODO Auswechseln mit Log System sobald vorhanden.
        free(RAM);
        RAM = NULL;
        return false;
    }
    for (int i = 0; i < MAX_RAM; ++i) {
        *(RAM->cell + i) = 0;
    }
    return true;
}

byte LoadRAM(unsigned const short _adress) {
    if (_adress < MAX_RAM) {
        return *(RAM->cell + _adress);
    }
    return 0;
}

void StoreRAM(unsigned const _adress, const byte _value) {
    if (_adress < MAX_RAM) {
        *(RAM->cell + _adress) = _value;
    }
}

unsigned short LoadRAM16(unsigned const short _adress) {
    if (_adress < MAX_RAM) {
        return LoadRAM(_adress) << 8 | LoadRAM(_adress + 1);
    }
    return 0;
}

void StoreRAM16(unsigned const short _adress, unsigned const short _value) {
    if (_adress < MAX_RAM) {
        byte storeValue = _value >> 8; // Schauen ob das wirklich den erw�nschten erffekt erzielt
        *(RAM->cell + _adress) = storeValue;
        storeValue = _value & 0b0000000011111111;
        *(RAM->cell + (_adress + 1)) = storeValue;
    }
}

void PrintRAM(unsigned short _begin ,unsigned short _end) {
    for (int i = _begin; i < _end; ++i) {
        printf("RAMCell [0x%x] -> %x\n",i , *(RAM->cell + i));
    }
}

void ClearRAM() {
    for (int i = 0; i < MAX_RAM; ++i) {
        *(RAM->cell + i) = 0;
    }
}

void FreeRAM() {
    free(RAM->cell);
    free(RAM);
}