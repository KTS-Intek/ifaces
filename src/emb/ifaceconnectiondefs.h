#ifndef IFACECONNECTIONDEFS_H
#define IFACECONNECTIONDEFS_H

#define PORT_IS_BUSY        "\r\nUartIsBusy\r\n"
#define PORT_IS_FREE        "\r\nUartIsFree\r\n"
#define PORT_IS_BUSY_LOCAL  "\r\nUartIsBusyPrtt\r\n"
#define PORT_IS_NFREE       "\r\nUartIs"


//case 0: if(need2closeSerial) closeSerialPort(); r = serialPort->isOpen(); break;
//case 1: r = isTcpConnectionWorks(socket); break;
//case 2: r = svahaConnector->isOpen(); break;
#define IFACECONNTYPE_UART      0
#define IFACECONNTYPE_TCPCLNT   1
#define IFACECONNTYPE_M2MCLNT   2
#define IFACECONNTYPE_UNKN      0xFF


//network speed level kilobyte/msec
#define NET_SPEED_VERY_LOW      3
#define NET_SPEED_LOW           11
#define NET_SPEED_NORMAL        30   //if speed > NET_SPEED_NORMAL : disable compressing
#define NET_SPEED_HIGH          55//300
#define NET_SPEED_VERY_HIGH     200//1500
#define NET_SPEED_UFS_1         800//1500



#endif // IFACECONNECTIONDEFS_H
