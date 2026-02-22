#include "sys.h"

void SPI3_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // 使能GPIOC时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // PC10 - SPI3_SCK
    // PC11 - SPI3_MISO
    // PC12 - SPI3_MOSI
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;      // 复用功能
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;    // 推挽输出
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;  // 无上下拉
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // 配置AF功能（SPI3复用功能为AF6）
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);
}

void SPI3_Init(void) {
    SPI_InitTypeDef SPI_InitStruct;
    
    // 使能SPI3时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
    
    // 配置GPIO
    SPI3_GPIO_Init();
    
    // SPI3参数配置
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // 全双工
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                       // 主模式
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                   // 8位数据
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;                          // 时钟极性
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;                        // 时钟相位
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                           // 软件NSS
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; // 预分频
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                   // MSB在前
    SPI_InitStruct.SPI_CRCPolynomial = 7;                            // CRC多项式
    
    SPI_Init(SPI3, &SPI_InitStruct);
    
    // 使能SPI3
    SPI_Cmd(SPI3, ENABLE);
}


// 发送函数
uint8_t SPI3_SendData(uint8_t data) {

    while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI3, data);
    while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
}
