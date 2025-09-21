#include "..\Common.h"

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(0x0202, &wsa) != 0)
		return 1;
	printf("[알림] 윈속 초기화 성공\n");

	printf("wVersion:	%d.%d\n", LOBYTE(wsa.wVersion), HIBYTE(wsa.wVersion));
	printf("wHighVersion:	%d.%d\n", LOBYTE(wsa.wHighVersion), HIBYTE(wsa.wHighVersion));
	printf("szDescription:	%s\n", wsa.szDescription);
	printf("szSystemStatus:	%s\n", wsa.szSystemStatus);

	return 0;
}
