/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/interruptHandler.h"
#include <stdio.h>
#include <stdlib.h>

struct IPU* IPU = NULL;

bool initIPU() {
	struct IPU* _interruptHandler = (struct IPU*)malloc(sizeof(struct IPU));
	if (_interruptHandler == NULL) {
		printf("Error: IPU konnte nicht Initialisiert werden.\n");
		return false;
	}
	struct sysCall* _interrupt = (struct sysCall*)malloc(1000 * sizeof(struct sysCall));
	if (_interrupt == NULL) {
		printf("Initialisieren der Interrupts Eintr�ge sind fehlgeschlagen.\n");
		free(_interruptHandler);
		return false;
	}
	_interruptHandler->interrupt = _interrupt;
	_interruptHandler->RegisteredInterrupt = 0;
	for (unsigned short i = 0; i != 1000; ++i) {
		(_interrupt + i)->Interrupt_id = i;
		(_interrupt + i)->Adress = 0;
		(_interrupt + i)->Flag = 0b10000000;
	}
	IPU = _interruptHandler;
	return true;
}

// Registriert den Interrupt bei der IPU mit der gew�nschten ID.
bool RegisterInterrupt(unsigned short _interruptID, const unsigned short _adress, const byte _flags) {
	struct sysCall* _interrupt = IPU->interrupt;
	if (_interruptID >= 1000) {
		printf("Error: Interrupt ID ist zu gro�\n");
		return false;
	}
	if ((_interrupt + _interruptID)->Flag == 0b10000000) {
		(_interrupt + _interruptID)->Adress = _adress;
		(_interrupt + _interruptID)->Flag = _flags;
		return true;
	}
	return false;
}

// Es Obliegt dem Programmierer das keine Hardware Interrupts entfernt werden.
void RemoveInterrupt(const unsigned short _interruptID) {
	struct sysCall* _interrupt = IPU->interrupt;
	if (_interruptID >= 1000) {
		printf("Error: Interrupt ID ist zu gro�\n");
		return;
	}
	(_interrupt + _interruptID)->Adress = 0;
	(_interrupt + _interruptID)->Flag = 0b10000000;
}

struct sysCall* CallInterrupt(const unsigned short _interruptID) {
	struct sysCall* _interrupt = IPU->interrupt;
	if (_interruptID >= 1000) {
		printf("Error: Interrupt ID ist zu gro�\n");
		return NULL;
	}
	if (!(_interrupt[_interruptID].Flag & 0b10000000)) {
		return (_interrupt + _interruptID);
	}
	return NULL;
}

void ResetIPU() {
	struct sysCall* _interrupt = IPU->interrupt;
	for (unsigned short i = 0; i < 1000; ++i) {
		(_interrupt + i)->Adress = 0;
		(_interrupt + i)->Flag = 0b10000000;
	}
}

void FreeIPU() {
	free(IPU->interrupt);
	free(IPU);
}