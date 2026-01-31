#include "sys.h"
#include "delay.h"
#include "IOCtrl.h"
#include "Drv_TIM.h"
#include "LED.h"

int main(void)
{
	uint8_t step=0;
	
	delay_init(168);		  
	IO_Init();	
	LED_Init();
	TIM3_Int_Init(5000-1,8400-1);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms	
	
	while(1)
	{
	 
		SetPhase_120deg_Separate(step);
		step++;
		delay_us(5000);
		
		if(step>5) step=0;
	}
}
