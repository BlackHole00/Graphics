#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "renderer.h"

State gState;

void init(void) {
	gState.window = RGFW_createWindow("Boids", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, RGFW_windowNoResize);
	assert(gState.window);

	gState.triangles = (Position2*)calloc(TRIANGLES_RESERVE, sizeof(Position2));
	for (int i = 0; i < TRIANGLES_RESERVE; i++) {
		gState.triangles[i].x = ((i + 1) * 12) % WINDOW_WIDTH;
		gState.triangles[i].y = ((i + 1) * 12) / WINDOW_WIDTH;
	}
	gState.activeTriangles = 128;

	initRenderer();
}

void fini(void) {
	finiRenderer();
	destroyTimer(&gState.frameTimer);

	RGFW_deinit();
}

void tick(void) {
	// if (RGFW_isKeyPressed(RGFW_space)) {
	// 	gState.shouldAddTriangles = !gState.shouldAddTriangles;
	// }

	// if (gState.shouldAddTriangles && gState.activeTriangles < (TRIANGLES_RESERVE - 1)) {
	// 	gState.activeTriangles += 50;
	// }

	if (gState.currentFrame - 300 > 0 && ((gState.currentFrame - 300) % 5000) == 0) {
		gState.activeTriangles *= 2;
	}

	if (gState.activeTriangles >= 65536) {
		RGFW_window_setShouldClose(gState.window, true);
	}

	gState.currentFrame++;
}

int main(void) {
	init();

	RGFW_window_show(gState.window);
	while (!RGFW_window_shouldClose(gState.window)) {
		tick();
		draw();

		RGFW_pollEvents();
	}

}
