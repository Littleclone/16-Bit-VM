/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/SSD.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/bunnySystem.h"
#include "../header/RAM.h"
#include "../header/CPU.h"
#include "../header/interruptHandler.h"


char* SSD_Path = NULL; // Wird im Kernel ordner ein Ordner dann erstellt mit der SSD drin.
char* LowZone = NULL; // Der Path der zur Low Zone f�hrt (0x0000 -> 0x7FFF)
char* HighZone = NULL; // Der Path der zur High Zone f�hrt (0x8000 -> 0xFFFF)

struct fileNode {
	FILE* scriptFile;
	unsigned long fileSize;
	struct fileNode* next;
};
#if PLATFORM_WINDOWS
const char pathSymbol = '\\';
#elif PLATFORM_LINUX
const char pathSymbol = '/';
#endif


bool P_InitSSD() {
	if (SSD_Path == NULL) {
#if PLATFORM_WINDOWS
		SSD_Path = P_StrAdd(P_KernelPath, "SolidStateDrive\\");
		LowZone = P_StrAdd(SSD_Path, "lowZone\\");
		HighZone = P_StrAdd(SSD_Path, "highZone\\");
#elif PLATFORM_LINUX
		SSD_Path = P_StrAdd(G_KernelPath, "SolidStateDrive/");
		LowZone = P_StrAdd(SSD_Path, "lowZone/");
		HighZone = P_StrAdd(SSD_Path, "highZone/");
#endif
	}
	if (!DirectoryExists(SSD_Path)) {
		if (MakeDirectory(SSD_Path)) {
			log("Ordner konnte nicht erstellt werden.\n")
			return false;
		}
	}
	if (!DirectoryExists(LowZone)) {
		if (MakeDirectory(LowZone)) {
			log("Ordner konnte nicht erstellt werden.\n")
			return false;
		}
	}
	if (!DirectoryExists(HighZone)) {
		if (MakeDirectory(HighZone)) {
			log("Ordner konnte nicht erstellt werden.\n")
			return false;
		}
	}
	else {
		return true;
	}
	// TODO: In zwei teilen die von 2 Threads ausgeführt wird, einer für LowZone und einer für HighZone
	printf("Bitte warten sie, die Virtuelle SSD wird erstellt.[Wird nur einmal Pro OS gemacht]\nDies kann je nach System etwas zeit beanspruchen.\n");
	char* index = NULL, *path = NULL;
	FILE* file = NULL;
	// Alle files werden initialisiert.
	for (int i = 1; i < MaxStorageBlocks; ++i) {
		index = P_ToString(i);
		if (i <= HalfOfTheStorage) {
			path = P_StrAdd(LowZone, index);
		}
		else {
			path = P_StrAdd(HighZone, index);
		}
		char* temp = path;
		path = P_StrAdd(path, ".bin");
		file = fopen(path, "wb");
		free(temp);
		if (file == NULL) {
			perror("Fehler beim Öffnen einer File in SSD Initial\n");
			free(path);
			free(index);
			path = NULL;
			index = NULL;
			return false;
		}
		free(path);
		free(index);
		path = NULL;
		index = NULL;
		// Files werden bis zur StorageSize mit '0' befüllt
		for (unsigned short bytes = 0; bytes < StorageSize; ++bytes) {
			fputc(0, file);
		}
		fclose(file);
	}
	return true;
}

byte P_ResetSSD() {
	free(SSD_Path);
	free(LowZone);
	free(HighZone);
	SSD_Path = NULL;
	LowZone = NULL;
	HighZone = NULL;
	return true;
}

byte P_GetStorageBlock(const unsigned short adress, unsigned short ramAdress, const byte blockID) {
	FILE* file = NULL;
	char* binFile = NULL;
	char* strAdress = P_ToString(adress);
	if (adress <= HalfOfTheStorage) {
		binFile = P_StrAdd(LowZone, strAdress);
	}
	else {
		binFile = P_StrAdd(HighZone, strAdress);
	}
	free(strAdress);
	char* tempBinFile = binFile;
	binFile = P_StrAdd(binFile, ".bin");
	free(tempBinFile);
	file = fopen(binFile, "rb");
	free(binFile);
	if (file == NULL) {
		perror("GetStorageBlock konnte nicht ausgef�hrt werden [1].\n");
		return -1;
	}
	const unsigned short offset = (blockID * (StorageSize / 4));
	fseek(file, offset, SEEK_SET);
	for (unsigned short i = 0; i < (StorageSize / 4); ++i) {
		StoreRAM(ramAdress, fgetc(file));
		++ramAdress;
	}
	fclose(file);
	return 0;
}
byte P_WriteStorageBlock(const unsigned short adress, unsigned short ramAdress, const byte blockID) {
	FILE* file = NULL;
	char* binFile = NULL;
	char* strAdress = P_ToString(adress);
	if (adress <= HalfOfTheStorage) {
		binFile = P_StrAdd(LowZone, strAdress);
	}
	else {
		binFile = P_StrAdd(HighZone, strAdress);
	}
	free(strAdress);
	char* tempBinFile = binFile;
	binFile = P_StrAdd(binFile, ".bin");
	free(tempBinFile);
	file = fopen(binFile, "rb+");
	free(binFile);
	if (file == NULL) {
		perror("WriteStorageBlock konnte nicht ausgef�hrt werden [1].\n");
		return -1;
	}
	const unsigned short offset = (blockID * (StorageSize / 4));
	fseek(file, offset, SEEK_SET);
	for (unsigned short i = 0; i < (StorageSize / 4); ++i) {
		fputc(LoadRAM(ramAdress), file);
		++ramAdress;
	}
	fclose(file);
	return 0;
}

// Dev Funktionen:

sbyte P_CreateKernelBin(const unsigned short kernel_start, const unsigned short endBin) {
#if PLATFORM_WINDOWS
	char* kernel = P_StrAdd(G_KernelPath, "\\main.bin"); // Die kernel file
#elif PLATFORM_LINUX
	char* kernel = P_StrAdd(G_KernelPath, "/main.bin"); // Die kernel file
#endif
	FILE* file = fopen(kernel, "wb");
	if (file == NULL) {
		perror("Fehler beim Erstellen der Datei");
		printf("main.bin konnte nicht erstellt werden.\n");
		return -1;
	}
	free(kernel);
	char* kernelStart = P_ToString(kernel_start);
	fprintf(file, kernelStart);
	fputc('#', file);
	for (int i = 0; i < endBin; ++i) {
		fputc(LoadRAM(i), file);
	}
	fclose(file);
	free(kernelStart);
	return 0;
}

sbyte P_LoadKernelFile() {
#if PLATFORM_WINDOWS
	char* kernel = P_StrAdd(G_KernelPath, "main.bin"); // Die kernel file
#elif PLATFORM_LINUX
	char* kernel = P_StrAdd(G_KernelPath, "main.bin"); // Die kernel file
#endif
	unsigned short adress = 0;
	unsigned short cpuSTART = 0;
	char currentChar = '0';
	FILE* file = fopen(kernel, "rb");
	if (file == NULL) {
		printf("Kernel not found\n");
		return -1;
	}
	free(kernel);
	do
	{
		cpuSTART *= 10;
		cpuSTART += currentChar - '0';
		currentChar = (char)fgetc(file);
	} while (currentChar != '#');
	SetKernelPointer(cpuSTART);
	currentChar = (char)fgetc(file);
	while ((currentChar = (char)fgetc(file)) != ']') {
		unsigned short id = currentChar << 8;
		id |= (char)fgetc(file);
		unsigned short jmp_Adress = (char)fgetc(file) << 8;
		jmp_Adress |= (char)fgetc(file);
		RegisterInterrupt(id, jmp_Adress, 32); // Flag 32 sagt aus das dies ein Hardware Interrupt ist.
	}
	while ((feof(file)) == 0) {
		StoreRAM(adress, fgetc(file));
		++adress;
	}
	fclose(file);
	return 0;
}

struct fileNode* L_CollectFiles(const char* filePath_C);
void L_FreeFileNode(struct fileNode* firstNode);
unsigned long L_CountFileSize(FILE* file);

char* P_LoadAssemblyFile(const char* const C_filePath_C) {
    struct fileNode* mainNode = L_CollectFiles(C_filePath_C);
    if (mainNode == NULL) {
        return NULL;
    }
    unsigned long long scriptSize = 0;
    struct fileNode tempNodes = *mainNode;
    while (tempNodes.scriptFile != NULL) {
        scriptSize += tempNodes.fileSize;
        tempNodes = *tempNodes.next;
    }
    char* script = malloc(scriptSize + 1);
    if (script == NULL) {
        L_FreeFileNode(mainNode);
        return NULL;
    }
	int count = 0;
    tempNodes = *mainNode;
    char* tempScript = script;
    while (tempNodes.scriptFile != NULL) {
        FILE* currentFile = tempNodes.scriptFile;
        while (!feof(currentFile)) {
            const char currentChar = (char) fgetc(currentFile);
#if PLATFORM_LINUX
        	if (currentChar == '\r' || currentChar == '\377') {
        		continue;
        	}
#endif
            *tempScript = currentChar;
            ++tempScript;
        	++count;
        }
        *tempScript = '\n';
        ++tempScript;
        tempNodes = *tempNodes.next;
    }
    script[scriptSize] = '\0';
	// Auf -1 Überprüfen.
    L_FreeFileNode(mainNode);
    return script;
}

struct fileNode* L_CollectFiles(const char* filePath_C) {
	char* mainFilePath = P_StrAdd(filePath_C, "main.txt");
	if (mainFilePath == NULL) {
		log("Kein RAM")
		return NULL;
	}
    FILE* mainFile = fopen(mainFilePath, "r");
	free(mainFilePath);
    if (mainFile == NULL) {
        log("Main Script file konnte nicht geöffnet werden")
        return NULL;
    }
	char* projectPath = (char*)filePath_C;
    const char* includePath_C = P_StrAdd(projectPath, "include.txt");
    if (includePath_C == NULL) {
        log("Operation bei StrAdd ist fehlgeschlagen.")
        fclose(mainFile);
        return NULL;
    }
    FILE* includeFile = fopen(includePath_C, "r");
    free((char*)includePath_C);
    if (includeFile == NULL) {
        log("Include File konnte nicht geöffnet werden.")
        fclose(mainFile);
        return NULL;
    }
    struct fileNode* firstNode = malloc(sizeof(struct fileNode));
    if (firstNode == NULL) {
        fclose(mainFile);
        log("Kein RAM[1].")
        return NULL;
    }
    firstNode->scriptFile = mainFile;
    firstNode->next = NULL;
    firstNode->fileSize = L_CountFileSize(mainFile);


    // Hier beginnt der Part wo die Files aus der Include Collected werden
    struct fileNode* nextNode = firstNode;
    if (projectPath == NULL) {
        fclose(mainFile);
        free(firstNode);
        return NULL;
    }
    while (true) {
        char currentChar = '0';
        short pathLenght = 0;
        const long currentPos = ftell(includeFile);
        // Sammelt die Länge des Paths
        while (currentChar != '\0') {
            currentChar = (char)fgetc(includeFile);
            if (currentChar == ';') {
                break;
            }
#if PLATFORM_WINDOWS
        	if (currentChar == '\n') {
        		continue;
        	}
#elif PLATFORM_LINUX
        	if (currentChar == '\n' || currentChar == '\r') {
        		continue;
        	}
#endif
            if (!P_IsChar(currentChar) && !P_IsDigit(currentChar) && currentChar != pathSymbol && currentChar != '.' && currentChar != '-' && currentChar != '_') {
                log("Error, Path beinhaltet nicht Valides Zeichen.")
                fclose(mainFile);
                fclose(includeFile);
                free(firstNode);
                return NULL;
            }
            ++pathLenght;
        }
        // Vorbereitung für Path Extrahieren
        fseek(includeFile, currentPos, SEEK_SET);
        char* path = malloc(pathLenght + 1);
        if (path == NULL) {
            log("Kein RAM[2].")
            L_FreeFileNode(firstNode);
            fclose(includeFile);
            return NULL;
        }
        // Extrahiert den Path
        currentChar = '0';
        char* tempPointer = path;
        while (currentChar != '\0') {
            currentChar = (char)fgetc(includeFile);
            if (currentChar == ';') {
                break;
            }
#if PLATFORM_WINDOWS
            if (currentChar == '\n') {
                continue;
            }
#elif PLATFORM_LINUX
        	if (currentChar == '\n' || currentChar == '\r') {
        		continue;
        	}
#endif

            *tempPointer = currentChar;
            ++tempPointer;
        }
        *tempPointer = '\0';
        if (P_StrCmp(path, "END")) {
            free(path);
            struct fileNode theEnd;
            theEnd.scriptFile = NULL;
            theEnd.next = NULL;
            nextNode->next = &theEnd;
            break;
        }
        // Erstellt den Path und holt sich die File
        char* tempPointerChar = path;
        path = P_StrAdd(projectPath, path);
        free(tempPointerChar);          // Path wird ja dann mit einem anderem Pointer ausgewechselt.
        if (path == NULL) {
            log("Kein RAM [3].")
            L_FreeFileNode(firstNode);
            fclose(includeFile);
            return NULL;
        }
        FILE* currentFile = fopen(path, "r");
        if (currentFile == NULL) {
            log("File konnte nicht gefunden werden.")
            L_FreeFileNode(firstNode);
            fclose(includeFile);
            return NULL;
        }
        // Erstellt die FileNode und hängt sie an die Linked List
        struct fileNode* newNode = malloc(sizeof(struct fileNode));
        if (newNode == NULL) {
            log("Kein RAM[4].")
            L_FreeFileNode(firstNode);
            fclose(includeFile);
            return NULL;
        }
        newNode->scriptFile = currentFile;
        newNode->next = NULL;
        newNode->fileSize = L_CountFileSize(currentFile);
        nextNode->next = newNode;
        nextNode = newNode;
        free(path);
    }
    fclose(includeFile);
    return firstNode;
}

void L_FreeFileNode(struct fileNode* firstNode) {
    while (firstNode->next != NULL) {
        fclose(firstNode->scriptFile);
        struct fileNode *tempPointer = firstNode;
        firstNode = firstNode->next;
        free(tempPointer);
    }
}

unsigned long L_CountFileSize(FILE* file) {
    unsigned long counter = 0;
    while (!feof(file)) {
#if PLATFORM_WINDOWS
    	fgetc(file);
#elif PLATFORM_LINUX
        const char currentChar = (char)fgetc(file);
    	if (currentChar == '\r') {
    		continue;
    	}
#endif
        ++counter;
    }
	++counter;
    rewind(file);
    return counter;
}