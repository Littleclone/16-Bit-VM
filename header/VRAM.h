/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef vram_h
#define vram_h

#include "essentials.h"

#define MAX_VRAM 65537 // 16-Bit Adressbus oder 64KB VRAM
// Offset Berechnen für Sprites

struct vram {
	Color* Cell;
};

struct vram* initVRAM();

void StoreVRAM64(unsigned int _adress, Color color);

void ClearVRAM();

// Test Funktionen

void PrintVRAM(unsigned short _begin, unsigned short _end);


#endif // !vram_h

