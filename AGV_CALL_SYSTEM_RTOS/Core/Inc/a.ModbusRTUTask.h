/*
 * a.ModbusRTUTask.h
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */

#ifndef INC_A_MODBUSRTUTASK_H_
#define INC_A_MODBUSRTUTASK_H_

#include "modbus_crc.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "string.h"
#include <stdio.h>

#define ILLEGAL_FUNCTION       0x01
#define ILLEGAL_DATA_ADDRESS   0x02
#define ILLEGAL_DATA_VALUE     0x03

uint8_t readCoils_ModbusRTU (void);
uint8_t readInputs_ModbusRTU (void);
uint8_t writeSingleCoil_ModbusRTU(void);

void modbusException (uint8_t exceptioncode);
void Modbus_RTU_Communication(void const *argument);

#endif /* INC_A_MODBUSRTUTASK_H_ */
