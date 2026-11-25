#include "main.h"
#include "c.TCPProtocolTask.h"
#include "a.ModbusRTUTask.h"

extern osMailQId IOTaskReceive_MQ;
extern osMailQId IOTaskTransferToTCP_MQ;

extern uint8_t RX2_Buffer[RX2_SIZE];
extern volatile uint16_t RX2_ActualSize;
extern uint8_t TX2_Buffer[TX2_SIZE];

extern UART_HandleTypeDef huart2;
extern osThreadId ATCOMMAND_TASKHandle;
char json_str_app[64];

//***Only this task is allowed write this data***//
//***STA***
uint8_t Dhcp; // Allowed to use in AT task
char Ssid[32], Pass_STA[32], Ip_STA[20], Mask[20], Gateway[20];  // STA Allowed to use in AT task
char Ip_Eth[20], Mask_Eth[20];

//***HTTP***
uint8_t On_FMS = 1;
uint8_t St_ID = 1;
int          Port_HTTP; //****PORT HTTP DEFAUFT
char Endpoint[32], Pass_HTTP[20], Event[80], Ip_HTTP[32], User[20];  // Allowed to use in AT task + HTTP Task Token[60]
char Tk1[20], Tk2[20], Tk3[20], Tk4[20], Pt1[160], Pt2[160], Pt3[160], Pt4[160];

//***RTU + TCP***
uint8_t  On_TCP, On_RTU;
uint8_t  Slave_ID = 1;          //****ID RTU DEFAUFT
int          Port_TCP;    //****PORT TCP DEFAUFT
SystemConfig_t g_config;

uint8_t int_Send_STA_AP = 0, int_http = 0, int_fms = 0, int_rtu = 0, int_app = 0;

//***DATABASE***
uint8_t Output_TCPDatabase, Button_TCPDatabase, Input_TCPDatabase, Led_TCPDatabase;
extern uint8_t  author_Button, author_Output;

// Khai báo flask
void Flash_Write_Config(SystemConfig_t *cfg) // Write Flask Function
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = CONFIG_ADDR;
    erase.NbPages = 1;

    HAL_FLASHEx_Erase(&erase, &error);

    uint32_t *data = (uint32_t*)cfg;

    for (int i = 0; i < sizeof(SystemConfig_t)/4; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          CONFIG_ADDR + i*4,
                          data[i]);
    }

    HAL_FLASH_Lock();
}
void Flash_Read_Config(SystemConfig_t *cfg) // Read flask Function
{
    memcpy(cfg, (void*)CONFIG_ADDR, sizeof(SystemConfig_t));
}

//------------------------------------
void parse_JSON_RX2(uint8_t *RX2){
	char *RX2_str = (char *)RX2;

	//Frame JSON : IO: {"type":"io","out":13,"button":5}
//	if (strstr(RX2_str, "\"type\":\"io\"")){
//		sscanf(strstr(RX2_str, "\"out\""), "\"out\":%hhu", &Output_TCPDatabase);
//		sscanf(strstr(RX2_str, "\"button\""), "\"button\":%hhu", &Button_TCPDatabase);
//		int_app = 1;
//		IO_State_t *TransferToIOMail;
//		TransferToIOMail = osMailAlloc(IOTaskReceive_MQ, 0);
//		if(TransferToIOMail){
//			memset(TransferToIOMail, 0, sizeof(IO_State_t));
//			TransferToIOMail -> Output_Data = Output_TCPDatabase;
//			TransferToIOMail -> Button_Data  = Button_TCPDatabase;
//			osMailPut(IOTaskReceive_MQ, TransferToIOMail);
//		}
//	}
	//WIFI CONFIGURATION /
	if (strstr(RX2_str, "\"type\":\"sta\"")){ //Send to 1 task AT
		int_Send_STA_AP = 1;
		sscanf(strstr(RX2_str, "\"ipe\""), "\"ipe\":\"%19[^\"]\"", Ip_Eth);
		sscanf(strstr(RX2_str, "\"maske\""), "\"maske\":\"%19[^\"]\"", Mask_Eth);
		sscanf(strstr(RX2_str, "\"dhcp\""), "\"dhcp\":%hhu", &Dhcp);
		sscanf(strstr(RX2_str, "\"ssid\""), "\"ssid\":\"%31[^\"]\"", Ssid);
		sscanf(strstr(RX2_str, "\"pass\""), "\"pass\":\"%31[^\"]\"", Pass_STA);
		sscanf(strstr(RX2_str, "\"ip\""), "\"ip\":\"%19[^\"]\"", Ip_STA);
		sscanf(strstr(RX2_str, "\"mask\""), "\"mask\":\"%19[^\"]\"", Mask);
		sscanf(strstr(RX2_str, "\"gateway\""), "\"gateway\":\"%19[^\"]\"", Gateway);
		osSignalSet(ATCOMMAND_TASKHandle, SIG_ACCESS_AT);
	}
	//HTTP
	if (strstr(RX2_str, "\"type\":\"http\"")){ //Send to 2 task AT + HTTP
		int_http = 1;
//		memset(Token, 0,sizeof(Token));
		memset(Endpoint, 0,sizeof(Endpoint));
		memset(Pass_HTTP, 0,sizeof(Pass_HTTP));
		memset(Event, 0,sizeof(Event));
		memset(Ip_HTTP, 0,sizeof(Ip_HTTP));
		memset(User, 0,sizeof(User));
		Port_HTTP = 0;
		St_ID = 0;

		sscanf(strstr(RX2_str, "\"FMS\""), "\"FMS\":%hhu", &On_FMS);
		sscanf(strstr(RX2_str, "\"stid\""), "\"stid\":%hhu", &St_ID);
//		sscanf(strstr(RX2_str, "\"token\""), "\"token\":\"%59[^\"]\"", Token); //access_token -> token
		sscanf(strstr(RX2_str, "\"endpoint\""), "\"endpoint\":\"%31[^\"]\"", Endpoint);// get_endpoint -> endpoint
		sscanf(strstr(RX2_str, "\"pass\""), "\"pass\":\"%19[^\"]\"", Pass_HTTP); //password -> pass
		sscanf(strstr(RX2_str, "\"port\""), "\"port\":%d", &Port_HTTP);
		sscanf(strstr(RX2_str, "\"event\""), "\"event\":\"%79[^\"]\"", Event);      //set_event - > event
		sscanf(strstr(RX2_str, "\"ip\""), "\"ip\":\"%31[^\"]\"", Ip_HTTP);
		sscanf(strstr(RX2_str, "\"user\""), "\"user\":\"%19[^\"]\"", User);
		osSignalSet(ATCOMMAND_TASKHandle, SIG_ACCESS_AT);
	}

	if (strstr(RX2_str, "\"type\":\"server\"")){ //Send to 2 task AT + HTTP
		sscanf(strstr(RX2_str, "\"FMS\""), "\"FMS\":%hhu", &On_FMS);
		int_fms = 1;
		if(On_FMS == 0){
			memset(Tk1, 0,sizeof(Tk1));
			memset(Tk2, 0,sizeof(Tk2));
			memset(Tk3, 0,sizeof(Tk3));
			memset(Tk4, 0,sizeof(Tk4));
			memset(Pt1, 0,sizeof(Pt1));
			memset(Pt2, 0,sizeof(Pt2));
			memset(Pt3, 0,sizeof(Pt3));
			memset(Pt4, 0,sizeof(Pt4));

			sscanf(strstr(RX2_str, "\"tk1\""), "\"tk1\":\"%19[^\"]\"", Tk1); //task1_BT1
			sscanf(strstr(RX2_str, "\"tk2\""), "\"tk2\":\"%19[^\"]\"", Tk2); //task2_BT2
			sscanf(strstr(RX2_str, "\"tk3\""), "\"tk3\":\"%19[^\"]\"", Tk3); //task3_BT3
			sscanf(strstr(RX2_str, "\"tk4\""), "\"tk4\":\"%19[^\"]\"", Tk4); //task4_BT4
			sscanf(strstr(RX2_str, "\"pt1\""), "\"pt1\":\"%159[^\"]\"", Pt1); //task1_BT1
			sscanf(strstr(RX2_str, "\"pt2\""), "\"pt2\":\"%159[^\"]\"", Pt2); //task2_BT2
			sscanf(strstr(RX2_str, "\"pt3\""), "\"pt3\":\"%159[^\"]\"", Pt3); //task3_BT3
			sscanf(strstr(RX2_str, "\"pt4\""), "\"pt4\":\"%159[^\"]\"", Pt4); //task4_BT4
		}
	}

	// TCP / RTU
	if (strstr(RX2_str, "\"type\":\"tcp\"")){ //Send to 1 task RTU + TCP
		int_rtu = 1;
		Port_TCP = 0;
		sscanf(strstr(RX2_str, "\"on\"")   , "\"on\":%hhu", &On_TCP);
		sscanf(strstr(RX2_str, "\"port\""), "\"port\":%d" , &Port_TCP);
		if(On_TCP == 1){
			On_RTU = 0;
			osSignalSet(ATCOMMAND_TASKHandle, SIG_ACCESS_AT);
		}
	}
	else if(strstr(RX2_str, "\"type\":\"rtu\"")){
		sscanf(strstr(RX2_str, "\"id\"") , "\"id\":%hhu", &Slave_ID);
		sscanf(strstr(RX2_str, "\"on\""), "\"on\":%hhu", &On_RTU);
		if(On_RTU == 1){
			On_TCP = 0;
		}
	}

	//Ghi flask
	if (int_http == 1)
	{
	    int_http = 0;
	    g_config.On_FMS_Flask   = On_FMS;
	    g_config.Port_HTTP_Flask = Port_HTTP;
	    g_config.St_ID_Flask = St_ID;
//	    strncpy(g_config.Token_Flask, Token, sizeof(g_config.Token_Flask)-1);
	    strncpy(g_config.Endpoint_Flask, Endpoint, sizeof(g_config.Endpoint_Flask)-1);
	    strncpy(g_config.Pass_HTTP_Flask, Pass_HTTP, sizeof(g_config.Pass_HTTP_Flask)-1);
	    strncpy(g_config.Event_Flask, Event, sizeof(g_config.Event_Flask)-1);
	    strncpy(g_config.Ip_HTTP_Flask, Ip_HTTP, sizeof(g_config.Ip_HTTP_Flask)-1);
	    strncpy(g_config.User_Flask, User, sizeof(g_config.User_Flask)-1);

	    g_config.magic = 0xA5A5A5A5;
	    Flash_Write_Config(&g_config);
	}

	if (int_fms == 1) // FMS
	{
		int_fms  = 0;
		g_config.On_FMS_Flask   = On_FMS;

		strncpy(g_config.Tk1_Flask, Tk1, sizeof(g_config.Tk1_Flask)-1);
		strncpy(g_config.Tk2_Flask, Tk2, sizeof(g_config.Tk2_Flask)-1);
		strncpy(g_config.Tk3_Flask, Tk3, sizeof(g_config.Tk3_Flask)-1);
		strncpy(g_config.Tk4_Flask, Tk4, sizeof(g_config.Tk4_Flask)-1);
		strncpy(g_config.Pt1_Flask, Pt1, sizeof(g_config.Pt1_Flask)-1);
		strncpy(g_config.Pt2_Flask, Pt2, sizeof(g_config.Pt2_Flask)-1);
		strncpy(g_config.Pt3_Flask, Pt3, sizeof(g_config.Pt3_Flask)-1);
		strncpy(g_config.Pt4_Flask, Pt4, sizeof(g_config.Pt4_Flask)-1);

		g_config.magic = 0xA5A5A5A5;
		Flash_Write_Config(&g_config);
	}

	if (int_rtu == 1)
	{
	    int_rtu = 0;

	    g_config.On_TCP_Flask   = On_TCP;
	    g_config.On_RTU_Flask   = On_RTU;
	    g_config.Slave_ID_Flask = Slave_ID;
	    g_config.Port_TCP_Flask = Port_TCP;

	    g_config.magic = 0xA5A5A5A5;
	    Flash_Write_Config(&g_config);
	}
}
/*===============MOSBUS TCP================================================*/
uint8_t readCoils_mosbusTCP(void)
{
	memcpy(TX2_Buffer, RX2_Buffer, 6);   // MBAP
	TX2_Buffer[6] = RX2_Buffer[6];       // Unit ID

	TX2_Buffer[7] = 0x01;   // FC01
	TX2_Buffer[8] = 2;         // Byte count

	TX2_Buffer[9]   = Output_TCPDatabase;
	TX2_Buffer[10] = Button_TCPDatabase;

	uint16_t len = 3 + 2;   // UnitID + FC + BC + DATA
	TX2_Buffer[4] = len >> 8;
	TX2_Buffer[5] = len & 0xFF;

	uint8_t TX2_Len = 6 + len;

	HAL_UART_Transmit_DMA(&huart2, TX2_Buffer, TX2_Len);
	return 1;
}
// uint8_t Output_TCPDatabase, Button_TCPDatabase, Input_TCPDatabase;
uint8_t readInputs_mosbusTCP(void)
{
    memcpy(TX2_Buffer, RX2_Buffer, 6);   // MBAP
    TX2_Buffer[6] = RX2_Buffer[6];       // Unit ID

    TX2_Buffer[7] = 0x02;  // FC02
    TX2_Buffer[8] = 0x01;  // Byte count = 1
    TX2_Buffer[9] = Input_TCPDatabase;

    TX2_Buffer[4] = 0x00;
    TX2_Buffer[5] = 0x04;  // Length = 4

    uint8_t TX2_Len = 10;

    HAL_UART_Transmit_DMA(&huart2, TX2_Buffer, TX2_Len);
    return 1;
}
// uint8_t Output_TCPDatabase, Button_TCPDatabase, Input_TCPDatabase;
uint8_t writeSingleCoil_mosbusTCP(void)
{
    uint16_t coil_addr = (RX2_Buffer[8] << 8)   | RX2_Buffer[9];
    uint16_t value        = (RX2_Buffer[10] << 8) | RX2_Buffer[11];

    if (coil_addr >= 16)
        return 0;

    if (coil_addr < 8)   // OUTPUT
    {
    	author_Output = 1;
        if (value == 0xFF00) {
        	Output_TCPDatabase |= (1U << coil_addr);
        }
        else if (value == 0x0000) {
        	Output_TCPDatabase &= ~(1U << coil_addr);
        }
        else return 0;
    }
    else                 // BUTTON LED
    {
        uint8_t bt_index = coil_addr - 8;
        author_Button = 1;
        if (value == 0xFF00) {
        	Button_TCPDatabase |= (1U << bt_index);
        }
        else if (value == 0x0000) {
        	Button_TCPDatabase &= ~(1U << bt_index);
        }
        else return 0;
    }

    /* Response = echo request */
    IO_State_t *TransferToIOMail;
	TransferToIOMail = osMailAlloc(IOTaskReceive_MQ, 0);
	if(TransferToIOMail){
		//memset(TransferToIOMail, 0, sizeof(IO_State_t));
		TransferToIOMail -> Output_Data = Output_TCPDatabase;
		TransferToIOMail -> Button_Data  = Button_TCPDatabase;
		osMailPut(IOTaskReceive_MQ, TransferToIOMail);
	}
    return 1;
}

void Parse_JSON_TCP(void const *argument){
	osEvent MailTCPEvent;
	osEvent TCP_Task_Flag;
	IO_State_t *TCP_Receive_IO_State;
	while(1){
		TCP_Task_Flag = osSignalWait(SIG_DMA_UART2 | SIG_IOQUEUE_TO_TCP, osWaitForever);
		if(TCP_Task_Flag.value.signals & SIG_IOQUEUE_TO_TCP){
			MailTCPEvent = osMailGet(IOTaskTransferToTCP_MQ, 10);
			if(MailTCPEvent.status == osEventMail ){
				TCP_Receive_IO_State = (IO_State_t*)MailTCPEvent.value.p;
				Input_TCPDatabase     = TCP_Receive_IO_State -> Input_Data;    // Read from mail
				Led_TCPDatabase        = TCP_Receive_IO_State -> Led_Data;       // Read from mail
				Output_TCPDatabase = TCP_Receive_IO_State -> Output_Data; // Read from mail

				memset(json_str_app, 0, sizeof(json_str_app));
				sprintf(json_str_app, "{\"type\":\"io_status\" ,\"IN\": %d,\"BT\":%d,\"OUT\":%d}", Input_TCPDatabase, Led_TCPDatabase, Output_TCPDatabase);    //"{\"btn\": %hhu, \"in\": %hhu, \"out\": %hhu}\r\n"
				HAL_UART_Transmit_DMA(&huart2, (uint8_t*)json_str_app, strlen(json_str_app));
				vTaskDelay(100);

				if(On_TCP == 1){
					readInputs_mosbusTCP();
				}
				osMailFree(IOTaskTransferToTCP_MQ, TCP_Receive_IO_State);
			}
		}
		if(TCP_Task_Flag.value.signals & SIG_DMA_UART2){
			/* ======== JSON========= */
			if (RX2_Buffer[0] == '{')
			{
				RX2_Buffer[RX2_ActualSize] = '\0';
				parse_JSON_RX2(RX2_Buffer);
			}
			/* ===== MODBUS TCP ===== */
			else{
				if(On_TCP == 1)
				{
					if (RX2_ActualSize >= 8)
					{
						// Check Protocol ID = 0x0000
						if (RX2_Buffer[2] == 0x00 && RX2_Buffer[3] == 0x00)
						{
							uint16_t len = (RX2_Buffer[4] << 8) | RX2_Buffer[5];
							uint16_t total_len = 6 + len;
							if (RX2_SIZE >= total_len)
							{
								switch (RX2_Buffer[7])   // Function Code
								{
									case 0x01:
										readCoils_mosbusTCP();
										break;
									case 0x02:
										readInputs_mosbusTCP();
										break;
									case 0x05:
										writeSingleCoil_mosbusTCP();
										Button_TCPDatabase = 0;
										break;
									default:
										modbusException(ILLEGAL_FUNCTION);
										break;
								 }
							}
						}
					}
				}
			}
			memset(RX2_Buffer, 0, RX2_SIZE);
		}
	}
}
