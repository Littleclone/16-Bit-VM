/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/CPU.h"
#include "../header/RAM.h"
#include "../header/interruptHandler.h"
#include "../header/SSD.h"

char* AssemblyInput = NULL; // Der Code der in Maschienensprache �bersetzt werden soll
struct currentFile* head = NULL;
struct LabelData* HeadLabel = NULL, ** Head_ptr = &HeadLabel;
struct P_HardwareInterrupt* HeadInterrupt = NULL, ** InterruptHead_ptr = &HeadInterrupt;
unsigned short Char_Position = 0; // Sicher machen das es ab dem ersten Label / OpCode.
unsigned short ProgrammStart = 0;

// Debug
#if 1
unsigned short RowCounter = 0;
unsigned short LabelCounter = 0;
unsigned short InterruptCounter = 0;
#endif

sbyte L_ConfigInterrupt();
sbyte L_AnalyseCode();
sbyte L_CountRows();
sbyte L_ReplaceLabels(bool isKernelLoad);
unsigned short L_LoadProcessInRAM(const unsigned short processBegin);
void L_SetHardwareInterrupts();
void L_FreeNeededRessources(bool isKernelLoad);

sbyte L_MakeNewRow(const OpCodes opCode);
unsigned short L_LoadInterruptInRAM();
void L_LoadRowInRAM(const unsigned short _processBegin, const struct assemblyRow* currentRow);

unsigned short P_ExecuteAssembler(char* input, const bool isKernel_C, unsigned short beginAdress, bool needsBin) {
	head = (struct currentFile*)malloc(sizeof(struct currentFile));
	if (head == NULL) {
		printf("No Head for Assembler.\n");
		return 0;
	}
	sbyte result = 0;
	head->head = NULL;
	AssemblyInput = input;
	if (needsBin && isKernel_C) {
		result = L_ConfigInterrupt(); // TODO: Erkennt bei dem Pr�sentation Code nicht das die Funktion mit dem Interrupt nicht exisitert (Replace Labels muss dies machen)
		if (result == -1) {
			return 0;
		}
	}
	result = L_AnalyseCode(); // Hier wird der Code in seine Einzelteile zerlegt (zeile f�r zeile)
	if (result == -1) {
		return 0;
	}
	result = L_CountRows(); // Hier werden die Zeilen gez�hlt und die Labels mit ihren Adressen notiert.
	if (result == -1) {
		return 0;
	}
	result = L_ReplaceLabels(isKernel_C); // Hier werden die Labels im OpCode durch die wirkliche Adresse ersetzt
	if (result == -1) {
		return 0;
	}
	if (needsBin) {
		beginAdress += L_LoadInterruptInRAM();
	}
	unsigned short endProcess = L_LoadProcessInRAM(beginAdress); // Hier wird der Fertige Maschienen Code in den RAM geladen.
	if (isKernel_C && !needsBin) {
		SetKernelPointer(ProgrammStart); // Setzt den Pointer f�r die CPU wenn dies ein Kernel Boot ist
		L_SetHardwareInterrupts(); // Setzt die Hardware interrupts die n�tig sind damit das OS funktioniert
	}
	L_FreeNeededRessources(isKernel_C); // Hier werden alle Ressourcen (Linked Lists im Heap) wieder freigesetzt
#if IsDebug
	printf("Row: %i, Label: %i, Interrupt: %i\n", RowCounter, LabelCounter, InterruptCounter); // Debug
	printf("%i Bytes gebraucht oder [0x%x] im RAM als Endpoint\n", endProcess, endProcess);
#endif
	if (needsBin) {
		P_CreateKernelBin(ProgrammStart, endProcess);
	}
	return endProcess;
}

void L_SkipEmpty() {
	while (*(AssemblyInput + Char_Position) == ' ' || *(AssemblyInput + Char_Position) == '\n' || *(AssemblyInput + Char_Position) == '\t' || *(AssemblyInput + Char_Position) == '\r') {
		++Char_Position;
	}
}

void L_SkipComment() {
	while (*(AssemblyInput + Char_Position) != '\n') {
		++Char_Position;
	}
}

char* L_GetWord(char* string) {
	byte i = 0;
	while (P_IsChar(*(AssemblyInput + Char_Position)) || P_IsDigit(*(AssemblyInput + Char_Position))) {
		*(string + i) = *(AssemblyInput + Char_Position);
		++Char_Position;
		++i;
	}
	*(string + i) = '\0';
	return string;
}

unsigned short L_GetNumber(char* string) {
	unsigned short number = 0;
	bool couldBin = false, couldHex = false;
	byte i = 0;
	while (P_IsDigit(*(AssemblyInput + Char_Position)) || P_IsChar(*(AssemblyInput + Char_Position))) {
		*(string + i) = *(AssemblyInput + Char_Position);
		if (i == 0 && *(string + i) == '0' && *(AssemblyInput + (Char_Position + 1)) == 'b') {
			couldBin = true;
		}
		if (!couldBin && !couldHex && P_IsChar(*(string + i))) {
			couldHex = true;
		}
		++Char_Position;
		++i;
	}
	*(string + i) = '\0';
	if (couldBin) {
		// TODO: Muss ich schauen ob das immer noch funktioniert (Davor war die Alte StrRemove Funktion)
		char** splitArray = P_StrSplit(string, 'b');
		string = splitArray[1];
		number = (unsigned short)strtol(string, NULL, 2);
	}
	else if (couldHex) {
		number = (unsigned short)strtol(string, NULL, 16);
	}
	else {
		number = (unsigned short)strtol(string, NULL, 10);
	}
	return number;
}

void L_AddRow(struct assemblyRow* nextRow) {
	++RowCounter;
	if (head->head != NULL) {
		struct assemblyRow* temp = head->head;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = nextRow;
		nextRow->next = NULL;
	}
	else {
		head->head = nextRow;
		nextRow->next = NULL;
	}
}

void L_AddRowLabel(struct LabelData* nextLabel) {
	++LabelCounter;
	if (*Head_ptr != NULL) {
		struct LabelData* temp = *Head_ptr;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = nextLabel;
	}
	else {
		*Head_ptr = nextLabel;
	}
}

void L_AddRowInterrupt(struct P_HardwareInterrupt* nextInterrupt) {
	++InterruptCounter;
	if (*InterruptHead_ptr != NULL) {
		struct P_HardwareInterrupt* temp = *InterruptHead_ptr;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = nextInterrupt;
	}
	else {
		*InterruptHead_ptr = nextInterrupt;
	}
}


struct variable* L_GetNext(bool hasOpCode) {
	// Stellt sicher das wirklich ein Valider Zeichen als n�chstes kommt
	L_SkipEmpty();
	// Eine art Dynamische Variable
	struct variable* container = (struct variable*)malloc(sizeof(struct variable));
	if (container == NULL) {
		printf("Error: Container konnte nicht Allokiert werden.\n");
		return NULL;
	}
	container->word = NULL;
	container->flag = 0;
	// Die l�nge des N�chsten OpCodes / Werts (Danach wird ein char array mit dieser l�nge erstellt wo dann beispiel der OpCode gespeichert wird
	byte length = (byte)P_StrLenBeginAt(AssemblyInput, Char_Position);
	char* nextValue = malloc(length + 1);
	if (nextValue == NULL) {
		free(container);
		printf("Error: Next Value konnte nicht Allokiert werden.\n");
		return NULL;
	}
	// Hier wird geschaut ob wir schon den OpCode haben und das currentChar ein Buchstabe ist
	if (hasOpCode && P_IsChar(AssemblyInput[Char_Position])) {
		container->word = L_GetWord(nextValue);
		// OpCodes opCode = P_ValidateOpCode(nextValue);
		// if (opCode != notDefined) {
		// 	free(container);
		// 	free(nextValue);
		// 	return NULL;
		// }
		container->flag = 1;
		return container;
	}
	// Hier wird geschaut ob wir schon den OpCode haben und das currentChar eine Zahl ist
	else if (hasOpCode && P_IsDigit(AssemblyInput[Char_Position])) {
		container->number = L_GetNumber(nextValue);
		free(nextValue);
		container->flag = 1;
		return container;
	}
	// Wird aufgerufen wenn ein Command gefunden wurde
	else if (AssemblyInput[Char_Position] == ';') {
		//Comment Logic
		free(nextValue);
		container->flag = 1;
		++Char_Position;
		L_SkipComment();
		return container;
	}
	// Wenn wir noch kein OpCode haben wird hier geschaut obs ein Valider OpCode ist, ein Label oder etwas ung�ltiges
	else if (!hasOpCode && P_IsChar(AssemblyInput[Char_Position])) {
		nextValue = L_GetWord(nextValue);
		OpCodes opCode = P_ValidateOpCode(nextValue);
		if (opCode != notDefined) {
			container->flag = 1;
			free(nextValue);
			sbyte result = L_MakeNewRow(opCode);
			if (result == -1) {
				free(container);
				return NULL;
			}
		}
		else {
			if (AssemblyInput[Char_Position] == ':') {
				++Char_Position;
				container->flag = 1;
				struct assemblyRow* labelRow = malloc(sizeof(struct assemblyRow));
				if (labelRow == NULL) {
					printf("Error: LabelRow konnte nicht allokiert werden\n");
					free(nextValue);
					free(container);
					return NULL;
				}
				labelRow->label = nextValue;
				labelRow->flags = 0b00000010;
				L_AddRow(labelRow);
			}
			else {
				free(nextValue);
				free(container);
				return NULL;
			}
		}
		return container;
	}
	if (AssemblyInput[Char_Position] == '\0') {
		free(nextValue);
		return container;
	}
	free(container);
	free(nextValue);
	return NULL;
}

sbyte L_ConfigInterrupt() {
	struct variable* container = NULL;
	container = L_GetNext(true); // Es wird sichergestellt das alles klappt in dem beim OS Maker (in Godot) man �berpr�fen kann ob alles den Regeln entspricht
	P_ToLower(container->word);
	if (!P_StrCmp(container->word, "interrupts")) {
		printf("Interrupts wurde nicht gefunden wo es sein sollte.\n");
		return -1;
	}
	free(container->word);
	free(container);
	if (AssemblyInput[Char_Position] != '[') {
		return -1;
	}
	++Char_Position;
	container = L_GetNext(true);
	do {
		struct P_HardwareInterrupt* interrupt = malloc(sizeof(struct P_HardwareInterrupt));
		if (interrupt == NULL) {
			printf("Interrupt konnte nicht Allokiert werden.\n");
			return -1;
		}
		interrupt->next = NULL;
		interrupt->adress = 0;
		interrupt->interruptID = container->number;
		free(container);
		++Char_Position;
		container = L_GetNext(true);
		P_ToLower(container->word);
		interrupt->label = container->word;
		free(container);
		L_AddRowInterrupt(interrupt);
		container = L_GetNext(true);
	} while (container != NULL);
	free(container);
	++Char_Position;
	return 1;
}

sbyte L_AnalyseCode() {
	struct variable* container = NULL;
	do {
		if (container != NULL && container->flag != 0) {
			free(container);
		}
		container = L_GetNext(false);
		if (container == NULL) { // Taucht nur auf wenn was ung�ltiges gefunden wurde
			return -1;
		}
	} while (container->flag != 0); // Taucht nur auf wenn wir am ende der File sind
	free(container);
	return 1;
}

sbyte L_CountRows() { // Optimierungsm�glichkeiten?
	unsigned short counter = 0;
	struct assemblyRow* currentRow = head->head;
	while (currentRow->next != NULL) {
        if (!(currentRow->flags & 0b00000010)) {
        counter += currentRow->needBytes;
        }
		else {
			struct LabelData* data = malloc(sizeof(struct LabelData));
			if (data == NULL) {
				printf("Error: Data konnte nicht allokiert werden.\n");
				return -1;
			}
			data->label = currentRow->label;
			data->adress = counter;
			data->next = NULL;
			if (P_StrCmp(data->label, "main")) {
				data->isMain = true;
				ProgrammStart = counter;
			}
			L_AddRowLabel(data);
		}
		currentRow = currentRow->next;
	}
	return 1;
}

sbyte L_ReplaceLabels(bool isKernelLoad) {
	struct P_HardwareInterrupt* currentInterrupt = HeadInterrupt;
	struct assemblyRow* currentRow = head->head;
	struct LabelData* currentLabel = NULL;
	while (currentRow != NULL) {
		if (currentRow->flags & 0b00000100 && !(currentRow->flags & 0b00000010)) {
			currentLabel = HeadLabel;
			if (currentLabel == NULL) {
				printf("CurrentLabel ist NULL[1]\n");
				return -1;
			}
			while (!P_StrCmp(currentRow->label, currentLabel->label) && currentLabel != NULL) {
				currentLabel = currentLabel->next;
			}
			if (currentLabel == NULL) {
				printf("CurrentLabel ist NULL[2]\n");
				return -1;
			}
			currentRow->adress = currentLabel->adress;
		}
		currentRow = currentRow->next;
	}
	if (isKernelLoad) {
		while (currentInterrupt != NULL) {
			currentLabel = HeadLabel;
			if (currentLabel == NULL) {
				printf("CurrentLabel ist NULL[3]\n");
				return -1;
			}
			while (!P_StrCmp(currentInterrupt->label, currentLabel->label) && currentLabel->next != NULL) {
				currentLabel = currentLabel->next;
			}
			if (currentLabel == NULL) {
				printf("CurrentLabel ist NULL[4]\n");
				return -1;
			}
			currentInterrupt->adress = currentLabel->adress;
			currentInterrupt = currentInterrupt->next;
		}
	}
	return 1;
}

unsigned short L_LoadInterruptInRAM() {
	struct P_HardwareInterrupt* currentInterrupt = HeadInterrupt;
	unsigned short adressCounter = 0;
	StoreRAM(adressCounter, '[');
	++adressCounter;
	while (currentInterrupt != NULL) {
		StoreRAM16(adressCounter, currentInterrupt->interruptID);
		adressCounter += 2;
		StoreRAM16(adressCounter, currentInterrupt->adress);
		adressCounter += 2;
		currentInterrupt = currentInterrupt->next;
	}
	StoreRAM(adressCounter, ']');
	++adressCounter;
	return adressCounter;
}

unsigned short L_LoadProcessInRAM(const unsigned short processBegin) {
	struct assemblyRow* currentRow = head->head;
	unsigned short beginProcess = processBegin;
	while (currentRow != NULL) {
		if (currentRow->flags & 0b00000010) {
			currentRow = currentRow->next;
			continue;
		}
		L_LoadRowInRAM(beginProcess, currentRow);
		beginProcess += currentRow->needBytes;
		currentRow = currentRow->next;
	}
	return beginProcess; // Ist hier nun EndProcess
}

void L_SetHardwareInterrupts() {
	struct P_HardwareInterrupt* interrupt = HeadInterrupt;
	while (interrupt != NULL) {
		RegisterInterrupt(interrupt->interruptID, interrupt->adress, 0b00100000);
		interrupt = interrupt->next;
	}
}

void L_FreeNeededRessources(bool isKernelLoad) {
	struct assemblyRow* currentRow = head->head, *temp = NULL;
	while (currentRow != NULL) {
		--RowCounter;
		if (currentRow->flags & 0b00000100 && !(currentRow->flags & 0b00000010)) {
			free(currentRow->label);
		}
		temp = currentRow;
		currentRow = currentRow->next;
		free(temp);
	}
	struct LabelData* tempLabel = NULL;
	while (HeadLabel != NULL) {
		--LabelCounter;
		tempLabel = HeadLabel;
		free(HeadLabel->label);
		HeadLabel = HeadLabel->next;
		free(tempLabel);
	}
	if (isKernelLoad) {
		while (HeadInterrupt != NULL) {
			--InterruptCounter;
			struct P_HardwareInterrupt *currentInterrupt = HeadInterrupt;
			free(HeadInterrupt->label);
			HeadInterrupt = HeadInterrupt->next;
			free(currentInterrupt);
		}
	}
	free(AssemblyInput);
	free(head);
	AssemblyInput = NULL;
	head = NULL;
	Char_Position = 0;
}


sbyte L_Reg1_Value(const OpCodes L_opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = L_opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	byte reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte = reg;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word != NULL) {
		free(container->word);
		free(container);
		free(row);
		return -1;
	}
	row->adress = container->number;
	free(container);
	row->needBytes = 5;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_Value_Value_Value(const OpCodes opCode) /* F�r SSD Kernel OpCode kann auch 3 Register eintr�ge nehmen.*/ {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	if (container->word == NULL) {
		row->adress = container->number;
		free(container);
		container = L_GetNext(true);
		if (container == NULL) {
			free(row);
			return -1;
		}
		else if (container->word != NULL) {
			free(container->word);
			free(container);
			free(row);
			return -1;
		}
		row->adressOffset = container->number;
		free(container);
		container = L_GetNext(true);
		if (container == NULL) {
			free(row);
			return -1;
		}
		else if (container->word != NULL) {
			free(container->word);
			free(container);
			free(row);
			return -1;
		}
		row->adressByte = (byte)container->number;
		free(container);
		row->needBytes = 7;
		row->flags = 1;
		L_AddRow(row);
		return 1;
	}
	else {
		row->opCode = (opCode + 1);
		byte reg = P_IsRegister(container->word);
		free(container->word);
		free(container);
		if (reg == 255) {
			free(row);
			return -1;
		}
		container = L_GetNext(true);
		row->adressByte = reg << 4;
		if (container == NULL) {
			free(row);
			return -1;
		}
		else if (container->word == NULL) {
			free(container);
			free(row);
			return -1;
		}
		reg = P_IsRegister(container->word);
		free(container->word);
		free(container);
		if (reg == 255) {
			free(row);
			return -1;
		}
		row->adressByte |= reg;
		container = L_GetNext(true);
		if (container == NULL) {
			free(row);
			return -1;
		}
		else if (container->word == NULL) {
			free(container);
			free(row);
			return -1;
		}
		reg = P_IsRegister(container->word);
		free(container->word);
		free(container);
		if (reg == 255) {
			free(row);
			return -1;
		}
		row->adress = reg;
		row->needBytes = 4;
		row->flags = 0;
		L_AddRow(row);
		return 1;
	}
}
sbyte L_Reg1(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	byte _reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (_reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte = _reg;
	row->needBytes = 3;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_Reg1_Reg2(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	byte reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	if (opCode == add || opCode == sub || opCode == mul || opCode == division || opCode == mod ||
		opCode == and || opCode == or || opCode == xor || opCode == shl || opCode == shr || opCode == cmp) {
		if (container->word == NULL) {
			row->opCode = (opCode + 1);
			row->adressByte = reg;
			row->adress = container->number;
			free(container);
			row->needBytes = 5;
			row->flags = 0b00010000;
			L_AddRow(row);
			return 1;
		}
	}
	row->adressByte = reg << 4;
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte |= reg;
	row->needBytes = 3;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_Reg1_Label_Reg2(const OpCodes opCode) {
	// Derzeit nur von einem OpCode in benutzung
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	byte reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	if (opCode == addInt) {
		if (container->word == NULL) {
			free(container);
			free(row);
			return -1;
		}
		reg = P_IsRegister(container->word);
		if (reg != 255) {
			row->opCode = (opCode + 1);
			row->adress = reg;
			free(container->word);
			free(container);
			row->needBytes = 4;
			row->flags = 0b00001000;
		}
		else {
			row->label = container->word;
			P_ToLower(row->label);
			row->label = row->label;
			free(container);
			row->needBytes = 5;
			row->flags = 0b00000100;
		}
	}
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	row->adressByte = reg << 4;
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte |= reg;
	L_AddRow(row);
	return 1;
}
sbyte L_Reg1_Reg2_Reg3_Reg4_Reg5_Reg6(const OpCodes opCode) {
	// TODO Optimierungsm�glickeit: Man k�nnte Adress im ersten byte die 2 Register f�r RG und im zweiten byte f�r BA machen (Function: SetPx)
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	byte reg = P_IsRegister(container->word);
	row->adressByte = reg << 4;
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte |= reg;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adress = reg << 8;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adress |= reg;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressOffset = reg << 8;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressOffset |= reg;
	row->needBytes = 7;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_StandAlone(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	row->needBytes = 2;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_Reg1_Adress(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->flags = 0;
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	byte reg = P_IsRegister(container->word);
	free(container->word);
	free(container);
	if (reg == 255) {
		free(row);
		return -1;
	}
	row->adressByte = reg;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	if (container->word != NULL) {
		reg = P_IsRegister(container->word);
		free(container->word);
		free(container);
		if (reg == 255) {
			free(row);
			return -1;
		}
		row->opCode = (opCode + 1);
		row->adress = reg;
		row->flags = 0b00001000;
		row->needBytes = 4;
	}
	else {
		row->adress = container->number;
		row->needBytes = 5;
		free(container);
	}
	L_AddRow(row);
	return 1;
}
sbyte L_JMP_Label(OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word == NULL) {
		free(container);
		free(row);
		return -1;
	}
	row->label = container->word;
	P_ToLower(row->label);
	row->label = row->label;
	free(container);
	row->needBytes = 4;
	row->flags = 0b00000100;
	L_AddRow(row);
	return 1;
}
sbyte L_Value(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	if (opCode == setClock) {
		if (container->word != NULL) {
			byte reg = P_IsRegister(container->word);
			if (reg == 255) {
				free(row);
				return -1;
			}
			row->opCode = (opCode + 1);
			row->adressByte = reg;
			free(container->word);
			free(container);
			row->needBytes = 3;
			row->flags = 8;
			L_AddRow(row);
			return 1;
		}
	}
	else if (container->word != NULL) {
		free(row);
		free(container->word);
		free(container);
		return -1;
	}
	row->adress = container->number;
	free(container);
	row->needBytes = 4;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_ValueB(const OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word != NULL) {
		free(row);
		free(container->word);
		free(container);
		return -1;
	}
	row->adressByte = (byte)container->number;
	free(container);
	row->needBytes = 3;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_HopRow(OpCodes opCode) {
	struct assemblyRow* row = (struct assemblyRow*)malloc(sizeof(struct assemblyRow));
	struct variable* container = NULL;
	if (row == NULL) {
		return -1;
	}
	row->opCode = opCode;
	L_SkipEmpty();
	if (AssemblyInput[Char_Position] == '!') {
		++Char_Position;
		row->opCode = hop_i;
	}
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word != NULL) {
		free(row);
		free(container->word);
		free(container);
		return -1;
	}
	row->adress = container->number << 8;
	free(container);
	container = L_GetNext(true);
	if (container == NULL) {
		free(row);
		return -1;
	}
	else if (container->word != NULL) {
		free(row);
		free(container->word);
		free(container);
		return -1;
	}
	row->adress |= container->number;
	free(container);
	row->needBytes = 4;
	row->flags = 0;
	L_AddRow(row);
	return 1;
}
sbyte L_MakeNewRow(const OpCodes opCode) {
	sbyte result = 1;
	unsigned short modeOpCode = opCode >> 12;
	if (modeOpCode & 0b00001111) {
		// Hat Kernel OpCodes dabei
		switch (opCode)
		{
			// CPU[0b0001|0000|0000|0000]:
		case setMMU:
			result = L_Reg1(opCode);
			break;
		case setStkPtr:
			result = L_Reg1(opCode);
			break;
		case setStkAdress:
			result = L_Reg1_Reg2(opCode);
			break;
		case getSReg:
			result = L_Reg1_Reg2(opCode);
			break;
		case setPC:
			result = L_Reg1(opCode);
			break;
		case setAdressSpace:
			result = L_Reg1_Reg2(opCode);
			break;
		case setClock:
			result = L_Value(opCode);
			break;
		case getClock:
			result = L_Reg1(opCode);
			break;
		case sysRet:
			result = L_StandAlone(opCode);
			break;
		case getRegs:
			result = L_Reg1_Value(opCode);
			break;
		case setRegs:
			result = L_Reg1_Value(opCode);
			break;
		case exec:
			result = L_Reg1(opCode);
			break;
		case setPx:
			result = L_Reg1_Reg2_Reg3_Reg4_Reg5_Reg6(opCode);
			break;
		case startFrame:
			result = L_StandAlone(opCode);
			break;
		case addInt:
			result = L_Reg1_Label_Reg2(opCode);
			break;
		case rmvInt:
			result = L_Reg1(opCode);
			break;
		case getInt:
			result = L_ValueB(opCode);
			break;
		case di:
			result = L_StandAlone(opCode);
			break;
		case ei:
			result = L_StandAlone(opCode);
			break;
		case retInt:
			result = L_StandAlone(opCode);
			break;
		case loadBlock:
			result = L_Value_Value_Value(opCode);
			break;
		case writeBlock:
			result = L_Value_Value_Value(opCode);
			break;
		case restart:
			result = L_StandAlone(opCode);
			break;
		case terminate:
			result = L_StandAlone(opCode);
			break;
		default:
			break;
		}
	}
	else {
		// Hat keine Kernel OpCodes
		switch (opCode)
		{
			// Register -> [0b0000|0000|0000|0000]
		case set: // _Set Reg1 Value
			result = L_Reg1_Value(opCode);
			break;
		case inc: // _Inc Reg1
			result = L_Reg1(opCode);
			break;
		case dec: // _Dec Reg1
			result = L_Reg1(opCode);
			break;
		case neg: // _Neg Reg1
			result = L_Reg1(opCode);
			break;
		case cmp: // _CMP Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case mov: // _Mov Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case swi: // _Swi Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case clr: // Clr
			result = L_StandAlone(opCode);
			break;
		case signedFlag:
			result = L_ValueB(opCode);
			break;
			// Memory -> [0b0000|0001|0000|0000]
		case load: // _Load Reg1 $Adress
			result = L_Reg1_Adress(opCode);
			break;
		case store: // _Store Reg1 $Adress
			result = L_Reg1_Adress(opCode);
			break;
		case loadB: // LoadB Reg1 $Adress
			result = L_Reg1_Adress(opCode);
			break;
		case storeB: // StoreB Reg1 $Adress
			result = L_Reg1_Adress(opCode);
			break;
		case rswitch: // Noch nicht in Use [rswitch Value]
			result = L_Reg1(opCode);
			break;
			// Math -> [0b0000|0010|0000|0000]
		case add: // _Add Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case sub: // _Sub Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case mul: // _Mul Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case division: // _Div Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case mod: // _Mod Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case getHigh: // _GetHigh Reg1
			result = L_Reg1(opCode);
			break;
		case getLow: // _GetLow Reg1
			result = L_Reg1(opCode);
			break;
		case rmc:
			result = L_StandAlone(opCode);
			break;
			// Bit-Operation -> [0b0000|0011|0000|0000] (Nochmal schauen ob das richtig funktioniert oder ob beim Enum noch bei Bit Was dazu muss)
		case and : // _And Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case or : // OR Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case xor : // _XOR Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case not: // _Not Reg1
			result = L_Reg1(opCode);
			break;
		case shl: // shl Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
		case shr: // shr Reg1 Reg2
			result = L_Reg1_Reg2(opCode);
			break;
			// Jump -> [0b0000|0100|0000|0000]
		case jmp: // _JMP Label
			result = L_JMP_Label(opCode);
			break;
		case call: // _Call Label
			result = L_JMP_Label(opCode);
			break;
		case je: // _JE Label
			result = L_JMP_Label(opCode);
			break;
		case jne: // _JNE Label
			result = L_JMP_Label(opCode);
			break;
		case jz: // _JZ Label
			result = L_JMP_Label(opCode);
			break;
		case jnz: // _JNZ Label
			result = L_JMP_Label(opCode);
			break;
		case jg: // _JG Label
			result = L_JMP_Label(opCode);
			break;
		case jge: // _JGE Label
			result = L_JMP_Label(opCode);
			break;
		case jl: // _JL Label
			result = L_JMP_Label(opCode);
			break;
		case jle: // _JLE Label
			result = L_JMP_Label(opCode);
			break;
		case jc: // _JC Label
			result = L_JMP_Label(opCode);
			break;
		case jnc: // _JNC Label
			result = L_JMP_Label(opCode);
			break;
		case jNegativ: // _JN Label
			result = L_JMP_Label(opCode);
			break;
		case jnn: // _JNN Label
			result = L_JMP_Label(opCode);
			break;
		case ret:
			result = L_StandAlone(opCode);
			break;
		case hop: // _Hop Flag Offset
			result = L_HopRow(opCode);
			break;
			// Stack -> [0b0000|0101|0000|0000]:
		case push: // _Push Reg1
			result = L_Reg1(opCode);
			break;
		case pop: // _Pop Reg1
			result = L_Reg1(opCode);
			break;
		case peek: // _Peek Reg1
			result = L_Reg1(opCode);
			break;
		case pushB: // _PushB Reg1
			result = L_Reg1(opCode);
			break;
		case popB: // _PopB Reg1
			result = L_Reg1(opCode);
			break;
		case peekB: // _PeekB Reg1
			result = L_Reg1(opCode);
			break;
			// _Syscall -> [0b0000|0110|0000|0000]:
		case syscall: // _Syscall Reg1
			result = L_Value(opCode);
			break;
			// Others -> [0b0000|1111|0000|0000]:
		case nop: // NOP (No Operation)
			result = L_StandAlone(opCode);
			break;
		case halt: // Halt
			result = L_StandAlone(opCode);
			break;
		default:
			printf("ERROR_Assembler");
			break;
		}
	}
	if (result == -1) {
		return -1;
	}
	return result;
}

// _Load in RAM
void L_LReg1_Value(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM(adress + 2, currentRow->adressByte);
	StoreRAM16(adress + 3, currentRow->adress);
}
void L_LValue_Value_Value(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	if (currentRow->flags == 1) {
		StoreRAM16(adress + 2, currentRow->adress);
		StoreRAM16(adress + 4, currentRow->adressOffset);
		StoreRAM(adress + 6, currentRow->adressByte);
	}
	else {
		StoreRAM(adress + 2, currentRow->adressByte);
		StoreRAM(adress + 3, (byte)currentRow->adress);
	}
}
void L_LReg1(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM(adress + 2, currentRow->adressByte);
}
void _LReg1_Reg2(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	if (currentRow->flags & 0b00010000) {
		StoreRAM(adress + 2, currentRow->adressByte);
		StoreRAM16(adress + 3, currentRow->adress);
	}
	else {
		StoreRAM(adress + 2, currentRow->adressByte);
	}
}
void L_LReg1_Label_Reg2(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	if (currentRow->flags & 0b00001000) {
		StoreRAM(adress + 2, currentRow->adressByte);
		StoreRAM(adress + 3, (currentRow->adress >> 8));
	}
	else {
		StoreRAM(adress + 2, currentRow->adressByte);
		StoreRAM16(adress + 3, currentRow->adress);
	}
}
void L_LReg1_Reg2_Reg3_Reg4_Reg5_Reg6(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM(adress + 2, currentRow->adressByte);
	StoreRAM16(adress + 3, currentRow->adress);
	StoreRAM16(adress + 5, currentRow->adressOffset);
}
void L_LStandalone(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
}
void L_LReg1_Adress(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM(adress + 2, currentRow->adressByte);
	if (currentRow->flags & 0b00001000) {
		StoreRAM(adress + 3, (byte)currentRow->adress);
	}
	else {
		StoreRAM16(adress + 3, currentRow->adress);
	}
}
void L_LJMP_Label(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM16(adress + 2, currentRow->adress);
}
void L_LValue(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	if (currentRow->flags == 8) {
		StoreRAM(adress + 2, currentRow->adressByte);
	}
	else {
		StoreRAM16(adress + 2, currentRow->adress);
	}
}
void L_LValueB(const unsigned short adress, const struct assemblyRow* currentRow) {
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM(adress + 2, currentRow->adressByte);
}
void L_LHop(const unsigned short adress, struct assemblyRow* currentRow) {
	struct assemblyRow* nextRow = currentRow;
	byte offSet = (byte)currentRow->adress, counter = 0;
	while (offSet != counter) {
		nextRow = nextRow->next;
		if (nextRow->flags & 2) {
			continue;
		}
		currentRow->adress += nextRow->needBytes;
		++counter;
	}
	currentRow->adress -= offSet;
	StoreRAM16(adress, currentRow->opCode);
	StoreRAM16(adress + 2, currentRow->adress);
}
void L_LoadRowInRAM(const unsigned short adress, const struct assemblyRow* currentRow) {

	OpCodes opCode = currentRow->opCode;
	unsigned short modeOpCode = opCode >> 12;
	if (modeOpCode & 0b00001111) {
		// Hat Kernel OpCodes dabei
		switch (opCode)
		{
			// CPU[0b0001|0000|0000|0000]:
		case setMMU:
			L_LReg1(adress, currentRow);
			break;
		case setStkPtr:
			L_LReg1(adress, currentRow);
			break;
		case setStkAdress:
			_LReg1_Reg2(adress, currentRow);
			break;
		case getSReg:
			_LReg1_Reg2(adress, currentRow);
			break;
		case setPC:
			L_LReg1(adress, currentRow);
			break;
		case setAdressSpace:
			_LReg1_Reg2(adress, currentRow);
			break;
		case setClock:
			L_LValue(adress, currentRow);
			break;
		case setClock_r:
			L_LValue(adress, currentRow);
			break;
		case getClock:
			L_LReg1(adress, currentRow);
			break;
		case sysRet:
			L_LStandalone(adress, currentRow);
			break;
		case getRegs:
			L_LReg1_Value(adress, currentRow);
			break;
		case setRegs:
			L_LReg1_Value(adress, currentRow);
			break;
		case exec:
			L_LReg1(adress, currentRow);
			break;
		case setPx:
			L_LReg1_Reg2_Reg3_Reg4_Reg5_Reg6(adress, currentRow);
			break;
		case startFrame:
			L_LStandalone(adress, currentRow);
			break;
		case addInt:
			L_LReg1_Label_Reg2(adress, currentRow);
			break;
		case addIntReg:
			L_LReg1_Label_Reg2(adress, currentRow);
			break;
		case rmvInt:
			L_LReg1(adress, currentRow);
		case getInt:
			L_LValueB(adress, currentRow);
			break;
		case di:
			L_LStandalone(adress, currentRow);
			break;
		case ei:
			L_LStandalone(adress, currentRow);
			break;
		case retInt:
			L_LStandalone(adress, currentRow);
			break;
		case loadBlock:
			L_LValue_Value_Value(adress, currentRow);
			break;
		case loadBlock_r:
			L_LValue_Value_Value(adress, currentRow);
			break;
		case writeBlock:
			L_LValue_Value_Value(adress, currentRow);
			break;
		case writeBlock_r:
			L_LValue_Value_Value(adress, currentRow);
			break;
		case restart:
			L_LStandalone(adress, currentRow);
			break;
		case terminate:
			L_LStandalone(adress, currentRow);
			break;
		default:
			break;
		}
	}
	else {
		// Hat keine Kernel OpCodes
		switch (opCode)
		{
			// Register -> [0b0000|0000|0000|0000]
		case set: // _Set Reg1 Value
			L_LReg1_Value(adress, currentRow);
			break;
		case inc: // _Inc Reg1
			L_LReg1(adress, currentRow);
			break;
		case dec: // _Dec Reg1
			L_LReg1(adress, currentRow);
			break;
		case neg: // _Neg Reg1
			L_LReg1(adress, currentRow);
			break;
		case cmp: // _CMP Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case cmp_i: // _CMP Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case mov: // _Mov Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case swi: // _Swi Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case clr: // Clr
			L_LStandalone(adress, currentRow);
			break;
		case signedFlag: // signed bool
			L_LReg1(adress, currentRow); // Funktioniert identisch wie als w�re es ein Value da hier der Value eh nur 1 Byte ist
			break;
			// Memory -> [0b0000|0001|0000|0000]
		case load: // _Load Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case load_i: // _Load Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case store: // _Store Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case store_i: // _Store Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case loadB: // LoadB Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case loadB_i: // LoadB Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case storeB: // StoreB Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case storeB_i: // StoreB Reg1 $Adress
			L_LReg1_Adress(adress, currentRow);
			break;
		case rswitch: // Noch nicht in Use [rswitch Value]
			L_LReg1(adress, currentRow);
			break;
			// Math -> [0b0000|0010|0000|0000]
		case add: // _Add Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case add_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case sub: // _Sub Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case sub_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case mul: // _Mul Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case mul_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case division: // _Div Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case division_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case mod: // _Mod Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case mod_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case getHigh: // _GetHigh Reg1
			L_LReg1(adress, currentRow);
			break;
		case getLow: // _GetLow Reg1
			L_LReg1(adress, currentRow);
			break;
		case rmc: // rmc
			L_LStandalone(adress, currentRow);
			// Bit-Operation -> [0b0000|0011|0000|0000] (Nochmal schauen ob das richtig funktioniert oder ob beim Enum noch bei Bit Was dazu muss)
		case and : // _And Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case and_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case or : // OR Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case or_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case xor : // _XOR Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case xor_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case not: // _Not Reg1
			L_LReg1(adress, currentRow);
			break;
		case shl: // shl Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case shl_i:
			_LReg1_Reg2(adress, currentRow);
			break;
		case shr: // shr Reg1 Reg2
			_LReg1_Reg2(adress, currentRow);
			break;
		case shr_i:
			_LReg1_Reg2(adress, currentRow);
			break;
			// Jump -> [0b0000|0100|0000|0000]
		case jmp: // _JMP Label
			L_LJMP_Label(adress, currentRow);
			break;
		case call: // _Call Label
			L_LJMP_Label(adress, currentRow);
			break;
		case je: // _JE Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jne: // _JNE Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jz: // _JZ Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jnz: // _JNZ Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jg: // _JG Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jge: // _JGE Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jl: // _JL Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jle: // _JLE Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jc: // _JC Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jnc: // _JNC Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jNegativ: // _JN Label
			L_LJMP_Label(adress, currentRow);
			break;
		case jnn: // _JNN Label
			L_LJMP_Label(adress, currentRow);
			break;
		case ret:
			L_LStandalone(adress, currentRow);
			break;
		case hop: // _Hop Flag Offset
			L_LHop(adress, currentRow);
			break;
		case hop_i:
			L_LHop(adress, currentRow);
			break;
			// Stack -> [0b0000|0101|0000|0000]:
		case push: // _Push Reg1
			L_LReg1(adress, currentRow);
			break;
		case pop: // _Pop Reg1
			L_LReg1(adress, currentRow);
			break;
		case peek: // _Peek Reg1
			L_LReg1(adress, currentRow);
			break;
		case pushB: // _PushB Reg1
			L_LReg1(adress, currentRow);
			break;
		case popB: // _PopB Reg1
			L_LReg1(adress, currentRow);
			break;
		case peekB: // _PeekB Reg1
			L_LReg1(adress, currentRow);
			break;
			// _Syscall -> [0b0000|0110|0000|0000]:
		case syscall: // _Syscall Reg1
			L_LValue(adress, currentRow);
			break;
			// Others -> [0b0000|1111|0000|0000]:
		case nop: // NOP (No Operation)
			L_LStandalone(adress, currentRow);
			break;
		case halt: // Halt
			L_LStandalone(adress, currentRow);
			break;
		default:
			printf("ERROR_Assembler");
			break;
		}
	}
}