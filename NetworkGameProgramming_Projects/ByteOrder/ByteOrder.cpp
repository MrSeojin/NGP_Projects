#include "..\Common.h"

u_short x1 = 0x1234;        // 호스트 바이트
u_short x2 = htons(x1);     // 네트워크 바이트

void GetHostEndian();

int main() {
    printf("[호스트 바이트 -> 네트워크 바이트 (빅 엔디언)]\n");
    printf("%#x(%hu) -> %#x(%hu)\n", x1, x1, x2, x2);
    printf("\n");

    printf("[빅 엔디언 -> 빅엔디언]\n");
    printf("%#x(%hu) -> %#x(%hu)\n", htons(htons(x2)), x2, htons(x2), htons(x2));
    printf("\n");

    printf("[호스트 바이트 정렬]\n");
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