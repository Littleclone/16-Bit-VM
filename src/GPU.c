/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/GPU.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/VRAM.h"

struct vram* VRAM_Chip = NULL;

byte G_GPU_Status;

double ScaleFactor = 0;
int SpriteSize = 16;
bool initGPU() {
	VRAM_Chip = initVRAM();
	if (VRAM_Chip == NULL) {
		return false;
	}
	G_GPU_Status = 0b00000011;
	return true;
}

byte SetGPUOn() {
	InitWindow(ScreenWidth, ScreenHeight, "BunnySystem");
	SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE | FLAG_WINDOW_ALWAYS_RUN);
	SetExitKey(KEY_NULL);
	SetTargetFPS(60);
	G_GPU_Status = 0b00000011;
	ScaleFactor = (double)GetMonitorWidth(GetCurrentMonitor()) / ScreenWidth;
	return true;
}

byte SetGPUOff() {
	G_GPU_Status &= 0;
	CloseWindow();
#if IsDebug
	printf("Thread für GPU Geschlossen\n");
#endif
	return true;
}

void FreeGPU() {
	free(VRAM_Chip);
}

void RenderFrame();

byte ExecuteGPU() { // Thread
	G_GPU_Status ^= 0b00000110;
#if IsDebug
	printf("GPU ist gestartet\n");
#endif
	while (G_GPU_Status & 0b00000001) {
		if (G_GPU_Status & 0b00000100) { // Nur wenn die GPU Running ist Executet sie, dies wird von der CPU gesetzt (Die GPU setzt nach jedem Frame auf Idle)
			RenderFrame();
		}
	}
	return 0;
}

void RenderFrame() {
	BeginDrawing();
	ClearBackground(RAYWHITE);

	// Da jeder "Pixel" eigentlich ein SpriteSize x SpriteSize Rechteck ist
	for (int y = 0; y < ScreenHeight; y++) {
		for (int x = 0; x < ScreenWidth; x++) {
			//ImageDrawPixel()
			DrawPixel(x, y, BLACK);
		}
	}

	EndDrawing();

	// Status-Flag zurücksetzen
	G_GPU_Status &= ~0b00000100;
}
