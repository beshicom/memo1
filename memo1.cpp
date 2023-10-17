


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
char	AppName[] = "Memo1    RETURN for SAVE  R-BUTTON for MENU";

char	EditClass[] = "EDIT";
HWND	hwndEdit;
WNDPROC	OldWndProc;

#define	MEMOSIZE	1024
#define	BUFFERSIZE	MEMOSIZE+100
char	buffer[ BUFFERSIZE + 1 ];

// 過去メモの表示
DWORD	nMaxDsp = 30;	// 最大表示数

struct MemoInfo {
	MemoInfo *	pPrev;
	MemoInfo *	pNext;
	char *		pText;
};

MemoInfo *	pTopMemoData = NULL;
MemoInfo *	pLastMemoData = NULL;



//
// 改行コードまで移動する
//
char * MoveToCRLF ( char * pText )
{
	char *	p = pText;
	if( p == NULL )		return NULL;
	while( *p != 0 ){
		if( *p == 0x0d )	break;
		if( *p == 0x0a )	break;
		++p;
	}
	return p;
}



//
// 改行コードをスキップする
//
char * SkipCRLF ( char * pText )
{
	char *	p = pText;
	if( p == NULL )		return NULL;
	while( *p != 0 ){
		if( *p == 0x0d ){ ++p; continue; }
		if( *p == 0x0a ){ ++p; continue; }
		break;
	}
	return p;
}



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
	//wc.lpszMenuName = MAKEINTRESOURCE( IDR_MAINMENU );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ClassName;
	wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDR_ICON) );
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	RegisterClassEx( &wc );
	}


	// 前回のメモを取得
	{

	char	path[ MAX_PATH ];
	BCCGetDataPath( path );

	HANDLE	hFile = CreateFile( path, GENERIC_READ,
			NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	while( hFile != NULL ){

		LARGE_INTEGER	fs;
		fs.QuadPart = 0;

		BOOL	fOK = GetFileSizeEx( hFile, &fs );
		if( ! fOK ){  CloseHandle(hFile);  break;  }

		int	sz = fs.u.LowPart + 100;
		char *	pb = new char[sz];
		if( pb == NULL ){  CloseHandle(hFile);  break;  }
		ZeroMemory( pb, sz );

		DWORD	bytesRead;
		fOK = ReadFile( hFile, pb, fs.u.LowPart, &bytesRead, NULL );
		CloseHandle( hFile );
		if( ! fOK ){  delete[] pb;  break;  }
		pb[bytesRead] = 0;

		char *	p = pb;
		int		nLine = 0;
		while( p < pb+bytesRead-10 ){
			if( *p == 0 )	break;
			int		n = MoveToCRLF(p) - p;
			if( n <= 0 )	break;
			MemoInfo *	mi = new MemoInfo;
			if( mi == NULL )	break;
			mi->pPrev = NULL;
			mi->pNext = NULL;
			mi->pText = new char[n+1];
			if( mi->pText == NULL ){
				delete mi;  break;
			}
			CopyMemory( mi->pText, p, n );
			*(mi->pText+n) = 0;
			//MessageBox( NULL, mi->pText, AppName, MB_OK );
			p += n+1;
			p = SkipCRLF(p);
			// リンク
			if( pLastMemoData == NULL ){	// ひとつめ
				pTopMemoData = mi;
			}
			else{	// miはpLastMemoDataの次
				mi->pPrev = pLastMemoData;
				pLastMemoData->pNext = mi;
			}
			pLastMemoData = mi;
			if( nLine >= nMaxDsp ){
				// 先頭を削除
				MemoInfo *	pm = pTopMemoData->pNext;
				delete[] pTopMemoData->pText;
				delete	pTopMemoData;
				pTopMemoData = pm;
				pTopMemoData->pPrev = NULL;
			}
			else	++nLine;
		}// while p

		delete[] pb;

		break;

	}// while hFile

	}// end


	// ウインドウの作成
	{
	char * pt = AppName;
	if( pLastMemoData != NULL ){
		wsprintf( buffer, "%s      %-100s", AppName, pLastMemoData->pText );
		pt = buffer;
	}
	hwndMain = CreateWindowEx( WS_EX_CLIENTEDGE, ClassName, pt,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
		CW_USEDEFAULT, CW_USEDEFAULT, 1000, 80,
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

	MemoInfo *	p = pTopMemoData;
	while( p != NULL ){
		MemoInfo *	mp = p->pNext;
		delete[]	p->pText;
		delete		p;
		p = mp;
	}

	return msg.wParam;

	}// end

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

	case WM_RBUTTONDOWN:
		{

		POINT	pt;
		pt.x = LOWORD( lParam );
		pt.y = HIWORD( lParam );
		HMENU	h = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_MAINMENU ) );
		HMENU	hs = GetSubMenu( h, 0 );
		//CheckMenuItem( h, IDM_TOPMOST,
		//		MF_BYCOMMAND | ( gfTopMost ? MF_CHECKED : MF_UNCHECKED )  );
		ClientToScreen( hWnd, &pt );
		TrackPopupMenu( hs, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu( h );

		}	// WM_RBUTTONDOWN
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

			MessageBox( NULL, "IDM_OPEN", AppName, MB_OK );

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

		if(  GetWindowText( hwndEdit, buffer+n, MEMOSIZE ) == 0  ){
			PostQuitMessage( NULL );
			break;
		}

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

	case WM_RBUTTONDOWN:
		{

		POINT	pt;
		pt.x = LOWORD( lParam );
		pt.y = HIWORD( lParam );
		HMENU	h = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_MAINMENU ) );
		HMENU	hs = GetSubMenu( h, 0 );
		//CheckMenuItem( h, IDM_TOPMOST,
		//		MF_BYCOMMAND | ( gfTopMost ? MF_CHECKED : MF_UNCHECKED )  );
		ClientToScreen( hWnd, &pt );
		TrackPopupMenu( hs, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu( h );

		}	// WM_RBUTTONDOWN
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

			MessageBox( NULL, "IDM_OPEN", AppName, MB_OK );

			}// end IDM_OPEN
			break;

		default:
			{

			DestroyWindow( hwndMain );

			}

		}// switch

		}// end WM_COMMAND
		break;

	default:
		return CallWindowProc( OldWndProc, hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;


}



// end of this file : memo1.cpp
