/*
* MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/bunnySystem.h"
#include <stdio.h>
#include "../header/CPU.h"
#include "../header/RAM.h"
#include "../header/interruptHandler.h"
#include "../header/GPU.h"
#include "../header/CHR_ROM.h"

char* G_KernelPath = NULL;

// Initialisiert das Gesammte System, sollte das nicht Funktionieren werden alle Ressourcen wieder freigegeben und ein fehler wird gemeldet (False)
bool P_InitSystem() {
    if (!initCPU()) {
        return false;
    }
    if (!initRAM()) {
        FreeCPU();
        return false;
    }
    if (!initIPU()) {
        FreeCPU();
        FreeRAM();
        return false;
    }
    if (!initGPU()) {
        FreeCPU();
        FreeRAM();
        FreeIPU();
        return false;
    }
    return true;
}