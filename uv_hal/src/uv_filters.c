
#include "uv_filters.h"


void uv_moving_aver_init (uv_moving_aver_st *avr, int cnt)
{
	avr->Sum = 0;
	avr->Count = cnt;
	avr->CurCount = 0;
}

void uv_moving_aver_reset (uv_moving_aver_st *avr)
{
	avr->Sum = 0;
	avr->CurCount = 0;
}

int uv_moving_aver_step (uv_moving_aver_st *avr, int val)
{
	avr->Sum += val;
	avr->CurCount += 1;
	val = avr->Sum / avr->CurCount;

	if (avr->CurCount > avr->Count)
	{
		avr->Sum -= val;
		avr->CurCount -= 1;
	}

	return val;
}

