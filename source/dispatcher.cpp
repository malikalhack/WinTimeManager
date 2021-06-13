#include "dispatcher.h" 
/**************************** Private prototypes ******************************/
void prcsREALTIME(uint8_t);
void prcsONETIME(uint8_t);
void prcsSPECPERIOD(uint8_t);
void prcsRUNTIME(uint8_t);
void prcsWAIT(uint8_t);
void prcsIDLE(uint8_t);

uint16_t GetTime(uint8_t);
uint8_t D_GetID(void);
/**************************** Private  variables ******************************/
int systime;
int* const p_systime = &systime;
#define GetCurTime *p_systime
static volatile uint8_t bCount;
static process prcs_queue[BUF_SIZE];
static uint8_t prcs_id[BUF_SIZE + 1];
static uint16_t idgen[MAX_ID + 1];
static tPM prcsMode[] = {
	&prcsREALTIME,
	&prcsONETIME,
	&prcsSPECPERIOD,
	&prcsRUNTIME,
	&prcsWAIT,
	&prcsIDLE
};
/******************************** Entry point *********************************/
void Dispatcher() {
	if (!bCount) {
		/* Process cycle */
		static uint8_t i;
		for (i = 0; i < bCount; i++) (*prcsMode[prcs_queue[i].prcs_mode])(i);
	}
}
/************************************ API ************************************/
void D_Reset() {
	bCount = 0;
	for (uint8_t tCount = 0; tCount < BUF_SIZE; tCount++) {
		prcs_id[tCount] = 0xFF;
		if (tCount<MAX_ID) idgen[tCount] = 0xFFFF;
	}
}
/******************************************************************************/
void D_ProcessAdd(uint8_t dscr, void(*pnt) (void*),
	 uint8_t* tls,  uint8_t mode, uint16_t time) {
	if (bCount >= BUF_SIZE - 1) return;
	uint8_t tempRegTime = D_GetID();
	prcs_id[tempRegTime] = bCount;
	prcs_queue[bCount].prcs = pnt;
	prcs_queue[bCount].tls = tls;
	prcs_queue[bCount].id = tempRegTime;
	prcs_queue[bCount].descriptor = dscr;
	prcs_queue[bCount].prcs_mode = mode;
	prcs_queue[bCount].cur_time = GetCurTime;
	prcs_queue[bCount++].run_time = time;
}
/******************************************************************************/
uint8_t D_GetProcessID(uint8_t dscr) {
	uint8_t result = 0;
	if ((dscr < MAX_DSCR) && (bCount)) {
		uint8_t i;
		for (i = 0; i < bCount; i++) {
			if (dscr == prcs_queue[i].descriptor) {
				result = prcs_queue[i].id;
				break;
			}
		}
	}
	return result;
}
/******************************************************************************/
void D_ProcessKill( uint8_t id) { //todo
	uint8_t N = prcs_id[id];
	if ((bCount) && (id) && (N < 0xFF)) {
		uint8_t temp = prcs_queue[N].id;
		if (id == temp) {
			if (--bCount) {
				prcs_queue[N] = prcs_queue[bCount];
				prcs_id[prcs_queue[bCount].id] = N;
			}
			prcs_id[id--] = 0xFF;
			temp = id % 16;
			temp = 1 << temp;
			idgen[id >> 4] |= temp;
		}
	}
}
/******************************************************************************/
void D_ProcessWait(uint8_t id, uint16_t time) {
	uint8_t svd_id = prcs_id[id];
	if (svd_id < 0xFF) {
		if (id == prcs_queue[svd_id].id) {
			prcs_queue[svd_id].prev_prcs_mode = prcs_queue[svd_id].prcs_mode;
			prcs_queue[svd_id].prcs_mode = WAIT;
			prcs_queue[svd_id].cur_time = GetCurTime;
			prcs_queue[svd_id].prev_run_time = prcs_queue[svd_id].run_time;
			prcs_queue[svd_id].run_time = time;
		}
	}
}
/******************************************************************************/
void D_ProcessSetMode(uint8_t id, uint8_t prcs_mode, uint16_t time) {
	uint8_t svd_id = prcs_id[id];
	if (svd_id < 0xFF) {
		if (id == prcs_queue[svd_id].id) {
			prcs_queue[svd_id].prcs_mode = prcs_mode;
			prcs_queue[svd_id].cur_time = GetCurTime;
			prcs_queue[svd_id].run_time = time;
		}
	}
}
/******************************************************************************/
void D_ProcessChange(
	uint8_t id,
	void(*func) (void*),
	uint8_t prcs_mode,
	uint16_t time
) {
	uint8_t svd_id = prcs_id[id];
	if (svd_id < 0xFF) {
		if (id == prcs_queue[svd_id].id) {
			prcs_queue[svd_id].prcs = func;
			prcs_queue[svd_id].prcs_mode = prcs_mode;
			prcs_queue[svd_id].cur_time = GetCurTime;
			prcs_queue[svd_id].run_time = time;
		}
	}
}
/******************************************************************************/
void prcsIDLE(uint8_t N) { return; }
void prcsREALTIME(uint8_t N) { (*prcs_queue[N].prcs)(prcs_queue[N].tls); }
void prcsONETIME(uint8_t N) {
	uint16_t ResTime = GetTime(N);
	if (ResTime >= prcs_queue[N].run_time) {
		(*prcs_queue[N].prcs)(prcs_queue[N].tls);
		D_ProcessKill(prcs_queue[N].id);
	}
}
/******************************************************************************/
void prcsSPECPERIOD(uint8_t N) {
	uint16_t ResTime = GetTime(N);
	if (ResTime >= prcs_queue[N].run_time) {
		(*prcs_queue[N].prcs)(prcs_queue[N].tls);
		prcs_queue[N].cur_time = GetCurTime;
	}
}
/******************************************************************************/
void prcsWAIT(uint8_t N) {
	uint16_t ResTime = GetTime(N);
	if (ResTime >= prcs_queue[N].run_time) {
		prcs_queue[N].prcs_mode = prcs_queue[N].prev_prcs_mode;
		prcs_queue[N].run_time = prcs_queue[N].prev_run_time;
		(*prcs_queue[N].prcs)(prcs_queue[N].tls);
		prcs_queue[N].cur_time = GetCurTime;
	}
}
/******************************************************************************/
void prcsRUNTIME(uint8_t N) {
	uint16_t ResTime = GetTime(N);
	if (ResTime >= prcs_queue[N].run_time) D_ProcessKill(prcs_queue[N].id);
	else (*prcs_queue[N].prcs)(prcs_queue[N].tls);
}
/******************************************************************************/
uint16_t GetTime(uint8_t N) {
	uint16_t fin_Val = GetCurTime;
	uint16_t strt_Val = prcs_queue[N].cur_time;
	if (fin_Val > strt_Val) {
		fin_Val -= strt_Val;
	}
	else {
		strt_Val = 0xFFFF - strt_Val;
		fin_Val += strt_Val;
		fin_Val++;
	}
	return fin_Val;
}
/******************************************************************************/
uint8_t D_GetID(void) {
	uint8_t i, j;
	uint16_t mask;
	uint16_t temp;
	for (i = 0; i < MAX_ID; i++) {
		mask = 1;
		temp = idgen[i];
		if (temp) {
			for (j = 0; j < 16; j++) {
				if (temp&mask) {
					temp &= ~mask;
					j++;
					idgen[i] = temp;
					temp = (i << 4) + j;
					return (uint8_t)temp;
				}
				else mask <<= 1;
			}
		}
	}
	return 0;
}
/******************************************************************************/
