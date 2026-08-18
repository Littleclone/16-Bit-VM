/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef mmu_h
#define mmu_h

#include "essentials.h"

extern byte P_Status;

unsigned short P_TranslateAdress(unsigned short adress);

void P_SetMMUPtr(unsigned const short adress);

#endif // !mmu_h

