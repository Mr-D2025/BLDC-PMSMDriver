#include "sys.h"
#include "IOCtrl.h"

void IO_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOA时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO

	GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIO

	GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14);

}

// 高侧换相表 [C_H B_H A_H]
const uint8_t HighSideTable[6] = {
    0b001,  // 步骤0: A高侧开
    0b001,  // 步骤1: A高侧开  
    0b010,  // 步骤2: B高侧开
    0b010,  // 步骤3: B高侧开
    0b100,  // 步骤4: C高侧开
    0b100   // 步骤5: C高侧开
};

// 低侧换相表 [C_L B_L A_L]
const uint8_t LowSideTable[6] = {
    0b010,  // 步骤0: B低侧开
    0b001,  // 步骤1: C低侧开
    0b001,  // 步骤2: C低侧开
    0b100,  // 步骤3: A低侧开
    0b100,  // 步骤4: A低侧开
    0b010   // 步骤5: B低侧开
};


void SetPhase_120deg_Separate(uint8_t step)
{
    uint8_t idx = step % 6;
    
    // 高侧输出
    PAout(8)  = (HighSideTable[idx] >> 0) & 0b1;  // A_H
    PAout(9)  = (HighSideTable[idx] >> 1) & 0b1;  // B_H
    PAout(10) = (HighSideTable[idx] >> 2) & 0b1;  // C_H
    
    // 低侧输出
    PBout(12) = (LowSideTable[idx] >> 0) & 0b1;   // A_L
    PBout(13) = (LowSideTable[idx] >> 1) & 0b1;   // B_L
    PBout(14) = (LowSideTable[idx] >> 2) & 0b1;   // C_L
}



void Brake(void)
{
	PAout(8)=0;
	PAout(9)=0;
	PAout(10)=0;
	
	PBout(12)=0;
	PBout(13)=0;
	PBout(14)=0;
	
}
