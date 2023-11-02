


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
	// エディトコントロール内でリターンキーが押された、もしくはその同等状態
#define	WM_SAVE		WM_USER+0x101
	// 保存処理のみを行う
#define	WM_EDITEND	WM_USER+0x102
	// エディット窓終了依頼
#define	WM_DSPEND	WM_USER+0x103
	// 過去メモ表示窓終了依頼



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

int		LineHeight = 20;	// 1行の高さ GetTextMetrics()

DWORD	nReadLine = 0;	// 読み込んでいる行数
DWORD	nCrntDsp = 0;

//HWND	hwndStatic[ nMaxDsp ];
//HWND	hwndDEdit[ nMaxDsp ];

// 読み込んだ各行についての情報
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
MemoInfo *	pDspTopMemoData = NULL;		// 表示行の先頭
MemoInfo *	pLastMemoData = NULL;

// 表示する各行についてのコメント
struct DspInfo {
	HWND	hwndStatic;		// メモの表示
	HWND	hwndDEdit;		// メモの編集
	HWND	hwndSLNum;		// 行番号の表示
	HWND	hwndSDT;		// 日付時刻の表示
	MemoInfo *	pMemo;		// 表示するメモの情報
};
DspInfo		DspData[ nMaxDsp ];

int		fDspEdit = 0;		// 過去メモ編集中
DWORD	nCrntDEdit = 0;		// 表示中の過去メモエディット

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
			nLine -= nMaxRead/2;
		}
		if( ! fError ){
			// 先頭から削除
			for( int i=0; i<nMaxRead/2; ++i ){
				MemoInfo *	pm = pTopMemoData->pNext;
				delete	pTopMemoData;
				pTopMemoData = pm;
				if( pTopMemoData == NULL )		break;
				pTopMemoData->pPrev = NULL;
			}//for
		}// if
		// 読み込み
		int	nL = ReadBufferLine( p, &pp, nMaxRead/2 );
		nLine += nL;
		if( nL < nMaxRead/2 )	break;
		p = pp;
		pp = pE;
		}//while TRUE
	}// if
	nReadLine = nLine;

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
		wsprintf( buffer, "%s  %d/%d    RETURN for SAVE  R-BUTTON for MENU"
																"      %-100s",
						AppName, nReadLine, nMaxDsp, pLastMemoData->pText );
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

/*

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

		case IDM_TEST:
			{
			}// IDM_TEST
			break;

		default:
			break;

		}// switch

		}// end WM_COMMAND
		break;

*/

	case WM_SAVE:
		{

		if(  GetWindowText( hwndEdit, buffer, MEMOSIZE ) == 0  ){
			break;
		}

		SYSTEMTIME	dt;
		GetLocalTime( &dt );
		char	buf[ 30 ];
		wsprintf(  buf,  "%04d/%02d/%02d %02d:%02d:%02d",
			dt.wYear, dt.wMonth, dt.wDay,  dt.wHour, dt.wMinute, dt.wSecond );

		MemoInfo *	pMemo = new MemoInfo( buf, buffer );
		if( pMemo == NULL )		break;
		if( pMemo->fError ){  delete pMemo;  break;  }

		SaveLines( 0, 0, pMemo, 1 );

		delete pMemo;

		SetWindowText( hwndEdit, "" );

		}// WM_SAVE
		break;

	case WM_RETURN:
		{

		SendMessage( hWnd, WM_SAVE, 0, 0 );

		PostQuitMessage( NULL );

		}// WM_RETURN
		break;

	case WM_DSPEND:
		{

		// 過去メモ表示窓終了処理
		if( hwndDsp != NULL )	DestroyWindow( hwndDsp );
		hwndDsp = NULL;

		}// WM_DSPEND
		break;

	case WM_CLOSE:
		{

		fDspWndQuit = 1;

		if( hwndDsp != NULL )	SendMessage( hwndDsp, WM_CLOSE, 0, 0 );
		if( hwndDsp != NULL )	break;	// 拒否された

		SendMessage( hWnd, WM_SAVE, 0, 0 );

		DestroyWindow( hWnd );

		}
		break;

	case WM_DESTROY:
		{

		// 破棄を拒否できない

		if( hwndDsp != NULL )		SendMessage( hWnd, WM_SAVE, 0, 0 );
		SendMessage( hWnd, WM_SAVE, 0, 0 );

		PostQuitMessage( NULL );	// メッセージループを抜ける

		}// end
		break;

	default:

		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;

}
// LRESULT CALLBACK WndProc(
//						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )



// 行にテキストをセットする SetTextLine()						//TAG_JUMP_MARK
//	nl : 行番号(0～)
void SetTextLine( int nl, DspInfo * pdd, MemoInfo * pmd )
{

	if( nl < 0 )				return;
	if( pdd == NULL )			return;
	if( pmd == NULL )			return;
	if( pmd->pMemo == NULL )	return;

	pdd->pMemo = pmd;

	char	buf[100];
	wsprintf( buf, "%d  ", nl+1 );
	SetWindowText( pdd->hwndSLNum, buf );

	wsprintf( buf, "%s   ", pmd->pDateTime );
	SetWindowText( pdd->hwndSDT, buf );

	SetWindowText( pdd->hwndStatic, pmd->pMemo );
	SetWindowText( pdd->hwndDEdit, pmd->pMemo );

}
// void SetTextLine( DspInfo * pdd, MemoInfo * pmd )



LRESULT CALLBACK DspWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

	static SCROLLINFO	ssi;
	static int			snMaxDisp;	// 最大表示可能行数
	static int			snPos;		// スクロール位置、１行目に表示する行の番号
	static int			snMaxPos;	// snPosの最大値+1


	switch( uMsg ){

	case WM_CREATE:
		{

		// クライアント領域の高さを行の高さの倍数にする。
		RECT	r;
		GetClientRect( hWnd, &r );	// 左上座標、幅、高さ
		snMaxDisp = r.bottom / LineHeight;
		snMaxPos = nReadLine - snMaxDisp;	// nPosの最大値+1
		snPos = 0;
		int		def = r.bottom - snMaxDisp * LineHeight;

		GetWindowRect( hWnd, &r );	// 左上座標、右下座標+1
		MoveWindow( hWnd,
			r.left, r.top, r.right - r.left, r.bottom - r.top - def, TRUE );
						// 左上座標、幅、高さ

		ssi.cbSize = sizeof(SCROLLINFO);
		ssi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE ;
		ssi.nPos = snPos;
		ssi.nMin = 0;
		ssi.nMax = nReadLine;
		ssi.nPage = snMaxDisp;
		SetScrollInfo( hWnd, SB_VERT, &ssi, TRUE );

		int		x = 0;
		int		y = 0;
		MemoInfo *	pM = pLastMemoData;
		pDspTopMemoData = pM;
		for( int i=0; i < snMaxDisp; ++i ){
			DspData[i].hwndSLNum = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, LineHeight*i, 40, LineHeight,
				hWnd, (HMENU)(i+nMaxDsp*2), hInstance, NULL );
			DspData[i].hwndSDT = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_RIGHT,
				40, LineHeight*i, 170, LineHeight,
				hWnd, (HMENU)(i+nMaxDsp), hInstance, NULL );
			DspData[i].hwndStatic = CreateWindowEx( NULL, "STATIC", NULL,
				WS_CHILD | WS_VISIBLE | SS_NOTIFY,
				210, LineHeight*i, 1000, LineHeight,
				hWnd, (HMENU)i, hInstance, NULL );
			DspData[i].hwndDEdit = CreateWindowEx( NULL, EditClass, NULL,
				WS_CHILD | ES_AUTOHSCROLL ,
				210, LineHeight*i, 770, LineHeight,
				hWnd, (HMENU)i, hInstance, NULL );
			if( pM == NULL )	continue;

			SetTextLine( i, &DspData[i], pM );	// 行にテキストをセット

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

		// クライアント領域の高さを行の高さの倍数にする。
		RECT	r;
		GetClientRect( hWnd, &r );
		snMaxDisp = r.bottom / LineHeight;
		snMaxPos = nReadLine - snMaxDisp;	// nPosの最大値+1
		int		def = r.bottom - snMaxDisp * LineHeight;

		GetWindowRect( hWnd, &r );	// 左上座標、右下座標+1
		MoveWindow( hWnd,
			r.left, r.top, r.right - r.left, r.bottom - r.top  - def, TRUE );
						// 左上座標、幅、高さ

		ssi.cbSize = sizeof(SCROLLINFO);
		ssi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE ;
		ssi.nPos = snPos;
		ssi.nMin = 0;
		ssi.nMax = nReadLine;
		ssi.nPage = snMaxDisp;
		SetScrollInfo( hWnd, SB_VERT, &ssi, TRUE );
		UpdateWindow( hWnd );

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

	case WM_VSCROLL:
		{

		int		dy;

		switch( LOWORD(wParam) ){

		case SB_LINEUP:		dy = -1;						break;
		case SB_LINEDOWN:	dy = 1;							break;
		case SB_PAGEUP:		dy = -1 * ssi.nPage;			break;
		case SB_PAGEDOWN:	dy = 1 * ssi.nPage;				break;
		case SB_TOP:		dy = -snPos;					break;
		case SB_BOTTOM:		dy = nReadLine - snPos;			break;
		case SB_THUMBTRACK:	dy = HIWORD(wParam) - ssi.nPos;	break;
		default:			dy = 0;							break;

		}// switch

		if( dy == 0 )	return 0;

		int	n = snPos + dy;
		if( n < 0 )				n = 0;
		if( n > snMaxPos )		n = snMaxPos;
		dy = n - snPos;

		//printf( "nPos = %d  n = %d  dy = %d  nMaxLine = %d\n",
		//											nPos, n, dy, nMaxLine );

		if( dy == 0 )	return 0;
		if( n == snPos )	return 0;

		//printf( "nPos = %d  n = %d  dy = %d  nMaxLine = %d\n",
		//											nPos, n, dy, nMaxLine );

		ssi.nPos = n;
		snPos = n;
		SetScrollInfo( hWnd, SB_VERT, &ssi, TRUE );

		MemoInfo *pM = pLastMemoData;
		for( int i=0; i<nReadLine; ++i ){
			if( i == snPos )	break;
			pM = pM->pPrev;
		}

		for( int i=0; i < snMaxDisp; ++i ){
			if( pM == NULL )	break;
			SetTextLine( i+snPos, &DspData[i], pM );
			pM = pM->pPrev;
		}// for

		}// WM_VSCROLL
		return 0;

	case WM_COMMAND:
		{

		if( lParam != 0 ){
			// lParam : コントロールウインドウのハンドル
			//	HIWORD(wParam) : 通知コード
			//	LOWORD(wParam) : コントロールID
			switch( HIWORD(wParam) ){
			case STN_CLICKED:
				{
				if( fDspEdit ){
					// すでにどこかが編集中
					break;
				}
				nCrntDEdit = LOWORD(wParam);
				if( nCrntDEdit >= nMaxDsp ){
					// 変数値が異常
					break;
				}
				if( DspData[nCrntDEdit].pMemo == NULL ){
					// 未使用欄、もしくは状態異常欄
					break;
				}
				fDspEdit = 1;
				ShowWindow( DspData[nCrntDEdit].hwndDEdit, SW_SHOW );
				ShowWindow( DspData[nCrntDEdit].hwndStatic, SW_HIDE );
				SetFocus( DspData[nCrntDEdit].hwndDEdit );
				}// STN_CLICKED
				break;
			}// switch
			break;
		}

/*

		if( HIWORD( wParam ) != 0 ){
			// HIWORD( wParam ) : アクセラレータのメッセージ
			break;
		}

		switch( LOWORD( wParam ) ){

		case IDM_TESTD:
			{
			}// IDM_TESTD
			break;

		default:
			break;

		}// switch

*/

		}// end WM_COMMAND
		break;

	case WM_EDITEND:
		{

		fDspEdit = 0;
		ShowWindow( DspData[nCrntDEdit].hwndDEdit, SW_HIDE );
		ShowWindow( DspData[nCrntDEdit].hwndStatic, SW_SHOW );

		}// WM_ENDEDIT
		break;

	case WM_SAVE:
		{

		DspInfo *	pdd = &DspData[nCrntDEdit];
		MemoInfo *	pM = pdd->pMemo;

		fDspEdit = 0;
		ShowWindow( pdd->hwndDEdit, SW_HIDE );
		ShowWindow( pdd->hwndStatic, SW_SHOW );

		// 変更された?
		GetWindowText( pdd->hwndDEdit, buffer, MEMOSIZE );
		if( lstrcmp( pM->pMemo, buffer ) == 0 ){
			break;
		}
		int	n = lstrlen( buffer );

		delete[] pM->pMemo;
		pM->pMemo = new char[ n+10 ];
		if( pM->pMemo == NULL ){
			break;
		}
		GetWindowText( pdd->hwndDEdit, pM->pMemo, n+10 );

		delete[] pM->pText;
		pM->pText = new char[ n+30+10 ];
		if( pM->pText == NULL ){
			break;
		}
		wsprintf( pM->pText, "%s  %s", pM->pDateTime, pM->pMemo );
		SetWindowText( pdd->hwndStatic, pM->pMemo );

		SaveLines( 0, 1, pTopMemoData, nMaxDsp*2 );

		}// WM_SAVE
		break;

	case WM_RETURN:
		{

		int		d = MessageBox( hwndDsp,
					"Save ?", AppName, MB_YESNOCANCEL | MB_ICONQUESTION );
		if( d == IDCANCEL )		break;
		if( d == IDNO ){
			SendMessage( hwndDsp, WM_EDITEND, 0, 0 );
			break;
		}

		SendMessage( hWnd, WM_SAVE, 0, 0 );

		SendMessage( hwndDsp, WM_EDITEND, 0, 0 );

		}// WM_RETURN
		break;

	case WM_CLOSE:
		{
		if( fDspEdit ){
			SendMessage( hWnd, WM_RETURN, 0, 0 );
			if( fDspEdit )	break;	// 破棄拒否
			//DestroyWindow( hWnd );
			SendMessage( hwndMain, WM_DSPEND, 0, 0 );
		}
		else{
			//DestroyWindow( hWnd );
			SendMessage( hwndMain, WM_DSPEND, 0, 0 );
		}
		}// WM_DESTROY
		break;

	case WM_DESTROY:
		{
		if( fDspEdit ){
			// 破棄を拒否できない
			SendMessage( hWnd, WM_RETURN, 0, 0 );
		}
		}// WM_DESTROY
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

		case IDM_PASTE:
			{

			SendMessage( hwndEdit, WM_PASTE, 0, 0 );

			}// IDM_PASTE
			break;

		case IDM_SLCTA:
			{

			SendMessage( hwndEdit, EM_SETSEL, 0, -1 );

			}// IDM_SLCTA
			break;

		case IDM_SLCTC:
			{

			SendMessage( hwndEdit, EM_SETSEL, 0, 0 );

			}// IDM_SLCTC
			break;

		case IDM_COPY:
			{

			SendMessage( hwndEdit, WM_COPY, 0, 0 );

			}// IDM_COPY
			break;

		case IDM_CUT:
			{

			SendMessage( hwndEdit, WM_CUT, 0, 0 );

			}// IDM_CUT
			break;

		case IDM_UNDO:
			{

			SendMessage( hwndEdit, WM_UNDO, 0, 0 );

			}// IDM_UNDO
			break;

		case IDM_EXIT:
			{

			SendMessage( hwndMain, WM_RETURN, 0, 0 );

			}// IDM_EXIT
			break;

		case IDM_EXITWS:
			{

			SetWindowText( hwndEdit, "" );
			SendMessage( hwndMain, WM_RETURN, 0, 0 );

			}// IDM_EXITWS
			break;

		default:
			{

			return CallWindowProc(
							OldEditWndProc, hWnd, uMsg, wParam, lParam );

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

			SendMessage( hwndDsp, WM_EDITEND, 0, 0 );

			}// IDM_EXIT
			break;

		case IDM_PASTED:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, WM_PASTE, 0, 0 );

			}// IDM_PASTED
			break;

		case IDM_SLCTAD:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, EM_SETSEL, 0, -1 );

			}// IDM_SLCTAD
			break;

		case IDM_SLCTCD:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, EM_SETSEL, 0, 0 );

			}// IDM_SLCTCD
			break;

		case IDM_COPYD:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, WM_COPY, 0, 0 );

			}// IDM_COPYD
			break;

		case IDM_CUTD:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, WM_CUT, 0, 0 );

			}// IDM_CUTD
			break;

		case IDM_UNDOD:
			{

			SendMessage( DspData[nCrntDEdit].hwndDEdit, WM_UNDO, 0, 0 );

			}// IDM_UNDOD
			break;

		default:
			break;

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
