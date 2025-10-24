#include "..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE 512

int main(int argc, char* argv[])
{
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)err_quit("socket()");

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	int total_len{};
	int cur_len{};
	int P{};
	char filename[BUFSIZE + 1];

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));

//		while (1) {
			{	// ������ �ޱ� (filename)
				int len;
				retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0);
//					break;
				retval = recv(client_sock, filename, len, MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0);
//					break;
			}
			filename[retval] = '\0';
			printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), filename);

			FILE* file = fopen(filename, "wb");
			if (!file) {
				err_display("fopen()");
				break;
			}
			{	// ������ ������(total len)
				int len;
				retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0);
//					break;
				retval = recv(client_sock, (char*)&total_len, len, MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0);
//					break;
			}
			while (cur_len < total_len) {	// ������ �ޱ�(file data)
				char buf[BUFSIZE];
				int len;
				retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0)
					break;
				retval = recv(client_sock, buf, len, MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0)
					break;
				fwrite(buf, 1, retval, file);
				cur_len += retval;
				printf("\r���۷�: %d %% (������)", 100 * cur_len / total_len);
			}
			printf("\r���۷�: %d %% (���ſϷ�)\n", 100 * cur_len / total_len);
			fclose(file);
//		}
		closesocket(client_sock);
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	}
	closesocket(listen_sock);

	WSACleanup();
	return 0;
}
