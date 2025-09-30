#include "..\Common.h"

int main(int argc, char* argv[])
{
	// ���� �ʱ�ȭ
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
		printf("\n=====���� ���=====\n");
		printf("������ �̸�: %s\n", name);

		// ���� ���
		if (*ptr->h_aliases == NULL)
			printf("ȣ��Ʈ ������ �������� �ʽ��ϴ�.\n");
		else {
			for (char** alias = ptr->h_aliases; *alias != NULL; ++alias)
				printf("ȣ��Ʈ ����: %s\n", *alias);
		}

		// IPv4 �ּ� ���
		if (ptr->h_addrtype == AF_INET) {
			for (int i = 0; ptr->h_addr_list[i] != NULL; ++i) {
				char str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, ptr->h_addr_list[i], str, sizeof(str));
				printf("ȣ��Ʈ IP �ּ�: %s\n", str);
			}
		};

		WSACleanup();
	}
	return 0;
}
