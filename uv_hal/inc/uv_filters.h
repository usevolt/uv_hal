
#ifndef UW_FILTERS_H_
#define UW_FILTERS_H_


#include "uv_hal_config.h"

#include "uv_types.h"



// ---------------------------       moving average filter       ---------------------------

typedef struct
{
	int Sum;
	int Count;
	int CurCount;
} uv_moving_aver_st;

void uv_moving_aver_init (uv_moving_aver_st *avr, int cnt);
void uv_moving_aver_reset (uv_moving_aver_st *avr);
int  uv_moving_aver_step (uv_moving_aver_st *avr, int val);



#endif /* UW_FILTERS_H_ */
