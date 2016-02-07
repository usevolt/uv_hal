
#ifndef HAL_GPIO_ACCESSORS_H_
#define HAL_GPIO_ACCESSORS_H_

#include "LPC11xx.h"
#include "hal_adc_controller.h"


// macros for accessing GPIO pins

// these macros expect signal port and pin to be defined as separate macros
//		#define SIGNAL_PORT		LPC_GPIO0
//		#define SIGNAL_PIN		4
// then current value can be read as GetPin(SIGNAL)

#define GetPin(signal)			((signal ## _PORT->DATA >> signal ## _PIN) & 1)
#define GetInvertedPin(signal)	(!((signal ## _PORT->DATA >> signal ## _PIN) & 1))
#define SetPinOn(signal)		signal ## _PORT->DATA |= (1 << signal ## _PIN)
#define SetPinOff(signal)		signal ## _PORT->DATA &= ~(1 << signal ## _PIN)
#define TogglePin(signal)		signal ## _PORT->DATA ^= (1 << signal ## _PIN)
#define SetPin(signal,state)	signal ## _PORT->DATA = (signal ## _PORT->DATA & ~(1 << signal ## _PIN)) | ((state) << signal ## _PIN)

#define InitInputPin(signal)	signal ## _PORT->DIR &= ~(1 << signal ## _PIN)
#define InitOutputPin(signal)	signal ## _PORT->DIR |= (1 << signal ## _PIN)


#endif /* HAL_GPIO_ACCESSORS_H_ */
