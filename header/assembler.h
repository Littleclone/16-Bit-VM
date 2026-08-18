/*
* MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef assembler_h
#define assembler_h

#include "essentials.h"

struct assemblyRow {
    unsigned short opCode;
    unsigned short adress;
    unsigned short adressOffset;
    char* label;
    byte adressByte;
    byte flags; // (8: hasAdressbyte, 7: hasAdressOffset, 6: hasAdress) = ? ,5: NeedsNumber,4: NeedRegister ,3: isJump , 2: isLabel, 1: Splitt
    byte needBytes;
    struct assemblyRow* next;
};
struct LabelData {
    char* label;
    unsigned short adress;
    bool isMain;
    struct LabelData* next;
};
struct P_HardwareInterrupt {
    unsigned short interruptID;
    unsigned short adress;
    char* label;
    struct P_HardwareInterrupt* next;
};

struct currentFile {
    struct assemblyRow* head;
};

struct variable {
    char* word;
    unsigned short number;
    byte flag;
};

unsigned short P_ExecuteAssembler(char* input, const bool isKernel_C, unsigned short beginAdress, bool needsBin);

#endif // !assembler_h

