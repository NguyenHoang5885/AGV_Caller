#ifndef INC_C_TCPPROTOCOLTASK_H_
#define INC_C_TCPPROTOCOLTASK_H_

#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "string.h"
#include <stdio.h>

typedef struct {
    uint32_t magic;
    // HTTP
    uint8_t St_ID_Flask;
    int    Port_HTTP_Flask;
//    char Token_Flask[60];
    char Endpoint_Flask[32];
    char Pass_HTTP_Flask[20];
    char Event_Flask[80];
    char Ip_HTTP_Flask[32];
    char User_Flask[20];

    //FMS
    uint8_t  On_FMS_Flask;
    char Tk1_Flask[20];
	char Tk2_Flask[20];
	char Tk3_Flask[20];
	char Tk4_Flask[20];
	char Pt1_Flask[160];
	char Pt2_Flask[160];
	char Pt3_Flask[160];
	char Pt4_Flask[160];

    // TCP + RTU
    uint8_t  On_TCP_Flask;
    uint8_t  On_RTU_Flask;
    uint8_t  Slave_ID_Flask;
    int      Port_TCP_Flask;

    uint32_t crc_Flask;
} SystemConfig_t;

extern int  Port_HTTP;
extern uint8_t St_ID;
extern char  Endpoint[32], Pass_HTTP[20], Event[80], Ip_HTTP[32], User[20]; //Token[60]
//extern char Ssid[32], Pass_STA[32], Ip_STA[20], Mask[20], Gateway[20];

extern uint8_t  On_FMS;
extern char Tk1[20], Tk2[20], Tk3[20], Tk4[20], Pt1[160], Pt2[160], Pt3[160], Pt4[160];

extern uint8_t  On_TCP;
extern uint8_t  On_RTU;
extern uint8_t  Slave_ID;
extern int      Port_TCP;

extern SystemConfig_t g_config;

void Flash_Write_Config(SystemConfig_t *cfg) ;
void Flash_Read_Config(SystemConfig_t *cfg) ;

uint8_t readCoils_mosbusTCP(void);
uint8_t readInputs_mosbusTCP(void);
uint8_t writeSingleCoil_mosbusTCP(void);

void parse_JSON_RX2(uint8_t *RX2);
void Parse_JSON_TCP(void const *argument);


#endif /* INC_C_TCPPROTOCOLTASK_C_ */
