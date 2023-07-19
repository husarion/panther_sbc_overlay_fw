# Panther Overlay Firmware
Panther Overlay Firmware is a modified version of [USBCanConverterBasic (UCCB)](https://github.com/UsbCANConverter-UCCbasic) which is open source USB converter for Controller Area Network (CAN). It has been adapted to replace previous Overlays' MCUs (ATTiny).

## Firmware
The original firmware comes from [ucandevices - USBCanConverterBasic(UCCB)](https://github.com/UsbCANConverter-UCCbasic/UCCBEmbedded) and has been rewritten for STM32F072CB MCU. The main changes made are the MCU change and the implementation of the DMA on the UART interface.

## MCU Flashing
To program the MCU use [CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), [ST-Link Utility](https://www.st.com/en/development-tools/stsw-link004.html) and 
select a binary found in **Release** folder or download [Atollic TrueSTUDIO](https://www.st.com/en/development-tools/truestudio.html) and compile the code yourself. Integration with
STM32CubeIDE is yet to be done.
 
## Panther SBC Overlay Integration

### UART improvements

#### UART Reception
UART communication utilizes DMA and STM32 UART Controller's character detection. 
In **slcan** every command is terminated with the **line return** character `\r`. 
With speeds of 4Mbps, UART RX interrupt would occur with every **byte** received, so the DMA
and character match combination is introduced to extend an overall device performance. 

DMA appends every received byte to an ***active buffer***. When `\r` is received, the interrupt is raised, the buffers' content is parsed from text to a raw CAN message.

However, before parsing a message, the ***active buffer*** is swapped, so the reception of the next message
can begin while the previous message is being parsed.

#### UART Transmission
Transmission also utilizes DMA, yet it is not as demanding as the reception. 
After receiving a CAN frame an interrupt raises which switches on a `canRxFlag`.
Then a received frame is parsed to slcan ASCII format, and then it is sent either by an UART
or USB (if connected).

### Panther Lights
Panther Lights are two strips based on APA102 LED modules located on both front and rear bumper. Panther's startup animation is sent by this MCU. 
Another DMA channel is used to transfer an LED color buffer via SPI. This channel is set to
**circular mode**, so the transmission request is called only once at the beginning of the program.
The animation is calculated in the main program loop once every 18ms.
