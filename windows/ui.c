#include <stdio.h>

#define _WIN32_WINNT 0x0501

#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <dokan.h>

#include "ui.h"

HMENU menu;
struct mount mounts[22];

void Unmount(TCHAR letter)
{
	WCHAR wletter;
	mbtowc(&wletter, &letter, 1);

	DokanUnmount(wletter);
}

BOOL CALLBACK DlgLetterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	DWORD drives;
	TCHAR str[3] = { '?', ':', '\0' };
	HWND hComboWnd;
	static LPTSTR word;
	INT i;

	switch(msg)
	{
		case WM_INITDIALOG:
			word = (LPTSTR)lParam;
			hComboWnd = GetDlgItem(hWnd, ID_DRIVE);
			drives = GetLogicalDrives();

			for(i = 4; i < 26; i++)
				if(!(drives & 1 << i))
				{
					str[0] = 'A'+i;
					(void)ComboBox_AddString(hComboWnd, str);
				}

			(void)ComboBox_SetCurSel(hComboWnd, 0);
			SetForegroundWindow(hWnd);
			break;

		case WM_COMMAND:
			if(LOWORD(wParam) == IDOK)
			{
				TCHAR b[2];
				GetDlgItemText(hWnd, ID_DRIVE, b, 2);
				*word = b[0];
				EndDialog(hWnd, IDOK);
			}
			else if(LOWORD(wParam) == IDCANCEL)
				EndDialog(hWnd, IDCANCEL);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

TCHAR GetDriveLetter(void)
{
	TCHAR letter = '\0';

	INT_PTR ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_CHOOSEDRIVE), NULL, DlgLetterProc, (LPARAM)&letter);

	if(ret == IDOK)
		return letter;

	return '\0';
}

VOID MountFile(TCHAR letter, LPTSTR path)
{
	struct mount *mount;
	HMENU submenu;
	LPTSTR label[11];

	printf("Mounting number %d\n", letter - 'D');

	mount = &mounts[letter - 'D'];
	mount->letter  = letter;
	mount->menu    = 200 + letter;
	mount->path    = strdup(path);
	mount->thread  = _beginthread(Mount, 0, mount);
	mount->mounted = TRUE;

	if(mount->thread == NULL)
	{
		fprintf(stderr, "Could not create thread.\n");
		CloseHandle(mount->thread);
		free(mount->path);
		free(mount);
	}

	submenu = GetSubMenu(menu, 0);

	sprintf(label, "Unmount %c:", mount->letter);
	InsertMenu(submenu, 0, MF_BYPOSITION, ID_MENU_FIRST + mount->letter, label);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT point;
	UINT uMsg;
	DWORD id;

	if(msg == WM_USER + 1)
	{
		uMsg = (UINT)lParam;

		switch(uMsg)
		{
			case WM_RBUTTONUP:
				GetCursorPos((LPPOINT)&point); 

				SetForegroundWindow(hWnd);

				id = TrackPopupMenu(GetSubMenu(menu, 0),
				                    TPM_RETURNCMD | TPM_RIGHTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
				                    point.x, point.y, 0, hWnd, NULL);

				PostMessage(hWnd, WM_NULL, 0, 0);

				if(id == ID_MENU_EXIT || id == ID_MENU_UNMOUNT)
				{
					int i;
					for(i = 0; i < 22; i++)
						if(mounts[i].mounted)
							Unmount(mounts[i].letter);

					if(id == ID_MENU_EXIT)
						PostQuitMessage(0);
				}
				else if(id > ID_MENU_FIRST)
					Unmount(id - ID_MENU_FIRST);

				break;

			default:
				break;
		}
	}
	else
		return DefWindowProc(hWnd, msg, wParam, lParam);

	return 0;
}

void ThreadMain(void *data)
{
	HANDLE pipe;
	DWORD r;
	TCHAR buffer[MAX_PATH];

	for(;;)
	{
		pipe = CreateNamedPipe("\\\\.\\pipe\\derar", PIPE_ACCESS_INBOUND,
		                       PIPE_TYPE_MESSAGE | PIPE_WAIT,
			                   1, 1024, 1024, 0, NULL);

		if(pipe == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "Could not create pipe.\n");
			return;
		}

		if(ConnectNamedPipe(pipe, NULL) == TRUE || GetLastError() == ERROR_PIPE_CONNECTED)
		{
			if(!ReadFile(pipe, buffer, MAX_PATH*sizeof(TCHAR), &r, NULL) || r == 0)
				break;

			MountFile(buffer[0], &buffer[1]);

			DisconnectNamedPipe(pipe);
			CloseHandle(pipe);
		}
		else
			CloseHandle(pipe);
	}
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow)
{
	HANDLE mutex, thread, pipe, hWnd;
	WNDCLASSEX wc;
	DWORD  w;
	MSG msg;
	NOTIFYICONDATA ni;

	mutex = CreateMutex(NULL, FALSE, "derar"); /* Add "Global\"? */

	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if(__argc == 2)
		{
			TCHAR letter;
			TCHAR buffer[strlen(__argv[1]) + 2];

			letter = GetDriveLetter();

			if(!letter)
				return 0;

			pipe = CreateFile("\\\\.\\pipe\\derar", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

			if(pipe == INVALID_HANDLE_VALUE)
				return 0;

			buffer[0] = letter;
			strcpy(&buffer[1], __argv[1]);

			WriteFile(pipe, buffer, (lstrlen(__argv[1]) + 2) * sizeof(TCHAR), &w, NULL);
		}

		return 0;
	}

	memset(mounts, 0, sizeof(struct mount) * 22);

	thread = _beginthread(ThreadMain, 0, NULL);

	if(thread == NULL)
	{
		fprintf(stderr, "Could not create thread.\n");
		return EXIT_FAILURE;
	}

	CloseHandle(thread);

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInst;
	wc.hIcon		 = LoadIcon(hInst, MAKEINTRESOURCE(101));
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "stig";
	wc.hIconSm		 = LoadIcon(hInst, MAKEINTRESOURCE(101));

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	hWnd = CreateWindow("stig", "deRAR", 0, CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, NULL, NULL, hInst, NULL);
	menu = LoadMenu(hInst, MAKEINTRESOURCE(ID_MENU));

	ni.cbSize = sizeof(ni);
	ni.hWnd   = hWnd;
	ni.uID    = ID_TRAYICON;
	ni.uCallbackMessage = WM_USER + 1;
	ni.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	ni.hIcon  = LoadIcon(hInst, MAKEINTRESOURCE(ID_TRAYICON));
	lstrcpyn(ni.szTip, "deRAR", sizeof(ni.szTip));

	Shell_NotifyIcon(NIM_ADD, &ni);

	if(__argc == 2)
	{
		TCHAR letter;
		letter = GetDriveLetter();

		if(letter)
			MountFile(letter, __argv[1]);
	}

	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &ni);

	return 0;
}

