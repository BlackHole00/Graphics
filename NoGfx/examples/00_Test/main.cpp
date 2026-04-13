#include <stdio.h>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include <random>

#include <lib/common/mutex.h>

static CmnMutex mutex = {};
static uint64_t a = 0;

static constexpr size_t THREADS = 256;
static constexpr size_t ITERATIONS = 2 * 1024 * 1024;

// per-thread randomness to break scheduling patterns
void incrementer(uint64_t seed) {
	std::mt19937_64 rng(seed);
	std::uniform_int_distribution<int> dist(0, 7);

	for (size_t i = 0; i < ITERATIONS; i++) {

		// randomize contention timing
		for (int j = 0; j < dist(rng); j++) {
			std::this_thread::yield();
		}

		cmnMutexLock(&mutex);

		uint64_t tmp = a;
		tmp++;
		a = tmp;

		// occasionally validate invariant inside critical section
		if ((tmp % (512 * 1024)) == 0) {
			printf("progress: %llu\n", tmp);
		}

		cmnMutexUnlock(&mutex);
	}
}

int main() {
	std::vector<std::thread> threads;
	threads.reserve(THREADS);

	for (size_t i = 0; i < THREADS; i++) {
		threads.emplace_back(incrementer, i * 1337);
	}

	for (auto &t : threads) {
		t.join();
	}

	uint64_t expected = THREADS * ITERATIONS;

	printf("final: %llu expected: %llu\n", a, expected);

	assert(a == expected);

	return 0;
}
