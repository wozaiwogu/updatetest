#include "StdAfx.h"
#include "c_helper.h"
 


c_helper::c_helper(void)
{
}


c_helper::~c_helper(void)
{
}


CString c_helper::helper_dir_browser_dir(CString str_title,CString str_dir_ini)
{
	::LPITEMIDLIST      p_item_id_list              = NULL  ;
	::TCHAR             sz_path[MAX_PATH]           = {0}   ;  
	::BROWSEINFO		bi;::memset(&bi,0,sizeof(bi))		;
	bi.lpszTitle    = str_title.GetBuffer();str_title.ReleaseBuffer();
	bi.ulFlags      = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON ;
	bi.lpfn         = NULL;
	bi.lpfn         = helper_dir_browser_dir_callback;
	if(str_dir_ini.IsEmpty())
	{
	}
	else
	{
		bi.lParam   = (LPARAM)str_dir_ini.GetBuffer();str_dir_ini.ReleaseBuffer();		
	}
	p_item_id_list  = ::SHBrowseForFolder(&bi);
	if(SUCCEEDED(p_item_id_list))
	{
		::SHGetPathFromIDList(p_item_id_list,sz_path);::ILFree(p_item_id_list);
		return CString(sz_path);
	}else
	{
		return CString();		
	}
}


CString c_helper::helper_string_load_by_id(int n_string_id)
{
	::TCHAR sz[1024]	= {0};::AfxLoadString(n_string_id,sz);return CString(sz);
}

CString c_helper::helper_profile_get(CString str_key )
{
	auto name = ::AfxGetAppName();
	return ::AfxGetApp()->GetProfileString(::AfxGetAppName(),str_key);
}
void c_helper::helper_profile_set(CString str_key, CString str_value)
{
	::AfxGetApp()->WriteProfileString(::AfxGetAppName(),str_key,str_value);
}
BOOL c_helper::	helper_dir_copy_src_2_dest(CString str_src,CString str_dest)
{
	/* src dir
	 *
	 */
	::TCHAR sz_src[MAX_PATH] = {0};::StrCpy(sz_src,str_src.GetBuffer());str_src.ReleaseBuffer();sz_src[str_src.GetLength()]	= '\0';
	
	/* copy source directory to destnation directory
	 *
	 */
	::SHFILEOPSTRUCT sh_file_op_struct;::memset(&sh_file_op_struct,0,sizeof(::SHFILEOPSTRUCT))		;
	sh_file_op_struct.wFunc		= FO_COPY															;
	sh_file_op_struct.pFrom		= sz_src															;
	sh_file_op_struct.pTo		= str_dest															;
	sh_file_op_struct.fFlags	= FOF_NO_UI															;
	int n_r						= ::SHFileOperation(&sh_file_op_struct)								;
	return n_r==0?TRUE:FALSE;
}


CString c_helper::helper_file_text_read_all_utf8_2_unicode(CString str_file_path)
{
	::CFile o_file;o_file.Open(str_file_path,::CFile::modeRead);

	::ULONGLONG ull_file_size	= o_file.GetLength();
	int			nxx				= (int)ull_file_size +3;
	char		*p_buffer		= new char[nxx];::memset(p_buffer,0,nxx);
	int			nx				= o_file.Read(p_buffer,(UINT)ull_file_size);o_file.Close();
	
    // UTF8 to Unicode
    // 由于中文直接复制过来会成乱码，编译器有时会报错，故采用16进制形式
	//
	char		*szU8			= p_buffer;

    // 预转换，得到所需空间的大小
	//
    int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, (int)strlen(szU8), NULL, 0);
    
	// 分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	//
	::TCHAR		*wszString		= new ::TCHAR[wcsLen + 1]; 
    
	// 转换
	//
    ::MultiByteToWideChar(CP_UTF8, NULL, szU8, (int)strlen(szU8), wszString, wcsLen);

    // 最后加上'\0'
	//
    wszString[wcsLen]			= '\0';	
	CString str_s				= CString(wszString);			
	delete []wszString;wszString=NULL;
	delete []p_buffer;p_buffer	= NULL;
	return str_s; 
 }

void c_helper::helper_file_text_write_unicode_2_utf8(CString str_file_path,CString str_text)
{
	// unicode to UTF8  
	//
	TCHAR * wszString = str_text.GetBuffer();str_text.ReleaseBuffer();

    // 预转换，得到所需空间的大小，这次用的函数和上面名字相反  
	//
    int u8Len = ::WideCharToMultiByte(CP_UTF8, NULL, wszString,(int) wcslen(wszString), NULL, 0, NULL, NULL);  

    // 同上，分配空间要给'\0'留个空间  
    //UTF8虽然是Unicode的压缩形式，但也是多字节字符串，所以可以以char的形式保存  
	//
    char* szU8 = new char[u8Len + 1];  


    // 转换  
    // unicode版对应的strlen是wcslen  
	//
    ::WideCharToMultiByte(CP_UTF8, NULL, wszString,(int) wcslen(wszString), szU8, u8Len, NULL, NULL);  

    // 最后加上'\0'  
	//
    szU8[u8Len] = '\0';  
      
    // 接下来写入文本  
    // 写文本文件，UTF8的BOM是0xbfbbef  
	//
    CFile cFile;cFile.Open(str_file_path, CFile::modeWrite | CFile::modeCreate);  
   
	// 写BOM，同样低位写在前  
    // cFile.Write("\xef\xbb\xbf", 3);  

    // 写入内容  
	//
    cFile.Write(szU8, u8Len * sizeof(char));cFile.Flush();cFile.Close();delete[] szU8;szU8=NULL;  
}
CString c_helper::helper_conver_char_2_unicode(char* p_src)
{
    // UTF8 to Unicode
	char		*szU8			= p_src;

    // 预转换，得到所需空间的大小
    int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, (int)strlen(szU8), NULL, 0);
    
	// 分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	::TCHAR		*wszString		= new ::TCHAR[wcsLen + 1]; 
    
	// 转换
    ::MultiByteToWideChar(CP_UTF8, NULL, szU8, (int)strlen(szU8), wszString, wcsLen);

    // 最后加上'\0'
    wszString[wcsLen]			= '\0';	
	CString str_s				= CString(wszString);		
	delete []wszString;wszString=NULL;
	return str_s;
}


CString c_helper::helper_app_get_root_dir(void)
{
	::TCHAR buffer[MAX_PATH];::GetModuleFileName(NULL,buffer,MAX_PATH);CString s=CString(buffer);s=s.Left(s.ReverseFind('\\'));return s;
}


BOOL c_helper::helper_dir_delete(CString str_dir_path)
{	
	CString str_cmd = CString("cmd.exe /c \"rd /q /s ")+str_dir_path+CString("\"");
 
	// create process
	//
	PROCESS_INFORMATION pi;::memset(&pi,0,sizeof(pi));
	STARTUPINFO			si;::memset(&si,0,sizeof(si));GetStartupInfo(&si);
	si.dwFlags				|= STARTF_USESHOWWINDOW;
	si.hStdOutput			= NULL;
	si.wShowWindow			= SW_HIDE;	
	BOOL				res = CreateProcess(NULL,str_cmd.GetBuffer(),NULL,NULL,FALSE,CREATE_NO_WINDOW ,NULL,NULL,&si,&pi);str_cmd.ReleaseBuffer();
	if (res)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	} 
	else
		return false;

	return true;
}


void c_helper::helper_log_delete(){

	CString str_log_file = ::c_helper::helper_app_get_root_dir()+CString("\\log.txt");::CFile o_file ;if(o_file.Open(str_log_file,::CFile::modeRead)){o_file.Close();

		/*
		 *
		 */
		::CFile::Remove(str_log_file);
	}
}
void c_helper::helper_log_write(CString str_text)
{	
	str_text			+= CString("\r\n");
	CString str_log_file = ::c_helper::helper_app_get_root_dir()+CString("\\log.txt");
	
	// unicode to UTF8  
	TCHAR * wszString = str_text.GetBuffer();str_text.ReleaseBuffer();

    //预转换，得到所需空间的大小，这次用的函数和上面名字相反  
    int u8Len = ::WideCharToMultiByte(CP_UTF8, NULL, wszString,(int) wcslen(wszString), NULL, 0, NULL, NULL);  
    //同上，分配空间要给'\0'留个空间  
    //UTF8虽然是Unicode的压缩形式，但也是多字节字符串，所以可以以char的形式保存  
    char* szU8 = new char[u8Len + 1];  
    //转换  
    //unicode版对应的strlen是wcslen  
    ::WideCharToMultiByte(CP_UTF8, NULL, wszString,(int) wcslen(wszString), szU8, u8Len, NULL, NULL);  
    //最后加上'\0'  
    szU8[u8Len] = '\0';  
    //MessageBox不支持UTF8,所以只能写文件  
  
    //接下来写入文本  
    //写文本文件，UTF8的BOM是0xbfbbef  	
	::CFile cFile;::CFile file;if(file.Open(str_log_file,::CFile::modeRead)){file.Close();}else
	{
		/* ini,create log file
		 *
		 */
		cFile.Open(str_log_file,::CFile::modeNoTruncate|::CFile::modeWrite|::CFile::modeCreate|::CFile::shareDenyNone);cFile.Close();   	
	}

	/* open file
	 *
	 */
	cFile.Open(str_log_file,::CFile::modeNoTruncate|::CFile::modeWrite|::CFile::shareDenyNone);cFile.SeekToEnd();
		
	//写BOM，同样低位写在前  
    // cFile.Write("\xef\xbb\xbf", 3);  

    //写入内容  
    cFile.Write(szU8, u8Len * sizeof(char));  
    cFile.Flush();  
    cFile.Close();  
    delete[] szU8;  
    szU8 =NULL;  	
}


CString c_helper::helper_convert_relative_path_2_absolute_path(CString str_relative_path)
{
	::WCHAR sz_buffer_path [MAX_PATH]={0};::PathCombine(sz_buffer_path,str_relative_path,NULL);return CString(sz_buffer_path);
}

void c_helper::string_to_strarray(CString str, CStringArray &strarray, CString tokenize)
{
	CString strTmp;
	int iPos = 0;
	strTmp = str.Tokenize(tokenize, iPos);
	while (strTmp.Trim() != _T(""))
	{
		strarray.Add(strTmp);
		strTmp = str.Tokenize(tokenize, iPos);
	}
}

CString c_helper::strarray_to_string(CStringArray &strarray, CString tokenize)
{
	CString str;

	int i = 0;
	for (i = 0; i < strarray.GetSize() - 1; i++){
		str = str + strarray.GetAt(i) + tokenize;
	}
	str = str + strarray.GetAt(i);

	return str;
}