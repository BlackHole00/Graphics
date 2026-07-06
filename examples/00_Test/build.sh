#!/usr/bin/env sh

c++ main.cpp -I../../include -I../../src -L../../build -lgpu -framework Metal -framework Foundation -g -o out

