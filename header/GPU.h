/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef gpu_h
#define gpu_h
#include "essentials.h"

#define ScreenWidth 640
#define ScreenHeight 360
#define Sprite_Width 16
#define Sprite_Height 16

// 1: Aktiv/Inaktiv, 2: Idle/Waiting, 3: Running, 8: Critical Error;
extern byte G_GPU_Status;


bool initGPU();
byte SetGPUOn();
byte SetGPUOff();
void FreeGPU();
sbyte PullEvents();
byte ExecuteGPU();

// Test:
void RenderFrame();

#endif
