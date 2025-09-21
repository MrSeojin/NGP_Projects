#include "..\Common.h"

u_short x1 = 0x1234;        // ȣ��Ʈ ����Ʈ
u_short x2 = htons(x1);     // ��Ʈ��ũ ����Ʈ

void GetHostEndian();

int main() {
    printf("[ȣ��Ʈ ����Ʈ -> ��Ʈ��ũ ����Ʈ (�� �����)]\n");
    printf("%#x -> %#x\n", x1, x2);
    printf("\n");

    printf("[ȣ��Ʈ ����Ʈ ����]\n");
    GetHostEndian();

    return 0;
}

void GetHostEndian()
{
    if (x1 == x2)
        printf("Big-endian\n");
    else
        printf("Little-endian\n");
}