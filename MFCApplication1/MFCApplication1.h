
// MFCApplication1.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// AutoUnpackToolsApp: 
// �йش����ʵ�֣������ MFCApplication1.cpp
//

class AutoUnpackToolsApp : public CWinApp
{
public:
	AutoUnpackToolsApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnOnlinePath();
};

extern AutoUnpackToolsApp theApp;