#include "timer.h"
#include "stm32g0xx.h"

static union
{
  uint64_t val64;
  uint32_t val32;
} tickCounter;

void __attribute__((interrupt)) SysTick_Handler(void)
{
  tickCounter.val64++;
}

uint32_t timer_getTickCount(void)
{
  return tickCounter.val32;
}

uint32_t timer_getTickDiff(uint32_t lastTickCount)
{
  return timer_getTickCount() - lastTickCount;
}
