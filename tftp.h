#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define max_data_size 512
// opcode
#define RRQ 1
#define WRQ 2
#define	DATA 3
#define	ACK	4
#define	ERROR 5

#define PKT_RCV_TIMEOUT (1500)
#define PKT_MAX_RXMT 3
// tftp packet
struct tftp_message
{
	uint16_t Opcode;
	union
	{
		uint16_t number;		//DATA ACK
		uint16_t error_code;	//ERROR
		char filename[2];		//RRQ WRQ
	};
	char data[max_data_size];
};

class tftp {
private:
public:
	tftp(void);
	//发送包和接受包
	tftp_message send_packet, recv_packet;
	// 服务端和客户端的ip地址
	sockaddr_in serverAddr, clientAddr;
	// 客户端socket
	SOCKET sock;
	// ip地址长短
	unsigned int addr_len;
	unsigned long Opt;
	//传送的字节数，总耗时
	double transByte, consumeTime;
	//日志文件
	FILE* log_fp;
	char logBuf[512];
	//出现问题的时间
	time_t rawTime;
	tm* info;
	//计时器
	clock_t start, end;
	// winsock
	WSADATA wsadata;
	int flag;
	//选择传输模式
	int choose;
	// upload  download file
	bool init_winsock();
	bool create_socket();
	bool bind_sock();
	bool open_log();
	bool upload(char* filename);
	bool download(char* remoteFile, char* localFile);
	void write_log();
};


