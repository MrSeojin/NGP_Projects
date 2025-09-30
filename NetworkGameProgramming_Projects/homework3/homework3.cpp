#include "..\Common.h"

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	char *name = argv[1];

	struct hostent* ptr = gethostbyname(name);
	if (ptr == NULL) {
		err_display("gethostbyname()");
		return 1;
	}
	else {
		printf("\n=====정보 출력=====\n");
		printf("도메인 이름: %s\n", name);

		// 별명 출력
		if (*ptr->h_aliases == NULL)
			printf("호스트 별명이 존재하지 않습니다.\n");
		else {
			for (char** alias = ptr->h_aliases; *alias != NULL; ++alias)
				printf("호스트 별명: %s\n", *alias);
		}

		// IPv4 주소 출력
		if (ptr->h_addrtype == AF_INET) {
			for (int i = 0; ptr->h_addr_list[i] != NULL; ++i) {
				char str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, ptr->h_addr_list[i], str, sizeof(str));
				printf("호스트 IP 주소: %s\n", str);
			}
		};

		WSACleanup();
	}
	return 0;
}
