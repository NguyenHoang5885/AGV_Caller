/*
 * b.IOManagerTask.c
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */
#include "main.h"
#include "b.IOManagerTask.h"
#include "c.TCPProtocolTask.h"

GPIO GPIO_OUTPUT[8] = { // Name - Port - Pin follow main.h
		{"OUT1", OUT1_GPIO_Port, OUT1_Pin},
		{"OUT2", OUT2_GPIO_Port, OUT2_Pin},
		{"OUT3", OUT3_GPIO_Port, OUT3_Pin},
		{"OUT4", OUT4_GPIO_Port, OUT4_Pin},
		{"OUT5", OUT5_GPIO_Port, OUT5_Pin},
		{"OUT6", OUT6_GPIO_Port, OUT6_Pin},
		{"OUT7", OUT7_GPIO_Port, OUT7_Pin},
		{"OUT8", OUT8_GPIO_Port, OUT8_Pin},
		};
GPIO GPIO_INPUT[8] = { // Name - Port - Pin follow main.h
		{"IN1", IN1_GPIO_Port, IN1_Pin},
		{"IN2", IN2_GPIO_Port, IN2_Pin},
		{"IN3", IN3_GPIO_Port, IN3_Pin},
		{"IN4", IN4_GPIO_Port, IN4_Pin},
		{"IN5", IN5_GPIO_Port, IN5_Pin},
		{"IN6", IN6_GPIO_Port, IN6_Pin},
		{"IN7", IN7_GPIO_Port, IN7_Pin},
		{"IN8", IN8_GPIO_Port, IN8_Pin},
		};
GPIO GPIO_BUTTON[4] = { // Name - Port - Pin follow main.h
		{"BT1", BT1_GPIO_Port, BT1_Pin},
		{"BT2", BT2_GPIO_Port, BT2_Pin},
		{"BT3", BT3_GPIO_Port, BT3_Pin},
		{"BT4", BT4_GPIO_Port, BT4_Pin},
		};
GPIO GPIO_BUTTON_LED[4] = { // Name - Port - Pin follow main.h
		{"BT1_LED", LED_BT1_GPIO_Port, LED_BT1_Pin},
		{"BT2_LED", LED_BT2_GPIO_Port, LED_BT2_Pin},
		{"BT3_LED", LED_BT3_GPIO_Port, LED_BT3_Pin},
		{"BT4_LED", LED_BT4_GPIO_Port, LED_BT4_Pin},
		};

extern osThreadId MODBUS_RTU_TASKHandle;  // From main.c
extern osThreadId TCP_PROTO_TASKHandle;      // From main.c
extern osThreadId HTTP_PROTO_TASKHandle;   // From main.c

extern osMailQId IOTaskReceive_MQ;                  // From main.c ( IO Manager Queue )
extern osMailQId IOTaskTransferToRTU_MQ;      // Mail Queue Notify Change To RTU Task
extern osMailQId IOTaskTransferToTCP_MQ;       // Mail Queue Notify Change To TCP Task
extern osMailQId IOTaskTransferToHTTP_MQ;    // Mail Queue Notify Change To HTTP Task

extern uint8_t  On_RTU; 		  //from "c.TCPProtocolTask.h"
extern uint8_t int_app;

uint8_t Output_MainDatabase, Button_MainDatabase, Input_MainDatabase, Led_MainDatabase; // Database

volatile uint8_t New_State_IO = 0;

//Turn off Button Led after 1.5s
extern uint8_t On_FMS;
TickType_t start_tick;
uint8_t timer_running = 0;
uint8_t AutoTurnOffLed = 0; // HIK ROBOT
uint8_t NoSendTurnOff = 0;
uint8_t author_Input = 0, author_Button = 0, author_Output= 0, server_reply = 1;

void Write_Output(void){
	for(uint8_t i = 0; i < 8; i++){
		if( Output_MainDatabase & (1 << i)) {
			GPIO_OUTPUT[i].port ->  BSRR = GPIO_OUTPUT[i].pin; // SET BIT 1
		}
		else{
			GPIO_OUTPUT[i].port ->  BSRR = GPIO_OUTPUT[i].pin << 16; // RESET BIT 0
		}
	}
}

void Write_ButtonLed(void){
	if(On_FMS == 1){
		for(uint8_t i = 0; i < 4; i++){
			if( Led_MainDatabase & (1 << i)) {
				GPIO_BUTTON_LED[i].port ->  BSRR = GPIO_BUTTON_LED[i].pin; // SET BIT 1
			}
			else{
				GPIO_BUTTON_LED[i].port ->  BSRR = GPIO_BUTTON_LED[i].pin << 16; // RESET BIT 0
			}
		}
	}
	if(On_FMS == 0){
		AutoTurnOffLed = 1;
		for(uint8_t i = 0; i < 4; i++){
			if( Button_MainDatabase & (1 << i)) {
				GPIO_BUTTON_LED[i].port ->  BSRR = GPIO_BUTTON_LED[i].pin; // SET BIT 1
			}
			else{
				GPIO_BUTTON_LED[i].port ->  BSRR = GPIO_BUTTON_LED[i].pin << 16; // RESET BIT 0
			}
		}
	}
}
//Manage loop speed
//TickType_t loop_start;
//TickType_t loop_end;
//uint32_t loop_time_ms;

void IOManagerTaskFunction(void const *argument){ // RTOS
	osEvent MailIOEvent;
	osEvent Interrupt_Button_Input;      // Flag RTOS Event from other Task
	IO_State_t *Receive_IO_State;         // pointer to IO_State_t : Value Queue Data
	IO_State_t *TransferToRTU_IO_State;
	IO_State_t *TransferToTCP_IO_State;
	IO_State_t *TransferToHTTP_IO_State;
	while(1){
//		loop_start = xTaskGetTickCount();
		/*=================RECEVER FROM OTHER TASK==========================*/
		MailIOEvent = osMailGet(IOTaskReceive_MQ, 10); // Other Task
		if(MailIOEvent.status == osEventMail){
			Receive_IO_State = (IO_State_t*)MailIOEvent.value.p;
			Output_MainDatabase = Receive_IO_State -> Output_Data; // Read from mail
			Button_MainDatabase  = Receive_IO_State -> Button_Data; // Read from mail
			Led_MainDatabase        = Receive_IO_State -> Led_Data;
			Write_Output();
			Write_ButtonLed();
			New_State_IO = 1;
			osMailFree(IOTaskReceive_MQ, Receive_IO_State);
		}
		/*=================READ BUTTON EXTI==================================*/
		Interrupt_Button_Input = osSignalWait(SIG_BT1| SIG_BT2 | SIG_BT3 | SIG_BT4|SIG_SCAN_INPUT|SIG_RST_WIFI, 10); //EXT interrupt
		if( Interrupt_Button_Input.status ==  osEventSignal){
			if(Interrupt_Button_Input.value.signals & SIG_BT1){
				Button_MainDatabase ^= (1 << 0);
				if(On_FMS == 1) author_Button = 1;
			}
			if(Interrupt_Button_Input.value.signals & SIG_BT2){
				Button_MainDatabase ^= (1 << 1);
				if(On_FMS == 1) author_Button = 1;
			}
			if(Interrupt_Button_Input.value.signals & SIG_BT3){
				Button_MainDatabase ^= (1 << 2);
				if(On_FMS == 1) author_Button = 1;
			}
			if(Interrupt_Button_Input.value.signals & SIG_BT4){
				Button_MainDatabase ^= (1 << 3);
				if(On_FMS == 1) author_Button = 1;
			}
			New_State_IO = 1;
			if(On_FMS == 0) Write_ButtonLed();
		}

		/*=================READ INPUT SCAN 10MS===============================*/
		if(Interrupt_Button_Input.	value.signals & SIG_SCAN_INPUT){ //Timer interrupt
			New_State_IO = 1 ;
			author_Input = 1;
			if(On_FMS == 0){
				Input_MainDatabase = Output_MainDatabase;    // With HIK robot Input => Output
			}
		}
//		/*============AUTO TURN OFF LED AFTER 1.5S WITH HIK ROBOT===========================*/
		if(AutoTurnOffLed == 1){
			if(timer_running == 0)
			{
				start_tick = xTaskGetTickCount();
				timer_running = 1;
			}
			if(timer_running && (xTaskGetTickCount() - start_tick >= pdMS_TO_TICKS(1500))) // Counter 1.5s to turn off led
			{
				// OFF LED
				for(uint8_t i = 0; i < 4; i++){
					GPIO_BUTTON_LED[i].port ->  BSRR = GPIO_BUTTON_LED[i].pin << 16; // RESET BIT 0
				}
				//Button_MainDatabase = 0; // Reset value
				New_State_IO = 1;
				timer_running = 0;
				AutoTurnOffLed = 0;
				NoSendTurnOff = 1;
			}
		}
		/*---------------------	AUTO RESEND WHEN SERVER DON'T REPLY FMS ROBOT------------------------------------*/
		if(server_reply == 0){
			if(timer_running == 0)
			{
				start_tick = xTaskGetTickCount();
				timer_running = 1;
			}
			if(timer_running && (xTaskGetTickCount() - start_tick >= pdMS_TO_TICKS(1000))) // Counter 0.5s to Resend
			{
				timer_running = 0;
				osSignalSet(HTTP_PROTO_TASKHandle, SIG_IOQUEUE_TO_HTTP);
			}
		}

		/*==============NOTIFY CHANGE IO_STATE TO OTHER TASK===================*/
		if(New_State_IO == 1 ){
			/*-----TO TCP----*/
			if(int_app == 0){
				TransferToTCP_IO_State = osMailAlloc(IOTaskTransferToTCP_MQ, 0);   //Memory allocation in MailQueue
				if(TransferToTCP_IO_State){
					TransferToTCP_IO_State -> Input_Data    = Input_MainDatabase;     // Write to mail
					TransferToTCP_IO_State -> Output_Data = Output_MainDatabase; // Write to mail
					TransferToTCP_IO_State -> Led_Data        = Led_MainDatabase;       // Write to mail
					osMailPut(IOTaskTransferToTCP_MQ, TransferToTCP_IO_State);
					osSignalSet(TCP_PROTO_TASKHandle, SIG_IOQUEUE_TO_TCP);
				}
			}
			else{
				int_app = 0;
			}
			/*-----TO RTU----*/
			if( On_RTU == 1){
				TransferToRTU_IO_State = osMailAlloc(IOTaskTransferToRTU_MQ, 0); //Memory allocation in MailQueue
				if(TransferToRTU_IO_State){
					TransferToRTU_IO_State -> Input_Data     = Input_MainDatabase;     // Write to mail
					TransferToRTU_IO_State -> Output_Data  = Output_MainDatabase; // Write to mail
					TransferToRTU_IO_State -> Button_Data  = Button_MainDatabase;  // Write to mail
					osMailPut(IOTaskTransferToRTU_MQ, TransferToRTU_IO_State);
					osSignalSet(MODBUS_RTU_TASKHandle, SIG_IOQUEUE_TO_RTU);
				}
			}
			/*-----TO HTTP----*/
			if(NoSendTurnOff == 1){ // If HIK mode won't send to Server (through HTTP) when button off
				NoSendTurnOff = 0;
			}
			else{
				TransferToHTTP_IO_State = osMailAlloc(IOTaskTransferToHTTP_MQ, 0); //Memory allocation in MailQueue
				if(TransferToHTTP_IO_State){
					TransferToHTTP_IO_State -> Input_Data     = Input_MainDatabase;     // Write to mail
					TransferToHTTP_IO_State -> Output_Data  = Output_MainDatabase; // Write to mail
					TransferToHTTP_IO_State -> Button_Data  = Button_MainDatabase;  // Write to mail
					osMailPut(IOTaskTransferToHTTP_MQ, TransferToHTTP_IO_State);
					osSignalSet(HTTP_PROTO_TASKHandle, SIG_IOQUEUE_TO_HTTP);
					//if(On_FMS == 0){
					Button_MainDatabase = 0;
					//}
				}
			}
			/*---------*/
			New_State_IO = 0;
		}

//
//		loop_end = xTaskGetTickCount();
//		loop_time_ms = (loop_end - loop_start) * portTICK_PERIOD_MS;
	}
}

