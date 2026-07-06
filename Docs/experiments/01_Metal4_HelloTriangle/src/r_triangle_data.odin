package main

R_Vertex :: struct {
	position:	[3]f32,
	_pad_0:		f32,
	color:		[3]f32,
	_pad_1:		f32,
}

@(rodata)
R_TRIANGLE_DATA := [?]R_Vertex{
	{ position = { -0.5, -0.5, 0.0 }, color = { 1.0, 0.0, 0.0 }},
	{ position = {  0.5, -0.5, 0.0 }, color = { 0.0, 1.0, 0.0 }},
	{ position = {  0.0,  0.5, 0.0 }, color = { 0.0, 0.0, 1.0 }},
}
