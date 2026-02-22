#ifndef __SPI_H
#define __SPI_H
#include "sys.h"

void SPI3_GPIO_Init(void);
void SPI3_Init(void);
void SPI3_SendData(uint8_t data);

#endif
