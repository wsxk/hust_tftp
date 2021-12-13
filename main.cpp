#include "tftp.h"
#pragma warning(disable : 4996) 

int main() {
	tftp interactive;
	//≥ı ºªØwinsock
	if (!interactive.init_winsock()) {
		return 0;
	}
	//create socket
	if (!interactive.create_socket()) {
		return 0;
	}
	//bind
	if (!interactive.bind_sock()) {
		return 0;
	}
	//open log
	if (!interactive.open_log()) {
		return 0;
	}
	// get command
	char buf[256];
	char* arg1;
	char* arg2;
	char* arg3;
	getchar();
	while (1) {
		//getchar();
		fflush(stdin);
		cout << "tftp>";
		gets_s(buf, sizeof(buf));
		arg1 = strtok(buf, " ");
		if (arg1 == NULL) {
			cout << "no input" << endl;
			continue;
		}
		if (!strcmp(arg1, "upload")) {
			arg2 = strtok(NULL, " ");
			if (arg2 != NULL) {
				//cout << arg2;
				interactive.upload(arg2);
			}
			else {
				cout << "command upload needs an arg:filename that you want to upload" << endl;
				cout << "example: upload filename" << endl;
				continue;
			}
		}
		else if (!strcmp(arg1, "download")) {
			arg2 = strtok(NULL, " ");
			arg3 = strtok(NULL, " ");
			if (arg2 == NULL || arg3 == NULL) {
				cout << "command download needs two args:remote_filename and local_filename" << endl;
				cout << "example:download remote_file local_file" << endl;
			}
			else {
				interactive.download(arg2, arg3);
			}
		}
		else if (!strcmp(arg1, "exit")) {
			cout << "good bye!" << endl;
			break;
		}
		else {
			cout << "unknown command!" << endl;
			cout << "the command list:" << endl;
			cout << "upload" << endl;
			cout << "download" << endl;
			cout << "exit" << endl;
		}
	}
	return 0;
}