#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#include "..\Common.h"
#include <commctrl.h>
#include <fstream>
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 50

// 대화상자 프로시저
INT_PTR CALLBACK FileSelectDlgProc(HWND, UINT, WPARAM, LPARAM);
// 대화상자 프로시저
INT_PTR CALLBACK DataTransferDlgProc(HWND, UINT, WPARAM, LPARAM);
// 폴더 탐색창 열기
void OpenSpecificFolder(HWND hWnd);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;
FILE* file;
char* filename;
char buf[BUFSIZE + 1];
char fileData[BUFSIZE];
int total_len;
HANDLE hWriteEvent; // 이벤트
HWND hSendButton, hFindButton; // 보내기, 파일 찾기 버튼
HWND hEdit1, hEdit2, hProgress; // 에디트 컨트롤, 프로그래스 컨트롤

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, FileSelectDlgProc);
	// 송신 프로그래스
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG2), NULL, DataTransferDlgProc);

	CloseHandle(hWriteEvent);

	WSACleanup();
	return 0;
}

// 파일 선택 프로시저
INT_PTR CALLBACK FileSelectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hSendButton = GetDlgItem(hDlg, IDOK);
		EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SetEvent(hWriteEvent); // 쓰기 완료 알림
			EndDialog(hDlg, IDCANCEL);	// 대화상자 닫기
			return TRUE;
		case IDC_Find:
			OpenSpecificFolder(hDlg);
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화

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
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);

		SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendMessage(hProgress, PBM_SETPOS, 0, 0);
		SendMessageA(hEdit2, WM_SETTEXT, FALSE, (LPARAM)"파일 전송 증...");
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, IDOK);	// 대화상자 닫기
			closesocket(sock); // 소켓 닫기
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void OpenSpecificFolder(HWND hWnd)
{
	OPENFILENAMEA ofn = { sizeof(ofn) };
	char szFile[MAX_PATH] = "";
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = "모든 파일\0*.*\0";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
	{
		SendMessageA(hEdit1, WM_SETTEXT, FALSE, (LPARAM)szFile);
		filename = _strdup(strrchr(szFile, '\\') ? strrchr(szFile, '\\') + 1 : szFile);
	}
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	sock = socket(AF_INET, SOCK_STREAM, 0);
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

	while (1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 대기

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

		file = fopen(filename, "rb");
		fseek(file, 0, SEEK_END);
		total_len = ftell(file);
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
		while ((len = fread(fileData, sizeof(char), BUFSIZE, file)) > 0) {
			retval = send(sock, (char*)&len, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			retval = send(sock, fileData, len, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			cur_len += len;
			SendMessage(hProgress, PBM_SETPOS, cur_len * 100 / total_len, 0);
		}
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SendMessageA(hEdit2, WM_SETTEXT, FALSE, (LPARAM)"파일 전송 완료");
	}
	fclose(file);
	return 0;
}
