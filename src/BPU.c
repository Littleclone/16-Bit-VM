/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/BPU.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../header/assembler.h"
#include "../header/SSD.h"
#include "../header/CPU.h"
#include "../header/essentials.h"
#include "../header/bunnySystem.h"
#include "../header/RAM.h"
#include "../header/VRAM.h"
#include "../header/interruptHandler.h"
#include "../header/GPU.h"
#include "../header/CHR_ROM.h"


byte L_PrintBootOptions(FILE* configFile, char** path_Array);

sbyte P_InitBPU() { // BPU = Boot Processing Unit
	printf("BPU is booting...\n");
	if (!P_InitSystem()) {
		printf("Error: System Konnte nicht Initalisiert werden\n");
		return -1;
	}
#if PLATFORM_WINDOWS
	const char* localAppData = getenv("LOCALAPPDATA");
#elif PLATFORM_LINUX
	const char* localAppData = getenv("HOME");
#endif

	if (localAppData == NULL) {
		perror("Fehler: LOCALAPPDATA/Home konnte nicht abgerufen werden");
		return -1;
	}
	char* systemPath = P_StrCpy(localAppData);
#if PLATFORM_WINDOWS
	systemPath = P_StrAdd(systemPath, "\\BunnySystem");
#elif PLATFORM_LINUX
	systemPath = P_StrAdd(systemPath, "/BunnySystem");
#endif

	if (!DirectoryExists(systemPath)) {
		if (MakeDirectory(systemPath)) {
			log("System Ordner konnte nicht erstellt werden.")
		}
	}
#if PLATFORM_WINDOWS
	char* configPath = P_StrAdd(systemPath, "\\config.bin");
#elif PLATFORM_LINUX
	char* configPath = P_StrAdd(systemPath, "/config.bin");
#endif
	FILE* configFile = fopen(configPath, "rb");
	if (configFile == NULL) {
		configFile = fopen(configPath, "wb");
		if (DirectoryExists("BunnyOS")) {
			if (MakeDirectory("BunnyOS")) {	// Returns 0 bei Sucess, means 1 by fail
				log("OS Ordner konnte nicht erstellt werden.")
			}
		}
#if PLATFORM_WINDOWS
		fprintf(configFile, "BunnyOS\\\n");
		fprintf(configFile, "BunnyOS\n");
#elif PLATFORM_LINUX
		fprintf(configFile, "BunnyOS/\n");
		fprintf(configFile, "BunnyOS\n");
#endif
		fclose(configFile);
		configFile = fopen(configPath, "rb");
	}
	free(configPath);
	sbyte systemStatus = 0;
	char** path_Array = malloc(20 * sizeof(byte*));
	if (path_Array == NULL) {
		return -1;
	}
	for (byte i = 0; i < 20; ++i) {
		path_Array[i] = NULL;
	}

	// --------------------------------------------------------------
	// Alles �ber hier ist nur f�rs Starten und f�r die Config File.
	// --------------------------------------------------------------

	printf("BPU ist gebootet\n\n");
	while (true) {
		const byte max_Options = L_PrintBootOptions(configFile, path_Array);
#if IsDebug
		printf("- Boot_Config [21]\n");
#endif
		printf("- Exit [0]\n");
		int userChoice = 0;
		scanf("%i", &userChoice); // TODO: Fget
		if (userChoice >= 1 && userChoice <= max_Options) {
			// Kernel
			--userChoice;
			do {
				if (userChoice == 0) {
					G_KernelPath = path_Array[userChoice];
				}
				else {
					G_KernelPath = P_StrAdd(systemPath, path_Array[userChoice]);
				}
				if (!P_InitSSD()) {
					printf("Error: SSD could not be initialised.\n");
					free(path_Array);
					return -1;
				}
				//if (!initCHR_ROM()) {
				//	printf("Error: CHR_ROM could not be initialised.\n");
				//	return -1;
				//}
				const sbyte result = P_LoadKernelFile(G_KernelPath);
				if (result == -1) {
					printf("Error: Kernel couldn't load.\n");
					free(path_Array);
					return -1;
				}
				if (!SetGPUOn()) {
					printf("Error: GPU could not be initialised.\n");
					free(path_Array);
					return -1;
				}
				const clock_t start = clock();
				systemStatus = StartExecuteCPU();
				const clock_t end = clock();
				const double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
				printf("Vergangene Zeit: %.6f Sekunden\n", elapsed);
				printf("Das sind %lf MHz\n", (P_OpCodeCounter / elapsed) / 1e6);
				printf("OpCodes: %i\n", P_OpCodeCounter);
				RestartCPU();
				ClearRAM();
				ClearVRAM();
				ResetIPU();
				P_ResetSSD();
				FreeCHR_ROM();
				SetGPUOff();
				for (byte i = 0; i < max_Options; ++i) {
					free(path_Array[i]);
					path_Array[i] = NULL;
				}
			} while (systemStatus == -1);
			printf("Kernel Closed!\n\n");
		}
		#if IsDebug
		else if (userChoice == 21) {
			while (true) {
				printf("Boot Config:\n- Add New OS [1]\n- Updated OS [2]\n- Exit [0]\n");
				printf("- Assembly BunnyOS [9]\n");
				userChoice = 19;
				scanf("%i", &userChoice);
				if (userChoice == 9) {
					#if PLATFORM_WINDOWS
					char* testInput = P_LoadAssemblyFile("C:\\Users\\Hannah\\Desktop\\Test_Assembly\\"); // F�r Testzwecke
					#elif PLATFORM_LINUX
					char* testInput = P_LoadAssemblyFile("/home/hannah/Desktop/Test_Assembly/");
					#endif
					if (testInput == NULL) {
						printf("Test Error_1");
						free(path_Array);
						return -1;
					}
					printf("Start Assembler\n");
					G_KernelPath = "BunnyOS";
					// Hier geben dir die File die wir bei GetAssemblyFile bekommen haben zum Assembler
					unsigned short result = P_ExecuteAssembler(testInput, true, 0, true);
					if (result == false) {
						printf("Assembler has found errors.\n");
						free(path_Array);
						return -1;
					}
					printf("Assembler Finished\n");
				}
				// Vorerst die gleiche abfrage f�r die Testphase wenn
                if (userChoice == 1 || userChoice == 2) {
					printf("Nothing here\n");
				} else if (userChoice == 0) {
					break;
				}
			}
		}
		#endif
		else if (userChoice == 0) {
			// Exit
			fclose(configFile);
			free(path_Array);
			return 0;
		}
		else {
			printf("Sorry, dies ist keine Valide Option!\n");
			for (byte i = 0; i < max_Options; ++i) {
				free(path_Array[i]);
				path_Array[i] = NULL;
			}
		}
	}
}
// Es muss sichergerstellt sein das es niemals mehr als 20 Eintr�ge gibt TODO: While Schleife hat Abfrage, testen
byte L_PrintBootOptions(FILE* configFile, char** path_Array) {
    byte indexCounter = 0;
    char currentChar = ' ';
    int charCounter = 0;
	printf("Welches Betriebssystem willst du Booten?\n");
    while ((feof(configFile)) == 0 && indexCounter != 20) {
        long filePosition = ftell(configFile);
		// Kernel Path
        while ((currentChar = (char)fgetc(configFile)) != '\n' && currentChar != EOF) {
            ++charCounter;
        }
        fseek(configFile, filePosition, SEEK_SET);
        char* path = malloc(charCounter + 1);
        if (path == NULL) {
            printf("Error: Path for Kernel string is not allocated\n");
            return 0;
        }
        charCounter = 0;
        while ((currentChar = (char)fgetc(configFile)) != '\n' && currentChar != EOF) {
			*(path + charCounter) = currentChar;
            ++charCounter;
        }
		// OS Name
		filePosition = ftell(configFile);
		int charCounter2 = 0;
		while ((currentChar = (char)fgetc(configFile)) != '\n' && currentChar != EOF) {
			++charCounter2;
		}
		fseek(configFile, filePosition, SEEK_SET);
		char* kernel_name = malloc(charCounter2 + 1);
		if (kernel_name == NULL) {
			free(path);
			printf("Error: Name for Kernel string is not allocated\n");
			return 0;
		}
		charCounter2 = 0;
		while ((currentChar = (char)fgetc(configFile)) != '\n' && currentChar != EOF) {
			*(kernel_name + charCounter2) = currentChar;
			++charCounter2;
		}
        *(path + charCounter) = '\0'; // Null-terminate the string
        *(kernel_name + charCounter2) = '\0'; // Null-terminate the string
        printf("- %s [%i]\n", kernel_name, ++indexCounter);
		free(kernel_name);
		path_Array[(indexCounter - 1)] = path;
		currentChar = (char)fgetc(configFile); // Fix das es EOF nicht erkennt?
    }
	rewind(configFile);
	return indexCounter;
}