

#pragma once


class CSHServiceManager
{
	public:
		CSHServiceManager(LPTSTR lpServiceName, BOOL stopFlag = TRUE, BOOL pauseContinueFlag = FALSE, BOOL shutdownFlag = FALSE, BOOL sessionChangeFlag = FALSE, BOOL powerEeventFlag = FALSE);


	/////////////////////////////////////////////////////////////


	public:
		BOOL ServiceRun(CSHServiceManager &serviceManager);

	public:
		static VOID WINAPI ServiceMain(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors);

	public:
		static DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

		
	/////////////////////////////////////////////////////////////


	private:
		LPTSTR serviceName;

	private:
		SERVICE_STATUS_HANDLE serviceStatusHandle;

	private:
		SERVICE_STATUS serviceStatus;


	/////////////////////////////////////////////////////////////

	public:
		static CSHServiceManager *pCSHServiceManager;

	
	/////////////////////////////////////////////////////////////


	public:
		HANDLE socketServerThread;


	/////////////////////////////////////////////////////////////


	public:
		VOID Start(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors);

	public:
		VOID Stop();

	public:
		VOID Pause();

	public:
		VOID Continue();

	public:
		VOID Shutdown();


	/////////////////////////////////////////////////////////////


	public:
		VOID SessionChange(WTSSESSION_NOTIFICATION wtSsessionNotification);

	public:
		VOID SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NULL, DWORD dwWaitHint = NULL);

	public:
		VOID ReportErrorEvent(LPTSTR lpFunctionName, DWORD dwErrorCode);


	/////////////////////////////////////////////////////////////
};
