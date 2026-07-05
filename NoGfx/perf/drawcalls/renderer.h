#pragma once

#include <stdint.h>

typedef float Vertex[2];

// struct {
// 	packed_float2 position;
// } Vertex;

static const Vertex TRIANGLE_VERTICES[] = {
	{ -5.0, -5.0 },
	{  5.0, -5.0 },
	{  0.0,  5.0 }
};

static const uint32_t TRIANGLE_INDICES[] = {
	0, 1, 2
};


void initRenderer(void);
void draw(void);
void finiRenderer(void);


