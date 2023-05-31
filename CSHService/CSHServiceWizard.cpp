
#include "windows.h"
#include "tchar.h"
#include "CSHServiceWizard.h"
#include "CSHService.h"


#ifndef _UNICODE
	#include "stdio.h"
#endif


DWORD InstallService()
{
	DWORD returnCode = 1;


	TCHAR serviceModulePath[MAX_PATH];

	memset(serviceModulePath, NULL, sizeof(serviceModulePath));


	GetModuleFileName(NULL, serviceModulePath, MAX_PATH - 1);


	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);


	if (schSCManager == NULL) 
	{
		returnCode = GetLastError();


		_tprintf(_T("\n\nOpenSCManager Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);


		return returnCode;
	}


	SC_HANDLE schService = CreateService(schSCManager, SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS /* | SERVICE_INTERACTIVE_PROCESS */, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, serviceModulePath, NULL, NULL, NULL, NULL, NULL);


	if (schService == NULL)
	{
		returnCode = GetLastError();


		_tprintf(_T("\n\nCreateService Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);


		CloseServiceHandle(schSCManager);


		return returnCode;
	}
	else
	{
		_tprintf(_T("\n\nCreateService Success, %s Service Has Been Installed"), SERVICE_NAME);


		SERVICE_DESCRIPTION serviceDescription;

		serviceDescription.lpDescription = (LPTSTR)SERVICE_DESCRIPTION_NAME;


		ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &serviceDescription);


		if (StartService(schService, NULL, NULL) == FALSE)
		{
			returnCode = GetLastError();


			_tprintf(_T("\n\nStartService Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);
		}
		else
		{
			returnCode = ERROR_SUCCESS;


			_tprintf(_T("\n\nStartService Success, %s Service Will Start As Soon As Possible\n\n"), SERVICE_NAME);
		}


		CloseServiceHandle(schSCManager);


		CloseServiceHandle(schService);
	}


	return returnCode;
}


DWORD UninstallService()
{
	DWORD returnCode = 1;


	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);


	if (schSCManager == NULL)
	{
		returnCode = GetLastError();


		_tprintf(_T("\n\nOpenSCManager Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);


		return returnCode;
	}


	SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);


	if (schService == NULL)
	{
		returnCode = GetLastError();


		_tprintf(_T("\n\nCreateService Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);


		CloseServiceHandle(schSCManager);


		return returnCode;
	}


	SERVICE_STATUS serviceStatus;

	memset(&serviceStatus, NULL, sizeof(serviceStatus));


	if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) 
	{
		while (QueryServiceStatus(schService, &serviceStatus)) 
		{
			if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) 
			{
				_tprintf(_T("."));


				Sleep(500);
			}
			else
			{
				break;
			}
		}


		if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
		{
			_tprintf(_T("\n\nControlService Success, %s Service Has Been Stopped"), SERVICE_NAME);
		}
	}


	if (DeleteService(schService) == NULL) 
	{
		returnCode = GetLastError();


		_tprintf(_T("\n\nDeleteService Failed, GetLastError Code = %ld\n\n"), (INT)returnCode);
	}
	else
	{
		returnCode = ERROR_SUCCESS;


		_tprintf(_T("\n\nDeleteService Success, %s Service Will Delete As Soon As Possible\n\n"), SERVICE_NAME);
	}


	CloseServiceHandle(schSCManager);


	CloseServiceHandle(schService);


	return returnCode;
}
