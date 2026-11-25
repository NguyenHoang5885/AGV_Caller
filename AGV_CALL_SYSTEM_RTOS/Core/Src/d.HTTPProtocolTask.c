#include "main.h"
#include <string.h>
#include <stdio.h>
#include "a.ModbusRTUTask.h"
#include "b.IOManagerTask.h"
#include "c.TCPProtocolTask.h"
#include "d.HTTPProtocolTask.h"

extern osMailQId IOTaskReceive_MQ;
extern osMailQId IOTaskTransferToHTTP_MQ;

extern UART_HandleTypeDef huart3;
extern uint8_t RX3_Buffer[RX3_SIZE];

char json_str[350];
char http_request[450];
uint8_t txFMSserver[10];

//HTTP
extern int Port_HTTP; //From c.TCPProtocolTask.h
extern char Pass_HTTP[20], Event[80], Ip_HTTP[32], User[20]; //From c.TCPProtocolTask.h
extern char Tk1[20], Tk2[20], Tk3[20], Tk4[20], Pt1[160], Pt2[160], Pt3[160], Pt4[160];//From c.TCPProtocolTask.h
char auth_raw[64];
char auth_base64[64];

//HTTP DATABASE
uint8_t Output_HTTPDatabase, Button_HTTPDatabase, Input_HTTPDatabase, Led_HTTPDatabase; //Mutual
uint8_t OldOutput_HTTPDatabase, OldButton_HTTPDatabase, OldInput_HTTPDatabase; // HIK ROBOT

//Choose Server FMS or HIK
extern uint8_t On_FMS;
extern uint8_t St_ID;

//Send button or input to Server
extern uint8_t author_Input, author_Button, author_Output, server_reply;

//JSON HIK
/* ================= BASE64 ENCODE (ONE FILE) ================= */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64_encode(const unsigned char *src, unsigned int len, char *out)
{
    unsigned int i = 0, j = 0;
    unsigned char a3[3];
    unsigned char a4[4];

    while (len--) {
        a3[i++] = *(src++);
        if (i == 3) {
            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2)  | ((a3[2] & 0xc0) >> 6);
            a4[3] =  a3[2] & 0x3f;

            out[j++] = b64_table[a4[0]];
            out[j++] = b64_table[a4[1]];
            out[j++] = b64_table[a4[2]];
            out[j++] = b64_table[a4[3]];
            i = 0;
        }
    }

    if (i) {
        for (unsigned int k = i; k < 3; k++) a3[k] = '\0';
        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
        a4[3] =  a3[2] & 0x3f;
        for (unsigned int k = 0; k < i + 1; k++) out[j++] = b64_table[a4[k]];
        while (i++ < 3) out[j++] = '=';
    }
    out[j] = '\0';
}
/* ================= END BASE64 ================= */

//------------------------UART3------------------------------------------------------
//FMS SERVER
void Button_IO_status(uint8_t ID_FMS) {
	txFMSserver[0] = 0x00;
	txFMSserver[1] = ID_FMS;
	txFMSserver[2] = 0x00;
	txFMSserver[3] = 0x00;
	txFMSserver[4] = 0x00;
	txFMSserver[5] = 0x00;
	txFMSserver[6] = 0x00;
	txFMSserver[7] = Button_HTTPDatabase & 0x0F;     // 0x01, 0x02, 0x04, 0x08...
	uint16_t crc = crc16(txFMSserver, 8);
	txFMSserver[8] = crc & 0xFF;         // CRC Low
	txFMSserver[9] = (crc >> 8) & 0xFF;  // CRC High
}
//------------------------UART3------------------------------------------------------
//FMS SERVER
void Button_Data0_status(uint8_t ID_FMS) {
	txFMSserver[0] = 0x00;
	txFMSserver[1] = ID_FMS;
	txFMSserver[2] = 0x00;
	txFMSserver[3] = 0x00;
	txFMSserver[4] = 0x00;
	txFMSserver[5] = 0x00;
	txFMSserver[6] = 0x00;
	txFMSserver[7] = 0x00;     // 0x01, 0x02, 0x04, 0x08...
	uint16_t crc = crc16(txFMSserver, 8);
	txFMSserver[8] = crc & 0xFF;         // CRC Low
	txFMSserver[9] = (crc >> 8) & 0xFF;  // CRC High
}

void InputOutput_status(uint8_t ID_FMS, uint8_t InOut) {
	txFMSserver[0] = 0x00;
	txFMSserver[1] = ID_FMS;
	txFMSserver[2] = 0x00;
	txFMSserver[3] = 0x00;
	txFMSserver[4] = 0x00;
	txFMSserver[5] = 0x00;
	txFMSserver[6] = 0x00;
	txFMSserver[7] = InOut; // 0x01, 0x02, 0x04, 0x08...
	uint16_t crc = crc16(txFMSserver, 8);
	txFMSserver[8] = crc & 0xFF;              // CRC Low
	txFMSserver[9] = (crc >> 8) & 0xFF;  // CRC High
}

void ConvertPathToJsonArray(const char *src, char *dst, size_t dst_size)
{
    char buf[160];
    char *token;
    size_t len = 0;

    if (src == NULL || dst == NULL || dst_size == 0)
        return;

    strncpy(buf, src, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    // Bắt đầu mảng JSON
    len += snprintf(dst + len, dst_size - len, "[");
    token = strtok(buf, ",");

    while (token != NULL)
    {
        while (*token == ' ') token++;
        if (len > 1) len += snprintf(dst + len, dst_size - len, ","); // Add ',' among objects
        // Add object
        len += snprintf(dst + len, dst_size - len, "\"%s\"", token); // "{\"positionCode\":\"%s\",\"type\":\"00\"}"
        token = strtok(NULL, ",");
    }

    snprintf(dst + len, dst_size - len, "]");
}

uint8_t Press;
//void Button_IO_status_HIKROBOT(void){ // Code Iplus Robot (Quang)
//	Press = Button_HTTPDatabase;
//	memset(json_str, 0, sizeof(json_str));
//
//	switch (Press) // Gửi JSON Button1 - 4 của HIK SERVER
//	{
//	    case 0x01:
//	    	sprintf(json_str, "{\"from\":\"carly\",\"task_list\":[{\"action\":\"%s\"}]}", Pt1); //"{\"reqCode\":\"%d\",""\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}"
//	        break;
//	    case 0x02:
//	    	sprintf(json_str, "{\"from\":\"carly\",\"task_list\":[{\"action\":\"%s\"}]}", Pt2);
//	        break;
//	    case 0x04:
//	    	sprintf(json_str, "{\"from\":\"carly\",\"task_list\":[{\"action\":\"%s\"}]}", Pt3);
//	        break;
//	    case 0x08:
//	    	sprintf(json_str, "{\"from\":\"carly\",\"task_list\":[{\"action\":\"%s\"}]}", Pt4);
//	        break;
//	    default:
//	        break;
//	}
//	Press = 0;
//}

void Button_IO_status_HIKROBOT(void){ // HIK Robot
	Press = Button_HTTPDatabase;
	char path_json[254];
	memset(json_str, 0, sizeof(json_str));

	switch (Press) // Gửi JSON Button1 - 4 của HIK SERVER
	{
	    case 0x01:
	    	ConvertPathToJsonArray(Pt1, path_json, sizeof(path_json));
	    	sprintf(json_str, "{\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}", Tk1, path_json); //"{\"reqCode\":\"%d\",""\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}"
	        break;
	    case 0x02:
	    	ConvertPathToJsonArray(Pt2, path_json, sizeof(path_json));
	    	sprintf(json_str, "{\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}", Tk2, path_json);
	        break;
	    case 0x04:
	    	ConvertPathToJsonArray(Pt3, path_json, sizeof(path_json));
	    	sprintf(json_str, "{\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}", Tk3, path_json);
	        break;
	    case 0x08:
	    	ConvertPathToJsonArray(Pt4, path_json, sizeof(path_json));
	    	sprintf(json_str, "{\"taskTyp\":\"%s\",""\"userCallCodePath\":%s}", Tk4, path_json);
	        break;
	    default:
	        break;
	}
	Press = 0;
}

void Send_IOStateToServer(void const *argument)
{
	osEvent MailHTTPEvent;
	osEvent HTTP_Task_Flag;
	IO_State_t *HTTP_Receive_IO_State;
	IO_State_t *HTTPTransferToIOMail;
	while(1){
		HTTP_Task_Flag = osSignalWait( SIG_DMA_UART3|SIG_IOQUEUE_TO_HTTP, osWaitForever); // Block to Wait flag

		if(HTTP_Task_Flag.value.signals & SIG_DMA_UART3){  //SIGNAL HARDWARE INTERRUPT
			//Analize data from UART 3 (SERVER FMS)
			if(RX3_Buffer[0] == St_ID ){
				if(RX3_Buffer[1] == 0x05){
					uint8_t led = RX3_Buffer[3];
					if (RX3_Buffer[4] == 0x00){ // LED Turn off
						Led_HTTPDatabase &= ~(1 << led);
						//GPIO_BUTTON_LED[led].port ->  BSRR = GPIO_BUTTON_LED[led].pin << 16; // RESET BIT 0
						server_reply = 1;
						author_Button = 0;
						//Button_HTTPDatabase = 0;
					}
					else{// LED Turn on
						Led_HTTPDatabase |= (1 << led);
						GPIO_BUTTON_LED[led].port ->  BSRR = GPIO_BUTTON_LED[led].pin; // SET BIT 1
						server_reply = 1;
						author_Button = 0;
						//Button_HTTPDatabase = 0;
					}
				}
				if(RX3_Buffer[1] == 0x07)
				{
					Output_HTTPDatabase = RX3_Buffer[3];
					server_reply = 1;
					author_Output = 0;
				}
			}
			HTTPTransferToIOMail = osMailAlloc(IOTaskReceive_MQ, 0);
			if(HTTPTransferToIOMail){
				memset(HTTPTransferToIOMail, 0, sizeof(IO_State_t));
				HTTPTransferToIOMail -> Led_Data = Led_HTTPDatabase;
				HTTPTransferToIOMail -> Output_Data = Output_HTTPDatabase;
//				HTTPTransferToIOMail -> Button_Data  = Button_HTTPDatabase;
				osMailPut(IOTaskReceive_MQ, HTTPTransferToIOMail);
			}
			memset(RX3_Buffer, 0, RX3_SIZE);
		}

		if(HTTP_Task_Flag.value.signals & SIG_IOQUEUE_TO_HTTP){ // DONE SIGNAL SOFTWARE INTERRUPT
			MailHTTPEvent = osMailGet(IOTaskTransferToHTTP_MQ, 10);
			if(MailHTTPEvent.status == osEventMail ){
				HTTP_Receive_IO_State = (IO_State_t*)MailHTTPEvent.value.p;
				Input_HTTPDatabase     = HTTP_Receive_IO_State -> Input_Data;       // Read from mail
				Output_HTTPDatabase  = HTTP_Receive_IO_State -> Output_Data;   // Read from mail
				Button_HTTPDatabase  = HTTP_Receive_IO_State -> Button_Data;    // Read from mail
				osMailFree(IOTaskTransferToHTTP_MQ, HTTP_Receive_IO_State);
			}
			sprintf(auth_raw, "%s:%s", User, Pass_HTTP);
			base64_encode((unsigned char*)auth_raw, strlen(auth_raw), auth_base64);
			memset(http_request, 0, sizeof(http_request));

			// SEND
			if(On_FMS == 1){ //FMS SERVER - SOCKET TCP
				if(author_Button == 1){
					//author_Button = 0;
					server_reply = 0;
					Button_IO_status(0x42); //Send Button state
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
					HAL_Delay(5);
					Button_IO_status(0x45); //Send Led state
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
					HAL_Delay(5);
					Button_Data0_status(0x42);
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
					HAL_Delay(5);
					Button_Data0_status(0x45);
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
//					Button_HTTPDatabase = 0;
				}
				if(author_Output == 1){
					//author_Output = 0;
					server_reply = 0;
					InputOutput_status(0x49, Output_HTTPDatabase);
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
				}
				if(author_Input == 1){
					author_Input = 0;
//					server_reply = 0;
					InputOutput_status(0x47,Input_HTTPDatabase); //Send Led state
					HAL_UART_Transmit_DMA(&huart3, txFMSserver, 10);
				}
			}
			else{ //HIK SERVER - HTTP
				if(Button_HTTPDatabase != 0){
					Button_IO_status_HIKROBOT();
					int idx = 0;
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "POST /%s HTTP/1.1\r\n", Event);
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "Host: %s:%d\r\n", Ip_HTTP, Port_HTTP);
	//				idx += snprintf(http_request + idx, sizeof(http_request) - idx, "Authorization: Basic %s\r\n", auth_base64);
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "Content-Type: application/json\r\n");
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "Content-Length: %d\r\n", strlen(json_str));
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "Connection: close\r\n\r\n");
					idx += snprintf(http_request + idx, sizeof(http_request) - idx, "%s", json_str);

					HAL_UART_Transmit_DMA(&huart3, (uint8_t*)http_request, idx);
				}
			}
		}
	}
}
