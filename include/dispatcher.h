#ifndef DISPATCHER_H
#define DISPATCHER_H

/****************************** Included files ********************************/
#include "time_manager.h"
/******************************** Definition **********************************/
#define BUF_SIZE 16
#define MAX_ID (BUF_SIZE/16) + 1

typedef struct {
	void(*prcs) (void*);
	void *tls;
	uint8_t id;
	uint8_t descriptor;
	uint8_t prcs_mode;
	uint8_t prev_prcs_mode;
	uint16_t cur_time;
	uint16_t run_time;
	uint16_t prev_run_time;
} process;

typedef void(*tPM) (uint8_t);
/******************************************************************************/

#endif // !DISPATCHER_H

