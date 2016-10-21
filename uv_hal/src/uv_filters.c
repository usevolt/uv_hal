
#include "uv_filters.h"


// ---------------------------       hysteresis filter       ---------------------------

void Hysteresis_Init (T_Hysteresis *hys, int dif)
{
	hys->Last = 0;
	hys->Diff = dif;
}

int Hysteresis_Step (T_Hysteresis *hys, int val)
{
	if (val < hys->Last - hys->Diff || val > hys->Last + hys->Diff)
		hys->Last = val;
	return hys->Last;
}


// ---------------------------       periodic average filter       ---------------------------

void Average_Init (T_Average *avr, int cnt)
{
	avr->Sum = 0;
	avr->Count = cnt;
	avr->CurCount = 0;
	avr->LastValue = 0;
}

void Average_Reset (T_Average *avr)
{
	avr->Sum = 0;
	avr->CurCount = 0;
}

int Average_Step (T_Average *avr, int val)
{
	if (avr->CurCount == 0)
		avr->LastValue = val;
	else if (avr->CurCount == avr->Count)
	{
		avr->LastValue = avr->Sum / avr->Count;
		avr->Sum = 0;
		avr->CurCount = 0;
	}

	avr->Sum += val;
	avr->CurCount += 1;
	return avr->LastValue;
}


// ---------------------------       moving average filter       ---------------------------

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


// ---------------------------       bounds filter       ---------------------------

void Bounds_Init (T_Bounds *p, int onval, int offval)
{
	p->On  = onval;
	p->Off = offval;
	p->State = false;
}

void Bounds_SetState (T_Bounds *p, bool state)
{
	p->State = state;
}

bool Bounds_Step (T_Bounds *p, int val)
{
	if (p->On >= p->Off)
	{
		if (val > p->On)
			p->State = true;
		else if (val < p->Off)
			p->State = false;
	}
	else
	{
		if (val < p->On)
			p->State = true;
		else if (val > p->Off)
			p->State = false;
	}

	return p->State;
}


// ---------------------------       rising edge filter       ---------------------------

void RisingEdge_Init (T_RisingEdge *p)
{
	p->Prev = false;
}

bool RisingEdge_Step (T_RisingEdge *p, bool val)
{
	bool result = val && !p->Prev;
	p->Prev = val;
	return result;
}


// ---------------------------       falling edge filter       ---------------------------

void FallingEdge_Init (T_FallingEdge *p)
{
	p->Prev = false;
}

bool FallingEdge_Step (T_FallingEdge *p, bool val)
{
	bool result = p->Prev && !val;
	p->Prev = val;
	return result;
}
