
#include "windows.h"
#include "tchar.h"
#include "CSHServiceExcuteProcess.h"
#include "CSHServiceLogger.h"
#include "tlhelp32.h"
#include "wtsApi32.h"
#include "userenv.h"
#include "sddl.h"
#include "atlstr.h"
#include "vector"


#ifndef _UNICODE
	#include "stdio.h"
#endif


using namespace std;


typedef struct _sessionManager
{
	STARTUPINFO userSi;


	PROCESS_INFORMATION userPi;


	DWORD userSessionID;


	TCHAR userName[64];


	TCHAR userSID[64];

} SESSIONMANAGER;


static vector<SESSIONMANAGER> sessionManager;


DWORD WINAPI EachUserTrayMonitoringThread(VOID *lpVoid)
{
	DWORD returnCode = 1;


	SESSIONMANAGER *pSessionManager = (SESSIONMANAGER *)lpVoid;


	PROCESSENTRY32 processEntry32;

	memset(&processEntry32, NULL, sizeof(processEntry32));


	processEntry32.dwSize = sizeof(processEntry32);


	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


	if (Process32First(hSnapShot, &processEntry32))
	{
		do
		{
			if (_tcscmp(processEntry32.szExeFile, MODULE_NAME) == ERROR_SUCCESS)
			{
				if (pSessionManager->userPi.dwProcessId == processEntry32.th32ProcessID)
				{
					CSHServiceLogger(_T("[%s:%d] Target ProcessID Detected, pSessionManager->userPi.dwProcessId = %ld, processEntry32.th32ProcessID = %ld"), _T(__FUNCTION__), __LINE__, pSessionManager->userPi.dwProcessId, processEntry32.th32ProcessID);


					HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pSessionManager->userPi.dwProcessId);


					if (hProcess != INVALID_HANDLE_VALUE)
					{
						CString commonRegistryPath;

						commonRegistryPath.Format(_T("%s\\software\\CSHPortPolio"), pSessionManager->userSID);


						HKEY hKeyUser;


						RegCreateKey(HKEY_USERS, commonRegistryPath, &hKeyUser);


						RegCloseKey(hKeyUser);


						CSHServiceLogger(_T("[%s:%d] User %ld OpenProcess Done, Waiting INFINITE..."), _T(__FUNCTION__), __LINE__, pSessionManager->userPi.dwProcessId);


						WaitForSingleObject(hProcess, INFINITE);


						CSHServiceLogger(_T("[%s:%d] User %ld Process was might be Terminated, Proceed Delete Residual Data"), _T(__FUNCTION__), __LINE__, pSessionManager->userPi.dwProcessId);


						CRegKey registryController;


						if ((registryController.Open(HKEY_USERS, commonRegistryPath, KEY_WRITE) == ERROR_SUCCESS))
						{
							registryController.SetStringValue(_T("userID"), _T("NONE"));


							registryController.SetStringValue(_T("userPassword"), _T("NONE"));


							registryController.SetStringValue(_T("userTimeStamp"), _T("00000000"));


							CSHServiceLogger(_T("[%s:%d] User %s Delete Residual Data Done"), _T(__FUNCTION__), __LINE__, commonRegistryPath);


							returnCode = ERROR_SUCCESS;
						}
						else
						{
							CSHServiceLogger(_T("[%s:%d] User %s Delete Residual Data Failed"), _T(__FUNCTION__), __LINE__, commonRegistryPath);
						}


						vector<SESSIONMANAGER>::iterator currentCursor = sessionManager.begin();


						for (INT index = NULL; index < sessionManager.size(); index++, currentCursor++)
						{
							if (sessionManager[index].userPi.dwProcessId == pSessionManager->userPi.dwProcessId) 
							{
								sessionManager.erase(currentCursor);


								break;
							}
						}


						CloseHandle(hProcess);


						CloseHandle(pSessionManager->userPi.hThread);


						CloseHandle(pSessionManager->userPi.hProcess);


						pSessionManager->userSessionID = NULL;


						memset(pSessionManager->userName, NULL, sizeof(TCHAR) * 64);


						memset(pSessionManager->userSID, NULL, sizeof(TCHAR) * 64);


						break;
					}
				}
			}
		}while (Process32Next(hSnapShot, &processEntry32));
	}


	CloseHandle(hSnapShot);


	return returnCode;
}


BOOL RegistrationSessionManager(SESSIONMANAGER &currentSession, DWORD currentActiveSessionId, DWORD dwQueryBuffer, LPCTSTR queryBuffer, HANDLE currentDuplicatedToken)
{
	BOOL returnCode = FALSE;


	currentSession.userSessionID = currentActiveSessionId;


	_tcscpy_s(currentSession.userName, dwQueryBuffer, queryBuffer);


	TOKEN_USER userSID;

	memset(&userSID, NULL, sizeof(TOKEN_USER));


	DWORD dwUserSID = NULL;


	GetTokenInformation(currentDuplicatedToken, TOKEN_INFORMATION_CLASS::TokenUser, &userSID, 256, &dwUserSID);


	LPTSTR pSID = NULL;


	ConvertSidToStringSid(userSID.User.Sid, &pSID);


	_stprintf_s(currentSession.userSID, 64 - 1, _T("%s"), pSID);


	LocalFree(pSID);


	sessionManager.push_back(currentSession);


	CSHServiceLogger(_T("[%s:%d] Save Session Done, Current sessionManager.size() -> %d"), _T(__FUNCTION__), __LINE__, sessionManager.size());

		
	if (CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EachUserTrayMonitoringThread, &sessionManager.back(), NULL, NULL) != NULL)
	{
		returnCode = TRUE;
	}


	return returnCode;
}


BOOL EstablishProcessAsUser(DWORD currentProcessID, DWORD currentActiveSessionId, LPCTSTR pModulePath, DWORD dwQueryBuffer, LPCTSTR queryBuffer)
{
	BOOL returnCode = FALSE;


	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, currentProcessID);


	if (hProcess != INVALID_HANDLE_VALUE) 
	{
		HANDLE processToken = NULL;


		BOOL bReturnValue = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &processToken);


		if (!bReturnValue) 
		{
			CSHServiceLogger(_T("[%s:%d] OpenProcessToken Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(hProcess);


			return returnCode;
		}


		LUID luid;

		memset(&luid, NULL, sizeof(luid));


		bReturnValue = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);


		if (!bReturnValue)
		{
			CSHServiceLogger(_T("[%s:%d] LookupPrivilegeValue Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(processToken);


			CloseHandle(hProcess);


			return returnCode;
		}


		TOKEN_PRIVILEGES tokenPriVileges;

		memset(&tokenPriVileges, NULL, sizeof(tokenPriVileges));


		tokenPriVileges.PrivilegeCount = 1;

		tokenPriVileges.Privileges[0].Luid = luid;

		tokenPriVileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


		bReturnValue = AdjustTokenPrivileges(processToken, FALSE, &tokenPriVileges, sizeof(tokenPriVileges), NULL, NULL);


		if (!bReturnValue)
		{
			CSHServiceLogger(_T("[%s:%d] AdjustTokenPrivileges Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(processToken);


			CloseHandle(hProcess);


			return returnCode;
		}


		HANDLE duplicatedToken = NULL;


		bReturnValue = DuplicateTokenEx(processToken, TOKEN_ALL_ACCESS, NULL, SECURITY_IMPERSONATION_LEVEL::SecurityDelegation, TOKEN_TYPE::TokenImpersonation, &duplicatedToken);


		if (!bReturnValue)
		{
			CSHServiceLogger(_T("[%s:%d] DuplicateTokenEx Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(processToken);


			CloseHandle(hProcess);


			return returnCode;
		}


		LPVOID pEnvironmentBlock = NULL;


		bReturnValue = CreateEnvironmentBlock(&pEnvironmentBlock, duplicatedToken, FALSE);


		if (!bReturnValue)
		{
			CSHServiceLogger(_T("[%s:%d] CreateEnvironmentBlock Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(duplicatedToken);


			CloseHandle(processToken);


			CloseHandle(hProcess);


			return returnCode;
		}


		TCHAR targetDesktop[64];

		memset(targetDesktop, NULL, sizeof(targetDesktop));
		
		
		_tcscpy_s(targetDesktop, 64 - 1, _T("winsta0\\default"));


		SESSIONMANAGER currentSession;

		memset(&currentSession, NULL, sizeof(currentSession));


		currentSession.userSi.cb = sizeof(currentSession.userSi);

		currentSession.userSi.lpDesktop = targetDesktop;


		bReturnValue = CreateProcessAsUser(duplicatedToken, pModulePath, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, pEnvironmentBlock, NULL, &currentSession.userSi, &currentSession.userPi);


		DestroyEnvironmentBlock(pEnvironmentBlock);


		CloseHandle(processToken);


		CloseHandle(hProcess);


		if (!bReturnValue)
		{
			CSHServiceLogger(_T("[%s:%d] CreateProcessAsUser Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			CloseHandle(currentSession.userPi.hThread);


			CloseHandle(currentSession.userPi.hProcess);


			CloseHandle(duplicatedToken);


			return returnCode;
		}


		returnCode = RegistrationSessionManager(currentSession, currentActiveSessionId, dwQueryBuffer, queryBuffer, duplicatedToken);


		CloseHandle(duplicatedToken);
	}


	return returnCode;
}


BOOL CreateCSHTrayProcess(VOID *lpVoid)
{
	BOOL returnCode = FALSE;


	if (lpVoid != NULL)
	{
		WTSSESSION_NOTIFICATION *wtSsessionNotification = (WTSSESSION_NOTIFICATION *)lpVoid;


		for (DWORD index = NULL; index < sessionManager.size(); index++)
		{
			if (sessionManager[index].userSessionID == wtSsessionNotification->dwSessionId)
			{
				returnCode = TRUE;


				CSHServiceLogger(_T("[%s:%d] Already Have Established, sessionManager.userSessionID = %ld"), _T(__FUNCTION__), __LINE__, sessionManager[index].userSessionID);


				return returnCode;
			}
		}
	}


	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);


	LPFN_ISWOW64PROCESS lpIsWow64Process = NULL;
	
	
	lpIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("Kernel32")), "IsWow64Process");


	if (lpIsWow64Process == NULL)
	{
		{
			CSHServiceLogger(_T("[%s:%d] CreateCSHTrayProcess Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			return returnCode;
		}
	}


	BOOL isWow64Flag = FALSE;


	lpIsWow64Process(GetCurrentProcess(), &isWow64Flag);


	TCHAR environmentVariablePath[MAX_PATH];

	memset(environmentVariablePath, NULL, sizeof(environmentVariablePath));


	if (isWow64Flag) 
	{
		GetEnvironmentVariable(_T("Programfiles(x86)"), environmentVariablePath, MAX_PATH - 1);
	}
	else 
	{
		GetEnvironmentVariable(_T("Programfiles"), environmentVariablePath, MAX_PATH - 1);
	}


	TCHAR modulePath[MAX_PATH];

	memset(modulePath, NULL, sizeof(modulePath));


	if (_tcslen(environmentVariablePath) > NULL)
	{
		_stprintf_s(modulePath, MAX_PATH - 1, _T("%s\\CSHPortPolio\\%s"), environmentVariablePath, MODULE_NAME);
	}


	PROCESSENTRY32 processEntry32;

	memset(&processEntry32, NULL, sizeof(processEntry32));


	processEntry32.dwSize = sizeof(processEntry32);


	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


	WTS_SESSION_INFO wtsSessionInfo;

	memset(&wtsSessionInfo, NULL, sizeof(wtsSessionInfo));


	PWTS_SESSION_INFO pWtsSessionInfo = NULL;


	DWORD sessionCount = NULL;


	WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWtsSessionInfo, &sessionCount);


	for (DWORD index = NULL; index < sessionCount; index++) 
	{
		wtsSessionInfo = pWtsSessionInfo[index];


		if (wtsSessionInfo.State == WTS_CONNECTSTATE_CLASS::WTSActive)
		{
			if (Process32First(hSnapShot, &processEntry32))
			{
				do 
				{
					if (_tcscmp(processEntry32.szExeFile, _T("winlogon.exe")) == ERROR_SUCCESS)
					{
						DWORD dwActiveSessionID = NULL;


						ProcessIdToSessionId(processEntry32.th32ProcessID, &dwActiveSessionID);
						

						if (wtsSessionInfo.SessionId == dwActiveSessionID) 
						{
							LPTSTR queryBuffer = NULL;


							DWORD dwQueryBuffer = NULL;


							if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, wtsSessionInfo.SessionId, WTSUserName, &queryBuffer, &dwQueryBuffer)) 
							{
								if (_tcslen(queryBuffer) > NULL) 
								{
									CSHServiceLogger(_T("[%s:%d] Proceed Establish To Module, wtsSessionInfo.SessionId = %ld, queryBuffer = %s"), _T(__FUNCTION__), __LINE__, wtsSessionInfo.SessionId, queryBuffer);


									returnCode = EstablishProcessAsUser(processEntry32.th32ProcessID, wtsSessionInfo.SessionId, modulePath, dwQueryBuffer, queryBuffer);


									WTSFreeMemory(pWtsSessionInfo);


									CloseHandle(hSnapShot);


									return returnCode;
								}
							}
						}
					}
				} while (Process32Next(hSnapShot, &processEntry32));
			}
		}
	}


	WTSFreeMemory(pWtsSessionInfo);


	CloseHandle(hSnapShot);


	return returnCode;
}
