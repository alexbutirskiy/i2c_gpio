#ifndef LIB_TIMER_TIMER_H_
#define LIB_TIMER_TIMER_H_
#include <stdint.h>

uint32_t timer_getTickCount(void);
uint32_t timer_getTickDiff(uint32_t lastTickCount);

#endif /* LIB_TIMER_TIMER_H_ */
