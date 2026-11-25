/*
 * e.ATcommandTask.h
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */

#ifndef INC_E_ATCOMMANDTASK_H_
#define INC_E_ATCOMMANDTASK_H_

#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"

void wifi_pulse(uint32_t low_delay_ms, uint32_t high_delay_ms);
void send_DefaultConfig();
void send_ATcommand(const char *command[], size_t len);
void send_AT_STA_AP(int dhcp_enabled);
void reset_config(void);
void send_AT_Serial3();
void send_AT_Serial3_Serial0() ;
void Reset_Wifi();
void ATcommand_Function(void const *argument);

#endif /* INC_E_ATCOMMANDTASK_H_ */
