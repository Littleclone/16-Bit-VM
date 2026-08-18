/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef ram_h
#define ram_h

#include "essentials.h"

#define MAX_RAM 65537 // 16-Bit Adressbus oder 64KB RAM //16777216 // 24-Bit Adressbus oder 16MB RAM

struct ram {
	byte* cell;
};

bool initRAM();

byte LoadRAM(unsigned const short _adress);

void StoreRAM(unsigned const _adress, const byte _value);

unsigned short LoadRAM16(unsigned const short _adress);

void StoreRAM16(unsigned const short _adress, unsigned const short _value);

// Test Funktionen
void PrintRAM(unsigned short _begin, unsigned short _end);
void ClearRAM();
void FreeRAM();

#endif // !ram_h
