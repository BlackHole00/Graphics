#pragma once

#include <stdio.h>

#define RGFW_IMPLEMENTATION
#define RGFW_METAL
#define RGFW_NATIVE
#include "RGFW.h"

#include "timer.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef struct {
	RGFW_window* window;
	Timer frameTimer;

	int frameCount;
} State;
extern State gState;
