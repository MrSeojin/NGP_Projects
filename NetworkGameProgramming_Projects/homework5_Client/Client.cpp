#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#include <fstream>
#include "..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE 50

int main(int argc, char* argv[])
{
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

//	const char* SERVERIP = argv[1];
	const char* SERVERIP = "127.0.0.1";
//	const char* filename = argv[2];
	const char* filename = "video.mp4";

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// ������ ��ſ� ����� ����
	char filedata[BUFSIZE]{};
	int cur_len{};

	// ������ ������(fileName)
	{
		int len = (int)strlen(filename);
		retval = send(sock, (char*)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
		}
		retval = send(sock, filename, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
		}
	}
	
	FILE* file = fopen(filename, "rb");
	fseek(file, 0, SEEK_END);
	int total_len = ftell(file);
	fseek(file, 0, SEEK_SET);

	// ������ ������(total len)
	{
		retval = send(sock, (char*)&total_len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
		}
	}
	// ������ ������(fileData)
	int len{};
	while ((len = fread(filedata, sizeof(char), BUFSIZE, file) )> 0) {
		retval = send(sock, (char*)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
		}
		retval = send(sock, filedata, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		cur_len += len;
	}

	closesocket(sock);

	WSACleanup();
	return 0;
}
