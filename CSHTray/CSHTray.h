

#pragma once


#include "resource.h"


class CCSHTrayApp : public CWinApp
{
	public:
		CCSHTrayApp();


	protected:
		virtual BOOL InitInstance();
};


extern CCSHTrayApp theApp;
