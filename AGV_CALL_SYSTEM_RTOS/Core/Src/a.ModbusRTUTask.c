/*
 * a.ModbusRTUTask.c
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */
#include "main.h"
#include "a.ModbusRTUTask.h"
#include "c.TCPProtocolTask.h"
#include "string.h"

extern osMailQId IOTaskReceive_MQ;
extern osMailQId IOTaskTransferToRTU_MQ;

extern uint8_t RX5_Buffer[RX5_SIZE];
extern uint8_t TX5_Buffer[TX5_SIZE];
extern UART_HandleTypeDef huart5;

uint8_t Output_RTUDatabase = 0, Button_RTUDatabase = 0, Input_RTUDatabase = 0;
uint16_t Coils_Database;
extern uint8_t  author_Button, author_Output;

extern uint8_t Slave_ID;
osEvent MailRTUEvent;
osEvent RTU_Task_Flag;
IO_State_t *RTU_Receive_IO_State;
IO_State_t *RTUTransferToIOMail;

void sendDataRTU(uint8_t *data, int size)
{
    uint16_t crc = crc16(data, size);
    data[size]   = crc & 0xFF;       // CRC low
    data[size+1] = crc >> 8;        // CRC high
    HAL_UART_Transmit(&huart5, data, size+2, 10);
}

void modbusException (uint8_t exceptioncode)
{
	//| SLAVE_ID | FUNCTION_CODE | Exception code |    CRC    |
	//| 1 BYTE      |  1 BYTE                   |    1 BYTE             | 2 BYTES |

	TX5_Buffer[0]  = RX5_Buffer[0];          // slave ID
	TX5_Buffer[1]  = RX5_Buffer[1] | 0x80;  // adding 1 to the MSB of the function code
	TX5_Buffer[2]  = exceptioncode;         // Load the Exception code
	sendDataRTU(TX5_Buffer, 3);                    // send Data... CRC will be calculated in the function
}

uint8_t readCoils_ModbusRTU (void)
{
	uint16_t startAddr = ((RX5_Buffer[2] << 8) | RX5_Buffer[3]);
	uint16_t numCoils = ((RX5_Buffer[4] << 8) | RX5_Buffer[5]);

	if ((numCoils < 1))
	{
		modbusException (ILLEGAL_DATA_VALUE);
		return 0;
	}

	Coils_Database = ((uint16_t)Button_RTUDatabase << 8)|Output_RTUDatabase;

	uint16_t endAddr = startAddr + numCoils - 1;
	if (endAddr >= 16)
	{
		modbusException(ILLEGAL_DATA_ADDRESS);
		return 0;
	}
	memset (TX5_Buffer, '\0', TX5_SIZE);
	// Prepare TxData buffer
	//| SLAVE_ID | FUNCTION_CODE | BYTE COUNT | DATA           |   CRC      |
	//| 1 BYTE      |  1 BYTE                   |  1 BYTE           | N*2 BYTES  | 2 BYTES |
	TX5_Buffer[0] = Slave_ID;
	TX5_Buffer[1] = RX5_Buffer[1];
	TX5_Buffer[2] = (numCoils / 8) + ((numCoils % 8) > 0 ? 1:0);
	uint8_t BYTE_COUNT = TX5_Buffer[2];

	int indx = 3;
	uint16_t bitPosition = startAddr;
	int indxPosition = 0;

	// Load the actual data into TxData buffer
	for (int i = 0; i < numCoils; i++)
	{
		TX5_Buffer[indx] |= ((Coils_Database >> bitPosition) & 0x01) << indxPosition;
		indxPosition++;
		bitPosition++;

		if (indxPosition == 8 )
		{
			indxPosition = 0;
			indx++;
		}
	}

	sendDataRTU(TX5_Buffer, 3+ BYTE_COUNT);
	return 1;
}

uint8_t readInputs_ModbusRTU(void)
{
    uint16_t startAddr = (RX5_Buffer[2] << 8) | RX5_Buffer[3];
    uint16_t numCoils  = (RX5_Buffer[4] << 8) | RX5_Buffer[5];
    /* Limited with 8 input */
    if (numCoils < 1 || numCoils > 8)
    {
        modbusException(ILLEGAL_DATA_VALUE);
        return 0;
    }
    if ((startAddr + numCoils) > 8)
    {
        modbusException(ILLEGAL_DATA_ADDRESS);
        return 0;
    }

    memset(TX5_Buffer, 0, TX5_SIZE);
    /* Header Modbus */
    TX5_Buffer[0] = Slave_ID;
    TX5_Buffer[1] = RX5_Buffer[1];     // FC = 0x02
    TX5_Buffer[2] = 1;
    uint8_t data = (Input_RTUDatabase >> startAddr) & ((1 << numCoils) - 1);
    TX5_Buffer[3] = data;

    sendDataRTU(TX5_Buffer, 4); // 3 header + 1 data
    return 1;
}

uint8_t writeSingleCoil_ModbusRTU(void)
{
    uint16_t coilAddr   = ((RX5_Buffer[2]<<8)|RX5_Buffer[3]);
    uint16_t coilValue  = ((RX5_Buffer[4]<<8)|RX5_Buffer[5]);

    if (coilAddr > 199)
    {
        modbusException(ILLEGAL_DATA_ADDRESS);
        return 0;
    }
    if ((coilValue != 0xFF00) && (coilValue != 0x0000))
    {
        modbusException(ILLEGAL_DATA_VALUE);
        return 0;
    }

    // Write to Coils_Database
    uint8_t bitIndex    = coilAddr % 16;

    if(bitIndex < 8){
    	author_Output = 1;
		if (coilValue == 0xFF00)
			Output_RTUDatabase |= (1 << bitIndex);   // Set bit
		else
			Output_RTUDatabase &= ~(1 << bitIndex);  // Clear bit
	}
    else{
    	if (coilValue == 0xFF00)
			Button_RTUDatabase |= (1 << (bitIndex - 8));   // Set bit
		else{
			Button_RTUDatabase &= ~(1 << (bitIndex - 8));  // Clear bit
		}
    	author_Button = 1;
    }

    RTUTransferToIOMail = osMailAlloc(IOTaskReceive_MQ, 0);
    if(RTUTransferToIOMail){
		//memset(RTUTransferToIOMail, 0, sizeof(IO_State_t));
		RTUTransferToIOMail -> Output_Data = Output_RTUDatabase;
		RTUTransferToIOMail -> Button_Data  = Button_RTUDatabase;
		osMailPut(IOTaskReceive_MQ, RTUTransferToIOMail);
	}
    return 1;
}

void Modbus_RTU_Communication(void const *argument){
	while(1){
		RTU_Task_Flag = osSignalWait(SIG_IT_UART5 | SIG_IOQUEUE_TO_RTU, osWaitForever);
		if(RTU_Task_Flag.value.signals & SIG_IT_UART5){
			switch (RX5_Buffer[1])
			{
				case 0x01:
					readCoils_ModbusRTU();
					break;
				case 0x02:
					readInputs_ModbusRTU();
					break;
				case 0x05:
					writeSingleCoil_ModbusRTU();
					Button_RTUDatabase = 0;
					break;
				default:
					modbusException(ILLEGAL_FUNCTION);
			}
			memset(RX5_Buffer, 0, 10);
		}
		if(RTU_Task_Flag.value.signals & SIG_IOQUEUE_TO_RTU){ // Update new data from other task
			MailRTUEvent = osMailGet(IOTaskTransferToRTU_MQ, 10);
			if(MailRTUEvent.status == osEventMail ){
				RTU_Receive_IO_State = (IO_State_t*)MailRTUEvent.value.p;
				Input_RTUDatabase     = RTU_Receive_IO_State -> Input_Data;       // Read from mail
				Output_RTUDatabase  = RTU_Receive_IO_State -> Output_Data;   // Read from mail
				Button_RTUDatabase  = RTU_Receive_IO_State -> Button_Data;    // Read from mail
				osMailFree(IOTaskTransferToRTU_MQ, RTU_Receive_IO_State);
			}
		}
	}
}
