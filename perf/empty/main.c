#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "renderer.h"

State gState;

void init(void) {
	gState.window = RGFW_createWindow("Boids", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, RGFW_windowNoResize);
	assert(gState.window);

	createTimer(&gState.frameTimer, "Frame timer", "./timings/frame.csv", true);

	initRenderer();
}

void fini(void) {
	finiRenderer();
	destroyTimer(&gState.frameTimer);

	RGFW_deinit();
}

void tick(void) {
	gState.frameCount++;

	if (gState.frameCount == 5300) {
		RGFW_window_setShouldClose(gState.window, true);
	}
}

int main(void) {
	init();

	RGFW_window_show(gState.window);
	while (!RGFW_window_shouldClose(gState.window)) {
		startTimer(&gState.frameTimer);
			tick();
			draw();
		stopTimer(&gState.frameTimer);

		RGFW_pollEvents();
	}

}
