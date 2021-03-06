/*!
 * @file       main.c
 * @brief      霍尼可燃气体数据转发器（CLAA版）主程序
 * @details    基于华大HC32L110C6UA及利尔达LSD4WN-2N717M90标准LoRaWAN模组，实现无线可燃气体报警器应用功能，主要功能包括：               			   
			   - 传感器数据采集
			   - LoRaWAN网络连接管理
			   - 心跳管理
			   - 浓度告警上报
			   - 掉电处理机制
			   - 测试/透传模式
			   
 * @copyright  Revised BSD License, see section LICENSE. \ref LICENSE
 * @author     Gucj
 * @date       2020-03-19
 * @version    V1.0
 */
 
/* -------------------------------------------------------------------------- 
*                               To Do List
* 1、
* 2、
* 3、
----------------------------------------------------------------------------- */

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"
#include "lpuart.h"
#include "uart.h"
#include "bt.h"
#include "clk.h"
#include "lpm.h"
#include "gpio.h"
#include "lpt.h"
#include "app.h"
#include "driver_mid.h"
#include "LSD_LoRaWAN_ICA_Driver.h"
#include "HDEE5.h"

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')                            
 ******************************************************************************/
#define __DEBUG
#define DEBUG_LOG_LEVEL_1

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
extern uint32_t uwTick;

extern uint8_t uart0_RxBuf[UART0_RXBUFSIZE];
extern uint8_t uart1_RxBuf[UART1_RXBUFSIZE];
extern uint8_t lpuart_RxBuf[LPUART_RXBUFSIZE];

extern int uart0_RxByteCnt,uart0_TxByteCnt;
extern int uart1_RxByteCnt,uart1_TxByteCnt;
extern int lpuart_RxByteCnt,lpuart_TxByteCnt;

extern bool getSensorData_Tp;
extern bool heartbeatReport_Tp;

extern volatile uint32_t RxDoneFlag_lpuart;
extern volatile uint32_t RxDoneFlag_uart0;
extern volatile uint32_t RxDoneFlag_uart1;

extern uint8_t keyFunTest;
extern uint8_t vdetectFunTest;
extern bool onlineFlag;
extern bool errorReportFlag;
extern bool powerDownFlag;
extern bool powerOnFlag;
extern ledStatType ledStat;
extern uint8_t vdetectEnable;
extern bool keyDetectedFlag;
/******************************************************************************
 * Local type definitions ('typedef')                                         
 ******************************************************************************/

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/******************************************************************************
 * Local variable definitions ('static')                                      *
 ******************************************************************************/
uint8_t logLevel=2;
uint8_t EeBuf[HDEE_EeSize];
/******************************************************************************
 * Local pre-processor symbols/macros ('#define')                             
 ******************************************************************************/

/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
 
/**
 ******************************************************************************
 ** \brief  Main function of project
 **
 ** \return uint32_t return value, if needed
 **
 ** This sample
 **
 ******************************************************************************/



int32_t main(void)
{    
	uint8_t led=0;
	volatile uint32_t u32Val = 0;
	
	SysTick_init();
	Uart0_init();  //报警器通信串口
	Uart1_init();  //调试串口
	Lpuart1_init();  //LoRaWAN模组通信串口
	Lptimer_Init();  //Uart1/2分帧定时器
	Adtimer5_init();  //掉电检测定时器
	Adtimer4_init();  //Lpuart分帧定时器
	Gpio_lora_Init();  //LoRaWAN模组初始化
	Gpio_key_init();  //按键
	Pca_led_init();	  //LED
	Gpio_vdetect_init();  //掉电检测引脚
	HDEE_Ini();  //Flash模拟EEPROM
	Wdt_init();  //软件看门狗
	

	//出厂测试
	while(1)
	{
		factoryTest();
		while(1)
		{
			system_delay_ms(200); 
			if(keyDetectedFlag) 
			{
				keyDetectedFlag=0;
				break;

			}
			
		}
	}

	//LoRaWAN模组初始化
	lorawanResetAndConfig();

	//获取DEVEUI
	getDeveui();
	
	//读取掉电标记
	pdRecovery_Judge();

	ledStat = OFF;

	while(1)
    {
		//网络连接
		if(!onlineFlag)
		{
			networkConnect_Task();
		}
		else
		{			
			//掉电恢复
			if(powerOnFlag)
			{
				powerOnFlag = 0;
				powerOn_Task();
			}
			
			//传感器数据获取
			if(getSensorData_Tp)
			{
				getSensorData_Tp=0;
				getSensorData_Task();
				
			}
			
			//告警上报
			if(errorReportFlag)
			{
				if(errorReportFlag)
					errorReport_Task();
					
			}
				
			//心跳上报
			if(heartbeatReport_Tp)
			{
				heartbeatReport_Tp = 0;
				heartBeatReport_Task();
			}
		}
			
		
			
		
			
		
			
		
		
//		
//		
//		//LED测试
//		if(led)
//		{
//			led=0;
//			Gpio_SetIO(3, 2, 0);
//		}
//		else
//		{
//			led=1;
//			Gpio_SetIO(3, 2, 1);
//		}		
//		
//		//按键测试
//		keyFunTest = 1;  //开启按键功能测试
//		
//		//掉电告警测试
//		vdetectFunTest = 1;  //开启掉电告警测试
//		
//		//Uart0测试
////		printf("Uart0-DTU transmit test\r\n");
////		Uart0_send_string("*0401000000000000FA\r\n");
////		while(RxDoneFlag!=1)
////		{}
////		printf("receive: %s \r\n",uart0_RxBuf);
////		system_delay_ms(2000);	
//			
////		//Lpuart测试
////		printf("Lpuart-Module transmit test\r\n");

////		//清除接收缓存
////		RxDoneFlag = 0;  
////		lpuart_RxByteCnt=0;
////		memset(lpuart_RxBuf,0,LPUART_RXBUFSIZE);
////		
////		Lpuart_send_string("at+ver?\r\n");
////		Gpio_SetIO(3, 2, 1);
////		while(RxDoneFlag!=1)
////		{}
////		printf("receive: %s \r\n",lpuart_RxBuf);	
////		system_delay_ms(5000);	
//		
//		//printf("%d",transfer_configure_command("AT+OTAA=1") );
//		if(Gpio_GetIO(0,3)==1)system_delay_ms(2000);
//		else system_delay_ms(200);
//		
//		//system_delay_ms(1000);
	}
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/


