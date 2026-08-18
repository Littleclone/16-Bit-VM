/*
* MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */

#include "../header/MMU.h"
#include <stdio.h>
#include "../header/RAM.h"
#include "../header/CPU.h"
#include "../header/interruptHandler.h"

unsigned short Pointer = 0;
byte P_Status = 0;


// Die CPU muss wenn 0b00000010 gesetzt ist die adresse inkrementieren und nochmal zur MMU senden
// Sollte 0b00000010 nicht gesetzt sein muss die Adresse nur einmal Inkrementiert werden damit der n�chste byte gespeichert wird.
/*Structure:
1 Byte->RAMBank
2 Byte->BlockBegin
2 Byte->BlockEnd
2 Byte->BlockNext
*/
unsigned short P_TranslateAdress(unsigned short adress) {
    // Schauen ob += funktioniert oder ob es mit |= sein muss.
    unsigned short ptr = Pointer;
    if (ptr == 0) {
        P_HardwareInterrupt(MMU_Error);
    }
    while (1) {
        byte ramBank = LoadRAM(ptr++);
        const unsigned short blockBegin = LoadRAM16(ptr);
        ptr += 2;
        const unsigned short blockEnd = LoadRAM16(ptr);
        ptr += 2;
        const unsigned short differens = blockEnd - blockBegin;
        const int tempAdress = adress + blockBegin;
        if (P_Status & 0b00000010) {
            P_Status ^= 0b00000010;
        }
        if (tempAdress <= differens) {
            if (tempAdress == differens) {
                P_Status = P_Status | 0b00000010;
            }
            return tempAdress;
        }
        // Interrupt fehlt falls kein Pointer vorhanden.
        adress -= differens;
        ptr = LoadRAM16(ptr);
        if (ptr == 0) {
            P_HardwareInterrupt(segmentation_fault);
        }
    }
}

void P_SetMMUPtr(unsigned const short adress) {
    // Setzt den Internen MMU Pointer zum ersten Speicherblock f�r das Program
    Pointer = adress;
}
