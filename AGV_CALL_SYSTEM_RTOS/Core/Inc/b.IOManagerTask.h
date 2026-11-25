/*
 * b.IOManagerTask.h
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */

#ifndef INC_B_IOMANAGERTASK_H_
#define INC_B_IOMANAGERTASK_H_

#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"

extern GPIO GPIO_INPUT[8];
extern GPIO GPIO_OUTPUT[8];
extern GPIO GPIO_BUTTON[4];
extern GPIO GPIO_BUTTON_LED[4];

void IOManagerTaskFunction(void const *argument);
void Read_Input(void);
void Write_Output(void);
void Write_Button(void);

#endif /* INC_B_IOMANAGERTASK_H_ */
