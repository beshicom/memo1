


//
//	memo1.h : １行メモ
//
//	since 2023/10/16 by beshi
//



#include	<windows.h>
#include	<shlobj.h>
#include	<direct.h>
#include	"memo1.h"

#define	WM_RETURN	WM_USER+0x100



HINSTANCE	hInstance;
HWND		hwndMain;

char	ClassName[] = "Memo1MainClass";
char	AppName[] = "Memo1";

char	EditClass[] = "EDIT";
HWND	hwndEdit;
WNDPROC	OldWndProc;

#define	MEMOSIZE	1024
#define	BUFFERSIZE	MEMOSIZE+100
char	buffer[ BUFFERSIZE + 1 ];



//
// データファイルのフルパスを得る BCCGetDataPath0()				//TAG_JUMP_MARK
//
BOOL  BCCGetDataPath0(  char *pBuf,  char *pFileName  )
{
	if( pBuf == NULL )						return(  0  );
	if( pFileName == NULL )					return(  0  );
	if( *pFileName == 0 )					return(  0  );
	SHGetFolderPath(
		NULL,			//HWND   hwndOwner,
		CSIDL_APPDATA,	//int    nFolder,
		NULL,			//HANDLE hToken,
		0,				//DWORD  dwFlags,
		pBuf			//LPTSTR pszPath
	);
	lstrcat( pBuf, "\\MEMO1" );
	char	mp[ MAX_PATH + 32 ];
	getcwd( mp, MAX_PATH );
	if(  chdir( pBuf )  !=  0  )	mkdir( pBuf );
	else	chdir( mp );
	lstrcat( pBuf, "\\" );
	lstrcat( pBuf, pFileName );

	return(  1  );
}



//
// データファイルのフルパスを得る BCCGetDataPath()				//TAG_JUMP_MARK
//
BOOL  BCCGetDataPath(  char *pBuf  )
{
	return(  BCCGetDataPath0( pBuf, "MEMO1.TXT" )  );
}



LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK EditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );



int WinMain( HINSTANCE hInst, HINSTANCE hPrevInst,
												char * CmdLine, int CmdShow )
{

	hInstance = hInst;

	// ウインドウクラスの登録
	{
	WNDCLASSEX	wc;
	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	wc.lpszMenuName = MAKEINTRESOURCE( IDR_MAINMENU );
	wc.lpszClassName = ClassName;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	RegisterClassEx( &wc );
	}

	// ウインドウの作成
	{
	hwndMain = CreateWindowEx( WS_EX_CLIENTEDGE, ClassName, AppName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
		NULL, NULL, hInst, NULL );
	//ShowWindow( hwnd, CmdShow );
	//UpdateWindow( hwnd );
	}

	// メッセージループ
	{
	MSG	msg;
	while( GetMessage( &msg, NULL, 0, 0 ) ){
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	return msg.wParam;
	}

}



LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_CREATE:
		{

		hwndEdit = CreateWindowEx( NULL, EditClass, NULL,
			WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL ,
			0, 0, 0, 0, hWnd, NULL, hInstance, NULL );
		SetFocus( hwndEdit );

		// サブクラス化
		OldWndProc = (WNDPROC)SetWindowLong(
								hwndEdit, GWL_WNDPROC, (DWORD)EditWndProc );

		}// WM_CREATE
		break;

	case WM_CTLCOLOREDIT:
		{

		SetTextColor( (HDC)wParam, RGB(255,255,0) );
		SetBkColor( (HDC)wParam, RGB(0,0,0) );
		return (LRESULT)GetStockObject( BLACK_BRUSH );

		}// WM_CTLCOLOREDIT
		break;

	case WM_SIZE:
		{

		MoveWindow( hwndEdit, 0, 0, lParam&0xffff, lParam>>16, TRUE );

		}// WM_SIZE
		break;

	case WM_COMMAND:
		{

		if( lParam != 0 ){
			// lParam : コントロールウインドウのハンドル
			//	HIWORD(wParam) : 通知コード
			//	LOWORD(wParam) : コントロールID
			break;
		}

		if( HIWORD( wParam ) != 0 ){
			// HIWORD( wParam ) : アクセラレータのメッセージ
			break;
		}

		switch( LOWORD( wParam ) ){

		case IDM_OPEN:
			{

			}// end IDM_OPEN
			break;

		default:
			{

			DestroyWindow( hWnd );

			}

		}// switch

		}// end WM_COMMAND
		break;

	case WM_RETURN:
		{

		SYSTEMTIME	dt;
		GetLocalTime( &dt );
		wsprintf(  buffer,  "%d/%02d/%02d %02d:%02d:%02d  ",
			dt.wYear, dt.wMonth, dt.wDay,  dt.wHour, dt.wMinute, dt.wSecond );

		int	n = lstrlen( buffer );

		GetWindowText( hwndEdit, buffer+n, MEMOSIZE );

		//MessageBox( NULL, buffer, AppName, MB_OK );

		char	path[ MAX_PATH ];
		BCCGetDataPath( path );

		HANDLE	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE,
			NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

		SetFilePointer( hFile, 0, NULL, FILE_END );

		n = lstrlen( buffer );
		buffer[n] = 0x0d;
		buffer[n+1] = 0x0a;
		buffer[n+2] = 0;
		DWORD	bytesRead;
		WriteFile( hFile, buffer, n+2, &bytesRead, NULL );

		CloseHandle( hFile );

		PostQuitMessage( NULL );

		}// WM_RETURN
		break;

	case WM_DESTROY:

		PostQuitMessage( NULL );
		break;

	default:

		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;

}



LRESULT CALLBACK EditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_KEYDOWN:
		{

		if( (char)wParam == VK_RETURN )
			SendMessage( hwndMain, WM_RETURN, 0, 0 );

		return CallWindowProc( OldWndProc, hWnd, uMsg, wParam, lParam );

		}// WM_KEYDOWN
		break;

	default:
		return CallWindowProc( OldWndProc, hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;


}



// end of this file : memo1.cpp
