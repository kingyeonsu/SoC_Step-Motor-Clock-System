/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "sleep.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xtmrctr.h"

#define CHANNEL_1		1
#define CHANNEL_2		2
#define GPIO_BUTTON_PHOTO_DEVICE_ID		XPAR_AXI_GPIO_0_DEVICE_ID

#define INTC_DEVICE_ID					XPAR_INTC_0_DEVICE_ID
#define INTC_BUTTON_PHOTO_INT_VEC_ID	XPAR_INTC_0_GPIO_0_VEC_ID

#define TMRCTR_DEVICE_ID				XPAR_TMRCTR_0_DEVICE_ID
#define TMRCTR_INT_VECTOR_ID			XPAR_INTC_0_TMRCTR_0_VEC_ID
#define	TIMER_CNT_0		0

#define STEPMOTOR_DRIVER_0_ADDR			XPAR_AXI4_STEPMOTOR_DRIVER_0_BASEADDR
#define SEC_STEPMOTOR_DIRECTION			*(uint32_t *) STEPMOTOR_DRIVER_0_ADDR
#define SEC_STEPMOTOR_CYCLE				*(uint32_t *) (STEPMOTOR_DRIVER_0_ADDR + 4)

#define STEPMOTOR_DRIVER_1_ADDR			XPAR_AXI4_STEPMOTOR_DRIVER_1_BASEADDR
#define MIN_STEPMOTOR_DIRECTION			*(uint32_t *) STEPMOTOR_DRIVER_1_ADDR
#define MIN_STEPMOTOR_CYCLE				*(uint32_t *) (STEPMOTOR_DRIVER_1_ADDR + 4)

#define STEPMOTOR_DRIVER_2_ADDR			XPAR_AXI4_STEPMOTOR_DRIVER_2_BASEADDR
#define HOUR_STEPMOTOR_DIRECTION		*(uint32_t *) STEPMOTOR_DRIVER_2_ADDR
#define HOUR_STEPMOTOR_CYCLE			*(uint32_t *) (STEPMOTOR_DRIVER_2_ADDR + 4)

#define PWM_GENERATOR_ADDR				XPAR_AXI4_PWM_GENERATOR_0_BASEADDR
#define BUZZER_FREQ						*(uint32_t *) PWM_GENERATOR_ADDR

#define FND_CLOCK_CONTROLLER_ADDR		XPAR_AXI4_FND_CONTROLLER_0_BASEADDR
#define FND_CLOCK_CONTROL				*(uint32_t *) FND_CLOCK_CONTROLLER_ADDR
#define FND_CLOCK_DATA					*(uint32_t *) (FND_CLOCK_CONTROLLER_ADDR + 4)

#define FORWARD			1
#define BACKWARD		0

#define MINUTE			1
#define HOUR			2
#define HALFDAY			3
#define BACK			7
#define WARNING			9

#define SYS_CLK	100000000
uint32_t resetValue = 0xffffffff - SYS_CLK/100;		// 10 msec (10Hz)

volatile int msec_10_count = 0;
volatile int sec_count = 0;
volatile int min_count = 0;
volatile int hour_count = 0;
volatile int fnd_sec_count = 0;
volatile int fnd_min_count = 0;
volatile int fnd_hour_count = 0;
volatile int half_day_count = 0;
volatile int warning_state = 0;
volatile int buzzer_state = 0;
volatile int photoInt_state = 0;
volatile int FND_Clock_state = 0;
volatile int StepMotorInit = 0;
volatile int StepMotor_Clock_Start_state = 0;
volatile int PhotoInit_state_1 = 0;
volatile int PhotoInit_state_2 = 0;
volatile int PhotoInit_state_3 = 0;
volatile int FND_Control_state = 0;

XTmrCtr TimerCounterInst;
XIntc Intc;
XGpio Gpio_Button;
XGpio Gpio_Photo;

void TimerCounterHandler(void* CallBackRef, uint8_t TmrCtrNumber);
void GpioHandler(void* CallBackRef);
void BuzzerSpeaker();

void ButtonInit()
{
	// Button Init
	XGpio_Initialize(&Gpio_Button, GPIO_BUTTON_PHOTO_DEVICE_ID);
	XGpio_SetDataDirection(&Gpio_Button, CHANNEL_1, 0x0f);
}

void PhotoInit()
{
	// Photo Init
	XGpio_Initialize(&Gpio_Photo, GPIO_BUTTON_PHOTO_DEVICE_ID);
	XGpio_SetDataDirection(&Gpio_Photo, CHANNEL_2, 0x0f);
}

void StepMotorClock_Init()
{
	StepMotor_Clock_Start_state = 0;
	SEC_STEPMOTOR_DIRECTION = BACKWARD;
	MIN_STEPMOTOR_DIRECTION = BACKWARD;
	HOUR_STEPMOTOR_DIRECTION = BACKWARD;

	SEC_STEPMOTOR_CYCLE = 30;
	MIN_STEPMOTOR_CYCLE = 30;
	HOUR_STEPMOTOR_CYCLE = 30;
	photoInt_state=0;
}

void StepMotorClock_Start()
{
	StepMotor_Clock_Start_state = 1;
	SEC_STEPMOTOR_DIRECTION = FORWARD;
	MIN_STEPMOTOR_DIRECTION = FORWARD;
	HOUR_STEPMOTOR_DIRECTION = FORWARD;

	SEC_STEPMOTOR_CYCLE = 60;
	MIN_STEPMOTOR_CYCLE = 60*60;
	HOUR_STEPMOTOR_CYCLE = 60*60*12;
	photoInt_state=1;
}

void IntInit()
{
	// Interrupt setup
	XIntc_Initialize(&Intc, INTC_DEVICE_ID);

	// Interrupt Controller의  Vector Table에 Jump할 함수 할당
	XIntc_Connect(&Intc, INTC_BUTTON_PHOTO_INT_VEC_ID, (Xil_ExceptionHandler)GpioHandler, &Gpio_Button);
	XIntc_Connect(&Intc, INTC_BUTTON_PHOTO_INT_VEC_ID, (Xil_ExceptionHandler)GpioHandler, &Gpio_Photo);

	// Enable the Interrput vector at the interrput controller
	XIntc_Enable(&Intc, INTC_BUTTON_PHOTO_INT_VEC_ID);

	// Start the Interrupt controller such that interrupts recognized
	// and handled by the processor
	XIntc_Start(&Intc, XIN_REAL_MODE);

	XGpio_InterruptEnable(&Gpio_Button, CHANNEL_1);
	XGpio_InterruptGlobalEnable(&Gpio_Button);
	XGpio_InterruptEnable(&Gpio_Photo, CHANNEL_2);
	XGpio_InterruptGlobalEnable(&Gpio_Photo);

	XIntc_Connect(&Intc, TMRCTR_INT_VECTOR_ID, (XInterruptHandler)XTmrCtr_InterruptHandler, &TimerCounterInst);
	XIntc_Enable(&Intc, TMRCTR_INT_VECTOR_ID);

	XTmrCtr_SetHandler(&TimerCounterInst, TimerCounterHandler, &TimerCounterInst);
	XTmrCtr_SetOptions(&TimerCounterInst, TIMER_CNT_0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNT_0, resetValue);
	XTmrCtr_Start(&TimerCounterInst, TIMER_CNT_0);

	// Exception Setup
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XIntc_InterruptHandler, &Intc);
	Xil_ExceptionEnable();
}

int main()
{
    init_platform();
    ButtonInit();
    PhotoInit();
    XTmrCtr_Initialize(&TimerCounterInst, TMRCTR_DEVICE_ID);
	XTmrCtr_SelfTest(&TimerCounterInst, TIMER_CNT_0);
	IntInit();

	print("Hello World\n\r");
    print("Successfully ran Hello World application");

    FND_CLOCK_DATA = 0;
    FND_CLOCK_CONTROL = 0;

    while(1)
    {
    	BuzzerSpeaker();
    }

    cleanup_platform();
    return 0;
}

void GpioHandler(void* CallBackRef)	// void pointer --> 자료형이 정해지지 않고 사용할때마다 지정
{
	XGpio* pGpio = (XGpio* )CallBackRef;

	// button
	if ((XGpio_DiscreteRead(pGpio, CHANNEL_1) & 0X01) )
	{
		StepMotorInit = 1;
		StepMotorClock_Init();
	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_1) & 0X02))
	{

	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_1) & 0X04))		FND_Clock_state = ~FND_Clock_state;
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_1) & 0X08))		FND_Control_state = ~FND_Control_state;
	XGpio_InterruptClear(pGpio, CHANNEL_1);

	// Photo
	if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X04) && StepMotorInit)
	{
		PhotoInit_state_1 = 1;
		SEC_STEPMOTOR_CYCLE = 0;
	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X02) && StepMotorInit)
	{
		PhotoInit_state_2 = 1;
		MIN_STEPMOTOR_CYCLE = 0;
	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X01) && StepMotorInit)
	{
		PhotoInit_state_3 = 1;
		HOUR_STEPMOTOR_CYCLE = 0;
	}

	if (PhotoInit_state_1 && PhotoInit_state_2 && PhotoInit_state_3)
	{
		PhotoInit_state_1=0;
		PhotoInit_state_2=0;
		PhotoInit_state_3=0;
		StepMotorInit = 0;
		StepMotorClock_Start();
	}

	if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X04) && photoInt_state)
	{
		buzzer_state = MINUTE;
		sec_count = 0;
	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X02) && photoInt_state)
	{
		buzzer_state = HOUR;
		min_count = 0;
	}
	else if ((XGpio_DiscreteRead(pGpio, CHANNEL_2) & 0X01) && photoInt_state)
	{
		buzzer_state = HALFDAY;
		hour_count = 0;
	}
	XGpio_InterruptClear(pGpio, CHANNEL_2);
}

void TimerCounterHandler(void* CallBackRef, uint8_t TmrCtrNumber)
{
	if (FND_Control_state) FND_CLOCK_CONTROL = 2;
	else	FND_CLOCK_CONTROL = 0;

	if (StepMotor_Clock_Start_state)	msec_10_count++;

	if (msec_10_count > 99)
	{
		msec_10_count=0;
		sec_count++;
		fnd_sec_count++;
	}

	if (sec_count > 59)		min_count++;
	if (min_count > 59)		hour_count++;
	if (hour_count > 11)	half_day_count++;

	if (fnd_sec_count > 59)
	{
		fnd_sec_count=0;
		fnd_min_count++;
	}
	if (fnd_min_count > 59)
	{
		fnd_min_count=0;
		fnd_hour_count++;
	}

	if (fnd_hour_count > 23)
	{
		fnd_hour_count=0;
	}

	if (half_day_count == 2)
	{
		msec_10_count=0;
		sec_count=0;
		hour_count=0;
		half_day_count=0;
		buzzer_state=0;
		photoInt_state = 0;
		FND_Clock_state = 0;
		StepMotorInit = 0;
		StepMotor_Clock_Start_state = 0;
		PhotoInit_state_1 = 0;
		PhotoInit_state_2 = 0;
		PhotoInit_state_3 = 0;
		FND_Control_state = 0;
		FND_CLOCK_CONTROL = 1;
	}

	if (FND_Clock_state)	FND_CLOCK_DATA = fnd_hour_count*100 + fnd_min_count;
	else					FND_CLOCK_DATA = fnd_sec_count*100 + msec_10_count;

	if (sec_count > 61)
	{
		sec_count--;
		min_count--;
		hour_count--;
		warning_state++;
		buzzer_state = BACK;
		SEC_STEPMOTOR_DIRECTION = BACKWARD;
		MIN_STEPMOTOR_DIRECTION = BACKWARD;
		HOUR_STEPMOTOR_DIRECTION = BACKWARD;
	}

	if (warning_state > 600)
	{
		warning_state=0;
		StepMotor_Clock_Start_state=0;
		buzzer_state = WARNING;
		sec_count=0;
		min_count=0;
		hour_count=0;
		SEC_STEPMOTOR_CYCLE=0;
		MIN_STEPMOTOR_CYCLE=0;
		HOUR_STEPMOTOR_CYCLE=0;
	}
}

void BuzzerSpeaker()
{
	switch (buzzer_state)
	{
	case MINUTE:
		BUZZER_FREQ = 700;
		sleep(5);
		BUZZER_FREQ=0;
		buzzer_state=0;
		break;
	case HOUR:
		BUZZER_FREQ = 900;
		sleep(5);
		BUZZER_FREQ=0;
		buzzer_state=0;
		break;
	case HALFDAY:
		BUZZER_FREQ = 1100;
		sleep(5);
		BUZZER_FREQ=0;
		buzzer_state=0;
		break;
	case BACK:
		BUZZER_FREQ = 1111;
		sleep(2);
		BUZZER_FREQ=0;
		sleep(1);
		BUZZER_FREQ = 1111;
		sleep(2);
		BUZZER_FREQ=0;
		buzzer_state=0;
		break;
	case WARNING:
		BUZZER_FREQ = 1111;
		sleep(1);
		BUZZER_FREQ=0;
		sleep(1);
		BUZZER_FREQ = 1111;
		sleep(1);
		BUZZER_FREQ=0;
		sleep(1);
		BUZZER_FREQ = 1111;
		sleep(1);
		BUZZER_FREQ=0;
		StepMotorInit = 1;
		StepMotorClock_Init();
		buzzer_state=0;
		break;
	}
}
