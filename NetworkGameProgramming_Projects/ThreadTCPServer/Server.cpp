#include "..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE 512


int next_line;
CRITICAL_SECTION cs;

void gotoy(int y)
{
	COORD Cur;
	Cur.X = 0;
	Cur.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	char filename[BUFSIZE + 1];
	int total_len{};
	int cur_len{};
	int P{};
	int addrlen;
	int Cur_y;

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		{	// ������ �ޱ� (filename)
			int len;
			retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;
			retval = recv(client_sock, filename, len, MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;
		}
		filename[retval] = '\0';
		EnterCriticalSection(&cs);
		gotoy(next_line);
		Cur_y = next_line + 1;
		printf("[TCP/%s:%d] %s", addr, ntohs(clientaddr.sin_port), filename);
		next_line += 3;
		LeaveCriticalSection(&cs);

		FILE* file = fopen(filename, "wb");
		if (!file) {
			err_display("fopen()");
			break;
		}

		{	// ������ ������(total len)
			retval = recv(client_sock, (char*)&total_len, sizeof(int), MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;
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
			EnterCriticalSection(&cs);
			gotoy(Cur_y);
			printf("���۷�: %d %% (������)", 100 * cur_len / total_len);
			LeaveCriticalSection(&cs);
		}
		EnterCriticalSection(&cs);
		gotoy(Cur_y);
		printf("���۷�: %d %% (���ſϷ�)", 100 * cur_len / total_len);
		LeaveCriticalSection(&cs);
		fclose(file);
	}

	closesocket(client_sock);
	EnterCriticalSection(&cs);
	gotoy(next_line++);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ��ȣ=%d", addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);
	return 0;
}

int main(int argc, char* argv[])
{
	int retval;
	InitializeCriticalSection(&cs);

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
	HANDLE hThread;

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
		EnterCriticalSection(&cs);
		gotoy(next_line++);
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d", addr, ntohs(clientaddr.sin_port));
		LeaveCriticalSection(&cs);

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}
	closesocket(listen_sock);

	WSACleanup();
	DeleteCriticalSection(&cs);
	return 0;
}
