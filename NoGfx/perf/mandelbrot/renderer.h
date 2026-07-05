#pragma once

#include <stdint.h>

typedef float Vertex[2];

static const Vertex TRIANGLE_VERTICES[] = {
	{ -1.0, -1.0 },
	{  3.0, -1.0 },
	{ -1.0,  3.0 }
};

static const uint32_t TRIANGLE_INDICES[] = {
	0, 1, 2
};


void initRenderer(void);
void draw(void);
void finiRenderer(void);


