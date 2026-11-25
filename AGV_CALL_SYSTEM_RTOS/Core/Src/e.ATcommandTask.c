/*
 * e.ATcommandTask.c
 *
 *  Created on: Jan 19, 2026
 *      Author: ASUS
 */
#include "main.h"
#include "c.TCPProtocolTask.h"

extern UART_HandleTypeDef huart3;
extern uint8_t Dhcp;           // from "c.TCPProtocolTask.h"
extern char Ssid[32], Pass_STA[32], Ip_STA[20], Mask[20], Gateway[20]; // from "c.TCPProtocolTask.h"
extern char Ip_Eth[20], Mask_Eth[20];
extern int     Port_HTTP, Port_TCP;     // from "c.TCPProtocolTask.h"
extern char Ip_HTTP[32];  // from "c.TCPProtocolTask.h"

extern uint8_t int_Send_STA_AP, int_http, int_rtu;  // from "c.TCPProtocolTask.h"
uint8_t uart3_tx_done = 0 ;

void wifi_pulse(uint32_t low_delay_ms, uint32_t high_delay_ms) {
	HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_RESET);
	vTaskDelay(low_delay_ms);
	HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_SET);
	vTaskDelay(high_delay_ms);
}

void send_ATcommand(const char *command[], size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        for (uint8_t retry = 0; retry < 3; retry++)
        {
            if (HAL_UART_Transmit(&huart3, (uint8_t *)command[i], strlen(command[i]), 20) == HAL_OK)
            {
                break;
            }
        }
    }
    //vTaskDelay(10);
}

void send_DefaultConfig() {
	static  const char *STA_AP_Mode[32];
    int idx = 0;

    //Mode
    STA_AP_Mode[idx++] = "at+Netmode=3\r\n";
    // AP setup
    STA_AP_Mode[idx++] = "at+Dhcpd=1\r\n";
    STA_AP_Mode[idx++] = "at+DhcpdIpStart=192.168.16.100\r\n";
    STA_AP_Mode[idx++] = "at+DhcpdIpStop=192.168.16.250\r\n";
    STA_AP_Mode[idx++] = "at+DhcpdMask=255.255.255.0\r\n";
    STA_AP_Mode[idx++] = "at+DhcpdTime=86400\r\n";

    STA_AP_Mode[idx++] = "at+APSsid=RTC_Caller_02\r\n";
    STA_AP_Mode[idx++] = "at+APEncType=6\r\n";
    STA_AP_Mode[idx++] = "at+APPasswd=12345678\r\n";
    STA_AP_Mode[idx++] = "at+LANIp=192.168.16.254\r\n";
    STA_AP_Mode[idx++] = "at+LANIpMask=255.255.255.0\r\n";

    // Serial1 & Network settings
    STA_AP_Mode[idx++] = "at+NProType0=1\r\n";
    STA_AP_Mode[idx++] = "at+NLPort0=8080\r\n";
    STA_AP_Mode[idx++] = "at+NTcpTo0=0\r\n";
    STA_AP_Mode[idx++] = "at+SBaud0=115200\r\n";
    STA_AP_Mode[idx++] = "at+SWidth0=8\r\n";
    STA_AP_Mode[idx++] = "at+SPari0=0\r\n";
    STA_AP_Mode[idx++] = "at+SStop0=1\r\n";
    // Save & apply
    STA_AP_Mode[idx++] = "at+Save=1\r\n";
    STA_AP_Mode[idx++] = "at+Apply=1\r\n";
    STA_AP_Mode[idx++] = "at+Reboot=1\r\n";
    send_ATcommand(STA_AP_Mode, idx);
}

void send_AT_STA_AP(int dhcp_enabled) {
	static  char cmd_eth_ip[64];
	static  char cmd_eth_mask[64];

	static  char cmd_sta_ssid[64];
	static  char cmd_sta_pass[64];
	static  char cmd_sta_ip[64];
	static  char cmd_sta_mask[64];
	static  char cmd_sta_gateway[64];

	snprintf(cmd_sta_ssid, sizeof(cmd_sta_ssid), "at+STASsid=%s\r\n", Ssid);
	snprintf(cmd_sta_pass, sizeof(cmd_sta_pass),"at+STAPasswd=%s\r\n", Pass_STA);
	snprintf(cmd_sta_ip,    sizeof(cmd_sta_ip),  "at+WANIp=%s\r\n", Ip_STA);//
	snprintf(cmd_sta_mask,sizeof(cmd_sta_mask),    "at+WANIpMask=%s\r\n", Mask);//
	snprintf(cmd_sta_gateway,sizeof(cmd_sta_gateway), "at+SGw=%s\r\n", Gateway);//

	snprintf(cmd_eth_ip, sizeof(cmd_eth_ip),      "at+LANIp=%s\r\n", Ip_Eth);//
	snprintf(cmd_eth_mask, sizeof(cmd_eth_mask),   "at+LANIpMask=%s\r\n", Mask_Eth);//

	static  const char *STA_AP_Mode[32]; // tuỳ chỉnh kích thước nếu cần
    int idx = 0;

    // STA mode
    STA_AP_Mode[idx++] = "at+Netmode=4\r\n";
    STA_AP_Mode[idx++] = cmd_sta_ssid;             //at+STASsid=%s
    STA_AP_Mode[idx++] = "at+STAEncType=6\r\n";
    STA_AP_Mode[idx++] = cmd_sta_pass;             //at+STAPasswd=%s
    if (dhcp_enabled == 1) {
        STA_AP_Mode[idx++] = "at+Dhcpc=1\r\n";  // DHCP ON
        STA_AP_Mode[idx++] = "at+IPType=0\r\n";
    } else {
        STA_AP_Mode[idx++] = "at+Dhcpc=0\r\n";  // DHCP OFF
        STA_AP_Mode[idx++] = "at+IPType=1\r\n";
        STA_AP_Mode[idx++] = cmd_sta_ip;              //at+WANIp=%s
        STA_AP_Mode[idx++] = cmd_sta_mask;        //at+WANIpMask=%s
        STA_AP_Mode[idx++] = cmd_sta_gateway;  //at+SGw=%s
        //STA_AP_Mode[idx++] = "at+STADns=8.8.8.8\r\n";
    }

    // AP setup
    STA_AP_Mode[idx++] = "at+Dhcpd=1\r\n";
//    STA_AP_Mode[idx++] = "at+DhcpdIpStart=192.168.16.100\r\n";
//    STA_AP_Mode[idx++] = "at+DhcpdIpStop=192.168.16.250\r\n";
//    STA_AP_Mode[idx++] = "at+DhcpdMask=255.255.255.0\r\n";
//    STA_AP_Mode[idx++] = "at+DhcpdTime=86400\r\n";
//
//    STA_AP_Mode[idx++] = "at+APSsid=rtc_agv_wifi\r\n";
//    STA_AP_Mode[idx++] = "at+APEncType=6\r\n";
//    STA_AP_Mode[idx++] = "at+APPasswd=12345678\r\n";
    STA_AP_Mode[idx++] = cmd_eth_ip;
    STA_AP_Mode[idx++] = cmd_eth_mask;

//    // Serial1 & Network settings
//    STA_AP_Mode[idx++] = "at+NProType0=1\r\n";
//    STA_AP_Mode[idx++] = "at+NLPort0=8080\r\n";
//    STA_AP_Mode[idx++] = "at+NTcpTo0=0\r\n";
//    STA_AP_Mode[idx++] = "at+SBaud0=115200\r\n";
//    STA_AP_Mode[idx++] = "at+SWidth0=8\r\n";
//    STA_AP_Mode[idx++] = "at+SPari0=0\r\n";
//    STA_AP_Mode[idx++] = "at+SStop0=1\r\n";
    // Save & apply
    STA_AP_Mode[idx++] = "at+Save=1\r\n";
    STA_AP_Mode[idx++] = "at+Apply=1\r\n";
    STA_AP_Mode[idx++] = "at+Reboot=1\r\n";
    send_ATcommand(STA_AP_Mode, idx);
}

void send_AT_Serial3() {
	static  char cmd_serial_ip[64];
	static  char cmd_serial_port[32];

	snprintf(cmd_serial_ip, sizeof(cmd_serial_ip),   "at+NDomain1=%s\r\n", Ip_HTTP);
	snprintf(cmd_serial_port, sizeof(cmd_serial_port), "at+NRPort1=%d\r\n", Port_HTTP );

    static  const char *Serial3_setup[64];
    int idx = 0;

    Serial3_setup[idx++] = "at+NProType1=2\r\n";
    Serial3_setup[idx++] = cmd_serial_ip;
    Serial3_setup[idx++] = cmd_serial_port;
//    Serial3_setup[idx++] = "at+NLPort1=8081\r\n";
//    Serial3_setup[idx++] = "at+NTcpTo1=0\r\n";
//    Serial3_setup[idx++] = "at+SBaud1=115200\r\n";
//    Serial3_setup[idx++] = "at+SWidth1=8\r\n";
//    Serial3_setup[idx++] = "at+SPari1=0\r\n";
//    Serial3_setup[idx++] = "at+SStop1=1\r\n";

    // Save & apply
    Serial3_setup[idx++] = "at+Save=1\r\n";
    Serial3_setup[idx++] = "at+Apply=1\r\n";
    Serial3_setup[idx++] = "at+Reboot=1\r\n";

    send_ATcommand(Serial3_setup, idx);
}

void send_AT_Serial3_Serial0() {
	static  char cmd_serial0_port[32];
	snprintf(cmd_serial0_port, sizeof(cmd_serial0_port), "at+NLPort0=%d\r\n", Port_TCP);

	static  const char *Serial3_setup0[32];
    int idx0 = 0;

    // Serial1 & Network settings
    Serial3_setup0[idx0++] = "at+NProType0=1\r\n";
    Serial3_setup0[idx0++] = cmd_serial0_port;
    Serial3_setup0[idx0++] = "at+NTcpTo0=0\r\n";
//    Serial3_setup0[idx0++] = "at+SBaud0=115200\r\n";
//    Serial3_setup0[idx0++] = "at+SWidth0=8\r\n";
//    Serial3_setup0[idx0++] = "at+SPari0=0\r\n";
//    Serial3_setup0[idx0++] = "at+SStop0=1\r\n";
	// Save & apply
    Serial3_setup0[idx0++] = "at+Save=1\r\n";
    Serial3_setup0[idx0++] = "at+Apply=1\r\n";
    Serial3_setup0[idx0++] = "at+Reboot=1\r\n";

    send_ATcommand(Serial3_setup0, idx0);
}

void Reset_Wifi() {
	HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_RESET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_SET);
	HAL_Delay(500);

	static  const char *Serial3_setup1[32];
    int idx1 = 0;

	// Save & apply
    Serial3_setup1[idx1++] = "at+Reboot=1\r\n";
    send_ATcommand(Serial3_setup1, idx1);
}

void reset_config(void){
	memset(Ssid, 0, sizeof(Ssid));
	memset(Pass_STA, 0, sizeof(Pass_STA));
	memset(Ip_STA, 0, sizeof(Ip_STA));
	memset(Mask, 0, sizeof(Mask));
	memset(Gateway, 0, sizeof(Gateway));
	Dhcp = 0;
}
void ATcommand_Function(void const *argument)
{
	osEvent AT_Task_Flag;
	while(1){
		AT_Task_Flag = osSignalWait( SIG_RST_WIFI | SIG_ACCESS_AT, osWaitForever); // Block to Wait flag
		if( AT_Task_Flag.status ==  osEventSignal){
			if(AT_Task_Flag.value.signals & SIG_RST_WIFI){      //SIGNAL HARDWARE INTERRUPT
				//wifi_pulse(6500, 0);
				wifi_pulse(1000, 1500);
				send_DefaultConfig();
			}
			if(AT_Task_Flag.value.signals & SIG_ACCESS_AT){   //SIGNAL HARDWARE INTERRUPT
				uint8_t http_flag = int_http;
				uint8_t rtu_flag  = int_rtu;
				uint8_t sta_flag  = int_Send_STA_AP;

				wifi_pulse(1100, 1500);

				if(sta_flag == 1){
					send_AT_STA_AP(Dhcp);
					reset_config();
					sta_flag = 0 ;
					int_Send_STA_AP = 0;
				}

				else if(http_flag == 1 ){
					send_AT_Serial3();
					http_flag = 0;
					int_http = 0;
				}

				else if(rtu_flag == 1 ){
					send_AT_Serial3_Serial0();
					rtu_flag = 0;
					int_rtu = 0;
				}
			}
		}
	}
}


