
#include "uv_filters.h"


void uv_moving_aver_init (uv_moving_aver_st *avr, int32_t cnt)
{
	avr->sum = 0;
	avr->count = cnt;
	avr->cur_count = 0;
}

void uv_moving_aver_reset (uv_moving_aver_st *avr)
{
	avr->sum = 0;
	avr->cur_count = 0;
}

int32_t uv_moving_aver_step (uv_moving_aver_st *avr, int32_t val)
{
	avr->sum += (val * 0x100);
	avr->cur_count += 1;
	avr->val = avr->sum / avr->cur_count;

	if (avr->cur_count > avr->count)
	{
		avr->sum -= avr->val;
		avr->cur_count -= 1;
	}

	return (avr->val / 0x100);
}

