/*
 * apa102.h
 *
 *  Created on: 26.04.2023
 *      Author: Husarion
 */

#ifndef APA102_H_
#define APA102_H_

#include "stdint.h"

#define COLOR_WHITE 255,255,255
#define COLOR_RED 255,0,0
#define COLOR_GREEN 0,255,0
#define COLOR_BLUE 0,0,255
#define NUM_LEDS 46

typedef struct
{
  union {
    uint8_t buf[4];
    struct {
      uint8_t fixed : 3;
      uint8_t brightness : 5;
      uint8_t blue;
      uint8_t green;
      uint8_t red;
    }fields;
  };
}Led;

static struct strip_buffer{
	  uint32_t startFrame;
	  Led strip[NUM_LEDS];
	  uint32_t endFrame;
  }strip_buffer;

void LED_setColor(Led* led, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
void LED_SendFrame(Led led);
void LED_updateFrame();

#endif /* APA102_H_ */
