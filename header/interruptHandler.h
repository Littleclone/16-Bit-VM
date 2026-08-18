/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef interruptHandler_h
#define interruptHandler_h
#include "essentials.h"

typedef enum {
	divion_with_0,
	stack_overflow,
	stack_underflow,
	previlige_Instruction,
	segmentation_fault,
	manipulated_Return_Value,
	timer,
	tryed_hardware_Interrupt,
	keyboard,
	mouse_Move,
	mouse_Click_down,
	mouse_Click_up,
	GPU_VBlank,
	user_Call,
	interupt_not_found,
	system_Force_Exit,
	CPU_Error,
	MMU_Error,
	assembler_Error,
	system_Error,
}HardwareInterrupts;

struct sysCall {
	unsigned short Interrupt_id;
	unsigned short Adress;
	byte Flag; // 8: Nicht genutzt, 7: UserCall, 6: Hardware.
};

struct IPU { // Interrupt Processing Unit
	struct sysCall* interrupt;			// Ist ein Array
	unsigned short RegisteredInterrupt; // Wie viele Interrupts Registriert sind.
};

bool initIPU();
bool RegisterInterrupt(unsigned short _interruptID, const unsigned short _adress, const byte _flags);
void RemoveInterrupt(const unsigned short _interruptID);
struct sysCall* CallInterrupt(const unsigned short _interruptID);
void ResetIPU();
void FreeIPU();
#endif