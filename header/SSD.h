/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef ssd_h
#define ssd_h
#include "essentials.h"

#define MaxStorageBlocks 0xFFFF
#define HalfOfTheStorage 0x7FFF
#define StorageSize 1024

bool P_InitSSD();
byte P_ResetSSD();
byte P_GetStorageBlock(unsigned short adress, unsigned short ramAdress, byte blockID);
byte P_WriteStorageBlock(unsigned short adress, unsigned short ramAdress, byte blockID);
sbyte P_CreateKernelBin(unsigned short kernel_start, unsigned short endBin);
sbyte P_LoadKernelFile();

char* P_LoadAssemblyFile(const char* C_filePath_C);

#endif // !ssd_h

