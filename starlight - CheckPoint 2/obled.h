// FIle: obled.h

extern int pico_led_init(void);      // Perform initialisation
extern void pico_set_led(bool led_on);       // Turn the led on or off

#define ObLED_On()  pico_set_led(true);
#define ObLED_Off() pico_set_led(false);


// EndFile: obled.h