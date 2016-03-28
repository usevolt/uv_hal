
#ifndef UW_FILTERS_H_
#define UW_FILTERS_H_


#include "uw_hal_config.h"

#include "uw_types.h"


// ---------------------------       hysteresis filter       ---------------------------

typedef struct
{
	int Last;
	int Diff;
} T_Hysteresis;

void Hysteresis_Init (T_Hysteresis *hys, int dif);
int  Hysteresis_Step (T_Hysteresis *hys, int val);


// ---------------------------       periodic average filter       ---------------------------

typedef struct
{
	int Sum;
	int Count;
	int CurCount;
	int LastValue;
} T_Average;

void Average_Init (T_Average *avr, int cnt);
void Average_Reset (T_Average *avr);
int  Average_Step (T_Average *avr, int val);


// ---------------------------       moving average filter       ---------------------------

typedef struct
{
	int Sum;
	int Count;
	int CurCount;
} T_MovingAver;

void MovingAver_Init (T_MovingAver *avr, int cnt);
void MovingAver_Reset (T_MovingAver *avr);
int  MovingAver_Step (T_MovingAver *avr, int val);


// ---------------------------       bounds filter       ---------------------------

typedef struct
{
	int On;			// edge to turn state on
	int Off;		// edge to turn state off
	bool State;		// current state
} T_Bounds;

void Bounds_Init (T_Bounds *p, int onval, int offval);
void Bounds_SetState (T_Bounds *p, bool state);
bool Bounds_Step (T_Bounds *p, int val);


// ---------------------------       rising edge filter       ---------------------------

typedef struct
{
	bool Prev;
} T_RisingEdge;

void RisingEdge_Init (T_RisingEdge *p);
bool RisingEdge_Step (T_RisingEdge *p, bool val);


// ---------------------------       falling edge filter       ---------------------------

typedef struct
{
	bool Prev;
} T_FallingEdge;

void FallingEdge_Init (T_FallingEdge *p);
bool FallingEdge_Step (T_FallingEdge *p, bool val);


#endif /* UW_FILTERS_H_ */
