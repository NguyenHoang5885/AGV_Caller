/*
 * d.HTTPProtocolTask.h
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */

#ifndef INC_D_HTTPPROTOCOLTASK_H_
#define INC_D_HTTPPROTOCOLTASK_H_

#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"

void base64_encode(const unsigned char *src, unsigned int len, char *out);
void Button_IO_status(uint8_t ID_FMS);
void Send_IOStateToServer(void const *argument);
void Button_IO_status_HIKROBOT(void);
void ConvertPathToJsonArray(const char *src, char *dst, size_t dst_size);

#endif /* INC_D_HTTPPROTOCOLTASK_H_ */
