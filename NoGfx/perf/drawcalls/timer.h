#pragma once

#include "stdio.h"
#include "stdint.h"

typedef struct {
	FILE*		outputFile;
	const char*	name;
	uint64_t	measureCount;
	uint64_t	startTime;
	bool		printToStdout;
} Timer;

void createTimer(Timer* timer, const char* name, const char* fileName, bool printToStdout);
void destroyTimer(Timer* timer);
void startTimer(Timer* timer);
void stopTimer(Timer* timer);
void stopTimerF(Timer* timer, const char* fmt, ...);

