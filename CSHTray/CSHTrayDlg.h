

#pragma once


#define UID_CSHTRAY_NOTIFY 1000


#define SECURITY_TIMER 1000


#define DEFAULT_TIMEOUT 300


#define DESKTOP_NAME _T("SecurityDesktop")


static UINT WM_INDICATE_TRAYNOTIFY = RegisterWindowMessage(_T("TrayNotifyCallBack"));


static UINT WM_INDICATE_SECURITY_ACTIVATE = RegisterWindowMessage(_T("SecurityModuleActivate"));


static UINT WM_INDICATE_SECURITY_DEACTIVATE = RegisterWindowMessage(_T("SecurityModuleDeActivate"));


UINT SecurityModuleMonitoringThread(VOID *lpVoid);


class CCSHTrayDlg : public CDialogEx
{
	DECLARE_MESSAGE_MAP()

		
	/////////////////////////////////////////////////////////////


	public:
		explicit CCSHTrayDlg(CWnd *pParent = nullptr);

	public:
		~CCSHTrayDlg();


	/////////////////////////////////////////////////////////////


	#ifdef AFX_DESIGN_TIME
		protected:
			enum { IDD = IDD_CSHTRAY_DIALOG };
	#endif


	protected:
		HICON m_hIcon;


	/////////////////////////////////////////////////////////////


	protected:
		afx_msg VOID OnPaint();

	protected:
		afx_msg HCURSOR OnQueryDragIcon();

	protected:
		virtual VOID DoDataExchange(CDataExchange *pDX);

	protected:
		virtual BOOL OnInitDialog();


	/////////////////////////////////////////////////////////////

	protected:
		virtual BOOL PreTranslateMessage(MSG *pMsg);

	protected:
		afx_msg VOID OnWindowPosChanging(WINDOWPOS *lpwndpos);

	protected:
		afx_msg VOID OnTimer(UINT_PTR nIDEvent);

	protected:
		afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);

	protected:
		afx_msg BOOL OnQueryEndSession();


	/////////////////////////////////////////////////////////////


	public:
		NOTIFYICONDATA notifyIconData;


	/////////////////////////////////////////////////////////////


	public:
		BOOL UI_Flag;

	public:
		BOOL securityModuleFlag;

	public:
		BOOL isActivated;

	public:
		BOOL entireRestoreFlag;


	/////////////////////////////////////////////////////////////


	public:
		HMODULE hModule;

	public:
		FARPROC InstallHook;

	public:
		FARPROC UnInstallHook;


	/////////////////////////////////////////////////////////////


	public:
		CWinThread *moniteringThread;


	/////////////////////////////////////////////////////////////


	public:
		DWORD timeOutSecond;


	/////////////////////////////////////////////////////////////


	public:
		CString libraryPath;

	public:
		CString modulePath;

	public:
		CString commonRegistryPath;


	/////////////////////////////////////////////////////////////

	public:
		VOID PathConfiguration();

	public:
		VOID CreateTrayNotify();

	public:
		BOOL InvestigateProcess(LPCTSTR targetProcessName);

	public:
		VOID InjectionDLL();

	public:
		VOID CleanUpAll();

	public:
		VOID TemporaryDisableOptions();

	public:
		VOID RestoreDisabledOptions();

	public:
		VOID GetCurrentScreenSaverInterval();


	/////////////////////////////////////////////////////////////


	public:
		afx_msg VOID OnVisibleUI();

	public:
		afx_msg VOID OnInVisibleUI();

	public:
		afx_msg VOID OnSecurityModuleActivate();

	public:
		afx_msg VOID OnSecurityModuleDeActivate();


	/////////////////////////////////////////////////////////////


	public:
		LRESULT TrayNotifyCallBack(WPARAM wParam, LPARAM lParam);

	public:
		LRESULT SecurityActivate(WPARAM wParam, LPARAM lParam);

	public:
		LRESULT SecurityDeActivate(WPARAM wParam, LPARAM lParam);
};
