package main

R_Vertex :: struct {
	position:	[3]f32,
	uv:		[2]f32,
}

@(rodata)
R_TRIANGLE_VERTICES := [?]R_Vertex{
	{ position = { -0.5, -0.5, 0.0 }, uv = { 0.0, 1.0 }},
	{ position = {  0.5, -0.5, 0.0 }, uv = { 1.0, 1.0 }},
	{ position = {  0.0,  0.5, 0.0 }, uv = { 0.5, 0.0 }},
}

@(rodata)
R_QUAD_VERTICES := [?]R_Vertex{
	{ position = { -0.5, -0.5, 0.0 }, uv = { 0.0, 1.0 }},
	{ position = {  0.5, -0.5, 0.0 }, uv = { 1.0, 1.0 }},
	{ position = {  0.5,  0.5, 0.0 }, uv = { 1.0, 0.0 }},
	{ position = { -0.5,  0.5, 0.0 }, uv = { 0.0, 0.0 }},
}

@(rodata)
R_QUAD_INDICES := [?]u32 {
	0, 1, 2,
	2, 3, 0,
}

@(rodata)
R_DEFAULT_TEXTURE := [?][4]u8{
	{ 255, 0, 255, 255 }, {   0, 0,   0, 255 },
	{   0, 0,   0, 255 }, { 255, 0, 255, 255 },
}




