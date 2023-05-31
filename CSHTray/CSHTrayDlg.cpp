
#include "framework.h"
#include "CSHTray.h"
#include "CSHTrayDlg.h"
#include "afxdialogex.h"
#include "tlhelp32.h"
#include "setupapi.h"


UINT SecurityModuleMonitoringThread(VOID *lpVoid)
{
	UINT returnCode = 1;


	CCSHTrayDlg *pContext = (CCSHTrayDlg *)lpVoid;


	TCHAR queryBuffer[MAX_PATH];


	DWORD dwQueryBuffer = NULL;


	BOOL isConnected = FALSE;


	while (TRUE) 
	{
		HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, _T("USB"), NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);


		SP_DEVINFO_DATA spDevInfoData;

		memset(&spDevInfoData, NULL, sizeof(spDevInfoData));


		spDevInfoData.cbSize = sizeof(spDevInfoData);


		for (INT index = NULL; SetupDiEnumDeviceInfo(hDevInfo, index, &spDevInfoData); index++)
		{
			memset(&queryBuffer, NULL, sizeof(queryBuffer));


			SetupDiGetDeviceInstanceId(hDevInfo, &spDevInfoData, queryBuffer, MAX_PATH - 1, &dwQueryBuffer);


			if (_tcscmp(queryBuffer, _T("VID_1532&PID_007B")) == ERROR_SUCCESS) 
			{
				isConnected = TRUE;


				break;
			}
		}


		SetupDiDestroyDeviceInfoList(hDevInfo);


		if ((isConnected == FALSE) && (pContext->isActivated == FALSE))
		{
			if (pContext->InvestigateProcess(_T("LogonUI.exe")) == FALSE)
			{
				PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);
			}
		}


		Sleep(800);
	}


	return returnCode;
}


BEGIN_MESSAGE_MAP(CCSHTrayDlg, CDialogEx)

	ON_WM_PAINT()

	ON_WM_QUERYDRAGICON()

	ON_WM_WINDOWPOSCHANGING()

	ON_WM_TIMER()

	ON_WM_POWERBROADCAST()

	ON_WM_QUERYENDSESSION()


	/////////////////////////////////////////////////////////////


	ON_COMMAND(IDI_VISIBLE_UI, &CCSHTrayDlg::OnVisibleUI)

	ON_COMMAND(IDI_INVISIBLE_UI, &CCSHTrayDlg::OnInVisibleUI)

	ON_COMMAND(IDI_SECURITYMODULE_ACTIVATE, &CCSHTrayDlg::OnSecurityModuleActivate)

	ON_COMMAND(IDI_SECURITYMODULE_DEACTIVATE, &CCSHTrayDlg::OnSecurityModuleDeActivate)


	/////////////////////////////////////////////////////////////


	ON_REGISTERED_MESSAGE(WM_INDICATE_TRAYNOTIFY, &CCSHTrayDlg::TrayNotifyCallBack)

	ON_REGISTERED_MESSAGE(WM_INDICATE_SECURITY_ACTIVATE, &CCSHTrayDlg::SecurityActivate)

	ON_REGISTERED_MESSAGE(WM_INDICATE_SECURITY_DEACTIVATE, &CCSHTrayDlg::SecurityDeActivate)


	/////////////////////////////////////////////////////////////

	
END_MESSAGE_MAP()


CCSHTrayDlg::CCSHTrayDlg(CWnd *pParent) : CDialogEx(IDD_CSHTRAY_DIALOG, pParent), notifyIconData({NULL, }), UI_Flag(FALSE), securityModuleFlag(FALSE), isActivated(FALSE), entireRestoreFlag(FALSE), hModule(NULL), InstallHook(NULL), UnInstallHook(NULL), moniteringThread(NULL), timeOutSecond(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_CSHTRAY_ICON);


	libraryPath.Empty();


	modulePath.Empty();


	commonRegistryPath.Empty();
}


CCSHTrayDlg::~CCSHTrayDlg()
{
	CleanUpAll();


	ShutdownBlockReasonDestroy(theApp.m_pMainWnd->GetSafeHwnd());
}


VOID CCSHTrayDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);


		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		
		int cxIcon = GetSystemMetrics(SM_CXICON);

		int cyIcon = GetSystemMetrics(SM_CYICON);


		CRect rect;

		GetClientRect(&rect);


		int x = (rect.Width() - cxIcon + 1) / 2;

		int y = (rect.Height() - cyIcon + 1) / 2;

		
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CCSHTrayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


VOID CCSHTrayDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BOOL CCSHTrayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	SetIcon(m_hIcon, TRUE);

	SetIcon(m_hIcon, FALSE);


	PathConfiguration();


	CreateTrayNotify();


	isActivated = InvestigateProcess(modulePath.GetBuffer());


	modulePath.ReleaseBuffer();


	return TRUE;
}


BOOL CCSHTrayDlg::PreTranslateMessage(MSG *pMsg)
{
	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_CANCEL))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_SYSKEYDOWN) && (pMsg->wParam == VK_F4))
	{
		return TRUE;
	}


	return CDialogEx::PreTranslateMessage(pMsg);
}


VOID CCSHTrayDlg::OnWindowPosChanging(WINDOWPOS *lpwndpos)
{
	if (UI_Flag == FALSE)
	{
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
	}
	else
	{
		lpwndpos->flags &= SWP_SHOWWINDOW;
	}


	CDialogEx::OnWindowPosChanging(lpwndpos);
}


VOID CCSHTrayDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case SECURITY_TIMER:
		{
			GetCurrentScreenSaverInterval();


			LASTINPUTINFO lastinfo;

			memset(&lastinfo, NULL, sizeof(lastinfo));


			lastinfo.cbSize = sizeof(lastinfo);


			GetLastInputInfo(&lastinfo);


			if ((GetTickCount64() - lastinfo.dwTime) > (timeOutSecond * 950))
			{
				if (securityModuleFlag == FALSE)
				{
					::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);
				}
			}
		}

		default:
		{
			break;
		}
	}


	CDialogEx::OnTimer(nIDEvent);
}


UINT CCSHTrayDlg::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
{
	switch (nPowerEvent)
	{
		case PBT_APMRESUMESUSPEND:
		{
			while (TRUE)
			{
				Sleep(1000);


				if (InvestigateProcess(_T("LogonUI.exe")) == FALSE)
				{
					if (securityModuleFlag == FALSE)
					{
						::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);
					}


					break;
				}
			}
		}

		default:
		{
			break;
		}
	}


	return CDialogEx::OnPowerBroadcast(nPowerEvent, nEventData);
}


BOOL CCSHTrayDlg::OnQueryEndSession()
{
	if (!CDialogEx::OnQueryEndSession())
	{
		return FALSE;
	}


	if ((hModule != NULL) || (notifyIconData.cbSize != NULL))
	{
		ShutdownBlockReasonCreate(theApp.m_pMainWnd->GetSafeHwnd(), _T("CSHTray Ready To Cleaning..."));


		return TRUE;
	}


	return FALSE;
}


VOID CCSHTrayDlg::PathConfiguration()
{
	TCHAR tempPath[MAX_PATH];

	memset(tempPath, NULL, sizeof(tempPath));


	GetModuleFileName(NULL, tempPath, MAX_PATH - 1);


	PathRemoveFileSpec(tempPath);


	libraryPath.Format(_T("%s\\"), tempPath);

	libraryPath.Append(_T("CSHMiniHook.dll"));


	modulePath.Format(_T("%s\\"), tempPath);

	modulePath.Append(_T("CSHSensor.exe"));


	commonRegistryPath.Format(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\"));


	HKEY hKeySystem;

	HKEY hKeyExplorer;


	RegCreateKey(HKEY_CURRENT_USER, commonRegistryPath + _T("System"), &hKeySystem);

	RegCreateKey(HKEY_CURRENT_USER, commonRegistryPath + _T("Explorer"), &hKeyExplorer);


	RegCloseKey(hKeySystem);

	RegCloseKey(hKeyExplorer);
}


VOID CCSHTrayDlg::CreateTrayNotify()
{
	notifyIconData.cbSize = sizeof(notifyIconData);

	notifyIconData.hWnd = theApp.m_pMainWnd->GetSafeHwnd();

	notifyIconData.uID = (UINT)UID_CSHTRAY_NOTIFY;

	notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;

	notifyIconData.uCallbackMessage = WM_INDICATE_TRAYNOTIFY;

	notifyIconData.hIcon = theApp.LoadIcon(IDI_CSHTRAY_NOTIFYICON);

	notifyIconData.uTimeout = 1500;

	notifyIconData.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;


	TCHAR title[64];

	memset(title, NULL, sizeof(title));


	_stprintf_s(title, 64 - 1, _T("%s"), _T("CSHTrayNotify"));


	_tcscpy_s(notifyIconData.szInfoTitle, 64 - 1, title);


	TCHAR infomation[256];

	memset(infomation, NULL, sizeof(infomation));


	_stprintf_s(infomation, 256 - 1, _T("%s"), _T("CreateTrayNotify Done"));


	_tcscpy_s(notifyIconData.szInfo, 256 - 1, infomation);


	TCHAR tip[128];

	memset(tip, NULL, sizeof(tip));


	_stprintf_s(tip, 128 - 1, _T("%s"), _T("You Can See And Use Other Fucntion, By Right Click"));


	_tcscpy_s(notifyIconData.szTip, 128 - 1, tip);


	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}


BOOL CCSHTrayDlg::InvestigateProcess(LPCTSTR targetProcessName)
{
	BOOL returnCode = FALSE;


	PROCESSENTRY32 processEntry32;

	memset(&processEntry32, NULL, sizeof(processEntry32));


	processEntry32.dwSize = sizeof(processEntry32);


	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


	processEntry32.dwSize = sizeof(processEntry32);


	if (Process32First(hSnapShot, &processEntry32))
	{
		do
		{
			if (_tcscmp(processEntry32.szExeFile, targetProcessName) == ERROR_SUCCESS)
			{
				CloseHandle(hSnapShot);


				returnCode = TRUE;


				return returnCode;
			}
		} while (Process32Next(hSnapShot, &processEntry32));
	}


	CloseHandle(hSnapShot);


	return returnCode;
}


VOID CCSHTrayDlg::InjectionDLL()
{
	if (hModule == NULL)
	{
		hModule = LoadLibrary(libraryPath.GetBuffer());


		libraryPath.ReleaseBuffer();


		if (hModule != NULL)
		{
			InstallHook = GetProcAddress(hModule, "InstallHook");


			UnInstallHook = GetProcAddress(hModule, "UnInstallHook");


			if ((InstallHook != NULL) && (UnInstallHook != NULL))
			{
				InstallHook();
			}
		}
	}
}


VOID CCSHTrayDlg::CleanUpAll()
{
	KillTimer(SECURITY_TIMER);


	entireRestoreFlag = TRUE;


	RestoreDisabledOptions();


	if (hModule != NULL)
	{
		FreeLibrary(hModule);
	}


	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}


VOID CCSHTrayDlg::TemporaryDisableOptions()
{
	CRegKey registryController;


	if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, commonRegistryPath + _T("System"), KEY_ALL_ACCESS))
	{
		registryController.SetDWORDValue(_T("DisableLockWorkStation"), 1);

		registryController.SetDWORDValue(_T("DisableTaskMgr"), 1);

		registryController.SetDWORDValue(_T("DisableChangePassword"), 1);
	}


	if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, commonRegistryPath + _T("Explorer"), KEY_ALL_ACCESS))
	{
		registryController.SetDWORDValue(_T("NoLogoff"), 1);

		registryController.SetDWORDValue(_T("NoClose"), 1);

		registryController.SetDWORDValue(_T("NoDriveTypeAutoRun"), 0xff);
	}


	if (ERROR_SUCCESS == registryController.Open(HKEY_LOCAL_MACHINE, commonRegistryPath + _T("System"), KEY_ALL_ACCESS))
	{
		registryController.SetDWORDValue(_T("HideFastUserSwitching"), 1);

		registryController.SetDWORDValue(_T("DisableAutomaticRestartSignOn"), 1);
	}


	if (ERROR_SUCCESS == registryController.Open(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Session Manager\\Power"), KEY_ALL_ACCESS))
	{
		registryController.SetDWORDValue(_T("HiberBootEnabled"), 0);
	}


	registryController.Close();
}


VOID CCSHTrayDlg::RestoreDisabledOptions()
{
	CRegKey registryController;


	if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, commonRegistryPath + _T("System"), KEY_ALL_ACCESS))
	{
		if (entireRestoreFlag)
		{
			registryController.DeleteValue(_T("DisableLockWorkStation"));
		}


		registryController.DeleteValue(_T("DisableTaskMgr"));

		registryController.DeleteValue(_T("DisableChangePassword"));
	}


	if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, commonRegistryPath + _T("Explorer"), KEY_ALL_ACCESS))
	{
		registryController.DeleteValue(_T("NoLogoff"));

		registryController.DeleteValue(_T("NoClose"));


		if (entireRestoreFlag)
		{
			registryController.DeleteValue(_T("NoDriveTypeAutoRun"));
		}
	}


	if (ERROR_SUCCESS == registryController.Open(HKEY_LOCAL_MACHINE, commonRegistryPath + _T("System"), KEY_ALL_ACCESS))
	{
		registryController.DeleteValue(_T("HideFastUserSwitching"));


		if (entireRestoreFlag)
		{
			registryController.DeleteValue(_T("DisableAutomaticRestartSignOn"));
		}
	}


	if (entireRestoreFlag)
	{
		if (ERROR_SUCCESS == registryController.Open(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Session Manager\\Power"), KEY_ALL_ACCESS))
		{
			registryController.SetDWORDValue(_T("HiberBootEnabled"), 1);
		}
	}


	registryController.Close();
}

VOID CCSHTrayDlg::GetCurrentScreenSaverInterval()
{
	TCHAR queryBuffer[MAX_PATH];

	memset(queryBuffer, NULL, sizeof(queryBuffer));


	DWORD dwQueryBuffer = MAX_PATH;


	CRegKey registryController;


	if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"), KEY_ALL_ACCESS))
	{
		if (ERROR_SUCCESS == registryController.QueryStringValue(_T("SCRNSAVE.EXE"), queryBuffer, &dwQueryBuffer))
		{
			dwQueryBuffer = NULL;


			SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, NULL, &dwQueryBuffer, NULL);


			timeOutSecond = dwQueryBuffer;
		}
		else
		{
			timeOutSecond = DEFAULT_TIMEOUT;
		}
	}


	registryController.Close();
}


VOID CCSHTrayDlg::OnVisibleUI()
{
	UI_Flag = TRUE;


	Invalidate();
}


VOID CCSHTrayDlg::OnInVisibleUI()
{
	UI_Flag = FALSE;


	Invalidate();
}


VOID CCSHTrayDlg::OnSecurityModuleActivate()
{
	if (isActivated == FALSE)
	{
		InjectionDLL();


		if (hModule != NULL) 
		{
			if (moniteringThread != NULL) 
			{
				moniteringThread = AfxBeginThread((AFX_THREADPROC)SecurityModuleMonitoringThread, this);
			}
			else 
			{
				moniteringThread->ResumeThread();
			}


			::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);
		}
	}
}


VOID CCSHTrayDlg::OnSecurityModuleDeActivate()
{
	if (isActivated == TRUE)
	{
		if (moniteringThread != NULL)
		{
			moniteringThread->SuspendThread();
		}


		::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_DEACTIVATE, NULL, NULL);
	}
}


LRESULT CCSHTrayDlg::TrayNotifyCallBack(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_RBUTTONUP)
	{
		CMenu popUpMenu;


		if (popUpMenu.LoadMenu(IDI_CSHTRAY_MENU))
		{
			SetForegroundWindow();


			CPoint currentPoint;


			GetCursorPos(&currentPoint);


			CMenu *pPopUpMenu = popUpMenu.GetSubMenu(NULL);


			if (UI_Flag)
			{
				pPopUpMenu->EnableMenuItem(IDI_VISIBLE_UI, MFS_DISABLED);

				pPopUpMenu->EnableMenuItem(IDI_INVISIBLE_UI, MFS_ENABLED);
			}
			else
			{
				pPopUpMenu->EnableMenuItem(IDI_VISIBLE_UI, MFS_ENABLED);

				pPopUpMenu->EnableMenuItem(IDI_INVISIBLE_UI, MFS_DISABLED);
			}


			if (securityModuleFlag)
			{
				pPopUpMenu->EnableMenuItem(IDI_SECURITYMODULE_ACTIVATE, MFS_DISABLED);

				pPopUpMenu->EnableMenuItem(IDI_SECURITYMODULE_DEACTIVATE, MFS_ENABLED);
			}
			else
			{
				pPopUpMenu->EnableMenuItem(IDI_SECURITYMODULE_ACTIVATE, MFS_ENABLED);

				pPopUpMenu->EnableMenuItem(IDI_SECURITYMODULE_DEACTIVATE, MFS_DISABLED);
			}


			pPopUpMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RIGHTALIGN, currentPoint.x, currentPoint.y, this);
		}
	}


	return LRESULT();
}


LRESULT CCSHTrayDlg::SecurityActivate(WPARAM wParam, LPARAM lParam)
{
	if (isActivated == FALSE) 
	{
		isActivated = TRUE;


		KillTimer(SECURITY_TIMER);


		HDESK hOriginalDesktop = GetThreadDesktop(GetCurrentThreadId());


		HDESK hNewDesktop = CreateDesktop(DESKTOP_NAME, NULL, NULL, NULL, GENERIC_ALL, NULL);


		STARTUPINFO startUpInfo;

		memset(&startUpInfo, NULL, sizeof(startUpInfo));


		startUpInfo.cb = sizeof(startUpInfo);

		startUpInfo.lpDesktop = (LPTSTR)DESKTOP_NAME;


		PROCESS_INFORMATION processInformation;


		if (CreateProcess(NULL, modulePath.GetBuffer(), NULL, NULL, FALSE, NULL, NULL, NULL, &startUpInfo, &processInformation))
		{
			if (SetThreadDesktop(hNewDesktop)) 
			{
				if (SwitchDesktop(hNewDesktop)) 
				{
					TemporaryDisableOptions();


					securityModuleFlag = TRUE;


					WaitForSingleObject(processInformation.hProcess, INFINITE);


					::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_DEACTIVATE, NULL, NULL);
				}
				else 
				{
					TerminateProcess(processInformation.hProcess, NULL);


					SetThreadDesktop(hOriginalDesktop);
				}
			}
		}


		modulePath.ReleaseBuffer();


		SwitchDesktop(hOriginalDesktop);


        SetThreadDesktop(hOriginalDesktop);


        CloseHandle(processInformation.hThread);


        CloseHandle(processInformation.hProcess);


        CloseHandle(hNewDesktop);
	}


	return LRESULT();
}


LRESULT CCSHTrayDlg::SecurityDeActivate(WPARAM wParam, LPARAM lParam)
{
	if (isActivated == TRUE)
	{
		isActivated = FALSE;


		TCHAR queryBuffer[MAX_PATH];

		memset(queryBuffer, NULL, sizeof(queryBuffer));


		DWORD dwQueryBuffer = MAX_PATH;


		CRegKey registryController;


		if (ERROR_SUCCESS == registryController.Open(HKEY_CURRENT_USER, _T("Software\\CSH\\userSession"), KEY_ALL_ACCESS))
		{
			if (ERROR_SUCCESS == registryController.QueryStringValue(_T("userCertificate"), queryBuffer, &dwQueryBuffer)) 
			{
				CString certificationStatement;

				certificationStatement.Format(_T("%s"), queryBuffer);


				if (certificationStatement.CompareNoCase(_T("Yes")) == ERROR_SUCCESS)
				{
					securityModuleFlag = FALSE;


					RestoreDisabledOptions();


					::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, NULL);


					SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
				}
				else
				{
					::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);


					return LRESULT();
				}


				SetTimer(SECURITY_TIMER, 500, NULL);
			}
			else
			{
				::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_INDICATE_SECURITY_ACTIVATE, NULL, NULL);


				return LRESULT();
			}
		}
	}


	return LRESULT();
}
