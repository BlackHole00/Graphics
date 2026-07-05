#pragma once

#include <stdio.h>

#define RGFW_IMPLEMENTATION
#define RGFW_METAL
#define RGFW_NATIVE
#include "RGFW.h"

#include "timer.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define TRIANGLES_RESERVE 1024 * 1024

typedef struct {
	float x;
	float y;
} Position2;

typedef struct {
	RGFW_window*	window;
	Timer		frameTimer;

	int		currentFrame;

	Position2*	triangles;
	int		activeTriangles;
	bool		shouldAddTriangles;
} State;
extern State gState;

