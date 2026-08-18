/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef cpu_h
#define cpu_h
#include "essentials.h"

/*Alle OpCodes sind hier zu finden un mit ihrem Binären Wert versehen.
Erhöht die Lesbarkeit drastisch*/
typedef enum {
	// User Mode
	set = 0b0000000000000000,
	inc,
	dec,
	neg,
	cmp,
	cmp_i, // TODO Nur Intern genutzt
	mov,
	swi,
	clr,
	signedFlag,
	load = 0b0000000100000000,
	load_i, // TODO Nur Intern genutzt
	store,
	store_i, // TODO Nur Intern genutzt
	loadB,
	loadB_i, // TODO Nur Intern genutzt
	storeB,
	storeB_i, // TODO Nur Intern genutzt
	rswitch, // Keine Funktion derzeit.

	add = 0b0000001000000000,
	add_i,
	sub,
	sub_i,
	mul,
	mul_i,
	division,
	division_i,
	mod,
	mod_i,
	getHigh,
	getLow,
	rmc,
	and = 0b0000001100000000,
	and_i,
	or ,
	or_i,
	xor,
	xor_i,
	not,
	shl,
	shl_i,
	shr,
	shr_i,
	jmp = 0b0000010000000000,
	call,
	je,
	jne,
	jz,
	jnz,
	jg,
	jge,
	jl,
	jle,
	jc,
	jnc,
	jNegativ,
	jnn,
	ret,
	hop,
	hop_i,
	push = 0b0000010100000000,
	pop,
	peek,
	pushB,
	popB,
	peekB,
	syscall = 0b0000011000000000,
	// Hier gehts weiter.

	nop = 0b0000111100000000,
	halt,
	notDefined,

	// Kernel
	// CPU
	setMMU = 0b0001000000000000,
	setStkPtr,
	setStkAdress,
	getSReg,
	setPC,
	setAdressSpace,
	setClock,
	setClock_r,
	getClock,
	sysRet,
	getRegs,
	setRegs,
	exec,
	// GPU
	setPx,
	startFrame,
	//Interrupts TODO: Noch nicht hinzugef�gt in die CPU.
	addInt = 0b0001001000000000,
	addIntReg,
	rmvInt,
	rmvIntReg,
	getInt,
	di,
	ei,
	retInt,

	// SSD
	loadBlock = 0b0001001100000000,
	loadBlock_r,
	writeBlock,
	writeBlock_r,

	// System
	restart = 0b0001111100000000,
	terminate,
}OpCodes;

typedef enum { // R0, R1, R2, R3, R4, R5, IR0, IR1, IR2
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	IR0,
	IR1,
	IR2,
}Register;
typedef enum {
	SR0 = 0,
	SR1,
	SR2,
	SR3
}SystemRegister;

extern unsigned int P_OpCodeCounter;

bool initCPU();

void SetKernelPointer(const unsigned short _pointer);

void P_HardwareInterrupt(const unsigned short _id);

sbyte StartExecuteCPU();

// Test Funktion
void RestartCPU();
void FreeCPU();

#endif // !cpu_h
