#include "tftp.h"
#pragma warning(disable : 4996) 
tftp::tftp(void) {
	Opt = 1;
	transByte = 0;
	consumeTime = 0;
	log_fp = NULL;
}

bool tftp::init_winsock() {
	//initial winsock
	addr_len = sizeof(struct sockaddr_in);
	flag = WSAStartup(0x0101, &wsadata);
	if (flag) {
		cout << "winsock init error!" << endl;
		return 0;
	}
	if (wsadata.wVersion != 0x0101) {
		cout << "winsock version error!" << endl;
		return 0;
	}
	cout << "winsock init successfully!" << endl;
	return 1;
}

bool tftp::create_socket() {
	char s_ip[20], c_ip[20];
	uint16_t s_port, c_port;
	cout << "input server IP:";
	cin >> s_ip;
	cout << "input client IP:";
	cin >> c_ip;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(69);
	serverAddr.sin_addr.S_un.S_addr = inet_addr(s_ip);
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(0);
	clientAddr.sin_addr.S_un.S_addr = inet_addr(c_ip);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	ioctlsocket(sock, FIONBIO, &Opt);
	if (sock == INVALID_SOCKET)
	{
		cout << "client create socket error!" << endl;
		WSACleanup();
		return 0;
	}
	cout << "client create socket successfully!" << endl;
	return 1;
}

bool tftp::bind_sock() {
	int nRC = bind(sock, (LPSOCKADDR)&clientAddr, sizeof(clientAddr));
	if (nRC == SOCKET_ERROR)
	{
		cout << "client socket bind error!" << endl;
		closesocket(sock);
		WSACleanup();
		return 0;
	}
	cout << "client socket bind successfully!" << endl;
	return 1;
}

bool tftp::open_log() {
	log_fp = fopen("tftp_error.log", "a");
	if (log_fp == NULL) {
		cout << "open log file error!" << endl;
		return 0;
	}
	cout << "open log file successfully!" << endl;
	return 1;
}

void tftp::write_log() {
	for (int i = 0; i < 512; i++) {
		if (logBuf[i] == '\n') {
			logBuf[i] = ' ';
			break;
		}
	}
	fwrite(logBuf, strlen(logBuf), 1, log_fp);
}

bool tftp::upload(char* filename) {
	int time_wait_ack,size;
	sockaddr_in sender;

	cout << "choose the file format(1.netascii 2.octet)" << endl;
	cin >> choose;
	getchar();

	//打开文件
	FILE* fp = NULL;
	if (choose == 1) {
		fp = fopen(filename, "r");
	}
	else {
		fp = fopen(filename, "rb");
	}
	if (fp == NULL) {
		cout << "File not exists!" << endl;
		time(&rawTime);
		info = localtime(&rawTime);
		sprintf(logBuf, "%s Error: upload %s, mode: %s, %s\n", asctime(info), filename, choose == 1 ? ("netascii") : ("octet"), "File not exists!");
		write_log();
		return 0;
	}
	//test
	int temp=0;
	for (int i = 0; i < strlen(filename); i++) {
		if (filename[i] == '\\') {
			temp = i+1;
		}
	}
	//发送初始包
	send_packet.Opcode = htons(WRQ);
	if (choose == 1) {
		sprintf(send_packet.filename, "%s%c%s%c", filename+temp, 0, "netascii", 0);
	}
	else {
		sprintf(send_packet.filename, "%s%c%s%c", filename+temp, 0, "octet", 0);
	}
	sendto(sock, (char*)&send_packet, sizeof(tftp_message), 0, (struct sockaddr*)&serverAddr, addr_len);
	//接收初始包回应包
	for (time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20) {
		size = recvfrom(sock, (char*)&recv_packet, sizeof(tftp_message), 0, (struct sockaddr*)&sender, (int*)&addr_len);
		if (size >= 4 && recv_packet.Opcode == htons(ACK) && recv_packet.number == htons(0)) {
			break;
		}
		Sleep(20);
	}
	//超时没有收到回复包
	if (time_wait_ack >= PKT_RCV_TIMEOUT) {
		cout << "could not receive from server!" << endl;
		time(&rawTime);
		info = localtime(&rawTime);
		sprintf(logBuf, "%s Error: upload %s,mode:%s, %s\n", asctime(info), filename, choose == 1 ? ("netascii") : ("octet"), "Could not receive from server!");
		write_log();
		fclose(fp);
		return 0;
	}
	cout << "get first packet" << endl;
	//开始发送数据包
	uint16_t block_num = 1;
	int counts = 0;
	transByte = 0;
	int read_size;
	start = clock();
	do {
		memset(send_packet.data, 0, sizeof(send_packet.data));
		send_packet.Opcode = htons(DATA);
		send_packet.number = htons(block_num);
		read_size = fread(send_packet.data, 1,max_data_size , fp);
		transByte += read_size;
		for (counts = 0; counts < PKT_MAX_RXMT; counts++) {
			cout << "send number:" << block_num << "data block."<<" resend times: "<<counts << endl;
			sendto(sock, (char*)&send_packet, read_size + 4, 0, (struct sockaddr*)&sender, addr_len);
			for (time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20) {
				size = recvfrom(sock, (char*)&recv_packet, sizeof(tftp_message), 0, (struct sockaddr*)&sender, (int*)&addr_len);
				if (size >= 4 && recv_packet.Opcode == htons(ACK) && recv_packet.number == htons(block_num)) {
					break;
				}
				Sleep(20);
			}
			if (time_wait_ack < PKT_RCV_TIMEOUT) {
				break;
			}
		}
		if (counts >= PKT_MAX_RXMT) {
			cout<< "can't receive from server" << endl;
			time(&rawTime);
			info = localtime(&rawTime);
			sprintf(logBuf, "%s Error: upload %s,mode:%s, %s\n", asctime(info), filename, choose == 1 ? ("netascii") : ("octet"), "Could not receive from server.");
			write_log();
			fclose(fp);
			return 0;
		}
		//传输下一个数据块
		block_num++;
	} while (read_size == max_data_size);
	end = clock();
	cout << "send file successfully!" << endl;
	fclose(fp);
	//计算耗时
	consumeTime=((double)(end - start)) / CLK_TCK;
	cout << "file size: " << transByte << " Bytes" << " time: " << consumeTime << " s" << endl;
	cout << "upload speed:" << transByte / consumeTime/1024 << " kB/s" << endl;
	
	return 1;
}

bool tftp::download(char* remoteFile, char* localFile) {
	int time_wait_ack, size;
	sockaddr_in sender;

	//选择方法
	cout << "choose the file format(1.netascii 2.octet)" << endl;
	cin >> choose;
	getchar();

	//创建本地文件
	FILE* fp = NULL;
	if (choose == 1) {
		fp = fopen(localFile, "w");
	}
	else {
		fp = fopen(localFile, "wb");
	}
	if (fp == NULL) {
		cout << "open/create file error" << endl;
		time(&rawTime);
		info = localtime(&rawTime);
		sprintf(logBuf, "%s Error: download %s, mode: %s, %s\n", asctime(info), localFile, choose == 1 ? ("netascii") : ("octet"), "open/create file error");
		write_log();
		return 0;
	}

	//发送初始数据包
	send_packet.Opcode = htons(RRQ);
	if (choose == 1) {
		sprintf(send_packet.filename, "%s%c%s%c", remoteFile, 0, "netascii", 0);
	}
	else {
		sprintf(send_packet.filename, "%s%c%s%c", remoteFile, 0, "octet", 0);
	}
	sendto(sock, (char*)&send_packet, sizeof(send_packet), 0, (struct sockaddr*)&serverAddr, addr_len);
	
	//接受数据包
	transByte = 0;
	uint16_t block = 1;
	start = clock();
	do {
		cout << "get number " << block << " data block" << endl;
		for (time_wait_ack = 0; time_wait_ack < (PKT_RCV_TIMEOUT * PKT_MAX_RXMT); time_wait_ack += 20) {
			size = recvfrom(sock, (char*)&recv_packet, sizeof(tftp_message), 0, (struct sockaddr*)&sender, (int*)&addr_len);
			if (size >= 4 && recv_packet.Opcode == htons(DATA) && recv_packet.number == htons(block)) {
				send_packet.Opcode = htons(ACK);
				send_packet.number = recv_packet.number;
				sendto(sock, (char*)&send_packet, sizeof(tftp_message), 0, (struct sockaddr*)&sender, addr_len);
				fwrite(recv_packet.data, size - 4, 1, fp);
				break;
			}
			Sleep(20);
		}
		if (time_wait_ack >= (PKT_RCV_TIMEOUT * PKT_MAX_RXMT)) {
			cout << "timeout! Can't receive data from server!" << endl;
			time(&rawTime);
			info = localtime(&rawTime);
			sprintf(logBuf, "%s Error: download %s, mode: %s, %s\n", asctime(info), localFile, choose == 1 ? ("netascii") : ("octet"), "timeout! Can't receive data from server!");
			write_log();
			fclose(fp);
			return 0;
		}
		transByte += (size - 4);
		block++;
	} while (size == max_data_size + 4);
	end = clock();
	consumeTime = ((double)(end - start)) / 1000;
	cout << "file size: " << transByte << " Bytes" << " time: " << consumeTime << " s" << endl;
	cout << "download speed: " << transByte / consumeTime/1024 << " kB/s" << endl;
	fclose(fp);
	return 1;
}