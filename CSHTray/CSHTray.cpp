
#include "framework.h"
#include "CSHTray.h"
#include "CSHTrayDlg.h"


CCSHTrayApp::CCSHTrayApp(){}


CCSHTrayApp theApp;


BOOL CCSHTrayApp::InitInstance()
{
	CWinApp::InitInstance();


	CCSHTrayDlg dlg;


	m_pMainWnd = &dlg;


	dlg.DoModal();


	return FALSE;
}
