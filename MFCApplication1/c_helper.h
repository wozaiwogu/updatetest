#pragma once
#include <vector>
#include <sstream>

class c_helper
{
private:
	static int CALLBACK helper_dir_browser_dir_callback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData){if(uMsg == BFFM_INITIALIZED ){::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpData);}return 0;}
public:
	c_helper(void);
	virtual ~c_helper(void);
	static CString	helper_dir_browser_dir(CString str_title,CString str_dir_ini=CString());
	static BOOL		helper_dir_copy_src_2_dest(CString str_src,CString str_dest);
	static CString	helper_string_load_by_id(int n_string_id);
	static void		helper_profile_set(CString str_key, CString str_value);
	static CString	helper_profile_get(CString str_Key);	
	template <class T> static CString	helper_convert_num_2_str(T t)		{::std::ostringstream oss;oss<<t;return CString(oss.str().c_str());}
	template <class T> static T			helper_convert_str_2_num(CString s)	{::std::wstring sx =s;::std::wstringstream ss(sx);T t;ss>>t;return t;}
	static CString helper_file_text_read_all_utf8_2_unicode(CString str_file_path);
	static void helper_file_text_write_unicode_2_utf8(CString str_file_path,CString str_text);
	static CString helper_conver_char_2_unicode(char* p_src);
	static CString helper_app_get_root_dir(void);
	static BOOL helper_dir_delete(CString str_dir_path);
	static void helper_log_write(CString str_log_info);
	static void helper_log_delete();
	static CString helper_convert_relative_path_2_absolute_path(CString str_relative_path);

	static void string_to_strarray(CString str, CStringArray &strarray, CString tokenize);
	static CString strarray_to_string(CStringArray &strarray, CString tokenize);

};

