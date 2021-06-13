#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

/****************************** Included files ********************************/
#include <stdint.h>
/******************************** Definition **********************************/
enum MODE {
	REALTIME = 0,
	ONETIME,
	SPECPERIOD,
	RUNTIME,
	WAIT,
	IDLE,
	MAX_MODE
};

enum DESCRIPTOR {
	THR_1,
	MAX_DSCR // MAX_DSCR > 0
};
/****************************** Prototypes API ********************************/
void Dispatcher(void);
void D_Reset(void);
void D_ProcessAdd(uint8_t, void(*) (void*), uint8_t*, uint8_t, uint16_t);
void D_ProcessKill(uint8_t);
void D_ProcessWait(uint8_t, uint16_t);
void D_ProcessSetMode(uint8_t, uint8_t, uint16_t);
void D_ProcessChange(uint8_t, void(*) (void*), uint8_t, uint16_t);
void D_SetCurTime(uint16_t);
uint8_t D_GetProcessID(uint8_t);
uint16_t D_GetCurTime(void);

/******************************************************************************/
#endif // !TIME_MANAGER_H

