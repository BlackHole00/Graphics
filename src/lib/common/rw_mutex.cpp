#include "rw_mutex.h"

#include <assert.h>

void cmnRWMutexLockRead(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	mutex->waitingReaders++;
	while (mutex->writerActive || (mutex->preferWriter && mutex->waitingWriters > 0)) {
		cmnConditionWait(&mutex->readers, &mutex->mutex);
	}
	mutex->waitingReaders--;
	mutex->activeReaders++;

	cmnMutexUnlock(&mutex->mutex);
}

bool cmnRWMutexTryLockRead(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	bool didLock = false;
	if (!mutex->writerActive && !(mutex->preferWriter && mutex->waitingWriters > 0)) {
		mutex->activeReaders++;
		didLock = true;
	}

	cmnMutexUnlock(&mutex->mutex);
	return didLock;
}

void cmnRWMutexUnlockRead(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	assert(mutex->activeReaders > 0 && "cmnRWMutexUnlockRead called without a matching read lock.");
	mutex->activeReaders--;
	if (mutex->activeReaders == 0) {
		if (mutex->waitingWriters > 0) {
			mutex->preferWriter = true;
			cmnConditionSignal(&mutex->writers);
		} else if (mutex->waitingReaders > 0) {
			cmnConditionBroadcast(&mutex->readers);
		}
	}

	cmnMutexUnlock(&mutex->mutex);
}

void cmnRWMutexLockWrite(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	mutex->waitingWriters++;
	mutex->preferWriter = true;
	while (mutex->writerActive || mutex->activeReaders > 0) {
		cmnConditionWait(&mutex->writers, &mutex->mutex);
	}
	mutex->waitingWriters--;
	mutex->writerActive = true;

	cmnMutexUnlock(&mutex->mutex);
}

bool cmnRWMutexTryLockWrite(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	bool didLock = false;
	if (!mutex->writerActive && mutex->activeReaders == 0) {
		mutex->writerActive = true;
		didLock = true;
	}

	cmnMutexUnlock(&mutex->mutex);
	return didLock;
}

void cmnRWMutexUnlockWrite(CmnRWMutex* mutex) {
	cmnMutexLock(&mutex->mutex);

	assert(mutex->writerActive && "cmnRWMutexUnlockWrite called without a matching write lock.");
	mutex->writerActive = false;

	if (mutex->waitingReaders > 0 && mutex->waitingWriters > 0) {
		if (mutex->preferWriter) {
			mutex->preferWriter = false;
			cmnConditionBroadcast(&mutex->readers);
		} else {
			mutex->preferWriter = true;
			cmnConditionSignal(&mutex->writers);
		}
	} else if (mutex->waitingWriters > 0) {
		mutex->preferWriter = true;
		cmnConditionSignal(&mutex->writers);
	} else if (mutex->waitingReaders > 0) {
		mutex->preferWriter = false;
		cmnConditionBroadcast(&mutex->readers);
	}

	cmnMutexUnlock(&mutex->mutex);
}
