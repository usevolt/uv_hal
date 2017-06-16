
#ifndef UW_FILTERS_H_
#define UW_FILTERS_H_


#include "uv_hal_config.h"
#include <uv_utilities.h>


typedef struct {
	int32_t sum;
	int32_t count;
	int32_t cur_count;
	int32_t val;
} uv_moving_aver_st;

/// @brief: Initializes the moving average filter
void uv_moving_aver_init (uv_moving_aver_st *avr, int32_t cnt);

/// @brief: Resets the moving average filter to zero
void uv_moving_aver_reset (uv_moving_aver_st *avr);

/// @brief: Moving average step function. Should be called with a constant step time
int32_t  uv_moving_aver_step (uv_moving_aver_st *avr, int32_t val);

/// @brief: Returns the current moving average value
static inline int32_t uv_moving_aver_get_val(uv_moving_aver_st *this) {
	return (this->val / 0x100);
}


typedef struct {
	int32_t trigger_value;
	int32_t hysteresis;
	uint8_t result;
	bool invert;
} uv_hysteresis_st;

/// @brief: Initializes the hysteresis filter
///
/// @param trigger_value: Value at where toggling the output is done
/// @param Hysteresis: The hysteresis around trigger_value
void uv_hysteresis_init(uv_hysteresis_st *this, int32_t trigger_value, int32_t hysteresis, bool invert);

void uv_hysteresis_step(uv_hysteresis_st *this, int32_t value);

static inline uint8_t uv_hysteresis_get_output(uv_hysteresis_st *this) {
	return this->result;
}


#endif /* UW_FILTERS_H_ */
