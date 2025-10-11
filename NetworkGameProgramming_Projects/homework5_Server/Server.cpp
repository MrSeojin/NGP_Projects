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

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	int total_len{};
	int cur_len{};
	char filename[BUFSIZE + 1];

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
			// 데이터 받기 (filename)
			{
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
				printf("total_len: %d", total_len);
			}
			
			filename[retval] = '\0';
			printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), filename);

			FILE* file = fopen(filename, "wb");
			if (!file) {
				err_display("fopen()");
				break;
			}

			// 데이터 보내기(total len)
			{
				printf("total_len: %d", total_len);
				//int len;
				//retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
				//if (retval == SOCKET_ERROR) {
				//	err_display("recv()");
				//	break;
				//}
				//else if (retval == 0)
				//	break;
				retval = recv(client_sock, (char*)&total_len, sizeof(int), MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0)
					break;
				printf("total_len: %d", total_len);
			}
			// 데이터 받기(file data)
			printf("\n전송률: 0% (수신중)");
			while (cur_len < total_len) {
				char buf[BUFSIZE];
				int len;
				retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0)
					break;
				printf("len: %d", len);
				retval = recv(client_sock, buf, len, MSG_WAITALL);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0) {
					break;
				}
				fwrite(buf, 1, retval, file);
				cur_len += retval;
				printf("\r전송률: %d% (수신중)", (cur_len / total_len * 100));
			}
			printf("\r전송률: %d% (수신완료)\n", (cur_len / total_len * 100));

			fclose(file);
		}
		// 소켓 닫기
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트번호=%d\n", addr, ntohs(clientaddr.sin_port));
	}
	// 소켓 닫기
	closesocket(listen_sock);

	//윈속 종료
	WSACleanup();
	return 0;
}
