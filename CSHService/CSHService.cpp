
#include "windows.h"
#include "tchar.h"
#include "CSHService.h"
#include "CSHServiceWizard.h"
#include "CSHServiceManager.h"


#ifndef _UNICODE
	#include "stdio.h"
#endif


INT _tmain(INT argc, TCHAR *argv[])
{
	INT returnCode = 1;


	if (GetConsoleWindow()) 
	{
		if ((argc > 1) && (((*argv[1] == '-') || (*argv[1] == '/'))))
		{
			if (_tcscmp(_T("install"), argv[1] + 1) == ERROR_SUCCESS)
			{
				returnCode = (INT)InstallService();
			}
			else if (_tcscmp(_T("uninstall"), argv[1] + 1) == ERROR_SUCCESS)
			{
				returnCode = (INT)UninstallService();
			}
			else 
			{
				_tprintf(_T("\n\nParameters : [ -install or /install ] to install the service\n\n"));

				_tprintf(_T("Parameters : [ -uninstall or /uninstall ] to uninstall the service\n\n"));
			}
		}
		else
		{
			_tprintf(_T("\n\nParameters : [ -install or /install ] to install the service\n\n"));

			_tprintf(_T("Parameters : [ -uninstall or /uninstall ] to uninstall the service\n\n"));
		}
	}
	else
	{
		CSHServiceManager serviceManager((LPTSTR)SERVICE_NAME);


		if (serviceManager.ServiceRun(serviceManager))
		{
			returnCode = ERROR_SUCCESS;
		}
	}


	return returnCode;
}
