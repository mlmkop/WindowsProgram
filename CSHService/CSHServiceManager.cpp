
#include "windows.h"
#include "tchar.h"
#include "CSHServiceManager.h"
#include "CSHServiceExcuteProcess.h"
#include "CSHServiceEstablishServer.h"
#include "CSHServiceLogger.h"
#include "wtsapi32.h"


#ifndef _UNICODE
    #include "stdio.h"
#endif


CSHServiceManager *CSHServiceManager::pCSHServiceManager = NULL;


CSHServiceManager::CSHServiceManager(LPTSTR lpServiceName, BOOL stopFlag, BOOL pauseContinueFlag, BOOL shutdownFlag, BOOL sessionChangeFlag, BOOL powerEventFlag) : serviceName(NULL), serviceStatusHandle(NULL), serviceStatus({NULL, }), socketServerThread(NULL)
{
    serviceName = lpServiceName;


    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;


    serviceStatus.dwCurrentState = SERVICE_START_PENDING;


    if (stopFlag)
    {
        serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    }


    if (pauseContinueFlag)
    {
        serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    }


    if (shutdownFlag)
    {
        serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    }


    if (sessionChangeFlag)
    {
        serviceStatus.dwControlsAccepted |= SERVICE_CONTROL_SESSIONCHANGE;
    }


    if (powerEventFlag)
    {
        serviceStatus.dwControlsAccepted |= SERVICE_CONTROL_POWEREVENT;
    }
    

    serviceStatus.dwControlsAccepted = 0xFF; // ALL FLAG
}


BOOL CSHServiceManager::ServiceRun(CSHServiceManager &serviceManager)
{
    pCSHServiceManager = &serviceManager;


    SERVICE_TABLE_ENTRY serviceTableEntry;

    memset(&serviceTableEntry, NULL, sizeof(serviceTableEntry));


    serviceTableEntry.lpServiceName = serviceName;

    serviceTableEntry.lpServiceProc = ServiceMain;


    return StartServiceCtrlDispatcher(&serviceTableEntry);
}


VOID WINAPI CSHServiceManager::ServiceMain(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors)
{
    pCSHServiceManager->serviceStatusHandle = RegisterServiceCtrlHandlerEx(pCSHServiceManager->serviceName, (LPHANDLER_FUNCTION_EX)ServiceCtrlHandler, NULL);


    pCSHServiceManager->Start(dwNumServicesArgs, lpServiceArgVectors);


    while (pCSHServiceManager->serviceStatus.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(1500);


        continue;
    }
}


DWORD WINAPI CSHServiceManager::ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    DWORD returnCode = 1;


    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        {
            pCSHServiceManager->Stop();


            break;
        }

        case SERVICE_CONTROL_PAUSE:
        {
            pCSHServiceManager->Pause();


            break;
        }

        case SERVICE_CONTROL_CONTINUE:
        {
            pCSHServiceManager->Continue();


            break;
        }

        case SERVICE_CONTROL_SHUTDOWN:
        {
            pCSHServiceManager->Shutdown();


            break;
        }

        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            if ((dwEventType == WTS_CONSOLE_CONNECT) || (dwEventType == WTS_SESSION_LOGON))
            {
                WTSSESSION_NOTIFICATION wtsSessionNotification;

                memset(&wtsSessionNotification, NULL, sizeof(wtsSessionNotification));


                memcpy_s(&wtsSessionNotification, sizeof(wtsSessionNotification), lpEventData, sizeof(wtsSessionNotification));


                LPTSTR queryBuffer = NULL;


                DWORD dwQueryBuffer = NULL;


                if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, wtsSessionNotification.dwSessionId, WTSUserName, &queryBuffer, &dwQueryBuffer))
                {
                    if (_tcslen(queryBuffer) > NULL)
                    {
                        CSHServiceLogger(_T("[%s:%d] Detected SERVICE_CONTROL_SESSIONCHANGE, dwSessionID = %ld, UserName = %s"), _T(__FUNCTION__), __LINE__, wtsSessionNotification.dwSessionId, queryBuffer);


                        pCSHServiceManager->SessionChange(wtsSessionNotification);


                        break;
                    }
                }
            }
        }

        default:
        {
            break;
        }
    }


    return returnCode;
}


VOID CSHServiceManager::Start(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors)
{
    try
    {
        SetServiceStatus(SERVICE_START_PENDING);


        CSHServiceLogger(_T("[%s:%d] Service Start"), _T(__FUNCTION__), __LINE__);


        if (CreateCSHTrayProcess(NULL))
        {
            CSHServiceLogger(_T("[%s:%d] CreateCSHTrayProcess Done, On Main SessionID"), _T(__FUNCTION__), __LINE__);


            socketServerThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateCSHSocketServer, NULL, NULL, NULL);
        }
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        SetServiceStatus(SERVICE_STOPPED, errorCode, 3000);


        ReportErrorEvent((LPTSTR)_T("Service Start"), errorCode);
    }
}


VOID CSHServiceManager::Stop()
{
    try
    {
        SetServiceStatus(SERVICE_STOP_PENDING);


        CSHServiceLogger(_T("[%s:%d] Service Stop"), _T(__FUNCTION__), __LINE__);


        if (socketServerThread != NULL)
        {
            SuspendThread(socketServerThread);
        }


        SetServiceStatus(SERVICE_STOPPED, NULL, 3000);
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        SetServiceStatus(SERVICE_RUNNING, errorCode, 3000);


        ReportErrorEvent((LPTSTR)_T("Service Stop"), errorCode);
    }
}


VOID CSHServiceManager::Pause()
{
    try
    {
        SetServiceStatus(SERVICE_PAUSE_PENDING);


        CSHServiceLogger(_T("[%s:%d] Service Pause"), _T(__FUNCTION__), __LINE__);


        if (socketServerThread != NULL)
        {
            SuspendThread(socketServerThread);
        }


        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        SetServiceStatus(SERVICE_RUNNING, errorCode, 3000);


        ReportErrorEvent((LPTSTR)_T("Service Pause"), errorCode);
    }
}


VOID CSHServiceManager::Continue()
{
    try
    {
        SetServiceStatus(SERVICE_CONTINUE_PENDING);

        
        CSHServiceLogger(_T("[%s:%d] Service Continue"), _T(__FUNCTION__), __LINE__);


        if (socketServerThread != NULL)
        {
            ResumeThread(socketServerThread);
        }


        SetServiceStatus(SERVICE_RUNNING, NULL, 3000);
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        SetServiceStatus(SERVICE_PAUSED, errorCode);


        ReportErrorEvent((LPTSTR)_T("Service Continue"), errorCode);
    }
}


VOID CSHServiceManager::Shutdown()
{
    try
    {
        CSHServiceLogger(_T("[%s:%d] Service Shutdown"), _T(__FUNCTION__), __LINE__);


        if (socketServerThread != NULL)
        {
            TerminateThread(socketServerThread, NULL);


            CloseHandle(socketServerThread);
        }
        

        SetServiceStatus(SERVICE_STOPPED, NULL, 3000);
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        ReportErrorEvent((LPTSTR)_T("Service Shutdown"), errorCode);
    }
}


VOID CSHServiceManager::SessionChange(WTSSESSION_NOTIFICATION wtSsessionNotification)
{
    try
    {
        CSHServiceLogger(_T("[%s:%d] Service SessionChange"), _T(__FUNCTION__), __LINE__);


        if (CreateCSHTrayProcess(&wtSsessionNotification))
        {
            CSHServiceLogger(_T("[%s:%d] CreateCSHTrayProcess Done, Target SessionID = %ld"), _T(__FUNCTION__), __LINE__, wtSsessionNotification.dwSessionId);
        }
    }
    catch (...)
    {
        DWORD errorCode = GetLastError();


        ReportErrorEvent((LPTSTR)_T("Service SessionChange"), errorCode);
    }
}


VOID CSHServiceManager::SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    serviceStatus.dwCurrentState = dwCurrentState;


    serviceStatus.dwWin32ExitCode = dwWin32ExitCode;


    serviceStatus.dwWaitHint = dwWaitHint;


    serviceStatus.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) ? NULL : serviceStatus.dwCheckPoint++;


    ::SetServiceStatus(serviceStatusHandle, &serviceStatus);
}


VOID CSHServiceManager::ReportErrorEvent(LPTSTR lpFunctionName, DWORD dwErrorCode)
{
    TCHAR errorMessage[MAX_PATH];

    memset(errorMessage, NULL, sizeof(errorMessage));


    _stprintf_s(errorMessage, MAX_PATH - 1, _T("%s Failed, Catched ErrorCode = %ld"), lpFunctionName, dwErrorCode);


    HANDLE hEventSource = RegisterEventSource(NULL, serviceName);


    if (hEventSource)
    {
        LPCTSTR eventLogEntry[2] = { NULL, NULL };


        eventLogEntry[0] = serviceName;


        eventLogEntry[1] = errorMessage;


        ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, NULL, NULL, NULL, 2, NULL, eventLogEntry, NULL);


        DeregisterEventSource(hEventSource);
    }
}
