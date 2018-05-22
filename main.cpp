/************************************************************************************/
/*								DISPLAY LOCKER										*/
/************************************************************************************/


#include <windows.h>
#include "resource.h"
#include <iostream>
#include <fstream>

#pragma comment(lib,"PowrProf.lib")

char* strPassword;

STICKYKEYS g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
TOGGLEKEYS g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
FILTERKEYS g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};   

BOOL CALLBACK PasswordDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK LockScreenDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
void tm(BOOL);
void kb(BOOL);
void ms(BOOL);
//void tb(BOOL);
void AllowAccessibilityShortcutKeys(bool);
DWORD WINAPI BackgroundThread( LPVOID lpParam );

HANDLE backgroundThreadHandle;
DWORD backgroundThreadId;

HHOOK kbHook;
HHOOK msHook;

HWND lockScreenHwnd;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    MSG Msg;

	int ret;
	std::ifstream pFile("dt", std::ifstream::in);
	if (pFile) {
    // get length of file:
    pFile.seekg (0, pFile.end);
    int length = pFile.tellg();
    pFile.seekg (0, pFile.beg);
	
	strPassword = (char*)GlobalAlloc(GPTR, length + 1);

    // read data as a block:
    pFile.read (strPassword,length);

    pFile.close();
	ret=IDOK;
	} else {


	ret = DialogBox(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(IDD_PASSWORD), NULL, PasswordDlgProc);
	}

    if(ret == IDOK){
		//Create the lock screen
		
		// Save the current sticky/toggle/filter key settings so they can be restored them later
		SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
		SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
		SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);

	kb(FALSE);
		ms(FALSE);
		tm(FALSE);
		//tb(FALSE);
		AllowAccessibilityShortcutKeys(false);

		DialogBox(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(IDD_LOCKSCREEN), NULL, LockScreenDlgProc);

		kb(TRUE);
		ms(TRUE);
		tm(TRUE);	
		//tb(TRUE);
		AllowAccessibilityShortcutKeys(true);

		PostQuitMessage(0);
    }
    else if(ret == IDCANCEL){
		PostQuitMessage(0);
    }
    else if(ret == -1){
        MessageBox(NULL, "Failed to load!", "Error",
            MB_OK | MB_ICONINFORMATION);
		PostQuitMessage(0);
    }

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

//The password Window Dialog Procedure
BOOL CALLBACK PasswordDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	RECT rc,rcDlg,rcOwner;
    switch(Message)
    {
        case WM_INITDIALOG:
			GetWindowRect(GetDesktopWindow(), &rcOwner); 
			GetWindowRect(hwnd, &rcDlg); 
			CopyRect(&rc, &rcOwner); 

			// Offset the owner and dialog box rectangles so that right and bottom 
			// values represent the width and height, and then offset the owner again 
			// to discard space taken up by the dialog box. 

			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
			OffsetRect(&rc, -rc.left, -rc.top); 
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

			// The new position is the sum of half the remaining space and the owner's 
			// original position. 

			SetWindowPos(hwnd, 
							HWND_TOP, 
							rcOwner.left + (rc.right / 2), 
							rcOwner.top + (rc.bottom / 2), 
							0, 0,          // Ignores size arguments. 
							SWP_NOSIZE); 

			if (GetDlgCtrlID((HWND) wParam) != IDC_PASSWORD) 
			{ 
				SetFocus(GetDlgItem(hwnd, IDC_PASSWORD)); 
				return FALSE; 
			} 
			return TRUE; 

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				// OK Button Pressed
                case IDOK:
					/*
						state = 0 -> password length not correct
						state = 1 -> password do not match
						state = 2 -> passwords match
					*/
					int state,len1,len2;
					//Get the length of 2 entered passwords
					len1 = GetWindowTextLength(GetDlgItem(hwnd, IDC_PASSWORD));
					len2 = GetWindowTextLength(GetDlgItem(hwnd, IDC_CONFIRM));
					if(len1 > 0 && len2 > 0 && len1<=25 && len2<=25) //check whether length is correct
					{
						char* password1;
						char* password2;
						
						//allocate memory for 2 passwords
						password1 = (char*)GlobalAlloc(GPTR, len1 + 1);
						password2 = (char*)GlobalAlloc(GPTR, len2 + 1);
						
						//Get the entered passwords
						GetDlgItemText(hwnd, IDC_PASSWORD, password1, len1 + 1);
						GetDlgItemText(hwnd, IDC_CONFIRM, password2, len2 + 1);
						
						//compare the 2 passwords
						if(strcmp(password1, password2) != 0)
							state = 1;
						else
						{
							//Passwords are correct and matching. So copy the password to a global variable for accessing later
							state = 2;
							strPassword = (char*)GlobalAlloc(GPTR, len1 + 1);
							strcpy(strPassword,password1);
						}

						//Free the 2 password variables 
						GlobalFree((HANDLE)password1);
						GlobalFree((HANDLE)password2);
					} else {
						state = 0; //password length incorrect
					}

					if(state== 0)
						MessageBox(NULL, "Password should have 1 to 25 characters", "Error!",
						MB_ICONEXCLAMATION | MB_OK);
					else if(state==1)
						MessageBox(NULL, "Passwords do not match!", "Error!",
						MB_ICONEXCLAMATION | MB_OK);
					else
						EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

//The password Window Dialog Procedure
BOOL CALLBACK LockScreenDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
	/*case WM_CTLCOLORDLG:
			//return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
			break;*/
	/*case WM_CTLCOLORSTATIC:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);

		break;*/

	case WM_ACTIVATE:
		SetCursorPos(0,0);
		if (GetDlgCtrlID((HWND) wParam) != IDC_UNLOCK)
			SetFocus(GetDlgItem(hwnd, IDC_UNLOCK)); 
		SetWindowLong (hwnd, GWL_EXSTYLE, GetWindowLong (hwnd, GWL_EXSTYLE ) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwnd,RGB(0, 0, 0),200,LWA_ALPHA);
		break;

			case WM_INITDIALOG:
			
				//important to set this.. backgroundThread uses this handle to do tasks
				lockScreenHwnd = hwnd;

				//start the background thread
				backgroundThreadHandle = CreateThread( 
				NULL,                   // default security attributes
				0,                      // use default stack size  
				BackgroundThread,       // thread function name
				NULL,	          // argument to thread function 
				0,                      // use default creation flags 
				&backgroundThreadId);

				RECT rcText, rcBackground;
				GetWindowRect(GetDlgItem(hwnd, IDC_UNLOCK), &rcText); 
				GetWindowRect(GetDlgItem(hwnd, IDC_BACKGROUND), &rcBackground); 

				// Offset the  rectangles so that right and bottom 
				// values represent the width and height, and then offset the owner again 
				// to discard space taken up by the dialog box. 

				OffsetRect(&rcText, -rcText.left, -rcText.top); 
				OffsetRect(&rcBackground, -rcBackground.left, -rcBackground.top); 

			
				int cxScreen, cyScreen;
				cxScreen = GetSystemMetrics(SM_CXSCREEN);
				cyScreen = GetSystemMetrics(SM_CYSCREEN);

				SetWindowPos(hwnd, 
							 HWND_TOPMOST, 
							 0, 
							 0, 
							 cxScreen, cyScreen, 
							SWP_NOMOVE | SWP_SHOWWINDOW); 

				SetWindowPos(
					GetDlgItem(hwnd, IDC_BACKGROUND), 
					NULL, 
					(cxScreen/2) - 300,
					(cyScreen/2) - 200,
					600,
					338,
					0
					);
				
				SetWindowPos(
					GetDlgItem(hwnd, IDC_UNLOCK), 
					NULL, 
					(cxScreen/2) - 57,
					(cyScreen/2)  - 33,
					rcText.right,
					14,
					SWP_SHOWWINDOW
					);


				if (GetDlgCtrlID((HWND) wParam) != IDC_UNLOCK) 
				{ 
					SetFocus(GetDlgItem(hwnd, IDC_UNLOCK)); 
					return true; 
				} 


				return TRUE; 
        case WM_COMMAND:
			if(LOWORD(wParam) == IDC_UNLOCK && HIWORD(wParam) == EN_CHANGE)
			{
				int len = GetWindowTextLength(GetDlgItem(hwnd, IDC_UNLOCK));
				char* enteredPassword = (char*)GlobalAlloc(GPTR, len + 1);
						
				//Get the entered passwords
				GetDlgItemText(hwnd, IDC_UNLOCK, enteredPassword, len + 1);

				if(strcmp(enteredPassword, strPassword) == 0) {
					
					GlobalFree((HANDLE)strPassword);
					EndDialog(hwnd, NULL);
				}

			}
			return true;
        break;

		case WM_QUERYENDSESSION:
			AbortSystemShutdown(NULL); 
			//SetSuspendState(FALSE,FALSE,FALSE);
			break;

		case WM_DESTROY:
			TerminateThread(backgroundThreadHandle,0);
			break;
		//case WM_WINDOWPOSCHANGED:
			//DefWindowProc(hwnd, Message, wParam, lParam);
			//windowPosPtr = (WINDOWPOS*) lParam;

			//char l[100];
			//GetWindowText(windowPosPtr->hwnd, l, 99);
			
			//OutputDebugStringA(l);
			
			//if(GetNextWindow(hwnd,GW_HWNDNEXT) != NULL && loaded==1)
			//SetWindowPos(windowPosPtr->hwndInsertAfter,hwnd,0,0,0,0,0);
			//SetWindowPos(hwnd, 
			//					HWND_TOP, 
			//					0, 
			//					0, 
			//					0, 0,          // Ignores size arguments. 
			//					0);			
			//return 0;

        default:
            return FALSE;
    }
    return TRUE;
}

void tm(BOOL enable)
{
	HKEY regHandle;
	
	DWORD dwValue;
	dwValue = (enable==TRUE)? 0 : 1;

	BYTE* data = (BYTE*)&dwValue;

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, NULL, NULL, KEY_WRITE | KEY_WOW64_32KEY,NULL , &regHandle ,NULL );
	
	RegSetValueEx(regHandle,"DisableTaskmgr",0, REG_DWORD,data ,sizeof(DWORD));
	
}

/*
void tb(BOOL enable) 
{
	 HWND hwnd1 = FindWindow("Shell_traywnd", "");
	 if(enable)
		SetWindowPos(hwnd1, 0, 0, 0, 0, 0, SWP_SHOWWINDOW);
	 else
		SetWindowPos(hwnd1, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
}*/

void kb(BOOL enable)
{
	if(enable==FALSE)
		kbHook = SetWindowsHookEx(WH_KEYBOARD_LL,LowLevelKeyboardProc,GetModuleHandle("user32.dll"),NULL);

	else
	{

		UnhookWindowsHookEx(kbHook);

		/* Ctrl and alt are pressed down when user presses ctrl+ alt+ del. Release them 
			TODO: Check if all the 6 are necessary 
			*/
		INPUT input;

		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_LCONTROL;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));

		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_RCONTROL;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));
		
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_LMENU;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));
		
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_RMENU;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));
		
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_CONTROL;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));
		
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_MENU;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(INPUT));
	}
}

void ms(BOOL enable)
{
	if(enable==FALSE)
		msHook = SetWindowsHookEx(WH_MOUSE_LL,LowLevelMouseProc,GetModuleHandle("user32.dll"),NULL);
	else
		UnhookWindowsHookEx(msHook);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	
	if (nCode < 0 || nCode != HC_ACTION ) return CallNextHookEx( NULL, nCode, wParam, lParam); 

    KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*) lParam;

    if(
		(p->vkCode == VK_BACK) ||
		(p->vkCode == VK_SHIFT) ||
		(p->vkCode == VK_CAPITAL) ||
		(p->vkCode == VK_SPACE) ||
		(p->vkCode == VK_HOME) ||
		(p->vkCode == VK_END) ||
		//(p->vkCode == VK_ESCAPE) ||
		//(p->vkCode == VK_LEFT) ||
		//(p->vkCode == VK_RIGHT) ||
		(p->vkCode >= 0x30 && p->vkCode <= 0x39) ||
		(p->vkCode >= 0x41 && p->vkCode <= 0x5A) ||
		(p->vkCode >= 0x60 && p->vkCode <= 0x6F) ||
		(p->vkCode == 0x90) ||
		(p->vkCode == 0x91) ||
		(p->vkCode == 0xA0) ||
		(p->vkCode == 0xA1) ||
		(p->vkCode >= 0xBA && p->vkCode <= 0xC0) ||
		(p->vkCode >= 0xDB && p->vkCode <= 0xDF) ||
		(p->vkCode == 0xE2)
		)
        {
            return CallNextHookEx(NULL, nCode, wParam, lParam);  
        }
		else
			return 1;
	/*
	bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	bool altDown = (p->flags & LLKHF_ALTDOWN);
	if(p->vkCode == VK_TAB && altDown) return 1;
	if(p->vkCode == VK_ESCAPE && ctrlDown) return 1;
	if(p->vkCode == VK_TAB && ctrlDown && altDown) return 1;*/

    return CallNextHookEx(NULL, nCode, wParam, lParam);     
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0 || nCode != HC_ACTION ) return CallNextHookEx( NULL, nCode, wParam, lParam); 

	if(wParam==WM_LBUTTONDOWN)
		return CallNextHookEx(NULL, nCode, wParam, lParam); 
	return 1; //full hook;
}

void AllowAccessibilityShortcutKeys( bool bAllowKeys )
{
    if( bAllowKeys )
    {
        // Restore StickyKeys/etc to original state and enable Windows key      
        STICKYKEYS sk = g_StartupStickyKeys;
        TOGGLEKEYS tk = g_StartupToggleKeys;
        FILTERKEYS fk = g_StartupFilterKeys;
        
        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
        SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
    }
    else
    {
        // Disable StickyKeys/etc shortcuts but if the accessibility feature is on, 
        // then leave the settings alone as its probably being usefully used
 
        STICKYKEYS skOff = g_StartupStickyKeys;
        if( (skOff.dwFlags & SKF_STICKYKEYSON) == 0 )
        {
            // Disable the hotkey and the confirmation
            skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
            skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;
 
            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
        }
 
        TOGGLEKEYS tkOff = g_StartupToggleKeys;
        if( (tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0 )
        {
            // Disable the hotkey and the confirmation
            tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
            tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;
 
            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
        }
 
        FILTERKEYS fkOff = g_StartupFilterKeys;
        if( (fkOff.dwFlags & FKF_FILTERKEYSON) == 0 )
        {
            // Disable the hotkey and the confirmation
            fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
            fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;
 
            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
        }
    }
}

/*
1. closes tmgr
2. sets always on top
*/
DWORD WINAPI BackgroundThread( LPVOID lpParam ) 
{ 
	HWND tmgrHwnd;
	while(1)
	{
		tmgrHwnd = FindWindow(NULL,"Windows Task Manager");
		SendMessage(tmgrHwnd, WM_CLOSE, (LPARAM) 0, (WPARAM) 0);
		
		if(lockScreenHwnd != GetForegroundWindow()) 
		{
			SetWindowPos(GetForegroundWindow(),HWND_NOTOPMOST,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(lockScreenHwnd,HWND_TOPMOST,0,0,0,0, SWP_NOSIZE | SWP_NOMOVE);
			SetActiveWindow(lockScreenHwnd);
			SetFocus(GetDlgItem(lockScreenHwnd, IDC_UNLOCK)); 
		}
		Sleep(10);
	}
    return 0; 
} 