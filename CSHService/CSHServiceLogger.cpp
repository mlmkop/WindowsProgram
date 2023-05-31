
#include "windows.h"
#include "tchar.h"
#include "CSHServiceLogger.h"
#include "time.h"
#include "fstream"


#ifndef _UNICODE
	#include "stdio.h"
#endif


using namespace std;


#ifdef _UNICODE
	typedef basic_fstream<wchar_t, char_traits<wchar_t>> wfstream, tfstream;
#else
	typedef basic_fstream<char, char_traits<char>> fstream, tfstream;
#endif


VOID CSHServiceLogger(LPCTSTR format, ...)
{
	#ifndef DISABLE_LOG
	{
		//TCHAR *logBuffer = (TCHAR *)calloc(1, sizeof(TCHAR) * 1024);


		TCHAR *logBuffer = new TCHAR[1024];


		if (logBuffer != NULL)
		{
			time_t currentTime = time(NULL);


			struct tm currentTimeSet;

			memset(&currentTimeSet, NULL, sizeof(currentTimeSet));


			localtime_s(&currentTimeSet, &currentTime);


			TCHAR timeBuffer[64];

			memset(timeBuffer, NULL, sizeof(timeBuffer));


			_tcsftime(timeBuffer, 64 - 1, _T("%Y-%m-%d %H:%M:%S"), &currentTimeSet);


			_tcscpy_s(logBuffer, 64 - 1, timeBuffer);


			va_list vaList = NULL;


			va_start(vaList, format);


			#ifdef _UNICODE
				vswprintf(logBuffer + _tcslen(timeBuffer), 1024 - 1, format, vaList);
			#else
				vsnprintf(logBuffer + _tcslen(timeBuffer), 1024 - 1, format, vaList);
			#endif


			va_end(vaList);


			memset(timeBuffer, NULL, sizeof(timeBuffer));


			_tcsftime(timeBuffer, 64 - 1, _T("%Y-%m-%d"), &currentTimeSet);


			TCHAR modulePath[MAX_PATH];

			memset(modulePath, NULL, sizeof(modulePath));


			GetModuleFileName(NULL, modulePath, MAX_PATH - 1);


			TCHAR drive[_MAX_DRIVE];

			memset(drive, NULL, sizeof(drive));


			TCHAR directory[_MAX_DIR];

			memset(directory, NULL, sizeof(directory));


			_tsplitpath_s(modulePath, drive, _MAX_DRIVE, directory, _MAX_DIR, NULL, NULL, NULL, NULL);
			

			_tmakepath_s(modulePath, drive, directory, NULL, NULL);


			TCHAR logPath[MAX_PATH];

			memset(logPath, NULL, sizeof(logPath));


			_stprintf_s(logPath, MAX_PATH - 1, _T("%s\\CSHService_%s.log"), modulePath, timeBuffer);


			tfstream logFile;

			
			logFile.open(logPath, ios::out | ios::app);


			if (logFile.is_open()) 
			{
				logFile.write(logBuffer, _tcslen(logBuffer));


				logFile << endl;


				logFile.close();
			}


			if (logBuffer != NULL)
			{
				// free(logBuffer);


				delete [] logBuffer;
			}
		}
	}
	#endif
}
