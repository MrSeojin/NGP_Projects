#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#include <fstream>
#include "..\Common.h"
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 50

// 대화상자 프로시저
INT_PTR CALLBACK FileSelectDlgProc(HWND, UINT, WPARAM, LPARAM);
// 대화상자 프로시저
INT_PTR CALLBACK DataTransferDlgProc(HWND, UINT, WPARAM, LPARAM);
// 프로그래스 컨트롤 출력 함수
void DisplayProg(const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;
char* filename;
char buf[BUFSIZE + 1];
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSendButton, hFindButton; // 보내기, 파일 찾기 버튼
HWND hEdit1, hEdit2; // 에디트 컨트롤
HWND hProgress; // 프로그래스 컨트롤

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, FileSelectDlgProc);
	// 송신 프로그래스
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG2), NULL, DataTransferDlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	WSACleanup();
	return 0;
}

// 파일 선택 프로시저
INT_PTR CALLBACK FileSelectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// IDD_DIALOG1
	// IDD_DIALOG2
	// IDC_PROGRESS1
	// IDC_EDIT1
	// IDC_Find
	// IDC_IPADDRESS
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);

		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		char* text = (char*)"찾아보기";
		SendMessageA(GetDlgItem(hDlg, IDC_Find), EM_REPLACESEL, FALSE, (LPARAM)text); 
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 대기
			GetDlgItemTextA(hDlg, IDC_EDIT1, buf, BUFSIZE + 1);
			SetEvent(hWriteEvent); // 쓰기 완료 알림
			SetFocus(hEdit1); // 키보드 포커스 전환
			SendMessage(hEdit1, EM_SETSEL, 0, -1); // 텍스트 전체 선택
			EndDialog(hDlg, IDCANCEL); // 대화상자 닫기
			return TRUE;
		case IDC_Find:

			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// 진행율 프로시저
INT_PTR CALLBACK DataTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT1);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);

		char* text = (char*)"파일 전송 시작...\r\n";
		SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)text);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL); // 대화상자 닫기
			closesocket(sock); // 소켓 닫기
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// 프로그래스 컨트롤 출력 함수
void DisplayProg(const char* fmt, ...)
{
	SetDlgItemInt(hProgress, IDC_PROGRESS1, atoi(fmt), FALSE);
}

// 소켓 함수 오류 출력
void DisplayError(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	int cur_len{};

	// 수정 필요.
	while (1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 대기

		// 버퍼에 담겨 있는데 없다면 보내지 않음
		if (strlen(filename) == 0) {
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알림
			continue;
		}

		// 데이터 보내기(fileName)
		retval = send(sock, buf, (int)strlen(filename), 0);
		if (retval == SOCKET_ERROR) {
			DisplayError("send()");
			break;
		}
		
		// 데이터 보내기(total len)
		retval = send(sock, buf, (int)strlen(filename), 0);
		if (retval == SOCKET_ERROR) {
			DisplayError("send()");
			break;
		}
		
		// 데이터 보내기(fileData)
		retval = send(sock, buf, (int)strlen(filename), 0);
		if (retval == SOCKET_ERROR) {
			DisplayError("send()");
			break;
		}

		char* text = (char*)"파일 전송 완료...\r\n";
		SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)text);

		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알림
	}
	// 데이터 보내기(fileName)
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

	// 데이터 보내기(total len)
	{
		retval = send(sock, (char*)&total_len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
		}
	}
	// 데이터 보내기(fileData)
	int len{};
	while ((len = fread(filedata, sizeof(char), BUFSIZE, file)) > 0) {
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
	
	return 0;
}
