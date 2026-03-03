// pintrace.h

#ifdef USE_PINTRACE
#ifndef PINTRACE_H_
#define PINTRACE_H_

#define PINTRACE_PORT GPIO_PORTF_BASE
#define PINTRACE_PIN  GPIO_PIN_1

extern void PinTrace_Init(void);
extern void PinTrace_Set(void);
extern void PinTrace_Clear(void);

#endif // PINTRACE_H_
#endif // USE_PINTRACE
