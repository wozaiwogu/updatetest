
// MFCApplication1Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "md5.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"
#include <string>
#include "json/json.h"
#include "json/value.h"
#include "zlib.h"//压缩文件相关
#include "zconf.h"
#include "CMD5Checksum.h"

#include <httpext.h>
#include <windef.h>
#include <Nb30.h>



#pragma comment(lib,"ws2_32.lib")

#pragma comment(lib,"netapi32.lib")

#pragma comment(lib,"zlib.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace Json;


typedef struct{
	ADAPTER_STATUS adapt;
	NAME_BUFFER NameBuff[30];
}ASTAT, *PASTAT;


typedef struct{
	UCHAR length;
	UCHAR lana[MAX_LANA];
}EX_LANA_ENUM;

int getMAC(char *mac)
{
	NCB ncb; 
	ASTAT Adapter;
	EX_LANA_ENUM lana_enum;
	int uRetCode;

	memset(&ncb, 0, sizeof(ncb));
	memset(&lana_enum, 0, sizeof(lana_enum));

	ncb.ncb_command = NCBENUM;
	ncb.ncb_buffer = (unsigned char*)&lana_enum;
	ncb.ncb_length = sizeof(EX_LANA_ENUM);
	uRetCode = Netbios(&ncb);

	if (uRetCode != NRC_GOODRET)
		return uRetCode;

	for (int lana = 0; lana < lana_enum.length; lana++)
	{
		ncb.ncb_command = NCBRESET;
		ncb.ncb_lana_num = lana_enum.lana[lana]; uRetCode = Netbios(&ncb);
	}

	if (uRetCode != NRC_GOODRET)
		return uRetCode;

	memset(&ncb, 0, sizeof(ncb));
	ncb.ncb_command = NCBASTAT; 
	ncb.ncb_lana_num = lana_enum.lana[0];
	strcpy((char*)ncb.ncb_callname, "*");
	ncb.ncb_buffer = (unsigned char*)&Adapter;
	ncb.ncb_length = sizeof(Adapter); 

	uRetCode = Netbios(&ncb);
	if (uRetCode != NRC_GOODRET) {
		return uRetCode;
	}

	sprintf(mac, "%02X-%02X-%02X-%02X-%02X-%02X", Adapter.adapt.adapter_address[0], Adapter.adapt.adapter_address[1], Adapter.adapt.adapter_address[2], Adapter.adapt.adapter_address[3], Adapter.adapt.adapter_address[4], Adapter.adapt.adapter_address[5]);
	return 0;
}


void CMFCApplication1Dlg::converToZip(CString target, CString zipname, CString outpath)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		::AfxMessageBox(CString("c_auto_compile_ggc_dlg.CreatePipe.error"));
		return;
	}

	STARTUPINFO			si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	
	CString zipName = outpath + L"\\" + zipname + L".zip";
	CString winRarInstallPath = L"C:\\Program Files\\WinRAR\\WinRAR.exe";
	CString strCmd = L"\"" + winRarInstallPath + "\" a -k -r -s -m1 \"" + zipName + "\" \"" + target + "\"";

	if (!CreateProcess(NULL, strCmd.GetBuffer(MAX_PATH), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		MessageBox(CString("CreateProcess failed!"));
		return;
	}
	CloseHandle(hWrite);
	strCmd.ReleaseBuffer();


	char buffer[4444] = { 0 };
	DWORD bytesRead;
	while (1)
	{
		if (NULL == ReadFile(hRead, buffer, 4443, &bytesRead, NULL))
		{
			break;
		}

		CString		str_s = ::c_helper::helper_conver_char_2_unicode(buffer);

		/* write to log file
		*
		*/
		::c_helper::helper_log_write(str_s);

	}
	CloseHandle(hRead);
}

void CMFCApplication1Dlg::UnpackFile(const CString & strFilePath)
{
	CString winRarInstallPath = L"C:\\Program Files\\WinRAR\\WinRAR.exe";
	CString strDestPath; //目标解压位置
	int pos = strFilePath.ReverseFind('.');
	strDestPath = strFilePath.Left(pos);

	// 清空文件
	DeleteDirectory(strDestPath);
	if (FALSE == ::CreateDirectory(strDestPath, NULL))
	{
		MessageBox(CString("创建解压路径失败"));
		return;
	}

	//x解压  -ibck后台执行 -o+如存在，则覆盖 -inul不弹出错误提示  
	//使用 -ibck，缩小到了系统托盘区域
	CString strCmd = L"\"" + winRarInstallPath + "\" x -ibck -o+ -inul \"" + strFilePath + "\" \"" + strDestPath + "\"";
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	BOOL bRet = CreateProcess(NULL, strCmd.GetBuffer(MAX_PATH), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	DWORD dwExit = 0;
	if (bRet)
	{
		//这个地方将导致该函数为阻塞状态  
		WaitForSingleObject(pi.hProcess, INFINITE);
		::GetExitCodeProcess(pi.hProcess, &dwExit);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	return;
}

TCHAR szDefaultDir[MAX_PATH];

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:    //初始化消息
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)"C:\\");   //初始化路径 （方法一）


		break;
	case BFFM_SELCHANGED:    //选择路径变化，
	{

		SHGetPathFromIDList((LPCITEMIDLIST)lParam, szDefaultDir);  //这里会清空szDefaultDir 

		::SendMessage(hwnd, BFFM_SETSTATUSTEXT, TRUE, (LPARAM)szDefaultDir);
	}
		break;
	default:
		break;
	}

	return 0;
}

//创建一个选择文件夹的对话框，返回所选路径  
static CString Show(LPCWSTR title)
{
	TCHAR           szFolderPath[MAX_PATH] = { 0 };
	CString         strFolderPath = TEXT("");

	BROWSEINFO      sInfo;
	::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
	sInfo.pidlRoot = 0;
	sInfo.lpszTitle = title;
	sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
	//sInfo.lpfn = BrowseCallbackProc;
	sInfo.lParam = long(L"D:\\");

	// 显示文件夹选择对话框  
	LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
	if (lpidlBrowse != NULL)
	{
		// 取得文件夹名  
		if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
		{
			strFolderPath = szFolderPath;
		}
	}
	if (lpidlBrowse != NULL)
	{
		::CoTaskMemFree(lpidlBrowse);
	}

	return strFolderPath;

}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCApplication1Dlg 对话框



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCApplication1Dlg::IDD, pParent)
	, m_strOnlinePath(_T(""))
	, m_strProjectPath(_T(""))
	, m_strOutPutPath(_T(""))
	, m_strOutPut(_T(""))
	, m_strUploadPath(_T(""))
	, m_strGameName(_T(""))
	, m_strPackName(_T(""))
	, m_strVirsion(_T(""))
	, m_strHotCfgPath(_T(""))
	, m_strVirsionId(_T(""))
	, m_strOnlineVirsion(_T(""))
	, m_strBaseVersion(_T(""))
	, m_commonString(FALSE)
	, m_rollVirsion(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ONLINE_PATH, m_editOnlinePath);
	DDX_Control(pDX, IDC_UPDATE_PATH, m_editProjectPath);
	DDX_Text(pDX, IDC_ONLINE_PATH, m_strOnlinePath);
	DDX_Text(pDX, IDC_UPDATE_PATH, m_strProjectPath);
	DDX_Control(pDX, IDC_OUTPUT_TEXT, m_outPutText);
	DDX_Text(pDX, IDC_OUTPUT_TEXT, m_strOutPut);
	DDX_Control(pDX, IDC_UPLOAD_PATH, m_editUploadPath);
	DDX_Text(pDX, IDC_UPLOAD_PATH, m_strUploadPath);
	DDX_Control(pDX, IDC_EDIT_GAMENAME, m_editGameName);
	DDX_Text(pDX, IDC_EDIT_GAMENAME, m_strGameName);
	DDX_Control(pDX, IDC_EDIT_VIRSION, m_editVirsion);
	DDX_Text(pDX, IDC_EDIT_VIRSION, m_strVirsion);
	DDX_Control(pDX, IDC_HOTUPDATE_CFG_PATH, m_editHotCfgPath);
	DDX_Text(pDX, IDC_HOTUPDATE_CFG_PATH, m_strHotCfgPath);
	DDX_Control(pDX, IDC_COMBO_LIST, m_comboGameList);
	DDX_Control(pDX, IDC_EDIT_ONLINE_VIRSION, m_editOnlineVirsion);
	DDX_Text(pDX, IDC_EDIT_ONLINE_VIRSION, m_strOnlineVirsion);
	DDX_Control(pDX, IDC_CHECK1, m_commonSrcCheckBox);
	DDX_Check(pDX, IDC_CHECK1, m_commonString);
	DDX_Control(pDX, IDC_EDIT_ROLL, m_editRoll);
	DDX_Text(pDX, IDC_EDIT_ROLL, m_rollVirsion);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SELECT_1, &CMFCApplication1Dlg::OnBnClickedSelect1)
	ON_BN_CLICKED(IDC_SELECT_2, &CMFCApplication1Dlg::OnBnProjectPath)
	ON_BN_CLICKED(IDC_GENERATE, &CMFCApplication1Dlg::OnBnClickedGenerate)
	ON_BN_CLICKED(IDC_LOOK, &CMFCApplication1Dlg::OnBnClickedLook)
	ON_BN_CLICKED(IDC_SELECT_4, &CMFCApplication1Dlg::OnBnClickedUploadPath)
	ON_EN_CHANGE(IDC_EDIT_GAMENAME, &CMFCApplication1Dlg::OnEnChangeEditGamename)
	ON_EN_CHANGE(IDC_EDIT_VIRSION, &CMFCApplication1Dlg::OnEnChangeEditVirsion)
	ON_EN_CHANGE(IDC_ONLINE_PATH, &CMFCApplication1Dlg::OnEnChangeOnlinePath)
	ON_BN_CLICKED(IDC_BUTTON_UPLOAD, &CMFCApplication1Dlg::OnBnClickedButtonUpload)
	ON_BN_CLICKED(IDC_BTN_HOTCFG, &CMFCApplication1Dlg::OnBnClickedBtnHotcfg)
	ON_BN_CLICKED(IDC_BTN_CFBALTER, &CMFCApplication1Dlg::OnBnClickedBtnCfbalter)
	ON_CBN_SELCHANGE(IDC_COMBO_LIST, &CMFCApplication1Dlg::OnCbnSelchangeComboList)
	ON_BN_CLICKED(IDC_SAVE, &CMFCApplication1Dlg::OnBnClickedSaveConfig)
	ON_EN_CHANGE(IDC_UPDATE_PATH, &CMFCApplication1Dlg::OnEnChangeUpdatePath)
	ON_EN_CHANGE(IDC_UPLOAD_PATH, &CMFCApplication1Dlg::OnEnChangeUploadPath)
	ON_EN_CHANGE(IDC_HOTUPDATE_CFG_PATH, &CMFCApplication1Dlg::OnEnChangeHotupdateCfgPath)
	ON_EN_CHANGE(IDC_EDIT_ONLINE_VIRSION, &CMFCApplication1Dlg::OnEnChangeEditOnlineVirsion)
	ON_BN_CLICKED(IDC_CHECK1, &CMFCApplication1Dlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_SYNC, &CMFCApplication1Dlg::OnBnClickedPublicUpload)
	ON_EN_CHANGE(IDC_EDIT_ROLL, &CMFCApplication1Dlg::OnEnChangeRollVirsion)
END_MESSAGE_MAP()


// CMFCApplication1Dlg 消息处理程序

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	CStringArray gameList;
	CString strGameList = ::c_helper::helper_profile_get(CString("strGameList"));
	::c_helper::string_to_strarray(strGameList, m_gameList, L",");
	for (int i = 0; i < m_gameList.GetSize(); i++){
		auto gamename = m_gameList.GetAt(i);
		m_comboGameList.AddString(m_gameList.GetAt(i));
	}

	m_comboGameList.SetCurSel(0);
	updateAllConfig(0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


TCHAR* CMFCApplication1Dlg::StringToChar(CString& str)
{
	int len = str.GetLength();
	TCHAR* tr = str.GetBuffer(len);
	str.ReleaseBuffer();
	return tr;
}

/*
* 执行管道命令
*/
CString CMFCApplication1Dlg::ExecuteCmd(CString str)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		MessageBox(_T("创建匿名管道失败，线程结束"));
		return NULL;
	}
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	TCHAR* cmdline = StringToChar(str);
	if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		MessageBox(_T("进程打开失败 线程结束"));
		return NULL;
	}
	CloseHandle(hWrite);

	char buffer[4096];
	memset(buffer, 0, 4096);
	CString output;
	DWORD byteRead;
	while (true)
	{
		if (ReadFile(hRead, buffer, 4095, &byteRead, NULL) == NULL)
		{
			break;
		}
		output += buffer;
	}
	return output;
}

bool CMFCApplication1Dlg::MakeDir(CString lpPath)
{
	CString pathname = lpPath;
	if (pathname.Right(1) != "\\")
		pathname += "\\" ;
		int end = pathname.ReverseFind('\\');
		int pt = pathname.Find('\\');
		if (pathname[pt - 1] == ':')
			pt = pathname.Find('\\', pt+1);
			CString path;
	while (pt != -1 && pt <= end)
	{
		path = pathname.Left(pt + 1);
		if (!PathIsDirectory(path))
			CreateDirectory(path, NULL); //创建目标文件夹  
		pt = pathname.Find('\\', pt+1);
	}
	return true;
}
/*
* 对比文件夹
*/
void CMFCApplication1Dlg::compDirectory(CString source, CString target, CString outpath){
	m_count += 1;
	CreateDirectory(target, NULL); //创建目标文件夹  
	CFileFind finder;
	CString path;
	path.Format(L"%s/*.*", target);
	bool bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();

		if (finder.IsDirectory() && !finder.IsDots()){ //是文件夹 而且 名称不含 . 或 ..  
			compDirectory(source + "/" + finder.GetFileName(), finder.GetFilePath(), outpath); //递归创建文件夹+"/"+finder.GetFileName()  
		}
		else{ //是文件 则直接复制  
			if (!finder.IsDots()){
			//if ( true ){
				auto file1 = source + "\\" + finder.GetFileName();
				auto file2 = target + "\\" + finder.GetFileName();
				
				auto ret = newCompfile(file1, file2);
				if (!ret)
				{
					m_compCount += 1;
					outPutLog(L"差异文件 " + finder.GetFileName(), false);

					CString nStr = outpath.Right(outpath.GetLength() - outpath.ReverseFind('\\'));
					CString nPath = outpath + file2.Right(file2.GetLength() - file2.Find(nStr) - nStr.GetLength());

					auto fileName = finder.GetFileName();
					CString nMakeDir = nPath.Left(nPath.GetLength() - fileName.GetLength());
					nMakeDir.Replace(L"/", L"\\");
					MakeDir(nMakeDir);

					CopyFile(finder.GetFilePath(), nPath, FALSE);
				}				
			}
		}
	}

	m_count -= 1;
	if (0 == m_count)
	{
		CString fileTips;
		fileTips.Format(L"=======%s差异文件化完成======", outpath);
		outPutLog(fileTips, false);
		CString fileNum;
	}
}
bool CMFCApplication1Dlg::DeleteDirectory(CString sDirName)
{
	CFileFind tempFind;
	CString sTempFileFind;
	sTempFileFind.Format(L"%s\\*.*", sDirName); //将第二个参数写入到第一个参数当中
	BOOL IsFinded = tempFind.FindFile(sTempFileFind);
	while (IsFinded)
	{
		IsFinded = tempFind.FindNextFile();
		//当这个目录中不含有.的时候，就是说这不是一个文件。
		if (!tempFind.IsDots())
		{
			CString sFoundFileName;
			sFoundFileName = tempFind.GetFileName().GetBuffer(200);
			//我感觉这里好像有问题。
			//如果是目录那么删除目录
			if (tempFind.IsDirectory())
			{
				CString sTempDir;
				sTempDir.Format(L"%s\\%s", sDirName, sFoundFileName);
				DeleteDirectory(sTempDir); //其实真正删除文件的也就这两句，别的都是陪衬
			}
			//如果是文件那么删除文件
			else
			{
				CString sTempFileName;
				sTempFileName.Format(L"%s\\%s", sDirName, sFoundFileName);
				DeleteFile(sTempFileName);
			}
		}
	}
	tempFind.Close();
	if (!RemoveDirectory(sDirName))
	{
		return FALSE;
	}
	return TRUE;

}

char *CMFCApplication1Dlg::coverToChar(CString str, char* _char)
{
	int len = str.GetLength();
	TCHAR* tr = str.GetBuffer(len);
	str.ReleaseBuffer();
	int iLength = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, str, -1, _char, iLength, NULL, NULL);
	return _char;
}
bool CMFCApplication1Dlg::newCompfile(CString source, CString target)
{
	char file1[MAX_PATH] = "1111111111";
	char file2[MAX_PATH] = "11111111112";

	coverToChar(source, file1);
	coverToChar(target, file2);
	int ch1, ch2;
	FILE *in1, *in2;

	// 被比较文件不存在
	if (!PathFileExists(source)){
		return false;
	}
	in1 = fopen(file1, "rb");
	in2 = fopen(file2, "rb");
	if (in2 == NULL)
	{
		fclose(in1);
		fclose(in2);
		return false;
	}
	//读文件比较
	ch1 = fgetc(in1);
	ch2 = fgetc(in2);
	while (!feof(in1) || !feof(in2))
	{
		if (ch1 != ch2)		//两文件内容不相同
		{
			fclose(in1);
			fclose(in2);
			return false;
		}
		ch1 = fgetc(in1);
		ch2 = fgetc(in2);
	}
	//两文件内容相同
	fclose(in1);
	fclose(in2);
	return true;
}

/*
* 对比文件
*/
bool CMFCApplication1Dlg::compfile(ifstream& in1, ifstream& in2){
	ifstream::pos_type size1, size2;
	size1 = in1.seekg(0, ifstream::end).tellg();
	in1.seekg(0, ifstream::beg);
	size2 = in2.seekg(0, ifstream::end).tellg();
	in2.seekg(0, ifstream::beg);
	if (size1 != size2)
		return false;
	size_t remaining = size1;//or size2
	while (remaining)
	{
		char buffer1[4096], buffer2[4096];//BOLCKSIZE=4096
		size_t size = min(4096, remaining);
		in1.read(buffer1, size);
		in2.read(buffer2, size);
		if (0 != memcmp(buffer1, buffer2, size))
			return false;
		remaining -= size;
	}
	return true;
}

/*
* 拷贝文件夹
*/
void CMFCApplication1Dlg::copyDirectory(CString source, CString target, bool showflag)
{
	m_count += 1;
	if (-1 == target.Find(L".svn"))
	{
		CreateDirectory(target, NULL); //创建目标文件夹  
	}
	//AfxMessageBox("创建文件夹"+target);  
	CFileFind finder;
	CString path;
	path.Format(L"%s/*.*", source);
	bool bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();

		if (finder.IsDirectory() && !finder.IsDots()){ //是文件夹 而且 名称不含 . 或 ..  
			copyDirectory(finder.GetFilePath(), target + "/" + finder.GetFileName(), showflag); //递归创建文件夹+"/"+finder.GetFileName()  
		}
		else{ //是文件 则直接复制  
			auto ret = source.Find(L".svn");
			if (!finder.IsDots() && -1 == ret){
				m_copyCount += 1;
				if (!showflag)
					outPutLog(L"添加到压缩文件 " + finder.GetFileName(), false);
				else
					outPutLog(L"复制 " + finder.GetFileName(), false);

				auto fileName = finder.GetFileName();
				CopyFile(finder.GetFilePath(), target + "/" + fileName, FALSE);
			}
			
		}
	}

	m_count -= 1;
	if (0 == m_count && showflag)
	{
		CString fileTips;
		fileTips.Format(L"拷贝文件数量：%d", m_copyCount);
		outPutLog(fileTips, false);
		outPutLog(L"==============复制完成==============", false);
	}
}

void CMFCApplication1Dlg::cleanLog()
{
	m_strOutPut = "";
	m_outPutText.SetWindowTextW(m_strOutPut);
	m_outPutText.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);

}
void CMFCApplication1Dlg::outPutLog(CString log, bool isLine)
{
	if (isLine)
		m_strOutPut = m_strOutPut + L"\r\n----------------------------------------\r\n";

	m_strOutPut = m_strOutPut + log + L"\r\n";
	m_outPutText.SetWindowTextW(m_strOutPut);
	m_outPutText.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

void CMFCApplication1Dlg::OnBnClickedSelect1()
{
	// TODO:  在此添加控件通知处理程序代码
	m_strOnlinePath = Show(_T("请选择线上脚本目录:"));
	m_editOnlinePath.SetWindowTextW(m_strOnlinePath);
	::c_helper::helper_profile_set(L"m_strOnlinePath", m_strOnlinePath);
}


void CMFCApplication1Dlg::OnBnProjectPath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_strProjectPath = Show(_T("请选择当前项目目录:"));
	m_editProjectPath.SetWindowTextW(m_strProjectPath);
	::c_helper::helper_profile_set(L"m_strProjectPath", m_strProjectPath);

	checkCodeVersion(m_strProjectPath);

}


void CMFCApplication1Dlg::OnBnOutPutPath()
{
	// TODO:  在此添加控件通知处理程序代码f
	m_strOutPutPath = Show(_T("请选择输出目录:"));
	::c_helper::helper_profile_set(L"m_strOutPutPath", m_strOutPutPath);
}

/*
* 拷贝备份文件事件
*/
void CMFCApplication1Dlg::OnBnClickedCopyfile()
{
	// TODO:  在此添加控件通知处理程序代码

	auto tset = m_strOnlinePath.Find(CString("\\"));
		
	m_count = 0;
	m_copyCount = 0;
	auto outPutPath = m_strOutPutPath + L"\\" + m_strGameName + L"_" + m_strVirsion;
	CreateDirectory(outPutPath, NULL); //创建目标文件夹 

	//拷贝ios文件
	auto srcPath = m_strProjectPath + L"\\src";
	copyDirectory(srcPath, outPutPath + L"\\src_ios", true);

	//拷贝资源文件
	srcPath = m_strProjectPath + L"\\res";;
	copyDirectory(srcPath, outPutPath + L"\\res_common", true);

	//拷贝android文件
	srcPath = m_strProjectPath + L"\\frameworks\\runtime-src\\proj.android\\assets\\src";;
	copyDirectory(srcPath, outPutPath + L"\\src_android", true);
}


void CMFCApplication1Dlg::zipPackage(CString& inputPath, CString& outPath, CString& version) {
	DeleteDirectory(L"C:\\src");
	DeleteDirectory(L"C:\\res");
	DeleteDirectory(L"D:\\src");

	//拷贝ios文件
	//copyDirectory(outPutPath + L"\\src_ios", L"C:\\src", false);

	//拷贝资源文件
	copyDirectory(inputPath + L"\\res_package", L"C:\\res", false);

	//拷贝android文件
	copyDirectory(inputPath + L"\\src_package", L"D:\\src", false);

	// TODO:  在此添加控件通知处理程序代码
	//CString srcIosName(SRC_IOS_NAME);
	//srcIosName = srcIosName + nStrVirsion;

	CString srcAndroidName(SRC_ANDROID_NAME);
	srcAndroidName = srcAndroidName + version;

	CString resCommonName(RES_COMMON_NAME);
	resCommonName = resCommonName + version;

	//converToZip(L"C:\\src", srcIosName, outPutPath);
	converToZip(L"C:\\res", resCommonName, outPath);
	converToZip(L"D:\\src", srcAndroidName, outPath);

	outPutLog(L"差异包处理完成", true);

	alterHotConfig();
}

void CMFCApplication1Dlg::generateHandle()
{

	do
	{
		CString nStrGameName = m_strGameName;
		CString nStrVirsion = m_strVirsion;
		CString nStrOnLineVirsion = m_strOnlineVirsion;

		CString nStrProjectPath = m_strProjectPath;
		CString nbackupsPath = m_strOnlinePath;
		m_strOutPutPath = nbackupsPath;

		if (nbackupsPath.IsEmpty() || !PathIsDirectory(nbackupsPath))
		{
			MessageBox(_T("备份目录不存在") + nbackupsPath);
			break;
		}
		
		if (nStrGameName.IsEmpty())
		{
			MessageBox(_T("产品名为空"));
			break;
		}
		
		/******************************拷贝开始****************************/
		m_count = 0;
		m_copyCount = 0;
		auto outPutPath = nbackupsPath + L"\\" + nStrGameName + L"_" + nStrVirsion;
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 

		// 编译成字节码
		CString jscompileCmd(L"cocos jscompile -s " + nStrProjectPath + L"\\src -d " + outPutPath + L"\\src_package");
		outPutLog(L"正在编译成字节码...，可能会有点卡，安静等就行V_V", true);
		execute_cmd_handle(jscompileCmd);

		//拷贝ios文件
		/*
		auto srcPath = nStrProjectPath + L"\\src";
		copyDirectory(srcPath, outPutPath + L"\\src_ios", true);
		*/	

		//拷贝资源文件
		auto srcPath = nStrProjectPath + L"\\res";;
		copyDirectory(srcPath, outPutPath + L"\\res_package", true);

		//删除热更包中的配置文件
		DeleteFile(outPutPath + L"\\res_package" + "\\android.manifest");
		DeleteFile(outPutPath + L"\\res_package" + "\\ios.manifest");

		if (outPutPath.Find(L"games\\game_") > -1) {
			DeleteFile(outPutPath + L"\\res_package" + "\\font\\label.ttf");
			DeleteFile(outPutPath + L"\\res_package" + "\\font\\main.ttf");
		}


		//srcPath = nStrProjectPath + L"\\frameworks\\runtime-src\\proj.android\\assets\\src";
		//copyDirectory(srcPath, outPutPath + L"\\src_package", true);
		

		CString nOnlinePath = m_strOnlinePath + L"\\" + nStrGameName + L"_" + m_strOnlineVirsion;
		srcPath = nOnlinePath;
		if (!PathIsDirectory(srcPath)){
		
			outPutLog(L"基本版本不存在，将生产基本版本", true);

			/*
			auto targetPath = nbackupsPath + L"\\pakcage_out";
			DeleteDirectory(targetPath);
			CreateDirectory(targetPath, NULL); //创建目标文件夹 

			copyDirectory(outPutPath + L"\\res_package", targetPath + L"\\res_package", true);
			if (targetPath.Find(L"game_") > 0) {
				DeleteFile(outPutPath + L"\\res_package" + "\\android.manifest");
				DeleteFile(outPutPath + L"\\res_package" + "\\ios.manifest");
			}

			copyDirectory(outPutPath + L"\\src_package", targetPath + L"\\src_package", true);
			*/

			CreateDirectory(m_strUploadPath, NULL); //创建目标文件夹 
			m_strBaseVersion = nStrVirsion;
			this->zipPackage(outPutPath, m_strUploadPath,nStrVirsion);

			return;
		};

		/******************************比较开始****************************/
		if (nStrOnLineVirsion.IsEmpty())
		{
			MessageBox(_T("线上版本为空"));
			break;
		}

		if (nStrVirsion.IsEmpty())
		{
			MessageBox(_T("比较版本为空"));
			break;
		}

		m_count = 0;
		m_compCount = 0;

		outPutPath = nbackupsPath + L"\\pakcage_out";
		auto compPath = nbackupsPath + L"\\" + nStrGameName + L"_" + nStrVirsion;
		DeleteDirectory(outPutPath);
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 


		//比较ios文件
		/*
		srcPath = nOnlinePath + L"\\src_ios";
		outPutPath = nbackupsPath + L"\\pakcage_out" + L"\\src_ios";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\src_ios", outPutPath);
		*/

		//比较资源文件
		srcPath = nOnlinePath + L"\\res_package";;
		outPutPath = nbackupsPath + L"\\pakcage_out" + L"\\res_package";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\res_package", outPutPath);

		//比较android文件
		srcPath = nOnlinePath + L"\\src_package";;
		outPutPath = nbackupsPath + L"\\pakcage_out" + L"\\src_package";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\src_package", outPutPath);

		// 特殊处理一下
		if (PathFileExists(outPutPath + L"\\assertmgr.jsc"))
		{
			CopyFile(outPutPath + L"\\assertmgr.jsc", outPutPath + "\\assertmgr.js", FALSE);
		}

		outPutLog(L"==========对比完成==========", false);
		CString tips;
		tips.Format(L"差异文件共：%d", m_compCount);
		outPutLog(tips, false);

		if (0 == m_compCount)
			break;

		CreateDirectory(m_strUploadPath, NULL); //创建目标文件夹 
		m_strBaseVersion = nStrOnLineVirsion;
		this->zipPackage(nbackupsPath + L"\\pakcage_out", m_strUploadPath, nStrVirsion);

		/******************************压缩开始****************************/
		/*
		DeleteDirectory(L"C:\\src");
		DeleteDirectory(L"C:\\res");
		DeleteDirectory(L"D:\\src");
		
		outPutPath = nbackupsPath + L"\\pakcage_out";
		//拷贝ios文件
		copyDirectory(outPutPath + L"\\src_ios", L"C:\\src", false);

		//拷贝资源文件
		copyDirectory(outPutPath + L"\\res_common", L"C:\\res", false);

		//拷贝android文件
		copyDirectory(outPutPath + L"\\src_android", L"D:\\src", false);

		// TODO:  在此添加控件通知处理程序代码
		CString srcIosName(SRC_IOS_NAME);
		srcIosName = srcIosName + nStrVirsion;

		CString srcAndroidName(SRC_ANDROID_NAME);
		srcAndroidName = srcAndroidName + nStrVirsion;

		CString resCommonName(RES_COMMON_NAME);
		resCommonName = resCommonName + nStrVirsion;

		converToZip(L"C:\\src", srcIosName, outPutPath);
		converToZip(L"C:\\res", resCommonName, outPutPath);
		converToZip(L"D:\\src", srcAndroidName, outPutPath);

		outPutLog(L"压缩完成", true);
		*/

	} while (false);
}

/*
* 对比文件夹事件
*/
void CMFCApplication1Dlg::OnBnClickedGenerate()
{
	// TODO:  在此添加控件通知处理程序代码
	generateHandle();
	return;

	cout << "---------------------";

	if (m_strOnlinePath.IsEmpty() || !PathIsDirectory(m_strOnlinePath))
	{
		MessageBox(_T("线上脚本路径出错"));
	}
	else{
		m_count = 0;
		m_compCount = 0;

		auto outPutPath = m_strOutPutPath + L"\\pakcage_out";
		auto compPath = m_strOutPutPath + L"\\" + m_strGameName + L"_" + m_strVirsion;
		DeleteDirectory(outPutPath);
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 

		//比较ios文件
		auto srcPath = m_strOnlinePath + L"\\src_ios";
		outPutPath = m_strOutPutPath + L"\\pakcage_out" + L"\\src_ios";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\src_ios", outPutPath);

		//比较资源文件
		srcPath = m_strOnlinePath + L"\\res_common";;
		outPutPath = m_strOutPutPath + L"\\pakcage_out" + L"\\res_common";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\res_common", outPutPath);

		//比较android文件
		srcPath = m_strOnlinePath + L"\\src_android";;
		outPutPath = m_strOutPutPath + L"\\pakcage_out" + L"\\src_android";
		CreateDirectory(outPutPath, NULL); //创建目标文件夹 
		compDirectory(srcPath, compPath + L"\\src_android", outPutPath);


		outPutLog(L"==============对比完成==============", false);

		CString tips;
		tips.Format(L"差异文件共：%d", m_compCount);
		outPutLog(tips, false);
		MessageBox(tips);
	}
}

/*
* 查看输出文件目录
*/
void CMFCApplication1Dlg::OnBnClickedLook()
{
	// TODO:  在此添加控件通知处理程序代码
	ShellExecute(NULL, NULL, _T("explorer"), m_strOnlinePath, NULL, SW_SHOW);
}

/*
* 上传目录处理
*/
void CMFCApplication1Dlg::OnBnClickedUploadPath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_strUploadPath = Show(_T("请选择上传目录:"));
	m_editUploadPath.SetWindowTextW(m_strUploadPath);
}


void CMFCApplication1Dlg::OnEnChangeEditGamename()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editGameName.GetWindowTextW(m_strGameName);
}


void CMFCApplication1Dlg::OnEnChangeEditPackname()
{
	// TODO:  在此添加控件通知处理程序代码
}

static bool isFristVirsion = false;
void CMFCApplication1Dlg::OnEnChangeEditVirsion()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editVirsion.GetWindowTextW(m_strVirsion);
	if (!isFristVirsion)
	{
		isFristVirsion = true;
		m_strOldVirsion = m_strVirsion;
	}
}

void CMFCApplication1Dlg::OnEnChangeRollVirsion()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editRoll.GetWindowTextW(m_rollVirsion);
}

void CMFCApplication1Dlg::OnEnChangeEditVirsionid()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editVirsionId.GetWindowTextW(m_strVirsionId);
}

void CMFCApplication1Dlg::OnEnChangeOnlinePath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editOnlinePath.GetWindowTextW(m_strOnlinePath);
}

void CMFCApplication1Dlg::OnBnClickedButtonUpload()
{

	CString nOnlinePath = m_strOnlinePath;
	m_strOutPutPath = nOnlinePath + L"\\pakcage_out\\";

	DeleteDirectory(L"C:\\src");
	DeleteDirectory(L"C:\\res");
	DeleteDirectory(L"D:\\src");


	CString nStrUploadPath = m_strUploadPath;
	copyDirectory(m_strOutPutPath, nStrUploadPath, true);

	CString delDir;
	delDir = nStrUploadPath + L"\\res_package";
	DeleteDirectory(delDir);
	delDir = nStrUploadPath + L"\\src_package";
	DeleteDirectory(delDir);
	//CString jscompileCmd(L"TortoiseProc.exe  /command:commit /path:" + nStrUploadPath);
	//execute_cmd_handle(jscompileCmd);

	//ShellExecute(NULL, NULL, _T("explorer"), nStrUploadPath, NULL, SW_SHOW);
}


void CMFCApplication1Dlg::OnBnClickedBtnHotcfg()
{
	// TODO:  在此添加控件通知处理程序代码
	m_strHotCfgPath = Show(_T("请选择热理配置目录:"));
	m_editHotCfgPath.SetWindowTextW(m_strHotCfgPath);
	::c_helper::helper_profile_set(L"m_strHotCfgPath", m_strHotCfgPath);

	int count = 0;
	CString manifestPath;
	CFileFind finder;
	CString path;
	path.Format(L"%s/*.*", m_strHotCfgPath);
	bool bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();
		CString fileName;
		if (!finder.IsDots()){
			count += 1;
			fileName = finder.GetFileName();
		}

		// 查找有效的配置文件
		if (-1 != fileName.Find(L".manifest")){
			manifestPath = fileName;
			break;
		}

		if (4 < count){
			MessageBox(L"无效的配置文件目录");
			return;
		}
	}

	CString strCurGameHotPath(m_strHotCfgPath);
	getVirsion(strCurGameHotPath + L"\\" + manifestPath);
}


void CMFCApplication1Dlg::OnBnClickedBtnCfbalter()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_strGameName.IsEmpty())
	{
		MessageBox(L"产品名为空");
		return;
	}
	if (m_strHotCfgPath.IsEmpty())
	{
		MessageBox(L"配置文件路径为空");
		return;
	}

	if (m_strVirsion.IsEmpty())
	{
		MessageBox(L"版本号为空");
		return;
	}

	if (m_strBaseVersion.IsEmpty()) {
		MessageBox(L"未确认基准版本");
		return;
	}

	/*
	if (m_strOldVirsion == m_strVirsion){
		MessageBox(L"版本号未修改");
		return;
	}
	*/
	alterHotConfig();
}


void CMFCApplication1Dlg::alterHotConfig()
{
	int count = 0;
	CString manifestPath;
	CFileFind finder;
	CString path;
	path.Format(L"%s/*.*", m_strHotCfgPath);
	bool bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();
		CString fileName;
		if (!finder.IsDots()){
			count += 1;
			fileName = finder.GetFileName();
		}

		// 查找有效的配置文件
		if (-1 != fileName.Find(L".manifest")){
			manifestPath = fileName;
			break;
		}

		if (4 < count){
			MessageBox(L"无效的配置文件目录");
			return;
		}
	}

	CString strCurGameHotPath(m_strHotCfgPath);
	//getVirsion(strCurGameHotPath + L"\\" + manifestPath);

	CString strMd5 = MD5::GetMd5();
	path.Format(L"%s/*.*", m_strHotCfgPath);
	bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();
		CString fileName;
		if (!finder.IsDots()){
			count += 1;
			fileName = finder.GetFileName();
		}	
				
		// 修改有效的配置文件
		if (-1 != fileName.Find(L".manifest")){
			CString  manifestFile(strCurGameHotPath + L"\\" + fileName);
			if (checkConfigVersion(manifestFile))
				alterOnecHotConfig(manifestFile, strMd5);
			else{
				outPutLog(manifestFile + L" 修改失败", true);
				break;
			}
		}
	}
	getVirsion(strCurGameHotPath + L"\\" + manifestPath);
	//alterOnecHotConfig(m_strHotCfgPath + L"\\hnmj_ios_v1.manifest", strMd5);

	CString svnCmd(L"TortoiseProc.exe  /command:commit /path:" + m_strHotCfgPath);
	execute_cmd_handle(svnCmd);

	ShellExecute(NULL, NULL, _T("explorer"), m_strHotCfgPath, NULL, SW_SHOW);
}

bool CMFCApplication1Dlg::checkConfigVersion(CString codepath)
{
	bool ret = true;
	CString	str_file_content = ::c_helper::helper_file_text_read_all_utf8_2_unicode(codepath);
	int versionId = 0;
	std::string version;

	Json::Reader json_reader;
	Json::Value json_object;

	char json_document[4096];//// = coverToChar
	coverToChar(str_file_content, json_document);
	if (json_reader.parse(json_document, json_object))
	{
		Json::Value list = json_object["groupVersions"];
		if (!list.isNull()){
			Json::Value::Members members(list.getMemberNames());
			for (auto it = members.begin(); it != members.end(); ++it)
			{
				const std::string &name = *it;

				CString strVeersion(list[name].asString().c_str());
				if (strVeersion == m_strVirsion){
					outPutLog(L"版本号已存在配置中", true);
					ret = false;
					break;
				}				
			}
		}
	}
	else
	{
		ret = false;
	}
	return ret;
}

void CMFCApplication1Dlg::getVirsion(CString path)
{
	CString	str_file_content = ::c_helper::helper_file_text_read_all_utf8_2_unicode(path);
	int versionId = 0;
	std::string version;

	Json::Reader json_reader;
	Json::Value json_object;

	char json_document[4096];//// = coverToChar
	coverToChar(str_file_content, json_document);
	if (json_reader.parse(json_document, json_object))
	{
		Json::Value list = json_object["groupVersions"];
		if (!list.isNull()){
			Json::Value::Members members(list.getMemberNames());
			for (auto it = members.begin(); it != members.end(); ++it)
			{
				const std::string &name = *it;
				int version_id = atoi(name.c_str());
				if (version_id > versionId){
					versionId = version_id;
					version = list[name].asCString();
				}
			}
		}
	}

	m_strVirsion = version.c_str();
	m_strVirsionId = to_string(versionId).c_str();

	m_editVirsion.SetWindowTextW(m_strVirsion);

	CString mainfirstName(path);
	mainfirstName = mainfirstName.Right(mainfirstName.GetLength() - mainfirstName.ReverseFind(L'\\') - 1);
	outPutLog(L"配置文件:" + mainfirstName, true);
	outPutLog(L"最大版本ID:" + m_strVirsionId, false);
	outPutLog(L"版本号:" + m_strVirsion, false);
}


void CMFCApplication1Dlg::alterOnecHotConfig(CString path, CString md5)
{
	// 版本号+1
	int nVersionId = _ttoi(m_strVirsionId) + 1;
	
	CString	str_file_content = ::c_helper::helper_file_text_read_all_utf8_2_unicode(path);

	Json::Reader json_reader;
	Json::Value json_object;
	Json::FastWriter fast_writer;

	char json_document[4096];//// = coverToChar
	char tempVersion[64];//// = coverToChar
	char tempMd5[100];//// = coverToChar

	bool isBaseVersion = false;

	coverToChar(md5, tempMd5);
	coverToChar(m_strVirsion, tempVersion);
	coverToChar(str_file_content, json_document);
	if (json_reader.parse(json_document, json_object))
	{
		Json::Value versionList = json_object["groupVersions"];
		if (!versionList.isNull()){
			Json::Value::Members members(versionList.getMemberNames());
			for (auto it = members.begin(); it != members.end(); ++it)
			{
				const std::string &name = *it;
				std::string str = versionList[name].asCString();
			}
			versionList[to_string(nVersionId)] = Json::Value(tempVersion);
			auto json_document = fast_writer.write(versionList);
			json_object["groupVersions"] = versionList;

			json_document = fast_writer.write(json_object);
		}

		Json::Value assets = json_object["assets"];
		if (!assets.isNull())
		{
			Json::Value json_assets;

			Json::Value::Members members(assets.getMemberNames());
			for (auto it = members.begin(); it != members.end(); ++it) {
				const std::string& name = *it;
				Json::Value& value = assets[name];

				CString strVeersion(value["version"].asString().c_str());
				if (strVeersion == m_strBaseVersion) {
					json_assets[name] = value;
				}
			}

			if (m_strVirsion != m_strBaseVersion || (m_strVirsion == m_strBaseVersion && members.size() == 0)) {
				std::string srcName = SRC_ANDROID_NAME;
				srcName = srcName + tempVersion;

				std::string resName = RES_COMMON_NAME;
				resName = resName + tempVersion;

				Json::Value json_assets_src;
				json_assets_src["compressed"] = Json::Value(true);
				json_assets_src["group"] = Json::Value(to_string(nVersionId));

				std::string uploadPath = CT2A(m_strUploadPath);
				std::string md5Str = CMD5Checksum::GetMD5(uploadPath  + "\\" + srcName + ".zip");
				
				json_assets_src["md5"] = Json::Value(md5Str);
				json_assets_src["path"] = Json::Value(srcName + ".zip");
				json_assets_src["version"] = Json::Value(tempVersion);
				auto json_document = fast_writer.write(json_assets_src);

				Json::Value json_assets_res;
				json_assets_res["compressed"] = Json::Value(true);
				json_assets_res["group"] = Json::Value(to_string(nVersionId));

				md5Str = CMD5Checksum::GetMD5(uploadPath + "\\" + resName + ".zip");
				json_assets_res["md5"] = Json::Value(md5Str);
				json_assets_res["path"] = Json::Value(resName + ".zip");
				json_assets_res["version"] = Json::Value(tempVersion);
				json_document = fast_writer.write(json_assets_res);

				json_assets[srcName] = Json::Value(json_assets_src);
				json_assets[resName] = Json::Value(json_assets_res);
				json_document = fast_writer.write(json_assets);

				json_object["assets"] = json_assets;
				json_document = fast_writer.write(json_object);
			}
		}
		else{
			json_object.removeMember("assets");
		}

		Json::StyledWriter styled_writer;
		auto json_document = styled_writer.write(json_object);

		CString new_file_content(json_document.c_str());
		::c_helper::helper_file_text_write_unicode_2_utf8(path, new_file_content);
	}
}
void CMFCApplication1Dlg::OnCbnSelchangeComboList()
{
	// TODO:  在此添加控件通知处理程序代码
	auto index = m_comboGameList.GetCurSel();

	isFristVirsion = false;
	updateAllConfig(index);
}


void CMFCApplication1Dlg::OnBnClickedSaveConfig()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_strGameName.IsEmpty()){
		MessageBox(L"产品名不能为空");
		return;
	}

	bool isExist = false;
	for (int i = 0; i < m_gameList.GetSize(); i++){
		if (m_strGameName == m_gameList.GetAt(i)){
			isExist = true;
			break;
		}
	}
	if (!isExist){
		m_comboGameList.AddString(m_strGameName);
		m_comboGameList.SetCurSel(m_comboGameList.GetCount() + 1);
		m_gameList.Add(m_strGameName);
	}

	CString strGameList = ::c_helper::strarray_to_string(m_gameList, L",");

	::c_helper::helper_profile_set(L"strGameList", strGameList);
	::c_helper::helper_profile_set(m_strGameName + L"m_strOnlinePath", m_strOnlinePath);
	::c_helper::helper_profile_set(m_strGameName + L"m_strProjectPath", m_strProjectPath);
	::c_helper::helper_profile_set(m_strGameName + L"m_strUploadPath", m_strUploadPath);
	::c_helper::helper_profile_set(m_strGameName + L"m_strGameName", m_strGameName);
	::c_helper::helper_profile_set(m_strGameName + L"m_strVirsion", m_strVirsion);
	::c_helper::helper_profile_set(m_strGameName + L"m_strHotCfgPath", m_strHotCfgPath);
	::c_helper::helper_profile_set(m_strGameName + L"m_strVirsionId", m_strVirsionId);
	::c_helper::helper_profile_set(m_strGameName + L"m_strOnlineVirsion", m_strOnlineVirsion);

	CString commonSrc;
	commonSrc.Format(L"%d", m_commonString);
	::c_helper::helper_profile_set(m_strGameName + L"m_commonString", commonSrc);


	outPutLog(L"保存成功", true);

}

void CMFCApplication1Dlg::updateAllConfig(int index)
{
	cleanLog();
	if (0 >= m_gameList.GetSize()) return;

	CString name = m_gameList.GetAt(index);

	this->m_strOnlinePath = ::c_helper::helper_profile_get(name + CString("m_strOnlinePath"));
	this->m_strProjectPath = ::c_helper::helper_profile_get(name + CString("m_strProjectPath"));
	this->m_strUploadPath = ::c_helper::helper_profile_get(name + CString("m_strUploadPath"));
	this->m_strGameName = ::c_helper::helper_profile_get(name + CString("m_strGameName"));
	this->m_strVirsion = ::c_helper::helper_profile_get(name + CString("m_strVirsion"));
	this->m_strHotCfgPath = ::c_helper::helper_profile_get(name + CString("m_strHotCfgPath"));
	this->m_strVirsionId = ::c_helper::helper_profile_get(name + CString("m_strVirsionId"));
	this->m_strOnlineVirsion = ::c_helper::helper_profile_get(name + CString("m_strOnlineVirsion"));
	this->m_commonString = _ttoi(::c_helper::helper_profile_get(name + CString("m_commonString")));

	// TODO:  在此添加控件通知处理程序代码
	if (m_strGameName.IsEmpty())
	{
		MessageBox(L"产品名为空");
		return;
	}
	if (m_strHotCfgPath.IsEmpty())
	{
		MessageBox(L"配置文件路径为空");
		return;
	}
	int count = 0;
	CString manifestPath;
	CFileFind finder;
	CString path;
	path.Format(L"%s/*.*", m_strHotCfgPath);
	bool bWorking = finder.FindFile(path);
	while (bWorking){
		bWorking = finder.FindNextFile();
		CString fileName;
		if (!finder.IsDots()){
			count += 1;
			fileName = finder.GetFileName();
		}

		// 查找有效的配置文件
		if (-1 != fileName.Find(L".manifest")){
			manifestPath = fileName;
			break;
		}

		if (4 < count){
			MessageBox(L"无效的配置文件目录");
			return;
		}
	}

	CString strCurGameHotPath(m_strHotCfgPath + L"\\" + manifestPath);
	
	if (PathFileExists(strCurGameHotPath)){
		getVirsion(strCurGameHotPath);
	}
	else{
		outPutLog(L"热更配置文件路径出错：\r\n " + strCurGameHotPath, true);
		this->m_strVirsion = "";
		this->m_strVirsionId = "";
	}

	m_editOnlinePath.SetWindowTextW(this->m_strOnlinePath);
	m_editProjectPath.SetWindowTextW(this->m_strProjectPath);
	m_editUploadPath.SetWindowTextW(this->m_strUploadPath);
	m_editGameName.SetWindowTextW(this->m_strGameName);
	m_editVirsion.SetWindowTextW(this->m_strVirsion);
	m_editHotCfgPath.SetWindowTextW(this->m_strHotCfgPath);
	m_editOnlineVirsion.SetWindowTextW(this->m_strOnlineVirsion);
	m_commonSrcCheckBox.SetCheck(m_commonString);

	//checkCodeVersion(m_strProjectPath);
}

void CMFCApplication1Dlg::checkCodeVersion(CString codepath)
{
	CString jsCodePath(codepath + L"\\src\\commonCtr\\platform.js");
	CString testKey("PlatformConf.isTest");
	CString versionKey("PlatformConf.Version");

	if (!PathFileExists(jsCodePath)){

		jsCodePath	= codepath + L"\\src\\conf.js";
		testKey		= "conf.isRelease";
		versionKey	= "conf.version";

		if (!PathFileExists(jsCodePath)){
			//outPutLog(jsCodePath + L" 文件不存在", true);
			return;
		}
	}

	CString checkName = jsCodePath.Right(jsCodePath.GetLength() - jsCodePath.ReverseFind(L'\\') - 1);
	outPutLog(L"\n检测到文件:" + checkName, true);

	CString	str_file_content = ::c_helper::helper_file_text_read_all_utf8_2_unicode(jsCodePath);

	CString	str_file_content_istest = str_file_content.Right(str_file_content.GetLength()-str_file_content.Find(testKey));
	str_file_content_istest = str_file_content_istest.Left(str_file_content_istest.Find(L";"));

	CString	str_file_content_version = str_file_content.Right(str_file_content.GetLength() - str_file_content.Find(versionKey));
	str_file_content_version = str_file_content_version.Left(str_file_content_version.Find(L";"));

	outPutLog(L"代码配置:", false);
	outPutLog(str_file_content_istest, false);
	outPutLog(str_file_content_version, false);
}


void CMFCApplication1Dlg::OnEnChangeUpdatePath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editOnlinePath.GetWindowTextW(m_strOnlinePath);
}


void CMFCApplication1Dlg::OnEnChangeUploadPath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editUploadPath.GetWindowTextW(m_strUploadPath);
}


void CMFCApplication1Dlg::OnEnChangeHotupdateCfgPath()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editHotCfgPath.GetWindowTextW(m_strHotCfgPath);
}


void CMFCApplication1Dlg::OnEnChangeEditOnlineVirsion()
{
	// TODO:  在此添加控件通知处理程序代码
	m_editOnlineVirsion.GetWindowTextW(m_strOnlineVirsion);
}


void CMFCApplication1Dlg::OnBnClickedCheck1()
{
	// TODO:  在此添加控件通知处理程序代码
	m_commonString = m_commonSrcCheckBox.GetCheck();
}

void CMFCApplication1Dlg::execute_cmd_handle(CString cmdline){
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		::AfxMessageBox(CString("c_auto_compile_ggc_dlg.CreatePipe.error"));
		return;
	}

	STARTUPINFO			si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	CString finalCmd = CString("cmd /c \"") + cmdline;

	if (!CreateProcess(NULL, finalCmd.GetBuffer(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		MessageBox(CString("CreateProcess failed!"));
		return;
	}
	CloseHandle(hWrite);
	finalCmd.ReleaseBuffer();

	char buffer[4444] = { 0 };
	DWORD bytesRead;
	//m_outPutText.SetWindowText(CString(""));
	while (1)
	{
		if (NULL == ReadFile(hRead, buffer, 4443, &bytesRead, NULL))
		{
			break;
		}
		/*
		this->m_outPutText.SetRedraw(FALSE);
		int LineNum = this->m_outPutText.GetLineCount();
		if (LineNum > 34)
		{
			this->m_outPutText.SetSel(0, -1);
			this->m_outPutText.Clear();
		}
		this->m_outPutText.SetLimitText(0);
		CString		str_s(buffer);
		*/
		/* write to log file
		*
		*//*
		::c_helper::helper_log_write(str_s);

		int	nLength = this->m_outPutText.GetWindowTextLength();
		this->m_outPutText.SetSel(nLength, nLength);
		this->m_outPutText.ReplaceSel(str_s);
		this->m_outPutText.LineScroll(this->m_outPutText.GetLineCount());
		this->m_outPutText.SetRedraw(TRUE);*/
	}
	CloseHandle(hRead);
}



void CMFCApplication1Dlg::OnBnClickedPublicUpload()
{
	char mac[200];
	getMAC(mac);
	if (strcmp("30-9C-23-CC-94-D2", mac) != 0) {
		MessageBox(CString("正在研发中"));
		return;
	}

	CString srcAndroidName(SRC_ANDROID_NAME);
	CString resCommonName(RES_COMMON_NAME);

	//解压
	//m_strOnlineVirsion
	CString unzipPath = m_strUploadPath + L"\\";
	UnpackFile(unzipPath + srcAndroidName + m_rollVirsion + L".zip");
	UnpackFile(unzipPath + resCommonName + m_rollVirsion + L".zip");

	UnpackFile(unzipPath + srcAndroidName + m_strVirsion + L".zip");
	UnpackFile(unzipPath + resCommonName + m_strVirsion + L".zip");

	//比对


	//CString publicPath( m_strHotCfgPath );
	//publicPath.Replace(L"publictest", L"public");


	//copyDirectory(m_strHotCfgPath, publicPath, true);

	//CString jscompileCmd(L"TortoiseProc.exe  /command:commit /path:" + publicPath);
	//execute_cmd_handle(jscompileCmd);

	//ShellExecute(NULL, NULL, _T("explorer"), publicPath, NULL, SW_SHOW);
}

