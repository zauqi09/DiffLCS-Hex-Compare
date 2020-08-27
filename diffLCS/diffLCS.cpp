#include "framework.h"
#include "diffLCS.h"
#include "commctrl.h"
#include "strsafe.h"

#define BUFFER MAX_PATH 

#define MAX_LOADSTRING 10000

enum myID {
    ID_TEXTBOX_1,
    ID_TEXTBOX_2,
	ID_TEXTBOX_3,
	ID_TEXTBOX_4,
	ID_TEXTBOX_5,
	ID_TEXTBOX_6,
    ID_LISTBOX_1,
    ID_LISTBOX_2,
    ID_BUTTON_LOAD_1,
    ID_BUTTON_LOAD_2,
    ID_BUTTON_COMPARE,
    ID_BUTTON_NEXT,
    ID_BUTTON_PREV,
    ID_LABEL_1,
    ID_LABEL_2,
	ID_LABEL_3,
	ID_LABEL_4,
	ID_LABEL_5,
	ID_LABEL_6,
	ID_LABEL_7,
	ID_LABEL_8,

};


HINSTANCE hInst;                                
WCHAR AppTitle[MAX_LOADSTRING];                  
WCHAR AppWindow[MAX_LOADSTRING];            
HWND hL1;
HWND hL2;
HWND hE3;
HWND hE4;
HWND hE5;
HWND hE6;
DWORD HexPerPage;
unsigned long long MaxHexFile;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL				initInterface(HWND hWnd);
VOID				getFilePath(HWND hWnd, HWND hEdit);
DWORD WINAPI		CheckingNotDiff(LPVOID lpParam);
void				SetFunction(HWND hwnd, LPCWSTR pstr, unsigned char data);
void				InfoDisplay(HWND hwnd1, HWND hwnd2, HWND hwnd3, HWND hwnd4, const WCHAR* pszFileName1, const WCHAR* pszFileName2, unsigned long long offset, int notDiff, int diff);
void				ViewPage(const WCHAR* pszFileName1, const WCHAR* pszFileName2,  unsigned long long offset);
LPWSTR				displayHex(const unsigned char* pView, int i, int offset);
int				notdiff(const WCHAR* pszFileName1, const WCHAR* pszFileName2);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, AppTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DIFFLCS, AppWindow, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIFFLCS));

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIFFLCS));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DIFFLCS);
	wcex.lpszClassName = AppWindow;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	HWND hWnd = CreateWindow(AppWindow, AppTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, CW_USEDEFAULT, 0, 1368, 700, nullptr, nullptr, hInstance, nullptr);
	HDC hDC = ::GetWindowDC(NULL);
	::SetWindowPos(hWnd, NULL, 0, 0, ::GetDeviceCaps(hDC, HORZRES), ::GetDeviceCaps(hDC, VERTRES), SWP_FRAMECHANGED);
	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HANDLE DiffChecking;
	static DWORD id;
	static unsigned long long offset;
	static HWND hE1;
	static HWND hE2;
	static HWND hE3;
	static HWND hE4;
	static HWND hE5;
	static HWND hE6;
	static TCHAR buff1[MAX_LOADSTRING];
	static TCHAR buff2[MAX_LOADSTRING];

	switch (message)
	{
	case WM_CREATE:
	{
		if (!initInterface(hWnd))
			return EXIT_FAILURE;

		offset = 0;
		hE1 = GetDlgItem(hWnd, ID_TEXTBOX_1);
		hE2 = GetDlgItem(hWnd, ID_TEXTBOX_2);
		hE3 = GetDlgItem(hWnd, ID_TEXTBOX_3);
		hE4 = GetDlgItem(hWnd, ID_TEXTBOX_4);
		hE5 = GetDlgItem(hWnd, ID_TEXTBOX_5);
		hE6 = GetDlgItem(hWnd, ID_TEXTBOX_6);
		SYSTEM_INFO sysinfo{};
		GetSystemInfo(&sysinfo);
		HexPerPage = sysinfo.dwAllocationGranularity;
	}
	break;

	case WM_DRAWITEM:
	{
		PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;

		if (pdis->itemID == -1)
		{
			break;
		}

		switch (pdis->itemAction)
		{
		case ODA_SELECT:
		case ODA_DRAWENTIRE:
		{
			TCHAR achBuffer[BUFFER];
			size_t cch;
			unsigned char data;
			int yPos;
			TEXTMETRIC tm;
			SIZE sz;

			SendMessage(pdis->hwndItem, LB_GETTEXT, pdis->itemID, (LPARAM)achBuffer);

			data = (unsigned char)SendMessage(pdis->hwndItem, LB_GETITEMDATA, pdis->itemID, NULL);

			SetTextColor(pdis->hDC, RGB(0, 0, 0));
			if (pdis->itemState & ODS_SELECTED)
			{
				SetBkColor(pdis->hDC, RGB(128, 128, 128));
				FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)GetStockObject(GRAY_BRUSH));
			}
			else
			{
				SetBkColor(pdis->hDC, RGB(255, 255, 255));
				FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
			}

			GetTextMetrics(pdis->hDC, &tm);
			yPos = (pdis->rcItem.bottom + pdis->rcItem.top - tm.tmHeight) / 2;

			StringCchLength(achBuffer, BUFFER, &cch);

			int offset = 0;
			int i = 0;
			int AddressSize = 10;

			for (; i < AddressSize; i++)
			{
				TextOut(pdis->hDC, pdis->rcItem.left + offset, yPos, &achBuffer[i], 1);
				GetTextExtentPoint32(pdis->hDC, &achBuffer[i], 1, &sz);
				offset += sz.cx;
			}

			offset = 9 * AddressSize;
			TextOut(pdis->hDC, pdis->rcItem.left + offset, yPos, L":", 1);
			offset += 175;
			TextOut(pdis->hDC, pdis->rcItem.left + offset, yPos, L"|", 1);

			int offsetASCII = offset + 15;

			for (int i = 0; i < 8; i++)
			{
				if (data & (1 << i))
				{
					SetTextColor(pdis->hDC, RGB(0, 0, 0));
				}
				else
				{
					SetTextColor(pdis->hDC, RGB(200, 0, 0));
				}

				int index;
				offset = 10 * AddressSize + i * 20;
				for (int j = 0; j < 2; j++)
				{
					index = AddressSize + i * 2 + j;
					TextOut(pdis->hDC, pdis->rcItem.left + offset, yPos, &achBuffer[index], 1);
					GetTextExtentPoint32(pdis->hDC, &achBuffer[index], 1, &sz);
					offset += sz.cx;
				}

				index = i + AddressSize + 16;
				TextOut(pdis->hDC, pdis->rcItem.left + offsetASCII, yPos, &achBuffer[index], 1);
				GetTextExtentPoint32(pdis->hDC, &achBuffer[index], 1, &sz);
				offsetASCII += sz.cx;
			}
		}
		break;
		case ODA_FOCUS:
			break;
		}
		return TRUE;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_BUTTON_LOAD_1:
		{
			SendMessage(hL1, LB_RESETCONTENT, 0, 0);
			SendMessage(hL2, LB_RESETCONTENT, 0, 0);
			getFilePath(hWnd, hE1);
		}
		break;

		case ID_BUTTON_LOAD_2:
		{
			SendMessage(hL1, LB_RESETCONTENT, 0, 0);
			SendMessage(hL2, LB_RESETCONTENT, 0, 0);
			getFilePath(hWnd, hE2);
		}
		break;
		case ID_BUTTON_COMPARE:
		{
			int notDiff = 0;
			int diff = 0;
			SendMessage(hE1, WM_GETTEXT, MAX_LOADSTRING, (LPARAM)buff1);
			SendMessage(hE2, WM_GETTEXT, MAX_LOADSTRING, (LPARAM)buff2);
			SendMessage(hL1, LB_RESETCONTENT, 0, 0);
			SendMessage(hL2, LB_RESETCONTENT, 0, 0);
			offset = 0;
			ViewPage(buff1, buff2, offset);
			DiffChecking = CreateThread(NULL, 0, CheckingNotDiff, hWnd, 0, &id);
			InfoDisplay(hE3,hE4,hE5, hE6, buff1, buff2, offset, notDiff, diff);
		}
		break;
		case ID_BUTTON_NEXT:
		{
			if (offset + HexPerPage < MaxHexFile)
			{
				offset += HexPerPage;
				ViewPage(buff1, buff2, offset);
			}
		}
		break;
		case ID_BUTTON_PREV:
		{
			if (offset != 0)
			{
				offset -= HexPerPage;
				ViewPage(buff1, buff2, offset);
			}
		}
		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL initInterface(HWND hWnd)
{
	RECT AppMain;
	GetClientRect(hWnd, &AppMain);

	const int DEFAULT_HEIGHT = 20;

	const SIZE constSize{ 5,5 };

	SIZE szLST{ (AppMain.right - 7 * constSize.cx) / 2.5,AppMain.bottom - DEFAULT_HEIGHT * 6 - constSize.cy * 3 };
	SIZE szLST2{ (AppMain.right - 2 * constSize.cx) / 5,AppMain.bottom - DEFAULT_HEIGHT * 4 - constSize.cy * 3 };
	SIZE szTEXT{ (AppMain.right - 4 * constSize.cx) / 3,AppMain.bottom - DEFAULT_HEIGHT * 4 - constSize.cy * 3 };
	SIZE szBtLOAD{ 100,DEFAULT_HEIGHT };
	SIZE szEd{ AppMain.right - constSize.cx - szBtLOAD.cx - 120, DEFAULT_HEIGHT };
	SIZE szLbl{ 100, DEFAULT_HEIGHT };
	SIZE szBtCmp{ 100, DEFAULT_HEIGHT };
	SIZE szBtPg{ szBtCmp.cx, DEFAULT_HEIGHT };
	

	POINT psLbl1{ constSize.cx, constSize.cy };
	POINT psLbl2{ constSize.cx, constSize.cy + 30 };

	POINT psEd1{ psLbl1.x + szLbl.cx , constSize.cy };
	POINT psEd2{ psLbl2.x + szLbl.cx , constSize.cy + 30};

	POINT psBtBrowse1{ psEd1.x + szEd.cx + constSize.cx, psEd1.y };
	POINT psBtBrowse2{ psEd2.x + szEd.cx + constSize.cx, psEd2.y };
	POINT psBtCMP{ psEd2.x + szEd.cx + constSize.cx, psEd1.y + szEd.cy + constSize.cy + 40};

	POINT psLST1{ psLbl1.x,psBtCMP.y + szBtCmp.cy + constSize.cy + DEFAULT_HEIGHT };
	POINT psLST2{ szLST.cx + 10 ,psBtCMP.y + szBtCmp.cy + constSize.cy + DEFAULT_HEIGHT };
	POINT psLST3{ szLST.cx * 2 + 15 ,psBtCMP.y + szBtCmp.cy + constSize.cy + DEFAULT_HEIGHT };


	POINT psPREV{ psLST1.x, psLST1.y + szLST.cy + DEFAULT_HEIGHT };
	POINT psNEXT{ psLST2.x + szLST.cx - szBtPg.cx, psLST2.y + szLST.cy + DEFAULT_HEIGHT };

	HWND label1 = CreateWindowEx(0, L"STATIC", L"File 1 :", WS_CHILD | WS_VISIBLE, psLbl1.x, psLbl1.y, szLbl.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_1), 0, 0);
	if (label1 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label2 = CreateWindowEx(0, L"STATIC", L"File 2 :", WS_CHILD | WS_VISIBLE, psLbl2.x, psLbl2.y, szLbl.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_2), 0, 0);
	if (label2 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label3 = CreateWindowEx(0, L"STATIC", L"Offset(h)         :  00 01 02 03 04 05 06 07  |  Decoded Text", WS_CHILD | WS_VISIBLE, psLST1.x, psLST2.y-20, szLST.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_3), 0, 0);
	if (label3 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label4 = CreateWindowEx(0, L"STATIC", L"Offset(h)         :  00 01 02 03 04 05 06 07  |  Decoded Text", WS_CHILD | WS_VISIBLE, psLST2.x, psLST2.y-20, szLST.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_4), 0, 0);
	if (label4 == INVALID_HANDLE_VALUE) return FALSE;
	
	HWND label5 = CreateWindowEx(0, L"STATIC", L"File Size", WS_CHILD | WS_VISIBLE, psLST3.x, psLST3.y - 20, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_5), 0, 0);
	if (label5 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label6 = CreateWindowEx(0, L"STATIC", L"Equal", WS_CHILD | WS_VISIBLE, psLST3.x, psLST3.y + 40, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_6), 0, 0);
	if (label6 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label7 = CreateWindowEx(0, L"STATIC", L"Different ", WS_CHILD | WS_VISIBLE, psLST3.x, psLST3.y + 100, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_7), 0, 0);
	if (label7 == INVALID_HANDLE_VALUE) return FALSE;

	HWND label8 = CreateWindowEx(0, L"STATIC", L"Different Rate ", WS_CHILD | WS_VISIBLE, psLST3.x, psLST3.y + 160, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_LABEL_8), 0, 0);
	if (label8 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE1 = CreateWindow(L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER, psEd1.x, psEd1.y, szEd.cx, szEd.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_1), hInst, 0);
	if (hE1 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE2 = CreateWindow(L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER, psEd2.x, psEd2.y, szEd.cx, szEd.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_2), hInst, 0);
	if (hE2 == INVALID_HANDLE_VALUE) return FALSE;

	hL1 = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED, psLST1.x, psLST2.y, szLST.cx, szLST.cy, hWnd, reinterpret_cast<HMENU>(ID_LISTBOX_1), hInst, 0);
	if (hL1 == INVALID_HANDLE_VALUE) return FALSE;

	hL2 = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED, psLST2.x, psLST2.y, szLST.cx, szLST.cy, hWnd, reinterpret_cast<HMENU>(ID_LISTBOX_2), hInst, 0);
	if (hL2 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE3 = CreateWindowEx(0, L"Edit", NULL , WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | LBS_OWNERDRAWFIXED, psLST3.x, psLST3.y, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_3), hInst, 0);
	if (hE3 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE4 = CreateWindowEx(0, L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | LBS_OWNERDRAWFIXED, psLST3.x, psLST3.y+60, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_4), hInst, 0);
	if (hE4 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE5 = CreateWindowEx(0, L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | LBS_OWNERDRAWFIXED, psLST3.x, psLST3.y+120, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_5), hInst, 0);
	if (hE5 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hE6 = CreateWindowEx(0, L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | LBS_OWNERDRAWFIXED, psLST3.x, psLST3.y + 180, szLST2.cx, szLbl.cy, hWnd, reinterpret_cast<HMENU>(ID_TEXTBOX_6), hInst, 0);
	if (hE6 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hwndBtCompare = CreateWindow(L"BUTTON", L"Compare", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, psBtCMP.x, psBtCMP.y, szBtCmp.cx, szBtCmp.cy, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_COMPARE), hInst, NULL);
	if (hwndBtCompare == INVALID_HANDLE_VALUE) return FALSE;

	HWND hwndBt1 = CreateWindow(L"BUTTON", L"Browse", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, psBtBrowse1.x, psBtBrowse1.y, szBtLOAD.cx, szBtLOAD.cy, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_LOAD_1), hInst, NULL);
	if (hwndBt1 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hwndBt2 = CreateWindow(L"BUTTON", L"Browse", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, psBtBrowse2.x, psBtBrowse2.y, szBtLOAD.cx, szBtLOAD.cy, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_LOAD_2), hInst, NULL);
	if (hwndBt2 == INVALID_HANDLE_VALUE) return FALSE;

	HWND hwndBtNEXT = CreateWindow(L"BUTTON", L"NextPage", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, psNEXT.x, psNEXT.y, szBtPg.cx, szBtPg.cy, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_NEXT), hInst, NULL);
	if (hwndBtNEXT == INVALID_HANDLE_VALUE) return FALSE;

	HWND hwndBtPREV = CreateWindow(L"BUTTON", L"PrevPage", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, psPREV.x, psPREV.y, szBtPg.cx, szBtPg.cy, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_PREV), hInst, NULL);
	if (hwndBtPREV == INVALID_HANDLE_VALUE) return FALSE;

	return TRUE;
}

VOID getFilePath(HWND hWnd, HWND hEdit)
{
	OPENFILENAME ofn;
	TCHAR szFile[260] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		SendMessage(hEdit, WM_SETTEXT, MAX_LOADSTRING, LPARAM(ofn.lpstrFile));
		UpdateWindow(hEdit);
	}
}

DWORD WINAPI CheckingNotDiff(LPVOID lpParam)
{
	HWND hWnd = (HWND)lpParam;

	HWND hE1 = GetDlgItem(hWnd, ID_TEXTBOX_1);
	HWND hE2 = GetDlgItem(hWnd, ID_TEXTBOX_2);
	TCHAR buff1[MAX_LOADSTRING];
	TCHAR buff2[MAX_LOADSTRING];
	SendMessage(hE1, WM_GETTEXT, MAX_LOADSTRING, (LPARAM)buff1);
	SendMessage(hE2, WM_GETTEXT, MAX_LOADSTRING, (LPARAM)buff2);

	if (notdiff(buff1, buff2)==1) MessageBox(hWnd, L"There is no different!", L"Message", MB_OK);

	return 0;
}

void ViewPage(const WCHAR* pszFileName1, const WCHAR* pszFileName2, unsigned long long offset)
{
	HANDLE hfile1 = CreateFile(pszFileName1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hfile2 = CreateFile(pszFileName2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hfile1 != INVALID_HANDLE_VALUE && hfile2 != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size1{};
		LARGE_INTEGER file_size2{};
		GetFileSizeEx(hfile1, &file_size1);
		GetFileSizeEx(hfile2, &file_size2);

		const unsigned long long cbFile1 = static_cast<unsigned long long>(file_size1.QuadPart);
		const unsigned long long cbFile2 = static_cast<unsigned long long>(file_size2.QuadPart);

		MaxHexFile = cbFile1 > cbFile2 ? cbFile1 : cbFile2;
		
		HANDLE hmap1 = CreateFileMapping(hfile1, NULL, PAGE_READONLY, 0, 0, NULL);
		HANDLE hmap2 = CreateFileMapping(hfile2, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hmap1 != NULL && hmap2 != NULL)
		{
			SendMessage(hL1, LB_RESETCONTENT, 0, 0);
			SendMessage(hL2, LB_RESETCONTENT, 0, 0);
			SendMessage(hL1, WM_SETREDRAW, FALSE, 0);
			SendMessage(hL2, WM_SETREDRAW, FALSE, 0);
			
			
			DWORD HexPerPage1 = HexPerPage;
			DWORD HexPerPage2 = HexPerPage;
			DWORD high = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFul);
			DWORD low = static_cast<DWORD>(offset & 0xFFFFFFFFul);
			if (offset + HexPerPage > cbFile1)
			{
				HexPerPage1 = static_cast<int>(cbFile1 - offset);
			}

			if (offset + HexPerPage > cbFile2)
			{
				HexPerPage2 = static_cast<int>(cbFile2 - offset);
			}

			const unsigned char* pView1 = NULL;
			const unsigned char* pView2 = NULL;

			if (offset + HexPerPage1 <= cbFile1) pView1 = static_cast<const unsigned char*>(MapViewOfFile(hmap1, FILE_MAP_READ, high, low, HexPerPage1));
			if (offset + HexPerPage2 <= cbFile2) pView2 = static_cast<const unsigned char*>(MapViewOfFile(hmap2, FILE_MAP_READ, high, low, HexPerPage2));

			if (pView1 != NULL || pView2 != NULL)
			{

				for (int i = 0; i < HexPerPage; i += 8)
				{
					unsigned char data = 0;
					if (pView1 != NULL && pView2 != NULL)
					{
						for (int j = 0; j < 8; j++)
						{
							int index = j + i;
							if (index >= HexPerPage1 || index >= HexPerPage2) break;

							if (pView1[index] == pView2[index]) data |= 1 << j;
						}
					}
					
					if (i < HexPerPage1) SetFunction(hL1, displayHex(pView1, i, offset), data);
					else SetFunction(hL1, displayHex(NULL, i, offset), data);

					if (i < HexPerPage2) SetFunction(hL2, displayHex(pView2, i, offset), data);
					else SetFunction(hL2, displayHex(NULL, i, offset), data);
					
				}
			}
			SendMessage(hL1, WM_SETREDRAW, TRUE, 0L);
			SendMessage(hL2, WM_SETREDRAW, TRUE, 0L);
			
			UpdateWindow(hL1);
			UpdateWindow(hL2);
			CloseHandle(hmap1);
			CloseHandle(hmap2);
		}
		CloseHandle(hfile1);
		CloseHandle(hfile2);
	}
}

void SetFunction(HWND hwnd, LPCWSTR pstr, unsigned char data)
{
	int lbItem;

	lbItem = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)pstr);
	SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)lbItem, (LPARAM)data);
}

void InfoDisplay(HWND hwnd1, HWND hwnd2, HWND hwnd3, HWND hwnd4, const WCHAR* pszFileName1, const WCHAR* pszFileName2, unsigned long long offset, int notDiff, int diff)
{
	HANDLE hfile1 = CreateFile(pszFileName1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hfile2 = CreateFile(pszFileName2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hfile1 != INVALID_HANDLE_VALUE && hfile2 != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size1{};
		LARGE_INTEGER file_size2{};
		GetFileSizeEx(hfile1, &file_size1);
		GetFileSizeEx(hfile2, &file_size2);

		const unsigned long long cbFile1 = static_cast<unsigned long long>(file_size1.QuadPart);
		const unsigned long long cbFile2 = static_cast<unsigned long long>(file_size2.QuadPart);

		MaxHexFile = cbFile1 > cbFile2 ? cbFile1 : cbFile2;
		HANDLE hmap1 = CreateFileMapping(hfile1, NULL, PAGE_READONLY, 0, 0, NULL);
		HANDLE hmap2 = CreateFileMapping(hfile2, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hmap1 != NULL && hmap2 != NULL)
		{
			DWORD HexPerPage1 = MaxHexFile;
			DWORD HexPerPage2 = MaxHexFile;
			DWORD high = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFul);
			DWORD low = static_cast<DWORD>(offset & 0xFFFFFFFFul);
			HexPerPage1 = static_cast<int>(cbFile1);
			HexPerPage2 = static_cast<int>(cbFile2);
			const unsigned char* pView1 = NULL;
			const unsigned char* pView2 = NULL;

			pView1 = static_cast<const unsigned char*>(MapViewOfFile(hmap1, FILE_MAP_READ, high, low, HexPerPage1));
			pView2 = static_cast<const unsigned char*>(MapViewOfFile(hmap2, FILE_MAP_READ, high, low, HexPerPage2));

			if (pView1 != NULL || pView2 != NULL)
			{
				
				for (int i = 0; i < MaxHexFile; i += 8)
				{

					unsigned char data = 0;
					if (pView1 != NULL && pView2 != NULL)
					{

						for (int j = 0; j < 8; j++)
						{

							int index = j + i;

							if (index >= HexPerPage1 || index >= HexPerPage2) break;

							if (pView1[index] == pView2[index])
								data |= 1 << j;

							if (data & (1 << j))
							{
								notDiff++;
							}
							else
							{
								diff++;
							}
							
						}
					}

				}
				
			}
			LPWSTR buff1 = new WCHAR[MAX_LOADSTRING];
			LPWSTR buff2 = new WCHAR[MAX_LOADSTRING];
			LPWSTR buff3 = new WCHAR[MAX_LOADSTRING];
			LPWSTR buff4 = new WCHAR[MAX_LOADSTRING];
			int diffx = diff / 1000;
			int maxcb = MaxHexFile / 1000;
			int notdiffx = notDiff / 1000;
			const float percent = (diff*10) / (MaxHexFile/1000);
			int x = int(percent/100);
			const float y = (percent - x*100);
			int z = int(y);

			wsprintf(buff1, L"%i KBytes", maxcb);
			wsprintf(buff2, L"%i KBytes",notdiffx);
			wsprintf(buff3, L"%i KBytes",diffx);
			wsprintf(buff4, L"%i.%i %%",x,z);

			SendMessage(hwnd1, WM_SETTEXT, MAX_LOADSTRING, (LPARAM)buff1);
			SendMessage(hwnd2, WM_SETTEXT, MAX_LOADSTRING, (LPARAM)buff2);
			SendMessage(hwnd3, WM_SETTEXT, MAX_LOADSTRING, (LPARAM)buff3);
			SendMessage(hwnd4, WM_SETTEXT, MAX_LOADSTRING, (LPARAM)buff4);
			UpdateWindow(hwnd1);
			UpdateWindow(hwnd2);
			UpdateWindow(hwnd3);
			UpdateWindow(hwnd4);
			CloseHandle(hmap1);
			CloseHandle(hmap2);
		}
		CloseHandle(hfile1);
		CloseHandle(hfile2);
	}
}

LPWSTR displayHex(const unsigned char* pView, int i, int offset)
{
	char c = '.';
	LPWSTR buff = new WCHAR[35];
	buff[34] = '\0';

	if (pView != NULL)
	{
		wsprintf(buff, L"%010X%02X%02X%02X%02X%02X%02X%02X%02X%c%c%c%c%c%c%c%c", offset + i, pView[i], pView[i + 1], pView[i + 2], pView[i + 3], pView[i + 4], pView[i + 5], pView[i + 6], pView[i + 7],
			pView[i] == 0 ? c : pView[i],
			pView[i + 1] == 0 ? c : pView[i + 1],
			pView[i + 2] == 0 ? c : pView[i + 2],
			pView[i + 3] == 0 ? c : pView[i + 3],
			pView[i + 4] == 0 ? c : pView[i + 4],
			pView[i + 5] == 0 ? c : pView[i + 5],
			pView[i + 6] == 0 ? c : pView[i + 6],
			pView[i + 7] == 0 ? c : pView[i + 7]);
	}
	else
	{
		wsprintf(buff, L"%010X                        ", offset + i);
	}

	return buff;
}

int notdiff(const WCHAR* pszFileName1, const WCHAR* pszFileName2)
{
	SYSTEM_INFO sysinfo{};
	GetSystemInfo(&sysinfo);
	DWORD HexPerPage = sysinfo.dwAllocationGranularity;

	BOOL isEqual = 1;

	HANDLE hfile1 = CreateFile(pszFileName1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hfile2 = CreateFile(pszFileName2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hfile1 != INVALID_HANDLE_VALUE && hfile2 != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size1{};
		LARGE_INTEGER file_size2{};

		GetFileSizeEx(hfile1, &file_size1);
		GetFileSizeEx(hfile2, &file_size2);

		const unsigned long long cbFile1 = static_cast<unsigned long long>(file_size1.QuadPart);
		const unsigned long long cbFile2 = static_cast<unsigned long long>(file_size2.QuadPart);

		const unsigned long long MaxHexFile = cbFile1 > cbFile2 ? cbFile1 : cbFile2;


		HANDLE hmap1 = CreateFileMapping(hfile1, NULL, PAGE_READONLY, 0, 0, NULL);
		HANDLE hmap2 = CreateFileMapping(hfile2, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hmap1 != NULL && hmap2 != NULL)
		{
			DWORD HexPerPage1 = HexPerPage;
			DWORD HexPerPage2 = HexPerPage;
			
			for (unsigned long long offset = 0; offset < MaxHexFile; offset += HexPerPage)
			{
				DWORD high = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFul);
				DWORD low = static_cast<DWORD>(offset & 0xFFFFFFFFul);

				if (offset + HexPerPage > cbFile1)
				{
					HexPerPage1 = static_cast<int>(cbFile1 - offset);
				}

				if (offset + HexPerPage > cbFile2)
				{
					HexPerPage2 = static_cast<int>(cbFile2 - offset);
				}

				if (HexPerPage1 != HexPerPage2)
				{
					isEqual = 0;
					break;
				}

				const char* pView1 = NULL;
				const char* pView2 = NULL;

				if (offset + HexPerPage1 <= cbFile1) pView1 = static_cast<const char*>(MapViewOfFile(hmap1, FILE_MAP_READ, high, low, HexPerPage1));
				if (offset + HexPerPage2 <= cbFile2) pView2 = static_cast<const char*>(MapViewOfFile(hmap2, FILE_MAP_READ, high, low, HexPerPage2));

				if (pView1 != NULL && pView2 != NULL)
				{
					if (memcmp(pView1, pView2, HexPerPage1))
					{
						isEqual = 0;
						break;
					}
				}
			}
			UpdateWindow(hL1);
			UpdateWindow(hL2);

			CloseHandle(hmap1);
			CloseHandle(hmap2);
		}
		else isEqual = 0;
		CloseHandle(hfile1);
		CloseHandle(hfile2);
	}
	else isEqual = 0;


	return isEqual;
}
