
// MFCApplication1Dlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include<iostream>
#include<fstream>
#include "c_helper.h"

#define SRC_IOS_NAME "src_ios_"
#define SRC_ANDROID_NAME "src_package_"
#define RES_COMMON_NAME "res_package_"

// CMFCApplication1Dlg 对话框
class CMFCApplication1Dlg : public CDialogEx
{
// 构造
public:
	CMFCApplication1Dlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	CString ExecuteCmd(CString str);
	TCHAR* StringToChar(CString& str);
	bool DeleteDirectory(CString sDirName);
	void copyDirectory(CString source, CString target, bool showflag);
	bool compfile(std::ifstream& in1, std::ifstream& in2);
	bool newCompfile(CString source, CString target);
	void compDirectory(CString source, CString target, CString outpath);
	void outPutLog(CString log, bool isLine);
	void cleanLog();
	void converToZip(CString target, CString zipname , CString outpath);
	void UnpackFile(const CString & strFilePath);
	bool MakeDir(CString szPath);
	char *coverToChar(CString str, char* _char);
	void zipPackage(CString& inputPath, CString& outPath, CString& version);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSelect1();
	CEdit m_editOnlinePath;
	afx_msg void OnBnProjectPath();
	CEdit m_editProjectPath;
	afx_msg void OnBnOutPutPath();
	CString m_strOnlinePath;
	CString m_strProjectPath;
	CString m_strOutPutPath;
	CEdit m_outPutText;
	afx_msg void OnBnClickedCopyfile();
	afx_msg void OnBnClickedGenerate();
	afx_msg void OnBnClickedLook();
	void generateHandle();
	CString m_strOutPut;

	int m_count;
	int m_copyCount;
	int m_compCount;
	CEdit m_editUploadPath;
	CString m_strUploadPath;
	afx_msg void OnBnClickedUploadPath();
	CEdit m_editGameName;
	CString m_strGameName;
	CString m_strPackName;
	CEdit m_editVirsion;
	CString m_strVirsion;
	CString m_strOldVirsion;
	CString m_strBaseVersion;

	CEdit m_editRoll;
	CString m_rollVirsion;

	afx_msg void OnEnChangeEditGamename();
	afx_msg void OnEnChangeEditPackname();
	afx_msg void OnEnChangeEditVirsion();
	afx_msg void OnEnChangeRollVirsion();
	afx_msg void OnEnChangeOnlinePath();
	afx_msg void OnBnClickedButtonUpload();
	afx_msg void OnBnClickedBtnHotcfg();
	CEdit m_editHotCfgPath;
	CString m_strHotCfgPath;
	afx_msg void OnBnClickedBtnCfbalter();
	void alterHotConfig();
	void alterOnecHotConfig(CString path,CString md5);
	void getVirsion(CString path);
	CEdit m_editVirsionId;
	CString m_strVirsionId;
	afx_msg void OnEnChangeEditVirsionid();
	CComboBox m_comboGameList;
	afx_msg void OnCbnSelchangeComboList();
	afx_msg void OnBnClickedSaveConfig();

	void updateAllConfig(int index);

	void checkCodeVersion(CString codepath);
	bool checkConfigVersion(CString codepath);


	CStringArray m_gameList;
	afx_msg void OnEnChangeUpdatePath();
	afx_msg void OnEnChangeUploadPath();
	afx_msg void OnEnChangeHotupdateCfgPath();
	CEdit m_editOnlineVirsion;
	afx_msg void OnEnChangeEditOnlineVirsion();
	CString m_strOnlineVirsion;
	CButton m_commonSrcCheckBox;
	BOOL m_commonString;
	afx_msg void OnBnClickedCheck1();

	void execute_cmd_handle(CString cmdline);
	void ggc_ant_auto_compile(void);
	afx_msg void OnBnClickedPublicUpload();
};
