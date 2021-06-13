#include "time_manager.h"
#include <stdio.h>
#include <Windows.h>

volatile uint32_t clock_tick;
HANDLE prcs_blocks[10] = {};
HANDLE thr;

DWORD WINAPI thread_run(LPVOID mut);

int main() {
	uint8_t timer = 10;
	D_Reset();


	for (uint8_t i = 0; i < 10; i++) {
		prcs_blocks[i] = CreateSemaphore(
			NULL,	// default security attributes
			0,		// initial count
			1,		// maximum count
			NULL	// unnamed semaphore
		);
	}

	thr = CreateThread(	// (void *)
		NULL,				// default security attributes
		0,					// (unsigned long)	StackSize
		thread_run,			// Thread function
		prcs_blocks[0],		// (void *)			Parameter
		0,					// (unsigned long)	Creation Flags
		NULL				// (unsigned long*) ThreadId
	);

	while (timer--) {
		Sleep(1000); //std::this_thread::sleep_for(std::chrono::seconds(1));
		printf("Tick #%d\n", ++clock_tick);
		//Dispatcher();
		ReleaseSemaphore(prcs_blocks[0], 1, NULL);
	}
	printf("Time is up\n");
	getchar();
	CloseHandle(thr);
	for (uint8_t i = 0; i < 10; i++) {
		CloseHandle(prcs_blocks[i]);
	}

	return 0;
}

DWORD WINAPI thread_run(LPVOID mut) {
	uint8_t c = 3;
	DWORD dwWaitResult;
	while (c--) {
		printf("wait\n");
		dwWaitResult = WaitForSingleObject(mut, INFINITE);
		printf("done\n");
	}
	return 0;
}

