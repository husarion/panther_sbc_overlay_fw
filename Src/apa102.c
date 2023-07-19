/*
 * apa102.c
 *
 *  Created on: 26.04.2023
 *      Author: Husarion
 */

#include "apa102.h"
#include "stm32f0xx_hal.h"

extern SPI_HandleTypeDef hspi1;
static uint16_t counter;

void LED_setColor(Led* led, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
  led->fields.blue = blue;
  led->fields.green = green;
  led->fields.red = red;
  led->fields.brightness = brightness;
  led->fields.fixed = 0b111;
}

void LED_SendFrame(Led led)
{
    uint16_t zero[2] = {0,0};
    HAL_SPI_Transmit(&hspi1, (uint8_t*)&zero, 2, 100);
  /*Transmit Data*/
    HAL_SPI_Transmit(&hspi1, led.buf, 4, 100);
  /*End frame*/
    HAL_SPI_Transmit(&hspi1, (uint8_t*)&zero, 2, 100);//spi_write16_blocking(led.spi_instance, zero, 2);
}

void LED_updateFrame()
{
#if 1
	strip_buffer.endFrame = 0;
	strip_buffer.startFrame = 0;

	for (uint8_t i = 0; i < NUM_LEDS; i++)
		{
		  // Fade out colors
		  int r = strip_buffer.strip[i].fields.red - 4;
		  int g = strip_buffer.strip[i].fields.green - 4;

		  // Draw white lines
		  if (i == counter - 0 || NUM_LEDS - i == (counter - 200) * 2 || NUM_LEDS - i == (counter - 200) * 2 - 1) {
			r += 255;
			g += 255;
		  }

		  // Draw red lines
		  if (NUM_LEDS - i == (counter * 2) - 50 || NUM_LEDS - i == (counter * 2) - 50 + 1 || i == counter - 160) {
			r += 255;
		  }

		  // Calculate uint8_t values
		  if(r>255)r = 255;
		  if(r<0) r = 0;
		  if(g>255)g = 255;
		  if(g<0) g = 0;
		  // Set color to pixel buffer
		  strip_buffer.strip[i].fields.red = (uint8_t) r;
		  strip_buffer.strip[i].fields.green = (uint8_t)g;
		  strip_buffer.strip[i].fields.blue = (uint8_t)g;
		  strip_buffer.strip[i].fields.brightness = (uint8_t)0xF;

		}
		// Reset counter
		if (counter == 380) {
		  counter = 0;
		}
		else {
		  counter++;
		}
#endif
}
