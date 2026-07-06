#include "timer.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <mach/mach_time.h>

uint64_t nsDurationBetween(uint64_t start, uint64_t stop) {
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);

	uint64_t elapsed = (stop - start) * info.numer / info.denom;
	return elapsed;
}

void createTimer(Timer* timer, const char* name, const char* fileName, bool printToStdout) {
	timer->outputFile = fopen(fileName, "w");
	assert(timer->outputFile != NULL && "Could not create a timer");

	timer->name = name;
	timer->printToStdout = printToStdout;

	timer->measureCount = 0;
	timer->startTime = 0;
}

void destroyTimer(Timer* timer) {
	fclose(timer->outputFile);
}

void startTimer(Timer* timer) {
	assert(timer->startTime == 0 && "Timer already started");

	timer->startTime =  mach_absolute_time();
}

void stopTimer(Timer* timer) {
	assert(timer->startTime != 0 && "Timer not started");

	uint64_t elapsed = nsDurationBetween(timer->startTime, mach_absolute_time());
	fprintf(timer->outputFile, "%lld,%lld\n", timer->measureCount, elapsed);

	if (timer->printToStdout) {
		printf("%s: %lld - %lld ns\n", timer->name, timer->measureCount, elapsed);
	}

	timer->measureCount++;
	timer->startTime = 0;
}

void stopTimerF(Timer* timer, const char* fmt, ...) {
	assert(timer->startTime != 0 && "Timer not started");

	va_list args;
	va_start(args, fmt);

	uint64_t elapsed = nsDurationBetween(timer->startTime, mach_absolute_time());
	fprintf(timer->outputFile, "%lld,%lld", timer->measureCount, elapsed);
	vfprintf(timer->outputFile, fmt, args);
	fprintf(timer->outputFile, "\n");

	if (timer->printToStdout) {
		printf("%s: %lld - %lld ns\n", timer->name, timer->measureCount, elapsed);
		vprintf(fmt, args);
		printf("\n");
	}

	timer->measureCount++;
	timer->startTime = 0;
	
	va_end(args);
}

