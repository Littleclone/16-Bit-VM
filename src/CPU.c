/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/CPU.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/MMU.h"
#include "../header/GPU.h"
#include "../header/RAM.h"
#include "../header/VRAM.h"
#include "../header/interruptHandler.h"
#include "../header/SSD.h"
#include "../header/CHR_ROM.h"

// Initialisieren von Variablen
unsigned short Registers[16]; // R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, IR0, IR1, IR2, IR3, IR4, IR5
unsigned short SystemRegisters[4]; // SR0 = [Hier wird der PC bei einem _Syscall gespeichert], SR1 = Hier wird bei einem UserCall die anfragende PC gespeichert.
unsigned int Buffer = 0;
short System_clock = 0;
unsigned short ProgrammCounter = 0, StackPointer = 0, StackAdress = 0x8FFF, StackAdressEnd = 0x9FFF, BasisAdress = 0, EndAdress = 0;
const short SMaxValue = 32767, SMinValue = -32768;
byte Flags = 0; // 1: Carry, 2: IsZero, 3: IsNegative, 4: IsGreater, 5: IsEqual, 6: IsLess, 7: IsSigned, 8: IsKernelMode
unsigned short OpCode = 0, ModeOpCode;

// CPU:
byte Status = 0; // 1: Aktiv/Inaktiv, 2: Idle/Waiting, 3: Running, 8: Critical Error;
unsigned int P_OpCodeCounter = 0;
bool P_InterruptsActive = true;
bool P_InInterruptMode = false;
unsigned short P_Buffer_Interrupt[9];

void L_Push(bool _calledFromCPU);
void L_Pop(bool _calledFromCPU);
void P_HardwareInterrupt(unsigned short _id);

void RestartCPU() {
	// Reset all registers
	for (byte i = 0; i < 9; ++i) {
		Registers[i] = 0;
	}
	for (byte i = 0; i < 4; ++i) {
		SystemRegisters[i] = 0;
	}
	// Reset other CPU state variables
	Buffer = 0;
	ProgrammCounter = 0;
	StackPointer = 0;
	StackAdress = 0x8FFF;
	StackAdressEnd = 0x9FFF;
	Flags = 0b10000000;
	OpCode = 0;
	ModeOpCode = 0;
	Status &= 0b00000000;
	Status |= 0b00000001;
	P_OpCodeCounter = 0;
	P_InterruptsActive = true;
	P_InInterruptMode = false;
}

void FreeCPU() {
	RestartCPU();
}

bool initCPU() {
	Status = 0b00000011;
	P_Status = 0b00000001;
	return true;
}

// TODO: Schauen ob die Variationen (_wantsNumber / wantsRegister) Richtig funktionieren
// Register
void L_Set() {
	byte wantedReg = LoadRAM(ProgrammCounter + 1);
	unsigned short value = LoadRAM16(ProgrammCounter + 2);
	ProgrammCounter += 3; // Um die 3 Bytes zu �berspringen)
	*(Registers + wantedReg) = value;
}
void L_Inc() {
	++ProgrammCounter;
	byte wantedReg = LoadRAM(ProgrammCounter);
	++*(Registers + wantedReg);
}
void L_Dec() {
	++ProgrammCounter;
	byte wantedReg = LoadRAM(ProgrammCounter);
	--*(Registers + wantedReg);
}
void L_Neg() {
	++ProgrammCounter;
	byte wantedReg = LoadRAM(ProgrammCounter);
	Registers[wantedReg] = -Registers[wantedReg];
}
void L_CMP(bool wantsNumber) {
	Flags &= 0b11000111;
	++ProgrammCounter;
	byte wantedReg1 = 0;
	if (wantsNumber) {
		wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		if (Flags & 0b01000000) {
			if ((signed short)*(Registers + wantedReg1) > (signed short)value) {
				Flags |= 0b00001000;
			}
			else if ((signed short)*(Registers + wantedReg1) == (signed short)value) {
				Flags |= 0b00010000;
			}
			else {
				Flags |= 0b00100000;
			}
		}
		else {
			if (*(Registers + wantedReg1) > value) {
				Flags |= 0b00001000;
			}
			else if (*(Registers + wantedReg1) == value) {
				Flags |= 0b00010000;
			}
			else {
				Flags |= 0b00100000;
			}
		}
	}
	else {
		wantedReg1 = LoadRAM(ProgrammCounter) >> 4;
		byte wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		if (Flags & 0b01000000) {
			if ((signed short)*(Registers + wantedReg1) > (signed short)*(Registers + wantedReg2)) {
				Flags |= 0b00001000;
			}
			else if ((signed short)*(Registers + wantedReg1) == (signed short)*(Registers + wantedReg2)) {
				Flags |= 0b00010000;
			}
			else {
				Flags |= 0b00100000;
			}
		}
		else {
			if (*(Registers + wantedReg1) > *(Registers + wantedReg2)) {
				Flags |= 0b00001000;
			}
			else if (*(Registers + wantedReg1) == *(Registers + wantedReg2)) {
				Flags |= 0b00010000;
			}
			else {
				Flags |= 0b00100000;
			}
		}
	}
}
void L_Mov() {
	++ProgrammCounter;
	byte wantedReg1 = LoadRAM(ProgrammCounter) >> 4, wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	*(Registers + wantedReg1) = *(Registers + wantedReg2);
}
void L_Swi() {
	++ProgrammCounter;
	byte wantedReg1 = LoadRAM(ProgrammCounter) >> 4, wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _temp = *(Registers + wantedReg1);
	*(Registers + wantedReg1) = *(Registers + wantedReg2);
	*(Registers + wantedReg2) = _temp;
}
void L_CLR() {
	for (int i = 0; i < 16; ++i) {
		*(Registers + i) = 0;
	}
}
void L_Signed() {
	++ProgrammCounter;
	byte flag = LoadRAM(ProgrammCounter);
	if (flag) {
		Flags |= 0b01000000;
	}
	else {
		Flags &= 0b10111111;
	}
}
// Memory
void L_Load(bool wantsNumber) {
	byte wantedReg = LoadRAM(ProgrammCounter + 1);
	unsigned short adress = 0;
	if (!wantsNumber) {
		adress = LoadRAM16(ProgrammCounter + 2);
		ProgrammCounter += 3;
	}
	else {
		byte adressReg = (LoadRAM(ProgrammCounter + 2));
		ProgrammCounter += 2;
		adress = *(Registers + adressReg);
	}
	if (!Flags & 0b10000000) { // Wenn nicht im Kernel Mode
		adress = P_TranslateAdress(adress);
	}
	if (P_Status & 0b00000010) {
		// Testen ob das Richtig Funktioniert.
		*(Registers + wantedReg) = LoadRAM(adress) << 8;
		++adress;
		adress = P_TranslateAdress(adress);
		*(Registers + wantedReg) |= LoadRAM(adress);
	}
	else {
		*(Registers + wantedReg) = LoadRAM16(adress);
	}
}
void L_Store(bool _wantsNumber) {
	byte _wantedReg = LoadRAM(ProgrammCounter + 1);
	unsigned short _adress = 0;
	if (!_wantsNumber) {
		_adress = LoadRAM16(ProgrammCounter + 2);
		ProgrammCounter += 3;
	}
	else {
		byte _adressReg = (LoadRAM(ProgrammCounter + 2));
		ProgrammCounter += 2;
		_adress = *(Registers + _adressReg);
	}
	if (!Flags & 0b10000000) { // Wenn nicht im Kernel Mode
		_adress = P_TranslateAdress(_adress);
	}
	if (P_Status & 0b00000010) {
		// Testen ob das Richtig Funktioniert.
		StoreRAM(_adress, ((byte)(*(Registers + _wantedReg) >> 8)));
		++_adress;
		_adress = P_TranslateAdress(_adress);
		StoreRAM(_adress, (byte) * (Registers + _wantedReg));
	}
	else {
		StoreRAM16(_adress, *(Registers + _wantedReg));
	}
}
void L_LoadBit(bool _wantsNumber) {
	byte _wantedReg = LoadRAM(ProgrammCounter + 1);
	unsigned short _adress = 0;
	if (!_wantsNumber) {
		_adress = LoadRAM16(ProgrammCounter + 2);
		ProgrammCounter += 3;
	}
	else {
		byte _adressReg = (LoadRAM(ProgrammCounter + 2));
		ProgrammCounter += 2;
		_adress = *(Registers + _adressReg);
	}
	if (!Flags & 0b10000000) { // Wenn nicht im Kernel Mode
		_adress = P_TranslateAdress(_adress);
	}
	*(Registers + _wantedReg) = LoadRAM(_adress);
}
void L_StoreBit(bool _wantsNumber) {
	byte _wantedReg = LoadRAM(ProgrammCounter + 1);
	unsigned short _adress = 0;
	if (!_wantsNumber) {
		_adress = LoadRAM16(ProgrammCounter + 2);
		ProgrammCounter += 3;
	}
	else {
		byte _adressReg = (LoadRAM(ProgrammCounter + 2));
		ProgrammCounter += 2;
		_adress = *(Registers + _adressReg);
	}
	if (!Flags & 0b10000000) { // Wenn nicht im Kernel Mode
		_adress = P_TranslateAdress(_adress);
	}
	StoreRAM(_adress, (byte)*(Registers + _wantedReg));
}
// Math TODO: Muss getestet werden.
void L_Add(bool _wantsNumber) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _oldValue = *(Registers + _wantedReg1);
	unsigned short _value2 = 0;
	if (!_wantsNumber) {
		_value2 = Registers[_wantedReg2];
	}
	else {
		_value2 = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		_wantedReg1 = _wantedReg2;
	}
	if (Flags & 0b01000000) {
		*(Registers + _wantedReg1) = ((signed short)Registers[_wantedReg1] + (signed short)_value2);
		signed short _sValue = *(Registers + _wantedReg1), _signedOldValue = _oldValue;
		if (_sValue < SMaxValue && _sValue < 0 && _sValue < _signedOldValue) {
			Flags |= 0b00000101;
		}
		else if (_sValue == 0) {
			Flags |= 0b00000010;
		}
		else { // if (_sValue > SMinValue)
			if (_sValue > 0) {
				Flags &= 0b11111011;
			}
			else {
				Flags |= 0b00000100;
			}
		}
	}
	else {
		*(Registers + _wantedReg1) += _value2;
		if (*(Registers + _wantedReg1) < _oldValue) {
			Flags |= 0b00000001;
		}
		else if (*(Registers + _wantedReg1) == 0) {
			Flags |= 0b00000010;
		}
	}
}
void L_Sub(bool _wantsNumber) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _oldValue = *(Registers + _wantedReg1);
	unsigned short _value2 = 0;
	if (!_wantsNumber) {
		_value2 = Registers[_wantedReg2];
	}
	else {
		_value2 = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		_wantedReg1 = _wantedReg2;
	}
	if (Flags & 0b01000000) {
		*(Registers + _wantedReg1) = (unsigned short)((signed short)Registers[_wantedReg1] - (signed short)_value2);
		signed short _sValue = *(Registers + _wantedReg1), _signedOldValue = _oldValue;
		if (_sValue < SMaxValue && _sValue < 0 && _sValue < _signedOldValue) {
			Flags |= 0b00000101;
		}
		else if (_sValue == 0) {
			Flags |= 0b00000010;
		}
		else { // if (_sValue > SMinValue)
			if (_sValue > 0) {
				Flags &= 0b11111011;
			}
			else {
				Flags |= 0b00000100;
			}
		}
	}
	else {
		*(Registers + _wantedReg1) -= _value2;
		if (*(Registers + _wantedReg1) < _oldValue) {
			Flags |= 0b00000001;
		}
		else if (*(Registers + _wantedReg1) == 0) {
			Flags |= 0b00000010;
		}
	}
}
void L_Mul(bool _wantsNumber) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _value2 = 0;
	if (!_wantsNumber) {
		_value2 = Registers[_wantedReg2];
	}
	else {
		_value2 = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		_wantedReg1 = _wantedReg2;
	}
	if (Flags & 0b01000000) {
		Buffer = (unsigned int)((signed int)Registers[_wantedReg1] * (signed int)_value2);
	}
	else {
		Buffer = (unsigned int)(Registers[_wantedReg1] * (unsigned int)_value2);
	}
}
void L_Div(bool _wantsNumber) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _value2 = 0;
	if (!_wantsNumber) {
		_value2 = Registers[_wantedReg2];
	}
	else {
		_value2 = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		_wantedReg1 = _wantedReg2;
	}
	if (_value2 == 0) {
		// Interrup wegen Division durch Null
		P_HardwareInterrupt(divion_with_0);
		return;
	}
	if (Flags & 0b01000000) {
		*(Registers + _wantedReg1) = (unsigned short)((signed short)*(Registers + _wantedReg1) / (signed short)_value2);
	}
	else {
		*(Registers + _wantedReg1) /= _value2;
	}
}
void L_Mod(bool _wantsNumber) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
	unsigned short _value2 = 0;
	if (!_wantsNumber) {
		_value2 = Registers[_wantedReg2];
	}
	else {
		_value2 = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		_wantedReg1 = _wantedReg2;
	}
	if (_value2 == 0) {
		// Interrup wegen Division durch Null
		P_HardwareInterrupt(divion_with_0);
		return;
	}
	if (Flags & 0b01000000) {
		*(Registers + _wantedReg1) = (unsigned short)(signed short)*(Registers + _wantedReg1) % (signed short)_value2;
	}
	else {
		*(Registers + _wantedReg1) %= _value2;
	}
}
void L_GetHigh() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	*(Registers + _wantedReg) = (unsigned short)Buffer >> 16;
}
void L_GetLow() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	*(Registers + _wantedReg) = (unsigned short)Buffer;
}
void L_RMC() {
	Flags &= 0b11111000;
}
// Bit Operation
void L_And(bool _wantsNumber) {
	if (!_wantsNumber) {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		*(Registers + _wantedReg1) &= *(Registers + _wantedReg2);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short _value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		*(Registers + _wantedReg1) &= _value;
	}
}
void L_OR(bool _wantsNumber) {
	if (!_wantsNumber) {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		*(Registers + _wantedReg1) |= *(Registers + _wantedReg2);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short _value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		*(Registers + _wantedReg1) |= _value;
	}
}
void L_XOR(bool _wantsNumber) {
	if (!_wantsNumber) {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		*(Registers + _wantedReg1) ^= *(Registers + _wantedReg2);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short _value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		*(Registers + _wantedReg1) ^= _value;
	}
}
void L_Not() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	Registers[_wantedReg] = ~Registers[_wantedReg];
}
void L_SHL(bool _wantsNumber) {
	if (!_wantsNumber) {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		*(Registers + _wantedReg1) <<= *(Registers + _wantedReg2);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short _value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		*(Registers + _wantedReg1) <<= _value;
	}
}
void L_SHR(bool _wantsNumber) {
	if (!_wantsNumber) {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4, _wantedReg2 = LoadRAM(ProgrammCounter) & 0b00001111;
		*(Registers + _wantedReg1) >>= *(Registers + _wantedReg2);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg1 = LoadRAM(ProgrammCounter);
		unsigned short _value = LoadRAM16(ProgrammCounter + 1);
		ProgrammCounter += 2;
		*(Registers + _wantedReg1) >>= _value;
	}
}
// Jump
void L_JMP() {
	++ProgrammCounter;
	ProgrammCounter = LoadRAM16(ProgrammCounter);
	ProgrammCounter += BasisAdress;
	--ProgrammCounter;
}
void L_Call() {
	L_Push(true); // Die CPU Funktion aufrufen um den derzeitigen PC zu pushen
	++ProgrammCounter;
	ProgrammCounter = LoadRAM16(ProgrammCounter);
	ProgrammCounter += BasisAdress;
	--ProgrammCounter;
}
void L_JE() {
	if (Flags & 0b00010000) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JNE() {
	if (!(Flags & 0b00010000)) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JZ() {
	if (Flags & 0b00000010) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JNZ() {
	if (!(Flags & 0b00000010)) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JG() {
	if (Flags & 0b00001000) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JGE() {
	if (Flags & 0b00011000) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JL() {
	if (Flags & 0b00100000) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JLE() {
	if (Flags & 0b00110000) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JC() {
	if (Flags & 0b00000001) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JNC() {
	if (!(Flags & 0b00000001)) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JN() {
	if (Flags & 0b00000100) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_JNN() {
	if (!(Flags & 0b00000100)) {
		++ProgrammCounter;
		ProgrammCounter = LoadRAM16(ProgrammCounter);
		ProgrammCounter += BasisAdress;
		--ProgrammCounter;
	}
	else {
		ProgrammCounter += 2;
	}
}
void L_Ret() {
	L_Pop(true);
	ProgrammCounter += 2;
	if (!(ProgrammCounter >= BasisAdress && ProgrammCounter <= EndAdress) && !(Flags & 0b10000000)) {
		P_HardwareInterrupt(manipulated_Return_Value);
	}
}
void L_Hop(bool _notFlag) {
	++ProgrammCounter;
	byte _flag = LoadRAM(ProgrammCounter);
	if (_notFlag) {
		if (!(_flag & Flags)) {
			++ProgrammCounter;
			byte _offSet = LoadRAM(ProgrammCounter);
			if (Flags & 0b00100000) {
				sbyte _sOffSet = (sbyte)_offSet;
				ProgrammCounter += _sOffSet;
			}
			else {
				ProgrammCounter += _offSet;
			}
			ProgrammCounter += BasisAdress;
			return;
		}
	}
	else {
		if (_flag & Flags) {
			++ProgrammCounter;
			byte _offSet = LoadRAM(ProgrammCounter);
			if (Flags & 0b00100000) {
				sbyte _sOffSet = (sbyte)_offSet;
				ProgrammCounter += _sOffSet;
			}
			else {
				ProgrammCounter += _offSet;
			}
			ProgrammCounter += BasisAdress;
			return;
		}
	}
	++ProgrammCounter;
}
// Stack
void L_Push(bool _calledFromCPU) {
	unsigned short _tempStackAdress = StackAdress;
	_tempStackAdress += StackPointer;
	if (_tempStackAdress >= StackAdressEnd) {
		P_HardwareInterrupt(stack_overflow);
		++ProgrammCounter;
		return;
	}
	if (_calledFromCPU) {

		StoreRAM16(_tempStackAdress, ProgrammCounter);
	}
	else {
		++ProgrammCounter;
		byte _wantedReg = LoadRAM(ProgrammCounter);
		StoreRAM16(_tempStackAdress, *(Registers + _wantedReg));
	}
	StackPointer += 2;
}
void L_Pop(bool _calledFromCPU) {
	unsigned short _tempStackAdress = StackAdress;
	if (StackPointer >= 2) {
		StackPointer -= 2;
	}
	else {
		// _Call Interrup (Stack Underflow)
		P_HardwareInterrupt(stack_underflow);
		return;
	}
	_tempStackAdress += StackPointer;
	if (_calledFromCPU) {
		ProgrammCounter = LoadRAM16(_tempStackAdress);
	}
	else {
		++ProgrammCounter;
		byte _wantedRegister = LoadRAM(ProgrammCounter);
		*(Registers + _wantedRegister) = LoadRAM16(_tempStackAdress);
	}
}
void L_Peek() {
	unsigned short _tempStackAdress = StackAdress;
	unsigned _tempStackPointer = 0;
	if (StackPointer >= 2) {
		_tempStackPointer = (StackPointer - 2);
	}
	else {
		// _Call Interrup (Stack Underflow)
		P_HardwareInterrupt(stack_underflow);
		return;
	}
	++ProgrammCounter;
	byte _wantedRegister = LoadRAM(ProgrammCounter);
	_tempStackAdress += _tempStackPointer;
	*(Registers + _wantedRegister) = LoadRAM16(_tempStackAdress);
}
void L_PushB() {
	unsigned short _tempStackAdress = StackAdress;
	_tempStackAdress += StackPointer;
	++ProgrammCounter;
	if (_tempStackAdress >= StackAdressEnd) {
		P_HardwareInterrupt(stack_overflow);
		return;
	}
	byte _wantedReg = LoadRAM(ProgrammCounter);
	StoreRAM(_tempStackAdress, (byte)*(Registers + _wantedReg));
	++StackPointer;
}
void L_PopB() {
	unsigned short _tempStackAdress = StackAdress;
	if (StackPointer >= 1) {
		--StackPointer;
	}
	else {
		// _Call Interrup (Stack Underflow)
		P_HardwareInterrupt(stack_underflow);
		return;
	}
	_tempStackAdress += StackPointer;
	++ProgrammCounter;
	byte _wantedRegister = LoadRAM(ProgrammCounter);
	*(Registers + _wantedRegister) = LoadRAM(_tempStackAdress);
}
void L_PeekB() {
	unsigned short _tempStackAdress = StackAdress;
	unsigned _tempStackPointer = 0;
	if (StackPointer >= 2) {
		_tempStackPointer = (StackPointer - 1);
	}
	else {
		// _Call Interrup (Stack Underflow)
		P_HardwareInterrupt(stack_underflow);
		return;
	}
	++ProgrammCounter;
	byte _wantedRegister = LoadRAM(ProgrammCounter);
	_tempStackAdress += _tempStackPointer;
	*(Registers + _wantedRegister) = LoadRAM(_tempStackAdress);
}
// _Syscall TODO: Unsicherheiten ob die sicherheit der Programm Counter gew�hrleistet ist. (Das nichts �berschrieben wird)
void L_Syscall() {
	++ProgrammCounter;
	unsigned short _wantedInterruped = LoadRAM16(ProgrammCounter);
	++ProgrammCounter;
	SystemRegisters[0] = ProgrammCounter; // Damit das OS zugriff auf den PC hat um diesen wiederherzustellen TODO: �berlegen ob das so gut ist.
	// _Call Interrup Handler f�rs suchen des Gew�nschten Interrupts.
	struct sysCall* _syscall = CallInterrupt(_wantedInterruped);
	if (_syscall == NULL) {
		SystemRegisters[2] = _wantedInterruped;		// Hier wird die ID des nicht gefundenen Interrupts im SReg2 gespeichert
		P_HardwareInterrupt(interupt_not_found);
		return; // Damit kein Warning erscheint
	}
	if (_syscall->Flag & 0b01000000) {
		// Interrupt an Kernel das ein User-_Call aufgerufen wurde.
		struct sysCall* _usercall = CallInterrupt(user_Call);
		ProgrammCounter = _usercall->Adress;
		--ProgrammCounter;
		SystemRegisters[1] = _syscall->Adress; // Bei einem Usercall wird die gew�nschte _Syscall adresse hier gespeichert damit das OS darauf zugreifen kann
		return;
	}
	else if (_syscall->Flag & 0b00100000) {
		// _Call interrupt weil versucht wird ein hardware interrupt zu rufen.
		P_HardwareInterrupt(tryed_hardware_Interrupt);
		return;
	}
	ProgrammCounter = _syscall->Adress;
	--ProgrammCounter;
	Flags |= 0b10000000;
}

// Kernel OpCodes
// CPU TODO: SetMMU und SetStkPtr brauchen Register wertung
void L_SetMMU() {
	++ProgrammCounter;
	byte _adressReg = LoadRAM(ProgrammCounter);
	P_SetMMUPtr(Registers[_adressReg]);
}
void L_SetStkPtr() {
	++ProgrammCounter;
	byte _adressReg = LoadRAM(ProgrammCounter);
	StackPointer = Registers[_adressReg];
}
void L_SetStkAdress() {
	++ProgrammCounter;
	byte _wantedRegValue = LoadRAM(ProgrammCounter);
	byte _Reg1 = (_wantedRegValue >> 4), _Reg2 = (_wantedRegValue & 0b1111);
	StackAdress = *(Registers + _Reg1);
	StackAdressEnd = *(Registers + _Reg2);
}
void L_GetSReg() {
	++ProgrammCounter;
	byte _wantedRegValue = LoadRAM(ProgrammCounter);
	byte _Reg1 = (_wantedRegValue >> 4), _Reg2 = (_wantedRegValue & 0b1111);
	*(Registers + _Reg1) = *(SystemRegisters + _Reg2);
}
void L_SetPC() {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter);
	ProgrammCounter = *(Registers + _wantedReg1);
	--ProgrammCounter;
}
void L_SetAdressSpace() {
	++ProgrammCounter;
	byte _wantedRegValue = LoadRAM(ProgrammCounter);
	byte _Reg1 = (_wantedRegValue >> 4), _Reg2 = (_wantedRegValue & 0b1111);
	BasisAdress = *(Registers + _Reg1);
	EndAdress = *(Registers + _Reg2);
}
void L_SetClock(bool _wantsRegister) {
	++ProgrammCounter;
	if (!_wantsRegister) {
		System_clock = LoadRAM16(ProgrammCounter);
	}
	else {
		byte _wantedReg = LoadRAM(ProgrammCounter);
		System_clock = Registers[_wantedReg];
	}
}
void L_GetClock() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	Registers[_wantedReg] = (unsigned short)System_clock;
}
void L_SysRet() {
	ProgrammCounter = SystemRegisters[0];
	--ProgrammCounter;						// TODO: Schauen ob es damit keine probleme gibt
	Flags &= 0b01111111;
}
void L_GetRegs() { // TODO: �berarbeitung? Vielleicht mehr von anderen dann nehmen
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	unsigned short _value = LoadRAM16(ProgrammCounter + 1);
	ProgrammCounter += 2;
	if (_value == 1) {
		Registers[_wantedReg] = Flags & 0b01111111;
	}
	else if (_value == 2) {
		Registers[_wantedReg] = StackPointer;
	}
}
void L_SetRegs() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	unsigned short _value = LoadRAM16(ProgrammCounter + 1);
	ProgrammCounter += 2;
	if (_value == 1) {
		Flags = Registers[_wantedReg] & 0b01111111;
	}
	else if (_value == 2) {
		Buffer = Registers[_wantedReg] << 16;
	}
	else if (_value == 3) {
		Buffer |= Registers[_wantedReg];
	}
}
void L_Exec() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	ProgrammCounter = Registers[_wantedReg];
	--ProgrammCounter;
	Flags &= 0b01111111;
}
// GPU TODO: Noch bald mit dem neuen System erweitern zum wechseln zwischen Zahlen und Registern.
void L_SetChr() {
	++ProgrammCounter;
	byte _wantedReg1 = (LoadRAM(ProgrammCounter) >> 4);
	byte _wantedReg2 = (LoadRAM(ProgrammCounter) & 0b1111);
	++ProgrammCounter;
	byte _idRegister = LoadRAM(ProgrammCounter);
	// Character-ROM (CHR-ROM) Logic hier einf�gen
	LoadCHR_Sprite(Registers[_wantedReg1], Registers[_wantedReg2], Registers[_idRegister]); // Warnung ist falls _wantedReg mehr ist als 9 (Die Register anzahl)
}
void L_SetSpr() {
	++ProgrammCounter;
	byte _wantedReg1 = (LoadRAM(ProgrammCounter) >> 4);
	byte _wantedReg2 = (LoadRAM(ProgrammCounter) & 0b1111);
	++ProgrammCounter;
	byte _adressRegister = LoadRAM(ProgrammCounter);
	unsigned int _x = (unsigned int)Registers[_wantedReg1], _posX = _x;
	unsigned int _y = (unsigned int)Registers[_wantedReg2], _posY = _y;
	for (byte y_modifier = 1; 0 < Sprite_Height; ++y_modifier) {
		_posY = _y * y_modifier;
		for (byte x_modifier = 0; 0 < Sprite_Width; ++x_modifier) {
			_posX = _x + x_modifier;
			_posX += _posY * ScreenWidth;
			Color wantedColor = {Registers[_adressRegister], Registers[_adressRegister + 1], Registers[_adressRegister + 2], Registers[_adressRegister + 3] };
			_adressRegister += 4;
			StoreVRAM64(_posX, wantedColor);
		}
	}
}
void L_SetPx() {
	++ProgrammCounter;
	byte _wantedReg1 = (LoadRAM(ProgrammCounter) >> 4);
	byte _wantedReg2 = (LoadRAM(ProgrammCounter) & 0b1111);
	++ProgrammCounter;
	byte _wantedRegR = LoadRAM(ProgrammCounter);
	++ProgrammCounter;
	byte _wantedRegG = LoadRAM(ProgrammCounter);
	++ProgrammCounter;
	byte _wantedRegB = LoadRAM(ProgrammCounter);
	++ProgrammCounter;
	byte _wantedRegA = LoadRAM(ProgrammCounter);
	Color wantedColor = { Registers[_wantedRegR], Registers[_wantedRegG], Registers[_wantedRegB], Registers[_wantedRegA] };
	unsigned int _x = Registers[_wantedReg1];
	unsigned int _y = *(Registers + _wantedReg2);
	_x += (_y * ScreenWidth);
	StoreVRAM64(_x, wantedColor); // x wird zur Adresse und der Offset
}
void L_SetGPUFlag() {
	G_GPU_Status |= 4;
}
// IPU
void L_AddInt(bool _wantsRegister) {
	++ProgrammCounter;
	byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4;
	byte _wantedReg3 = LoadRAM(ProgrammCounter) & 0b1111;
	++ProgrammCounter;
	// TODO: Vielleicht auch so machen das Reg3 auch fest sein kann
	if (_wantsRegister) {
		byte _wantedReg2 = LoadRAM(ProgrammCounter);
		RegisterInterrupt(Registers[_wantedReg1], Registers[_wantedReg2], (byte)*(Registers + _wantedReg3));
	}
	else {
		unsigned short _adress = LoadRAM16(ProgrammCounter);
		++ProgrammCounter;
		RegisterInterrupt(Registers[_wantedReg1], _adress, (byte)*(Registers + _wantedReg3));
	}
}
void L_RMVInt() {
	++ProgrammCounter;
	byte _wantedReg = LoadRAM(ProgrammCounter);
	RemoveInterrupt(*(Registers + _wantedReg));
}
void L_GetInt() {
	++ProgrammCounter;
	byte _value = LoadRAM(ProgrammCounter);
	for (byte i = 0; i < _value; ++i) {
		Registers[(i & 0x7)] = P_Buffer_Interrupt[(i & 0x7)];
	}
}
void L_DI() {
	P_InterruptsActive = false;
}
void L_EI() {
	P_InterruptsActive = true;
}
void L_RetInt() {
	ProgrammCounter = SystemRegisters[0];
	--ProgrammCounter;
	P_InInterruptMode = false;
}
// SSD [TODO: Testen ob wantsRegister funktionieren]
void L_LoadBlock(bool wantsRegister) {
	++ProgrammCounter;
	if (wantsRegister) {
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4;
		byte _wantedReg2 = LoadRAM(ProgrammCounter) & 0b1111;
		byte _wantedReg3 = LoadRAM(ProgrammCounter + 1);
		++ProgrammCounter;
		P_GetStorageBlock(*(Registers + _wantedReg1), *(Registers + _wantedReg2), (byte)*(Registers + _wantedReg3));
	}
	else {
		unsigned short _blockAdress = LoadRAM16(ProgrammCounter);
		unsigned short _RAMAdress = LoadRAM16(ProgrammCounter + 2);
		byte _blockID = LoadRAM(ProgrammCounter + 4);
		ProgrammCounter += 4;
		P_GetStorageBlock(_blockAdress, _RAMAdress, _blockID);
	}
}
void L_WriteBlock(bool wantsRegister) {
	++ProgrammCounter;
	if (wantsRegister) {
		byte _wantedReg1 = LoadRAM(ProgrammCounter) >> 4;
		byte _wantedReg2 = LoadRAM(ProgrammCounter) & 0b1111;
		byte _wantedReg3 = LoadRAM(ProgrammCounter + 1);
		++ProgrammCounter;
		P_WriteStorageBlock(*(Registers + _wantedReg1), *(Registers + _wantedReg2), (byte)*(Registers + _wantedReg3));
	}
	else {
		unsigned short _blockAdress = LoadRAM16(ProgrammCounter);
		unsigned short _RAMAdress = LoadRAM16(ProgrammCounter + 2);
		byte _blockID = LoadRAM(ProgrammCounter + 4);
		ProgrammCounter += 4;
		P_WriteStorageBlock(_blockAdress, _RAMAdress, _blockID);
	}
}

// Other

void P_HardwareInterrupt(const unsigned short _id) {
	P_InInterruptMode = true;
	SystemRegisters[0] = ProgrammCounter; // Damit das OS zugriff auf den PC hat um diesen wiederherzustellen TODO: �berlegen ob das so gut ist.
	struct sysCall* _syscall = CallInterrupt(_id);
	if (_syscall == NULL) {
		SystemRegisters[2] = _id;		// Hier wird die ID des nicht gefundenen Interrupts im SReg2 gespeichert
		P_HardwareInterrupt(interupt_not_found);
		return;
	}
	ProgrammCounter = _syscall->Adress;
}

void Execute();

void SetKernelPointer(const unsigned short _pointer) {
	ProgrammCounter = _pointer;
}

sbyte PullEvents() {
	if (P_InInterruptMode == true) {
		return 0;
	}
	if (!(G_GPU_Status & 4) && P_InterruptsActive == true) {
		P_HardwareInterrupt(GPU_VBlank);
		return 0;
	}
	if (WindowShouldClose()) {
		P_HardwareInterrupt(system_Force_Exit);
		return 0;
	}
	return 0;
}

sbyte StartExecuteCPU() {
	Status ^= 0b00000110; // Setzt die Running Flag zu 1 und die Idle _flag auf 0
	Flags |= 0b10000000; // Kernel Mode wird Aktiviert
	while (Status & 0b00000001) {
		// Vielleicht in Zukunft hier eine art Pipeline machen (Tendiere wenn dann mehr zu idee 2)
		/*
		Idee 1:
		Execute
		Predict [Als idee?]
		Fetch

		Idee 2:
		First Fetch // Vor while schleife
		Execute (in einem Thread)
		Fetch (in einem anderem Thread)
		*/
		Execute();
		if (P_InInterruptMode == false) {
			if (P_InInterruptMode == false) {
				PullEvents();
			}
			if (System_clock <= 0 && !(Flags & 0b10000000)) {
				P_HardwareInterrupt(timer);
			}
			else if (!(Flags & 0b10000000)) {
				--System_clock;
			}
		}
	}
	Status ^= 0b00000110; // Setzt die Running Flag zu 0 und die Idle _flag auf 1
	if (Status & 0b00001000) { // Wenn die Flag auf "Restart" gesetzt wird wird die BPU angewiesen das System mit gleichem Kernel neuzustarten.
		return -1;
	}
	return 1; // Gibt an das die CPU nicht mehr Arbeitet
}
void Execute() {
		++P_OpCodeCounter;
		OpCode = LoadRAM16(ProgrammCounter);
		++ProgrammCounter;
		ModeOpCode = OpCode >> 12;
		if (ModeOpCode & 0b00001111) {
			// Hat Kernel OpCodes dabei
			if (Flags & 0b10000000) {
				// CPU ist auch in Kernel Mode
				switch (OpCode)
				{
					// CPU[0b0001|0000|0000|0000]:
					case setMMU:
						L_SetMMU();
						break;
					case setStkPtr:
						L_SetStkPtr();
						break;
					case setStkAdress:
						L_SetStkAdress();
						break;
					case getSReg:
						L_GetSReg();
						break;
					case setPC:
						L_SetPC();
						break;
					case setAdressSpace:
						L_SetAdressSpace();
						break;
					case setClock:
						L_SetClock(false);
						break;
					case setClock_r:
						L_SetClock(true);
						break;
					case getClock:
						L_GetClock();
						break;
					case sysRet:
						L_SysRet();
						break;
					case getRegs:
						L_GetRegs();
						break;
					case setRegs:
						L_SetRegs();
						break;
					case exec:
						L_Exec();
						break;
					case setPx:
						L_SetPx();
						break;
					case startFrame:
						//L_SetGPUFlag();
						RenderFrame();
						break;
					case addInt:
						L_AddInt(false);
						break;
					case addIntReg:
						L_AddInt(true);
						break;
					case rmvInt:
						L_RMVInt();
						break;
					case getInt:
						L_GetInt();
						break;
					case di:
						L_DI();
						break;
					case ei:
						L_EI();
						break;
					case retInt:
						L_RetInt();
						break;
					case loadBlock:
						L_LoadBlock(false);
						break;
					case writeBlock:
						L_WriteBlock(false);
						break;
					case restart:
						Status &= 0b00000000; // <--- Tempor�r f�rs Testen
						Status |= 0b00001000; // <--- Tempor�r f�rs Testen
						break;
					case terminate:
						Status &= 0b00000000; // <--- Tempor�r f�rs Testen
						break;
					default:
						break;
				}
			}
			else {
				// CPU ist nicht in Kernel Mode, Interrup soll ausgel�st werden.
				P_HardwareInterrupt(previlige_Instruction);
			}
		}
		else {
			// Hat keine Kernel OpCodes
			switch (OpCode)
			{
				// Register -> [0b0000|0000|0000|0000]
				case set: // _Set Reg1 Value
					L_Set();
					break;
				case inc: // _Inc Reg1
					L_Inc();
					break;
				case dec: // _Dec Reg1
					L_Dec();
					break;
				case neg: // _Neg Reg1
					L_Neg();
					break;
				case cmp: // _CMP Reg1 Reg2
					L_CMP(false);
					break;
				case cmp_i:
					L_CMP(true);
					break;
				case mov: // _Mov Reg1 Reg2
					L_Mov();
					break;
				case swi: // _Swi Reg1 Reg2
					L_Swi();
					break;
				case clr: // Clr
					L_CLR();
					break;
				case signedFlag:
					L_Signed();
					break;
					// Memory -> [0b0000|0001|0000|0000]
				case load: // _Load Reg1 $Adress
					L_Load(false);
					break;
				case load_i: // _Load Reg1 $Adress
					L_Load(true);
					break;
				case store: // _Store Reg1 $Adress
					L_Store(false);
					break;
				case store_i: // _Store Reg1 $Adress
					L_Store(true);
					break;
				case loadB: // LoadB Reg1 $Adress
					L_LoadBit(false);
					break;
				case loadB_i: // LoadB Reg1 $Adress
					L_LoadBit(true);
					break;
				case storeB: // StoreB Reg1 $Adress
					L_StoreBit(false);
					break;
				case storeB_i: // StoreB Reg1 $Adress
					L_StoreBit(true);
					break;
				case rswitch: // Noch nicht in Use [rswitch Value]
					break;
					// Math -> [0b0000|0010|0000|0000]
				case add: // _Add Reg1 Reg2
					L_Add(false);
					break;
				case add_i:
					L_Add(true);
					break;
				case sub: // _Sub Reg1 Reg2
					L_Sub(false);
					break;
				case sub_i:
					L_Sub(true);
					break;
				case mul: // _Mul Reg1 Reg2
					L_Mul(false);
					break;
				case mul_i:
					L_Mul(true);
					break;
				case division: // _Div Reg1 Reg2
					L_Div(false);
					break;
				case division_i:
					L_Div(true);
					break;
				case mod: // _Mod Reg1 Reg2
					L_Mod(false);
					break;
				case mod_i:
					L_Mod(true);
					break;
				case getHigh: // _GetHigh Reg1
					L_GetHigh();
					break;
				case getLow: // _GetLow Reg1
					L_GetLow();
					break;
				case rmc:
					L_RMC();
					break;
					// Bit-Operation -> [0b0000|0011|0000|0000] (Nochmal schauen ob das richtig funktioniert oder ob beim Enum noch bei Bit Was dazu muss)
				case and: // _And Reg1 Reg2
					L_And(false);
					break;
				case and_i:
					L_And(true);
					break;
				case or: // OR Reg1 Reg2
					L_OR(false);
					break;
				case or_i:
					L_OR(true);
					break;
				case xor: // _XOR Reg1 Reg2
					L_XOR(false);
					break;
				case xor_i:
					L_XOR(true);
					break;
				case not: // _Not Reg1
					L_Not();
					break;
				case shl: // shl Reg1 Reg2
					L_SHL(false);
					break;
				case shl_i:
					L_SHL(true);
					break;
				case shr: // shr Reg1 Reg2
					L_SHR(false);
					break;
				case shr_i:
					L_SHR(true);
					break;
					// Jump -> [0b0000|0100|0000|0000]
				case jmp: // _JMP Label
					L_JMP();
					break;
				case call: // _Call Label
					L_Call();
					break;
				case je: // _JE Label
					L_JE();
					break;
				case jne: // _JNE Label
					L_JNE();
					break;
				case jz: // _JZ Label
					L_JZ();
					break;
				case jnz: // _JNZ Label
					L_JNZ();
					break;
				case jg: // _JG Label
					L_JG();
					break;
				case jge: // _JGE Label
					L_JGE();
					break;
				case jl: // _JL Label
					L_JL();
					break;
				case jle: // _JLE Label
					L_JLE();
					break;
				case jc: // _JC Label
					L_JC();
					break;
				case jnc: // _JNC Label
					L_JNC();
					break;
				case jNegativ: // _JN Label
					L_JN();
					break;
				case jnn: // _JNN Label
					L_JNN();
					break;
				case ret:
					L_Ret();
					break;
				case hop: // _Hop Flag Offset
					L_Hop(false);
					break;
				case hop_i:
					L_Hop(true);
					break;
					// Stack -> [0b0000|0101|0000|0000]:
				case push: // _Push Reg1
					L_Push(false);
					break;
				case pop: // _Pop Reg1
					L_Pop(false);
					break;
				case peek: // _Peek Reg1
					L_Peek();
					break;
				case pushB: // _PushB Reg1
					L_PushB();
					break;
				case popB: // _PopB Reg1
					L_PopB();
					break;
				case peekB: // _PeekB Reg1
					L_PeekB();
					break;
					// _Syscall -> [0b0000|0110|0000|0000]:
				case syscall: // _Syscall Reg1
					L_Syscall();
					break;
					// Others -> [0b0000|1111|0000|0000]:
				case nop: // NOP (No Operation)
					break;
				case halt: // Halt
					--ProgrammCounter;
					--ProgrammCounter;
					break;
				default:
					break;
			}
		}
		++ProgrammCounter;
	}