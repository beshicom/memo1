


//
//	memo1.h : １行メモ
//
//	since 2023/10/16 by beshi
//



#include	<windows.h>
#include	<shlobj.h>
#include	<direct.h>
#include	<time.h>

#include	"memo1.h"

#define	WM_RETURN	WM_USER+0x100



HINSTANCE	hInstance;
HWND		hwndMain;
HWND		hwndDsp;

int		fDspWndQuit = 0;

char	ClassName[] = "Memo1MainClass";
char	DspWndCN[] = "Memo1DisplayClass";
char	ApplicationName[] = "Memo1";
char	AppName[20];

int		MainX = 100;
int		MainY = 100;
int		MainWidth = 1000;
int		MainHeight = 80;

char	EditClass[] = "EDIT";
HWND	hwndEdit;
WNDPROC	OldEditWndProc;
WNDPROC	OldDEditWndProc;

#define	MEMOSIZE	1024
#define	BUFFERSIZE	MEMOSIZE+100
char	buffer[ BUFFERSIZE + 1 ];

// 過去メモの表示
const DWORD	nMaxDsp = 50;	// 最大表示行数
const DWORD	nMaxRead = nMaxDsp * 2;	// 最大読み込み行数

int		DspX = 100;
int		DspY = 100;
int		DspWidth = 1000;
int		DspHeight = 600;

DWORD	nCrntDsp = 0;

//HWND	hwndStatic[ nMaxDsp ];
//HWND	hwndDEdit[ nMaxDsp ];

struct MemoInfo {
	MemoInfo *	pPrev = NULL;
	MemoInfo *	pNext = NULL;
	char *		pText = NULL;
	char *		pDateTime = NULL;
	char *		pMemo = NULL;
	DWORD		nLine = 9999;
	int			fModified = 0;
	int			fError = 0;
	MemoInfo(){}
	MemoInfo( char *pDT, char *pM ){
		int		nDT = 0;
		if( pDT != NULL ){
			nDT = lstrlen( pDT );
			pDateTime = new char[ nDT+10 ];
			if( pDateTime == NULL )	fError = 1;
			else	lstrcpy( pDateTime, pDT );
		}
		int		nM = 0;
		if( pM != NULL ){
			nM = lstrlen( pM );
			pMemo = new char[ nM+10 ];
			if( pMemo == NULL )		fError =1;
			else	lstrcpy( pMemo, pM );
		}
		pText = new char[ nDT+nM+2+10 ];
		if( pText == NULL )		fError = 1;
		else	wsprintf( pText, "%s  %s", pDT, pM );
		if( fError ){
			delete[] pText;		pText = NULL;
			delete[] pDateTime;	pDateTime = NULL;
			delete[] pMemo;		pMemo = NULL;
		}
	}
	MemoInfo( int nDT, int nM ){
		pDateTime = new char[ nDT+10 ];
		if( pDateTime == NULL )	fError = 1;
		pMemo = new char[ nM+10 ];
		if( pMemo == NULL )		fError = 1;
		pText = new char[ nDT+nM+2+10 ];
		if( pText == NULL )		fError = 1;
		if( fError ){
			delete[] pText;		pText = NULL;
			delete[] pDateTime;	pDateTime = NULL;
			delete[] pMemo;		pMemo = NULL;
		}
	}
	~MemoInfo(){
		delete[] pText;		pText = NULL;
		delete[] pDateTime;	pDateTime = NULL;
		delete[] pMemo;		pMemo = NULL;
	}
};

MemoInfo *	pTopMemoData = NULL;
MemoInfo *	pLastMemoData = NULL;

struct DspInfo {
	HWND	hwndStatic;		// メモの表示
	HWND	hwndDEdit;		// メモの編集
	HWND	hwndSLNum;		// 行番号の表示
	HWND	hwndSDT;		// 日付時刻の表示
	MemoInfo *	pMemo;		// メモの情報
};
DspInfo		DspData[ nMaxDsp ];

int		fDspEdit = 0;		// 過去メモ編集中
int		nCrntDEdit = 0;		// 表示中の過去メモエディット

HBRUSH	hDEditBrush;



// 設定ファイル
struct ConfigInfo {
	char	Magic[5];			// "MEMO1"
	// メインウインドウ
	int		MainX;
	int		MainY;
	int		MainWidth;
	int		MainHeight;
	// 過去メモ表示ウインドウ
	int		DspX;
	int		DspY;
	int		DspWidth;
	int		DspHeight;
};



int GetCompileTime ( char * pStr )
{
							// 00000000001
							// 01234567890
	char *	dt = __DATE__;	// Oct 23 2023
	char *	tm = __TIME__;	// 04:54:31
	char	dt4, dt5;
	dt4 = dt[4];
	dt5 = dt[5];
	if( dt5 == ' ' ){ dt5 = dt4; dt4 = '0'; }
	int		d = (dt4-'0')*10 + (dt5-'0');
	int		t = d*24 + (tm[0]-'0')*10 + (tm[1]-'0');
	t = t*60 + (tm[3]-'0')*10 + (tm[4]-'0');
	pStr[0] = dt4;
	pStr[1] = dt5;
	pStr[2] = tm[0];
	pStr[3] = tm[1];
	pStr[4] = tm[3];
	pStr[5] = tm[4];
	pStr[6] = 0;
	return t;
}



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



//
// 設定データファイルのフルパスを得る BCCGetConfigPath()		//TAG_JUMP_MARK
//
BOOL  BCCGetConfigPath(  char *pBuf  )
{
	return(  BCCGetDataPath0( pBuf, "MEMO1CONFIG.DAT" )  );
}



// 指定行数読み込む ReadBufferLine()							//TAG_JUMP_MARK
//	読み込みバッファから読み込んだ行数を返す(最大nMaxRead)
//	pBufferEndはバッファの最終アドレスを保持する変数へのポインタ
//	次に読み込むアドレスを*pBufferEndに格納して返す。
DWORD ReadBufferLine( char * buffer, char ** pBufferEnd, int nLine )
{

	if( pBufferEnd == NULL )	return 0;
	if( buffer == NULL ){  *pBufferEnd = NULL;  return 0;  }

	char *	p = buffer;
	int		nL = 0;
	int		nBL = 0;
	if( pLastMemoData != NULL )
		nBL = pLastMemoData->nLine;
	while( p < *pBufferEnd-10 ){
		if( *p == 0 )	break;
		int		n = MoveToCRLF(p) - p;
		if( n <= 0 )	break;
		MemoInfo *	mi = new MemoInfo( 30, n );
		if( mi == NULL )	break;
		if( mi->fError ){  delete mi;  break;  }
		mi->nLine = nL + nBL;
		CopyMemory( mi->pText, p, n );
		*(mi->pText+n) = 0;
		//MessageBox( NULL, mi->pText, AppName, MB_OK );
		//000000000011111111112
		//012345678901234567890
		//2023/10/21 11:02:02  
		CopyMemory( mi->pDateTime, mi->pText, 19 );
		*(mi->pDateTime+19) = 0;
		lstrcpy( mi->pMemo, mi->pText+21 );
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
		if( ++nL >= nLine )		break;
	}// while p

	*pBufferEnd = p;

	return nL;

}
//DWORD ReadBufferLine( char * buffer, char ** pBufferEnd, int nLine )



// 指定行数を保存する SaveLines()								//TAG_JUMP_MARK
//	nFile == 0 : 普段の保存用ファイル(MEMO1.TXT)に保存
//			 not0 : 分割保存ファイル(MEMO1TXTnnnnnnnn.TXT)に保存
//	nMode == 0 : ファイルの最後に追加保存する
//			 not0 : ファイルに上書き保存する
DWORD SaveLines( int nFile, int nMode, MemoInfo * pMemo, int nLine )
{

	if( pMemo == NULL )		return 0;

	char	path[ MAX_PATH ];

	if( nFile == 0 )	BCCGetDataPath( path );
	else{
		char	fn[ MAX_PATH ];
		wsprintf( fn, "MEMO1TXT%d.TXT", time(NULL) );
		Sleep( 1000 );
		BCCGetDataPath0( path, fn );
	}

	HANDLE	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE,
			NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )		return 0;

	if( nMode == 0 ){
		DWORD	r = SetFilePointer( hFile, 0, NULL, FILE_END );
		if( r == INVALID_SET_FILE_POINTER ){  CloseHandle(hFile);  return 0;  }
	}

	int			nL = 0;
	MemoInfo *	pM = pMemo;
	for( ; nL<nLine; ++nL ){
		wsprintf( buffer, "%s\n", pM->pText );
		int		n = lstrlen( buffer );
		DWORD	bytesWrite;
		BOOL	fOK = WriteFile( hFile, buffer, n, &bytesWrite, NULL );
		if( ! fOK )		break;
		if( bytesWrite != n )	break;
		pM = pM->pNext;
		if( pM == NULL )	break;
	}
	CloseHandle( hFile );

	return nL;

}
//DOWRD SaveLines( int nFile, int nMode, MemoInfo * pMemo, int nLine )



LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK DspWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK EditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK DEditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );



int WinMain( HINSTANCE hInst, HINSTANCE hPrevInst,
												char * CmdLine, int CmdShow )
{

	hInstance = hInst;

	{
	char	buf[100];
	GetCompileTime( buf );
	wsprintf( AppName, "%s  %s", ApplicationName, buf );
	}

	hDEditBrush = CreateSolidBrush( RGB( 255, 200, 200 ) );

	// メインウインドウクラスの登録
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
	if( RegisterClassEx( &wc ) == 0 ){
		MessageBox( NULL, "Error : register ClassName", AppName,
													MB_OK | MB_ICONERROR );
		return 0;
	}
	}// end

	// 過去メモ表示ウインドウクラスの登録
	{
	WNDCLASSEX	wc;
	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DspWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	//wc.lpszMenuName = MAKEINTRESOURCE( IDR_MAINMENU );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = DspWndCN;
	wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDR_ICON) );
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	if( RegisterClassEx( &wc ) == 0 ){
		MessageBox( NULL, "Error : register DspWndCN", AppName,
													MB_OK | MB_ICONERROR );
		return 0;
	}
	}// end


	// 設定データ読み込み
	while(TRUE){
	ConfigInfo	cd;
	char	path[ MAX_PATH ];
	BCCGetConfigPath( path );
	HANDLE	hFile = CreateFile( path, GENERIC_READ,
		NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )	break;
	DWORD	bytesRead;
	/* BOOL fOK = */
	ReadFile( hFile, &cd, sizeof(ConfigInfo), &bytesRead, NULL );
	CloseHandle( hFile );
	if(
		( cd.Magic[0] != 'M' )||
		( cd.Magic[1] != 'E' )||
		( cd.Magic[2] != 'M' )||
		( cd.Magic[3] != 'O' )||
		( cd.Magic[4] != '1' )
	){
		break;
	}
	MainX = cd.MainX;
	MainY = cd.MainY;
	MainWidth = cd.MainWidth;
	MainHeight = cd.MainHeight;
	DspX = cd.DspX;
	DspY = cd.DspY;
	DspWidth = cd.DspWidth;
	DspHeight = cd.DspHeight;
	break;
	}// whilr

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
	char *	pE = pb + bytesRead - 10;	// パッファの終わり
	char *	pp = pE;
	DWORD	nLine = ReadBufferLine( p, &pp, nMaxRead );
	p = pp;
	pp = pE;
	int		fError = 0;
	if( nLine >= nMaxRead ){
		while(TRUE){
		if( ! fError ){
			// 半分を別ファイルに保存する
			DWORD	nS = SaveLines( 1, 1, pTopMemoData, nMaxRead/2 );
			if( nS != nMaxRead/2 )	fError = 1;
		}
		if( ! fError ){
			// 先頭から削除
			for( int i=0; i<nMaxDsp/2; ++i ){
				MemoInfo *	pm = pTopMemoData->pNext;
				delete	pTopMemoData;
				pTopMemoData = pm;
				if( pTopMemoData == NULL )		break;
				pTopMemoData->pPrev = NULL;
			}//for
		}// if
		// 読み込み
		nLine = ReadBufferLine( p, &pp, nMaxRead/2 );
		if( nLine < nMaxRead/2 )	break;
		p = pp;
		pp = pE;
		}//while TRUE
	}// if

	delete[] pb;

	MemoInfo *	pM = pTopMemoData;
	for( int i=0; i<nMaxRead; ++i ){
		if( pM == NULL )		break;
		pM->nLine = i;
		pM = pM->pNext;
	}

	break;

	}// while hFile

	}// end


	// ウインドウの作成
	{
	char * pt = AppName;
	if( pLastMemoData != NULL ){
		wsprintf( buffer, "%s    RETURN for SAVE  R-BUTTON for MENU      "
									"%-100s", AppName, pLastMemoData->pText );
		pt = buffer;
	}
	hwndMain = CreateWindowEx( WS_EX_CLIENTEDGE, ClassName, pt,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
		MainX, MainY, MainWidth, MainHeight,
		NULL, NULL, hInst, NULL );
	//ShowWindow( hwnd, CmdShow );
	//UpdateWindow( hwnd );
	}


	// メッセージループ
	MSG	msg;
	while( GetMessage( &msg, NULL, 0, 0 ) ){
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}


	// 設定データ保存
	while(TRUE){

	ConfigInfo	cd;
	cd.Magic[0] = 'M';
	cd.Magic[1] = 'E';
	cd.Magic[2] = 'M';
	cd.Magic[3] = 'O';
	cd.Magic[4] = '1';
	cd.MainX = MainX;
	cd.MainY = MainY;
	cd.MainWidth = MainWidth;
	cd.MainHeight = MainHeight;
	cd.DspX = DspX;
	cd.DspY = DspY;
	cd.DspWidth = DspWidth;
	cd.DspHeight = DspHeight;

	char	path[ MAX_PATH ];
	BCCGetConfigPath( path );
	HANDLE	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE,
		NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )	break;
	DWORD	bytesWrite;
	/* BOOL fOK = */
	WriteFile( hFile, &cd, sizeof(ConfigInfo), &bytesWrite, NULL );
	CloseHandle( hFile );

	break;

	}// while


	DeleteObject( hDEditBrush );

	MemoInfo *	p = pTopMemoData;
	while( p != NULL ){
		MemoInfo *	mp = p->pNext;
		delete		p;
		p = mp;
	}


	return msg.wParam;

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
		OldEditWndProc = (WNDPROC)SetWindowLong(
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

		RECT	r;
		GetWindowRect( hWnd, &r );
		MainX = r.left;
		MainY = r.top;
		MainWidth = r.right - r.left;
		MainHeight = r.bottom - r.top;

		}// WM_SIZE
		break;

	case WM_MOVE:
		{

		RECT	r;
		GetWindowRect( hWnd, &r );
		MainX = r.left;
		MainY = r.top;
		MainWidth = r.right - r.left;
		MainHeight = r.bottom - r.top;

		}// WM_MOVE
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

		case IDM_EXIT:
			{

			DestroyWindow( hWnd );

			}// IDM_EXIT
			break;

		default:
			break;

		}// switch

		}// end WM_COMMAND
		break;

	case WM_RETURN:
		{

		SYSTEMTIME	dt;
		GetLocalTime( &dt );
		char	buf[ 30 ];
		wsprintf(  buf,  "%04d/%02d/%02d %02d:%02d:%02d",
			dt.wYear, dt.wMonth, dt.wDay,  dt.wHour, dt.wMinute, dt.wSecond );

		if(  GetWindowText( hwndEdit, buffer, MEMOSIZE ) == 0  ){
			PostQuitMessage( NULL );
			break;
		}

		MemoInfo *	pMemo = new MemoInfo( buf, buffer );
		if( pMemo == NULL )		break;
		if( pMemo->fError ){  delete pMemo;  break;  }

		//MessageBox( NULL, buffer, AppName, MB_OK );

		SaveLines( 0, 0, pMemo, 1 );

		delete pMemo;

		PostQuitMessage( NULL );

		}// WM_RETURN
		break;

	case WM_DESTROY:
		{
		PostQuitMessage( NULL );
		}// end
		break;

	case WM_QUIT:
		{
		fDspWndQuit = 1;
		DestroyWindow( hwndDsp );
		}
		break;

	default:

		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;

}
// LRESULT CALLBACK WndProc(
//						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )



LRESULT CALLBACK DspWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_CREATE:
		{

		int		x = 0;
		int		y = 0;
		MemoInfo *	pM = pLastMemoData;
		for( int i=0; i < 25; ++i ){
			DspData[i].hwndSLNum = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, 20*i, 40, 20, hWnd, (HMENU)(i+nMaxDsp*2), hInstance, NULL );
			DspData[i].hwndSDT = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_RIGHT,
				40, 20*i, 170, 20, hWnd, (HMENU)(i+nMaxDsp), hInstance, NULL );
			DspData[i].hwndStatic = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_NOTIFY,
				210, 20*i, 1000, 20, hWnd, (HMENU)i, hInstance, NULL );
			DspData[i].hwndDEdit = CreateWindowEx( NULL, EditClass, NULL,
				WS_CHILD | ES_AUTOHSCROLL ,
				210, 20*i, 770, 20, hWnd, (HMENU)i, hInstance, NULL );
			if( pM == NULL )	continue;
			DspData[i].pMemo = pM;
			char	buf[100];
			wsprintf( buf, "%d  ", i+1 );
			SetWindowText( DspData[i].hwndSLNum, buf );
			wsprintf( buf, "%s   ", pM->pDateTime );
			SetWindowText( DspData[i].hwndSDT, buf );
			SetWindowText( DspData[i].hwndStatic, pM->pMemo );
			SetWindowText( DspData[i].hwndDEdit, pM->pMemo );
			// サブクラス化
			OldDEditWndProc = (WNDPROC)SetWindowLong(
					DspData[i].hwndDEdit, GWL_WNDPROC, (DWORD)DEditWndProc );
			pM = pM->pPrev;
		}// for

		}// WM_CREATE
		break;

	case WM_CTLCOLOREDIT:
		{

		SetTextColor( (HDC)wParam, RGB(255,255,0) );
		SetBkColor( (HDC)wParam, RGB(0,0,0) );
		return (LRESULT)GetStockObject( BLACK_BRUSH );

		}// WM_CTLCOLOREDIT
		break;

	case WM_CTLCOLORSTATIC:
		{

		int		nID = GetWindowLong( (HWND)lParam, GWL_ID );
		HBRUSH	hCrntBrush = (HBRUSH)SelectObject( (HDC)wParam, hDEditBrush );
		SelectObject( (HDC)wParam, hCrntBrush );
		HBRUSH	hBrush = hCrntBrush;

		if( nID >= nMaxDsp*2 ){
			SetTextColor( (HDC)wParam, RGB( 0, 0, 255 ) );
		}
		else if( nID >= nMaxDsp ){
			SetTextColor( (HDC)wParam, RGB( 116, 80, 48 ) );
		}

		if( nID % 2 ){
			SetBkColor( (HDC)wParam, RGB( 255, 200, 200 ) );
			hBrush = hDEditBrush;
		}

		return (LRESULT)hBrush;

		}// WM_CTLCOLORSTATIC
		break;

	case WM_SIZE:
		{

		//MoveWindow( hwndEditD, 0, 0, lParam&0xffff, lParam>>16, TRUE );

		RECT	r;
		GetWindowRect( hWnd, &r );
		DspX = r.left;
		DspY = r.top;
		DspWidth = r.right - r.left;
		DspHeight = r.bottom - r.top;

		}// WM_SIZE
		break;

	case WM_MOVE:
		{

		RECT	r;
		GetWindowRect( hWnd, &r );
		DspX = r.left;
		DspY = r.top;
		DspWidth = r.right - r.left;
		DspHeight = r.bottom - r.top;

		}// WM_MOVE
		break;

	case WM_RBUTTONDOWN:
		{

		POINT	pt;
		pt.x = LOWORD( lParam );
		pt.y = HIWORD( lParam );
		HMENU	h = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_DSPMENU ) );
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
			switch( HIWORD(wParam) ){
			case STN_CLICKED:
				{
				if( fDspEdit )		break;
				fDspEdit = 1;
				nCrntDEdit = LOWORD(wParam);
				ShowWindow( DspData[nCrntDEdit].hwndDEdit, SW_SHOW );
				SetFocus( DspData[nCrntDEdit].hwndDEdit );
				}// STN_CLICKED
				break;
			}// switch
			break;
		}

		if( HIWORD( wParam ) != 0 ){
			// HIWORD( wParam ) : アクセラレータのメッセージ
			break;
		}

		switch( LOWORD( wParam ) ){

		case IDM_OPEND:
			{

			MessageBox( NULL, "IDM_OPEND", AppName, MB_OK );

			}// end IDM_OPEND
			break;

		case IDM_EXITD:
			{

			DestroyWindow( hWnd );

			}// IDM_EXITD
			break;

		default:
			break;

		}// switch

		}// end WM_COMMAND
		break;

	case WM_RETURN:
		{

		DspInfo *	pdd = &DspData[nCrntDEdit];
		MemoInfo *	pM = pdd->pMemo;

		fDspEdit = 0;
		ShowWindow( pdd->hwndDEdit, SW_HIDE );

		// 変更された?
		GetWindowText( pdd->hwndDEdit, buffer, MEMOSIZE );
		if( lstrcmp( pM->pMemo, buffer ) == 0 )		break;
		int	n = lstrlen( buffer );

		delete[] pM->pMemo;
		pM->pMemo = new char[ n+10 ];
		if( pM->pMemo == NULL )		break;
		GetWindowText( pdd->hwndDEdit, pM->pMemo, n+10 );

		delete[] pM->pText;
		pM->pText = new char[ n+30+10 ];
		if( pM->pText == NULL )		break;
		wsprintf( pM->pText, "%s  %s", pM->pDateTime, pM->pMemo );
		SetWindowText( pdd->hwndStatic, pM->pText );

		SaveLines( 0, 1, pTopMemoData, nMaxDsp*2 );

		}// WM_RETURN
		break;

	case WM_DESTROY:
		{
		fDspEdit = 0;
		DestroyWindow( hWnd );
		}
		break;

	default:

		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;

}
//LRESULT CALLBACK DspWndProc(
//						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )



LRESULT CALLBACK EditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_KEYDOWN:
		{

		if( (char)wParam == VK_RETURN )
			SendMessage( hwndMain, WM_RETURN, 0, 0 );

		return CallWindowProc( OldEditWndProc, hWnd, uMsg, wParam, lParam );

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

		}// WM_RBUTTONDOWN
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

		case IDM_DISP:
			{

			// 過去メモ表示ウインドウの作成
			hwndDsp = CreateWindowEx( WS_EX_CLIENTEDGE, DspWndCN, AppName,
				WS_POPUP | WS_CAPTION | WS_SYSMENU |
									WS_HSCROLL | WS_VSCROLL | WS_VISIBLE ,
				DspX, DspY, DspWidth, DspHeight,
				NULL, NULL, hInstance, NULL );

			/*
			// メッセージループ
			MSG	msg;
			while( GetMessage( &msg, NULL, 0, 0 ) ){
				TranslateMessage( &msg );
				DispatchMessage( &msg );
				if( fDspWndQuit ){
					PostQuitMessage( 0 );
					break;
				}
			}// while
			*/

			}// end IDM_DISP
			break;

		case IDM_SAVE:
			{

			// 入力された最新の1行を保存
			SendMessage( hwndMain, WM_RETURN, 0, 0 );

			}// IDM_SAVE
			break;

		default:
			{

			DestroyWindow( hwndMain );

			}

		}// switch

		}// end WM_COMMAND
		break;

	default:
		return CallWindowProc( OldEditWndProc, hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;


}
//LRESULT CALLBACK EditWndProc(
//						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )



LRESULT CALLBACK DEditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_KEYDOWN:
		{

		if( (char)wParam == VK_RETURN )
			SendMessage( hwndDsp, WM_RETURN, 0, 0 );

		return CallWindowProc( OldDEditWndProc, hWnd, uMsg, wParam, lParam );

		}// WM_KEYDOWN
		break;

	case WM_RBUTTONDOWN:
		{

		POINT	pt;
		pt.x = LOWORD( lParam );
		pt.y = HIWORD( lParam );
		HMENU	h = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_DSPMENU ) );
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

		case IDM_SAVED:
			{

			// 修正された行を保存
			SendMessage( hwndDsp, WM_RETURN, 0, 0 );

			}// IDM_SAVED
			break;

		case IDM_EXITD:
			{

			DestroyWindow( hWnd );

			}// IDM_EXIT
			break;

		default:
			{

			DestroyWindow( hWnd );

			}

		}// switch

		}// end WM_COMMAND
		break;

	default:
		return CallWindowProc( OldDEditWndProc, hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;


}
//LRESULT CALLBACK DEditWndProc(
//						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )



// end of this file : memo1.cpp
