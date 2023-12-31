


//
//	memo1.h : １行メモ
//
//	since 2023/10/16 by beshi
//
// C:\Users\beshi\AppData\Roaming\MEMO1\MEMO1.TXT
//

double	ProgVersion = 0.003;
double	DataVersion = 0.001;

/*
MEMO1.CPP 108: // 読み込んだ各行についての情報 MemoInfo
MEMO1.CPP 184: // 設定ファイル ConfigInfo
MEMO1.CPP 217: int GetCompileTime ( char * pStr )
MEMO1.CPP 243: // 改行コードまで移動する MoveToCRLF()
MEMO1.CPP 260: // 改行コードをスキップする SkipCRLF()
MEMO1.CPP 276: char * GetAppDataFolder( char * pBuf )
MEMO1.CPP 297: // データファイルのフルパスを得る BCCGetDataPath0()
MEMO1.CPP 318: // データファイルのフルパスを得る BCCGetDataPath()
MEMO1.CPP 328: // 設定データファイルのフルパスを得る BCCGetConfigPath()
MEMO1.CPP 337: // 指定行数読み込む ReadBufferLine()
MEMO1.CPP 392: // 指定行数を保存する SaveLines()
MEMO1.CPP 456: // WinMain()
MEMO1.CPP 521: 	// 設定データ読み込み
MEMO1.CPP 553: 	// 前回のメモを取得
MEMO1.CPP 662: 	// 設定データ保存
MEMO1.CPP 726: // ポップアップメニューの表示 DspPMenu()
MEMO1.CPP 746: // メニュー項目の設定 SetMainMenuItem()
MEMO1.CPP 763: // WndProc()
MEMO1.CPP 1084: // 行にテキストをセットする SetTextLine()
MEMO1.CPP 1111: // メニュー項目の設定 SetDspMenuItem()
MEMO1.CPP 1128: // DspWndProc()
MEMO1.CPP 1547: // EditWndProc()
MEMO1.CPP 1794: // DEditWndProc()
*/



#include	<windows.h>
#include	<shlobj.h>
#include	<commdlg.h>
#include	<direct.h>
#include	<time.h>
#include	<stdio.h>

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

char	ClassName[] = "Memo1MainClass";
char	DspWndCN[] = "Memo1DisplayClass";
char	ApplicationName[] = "Memo1";
char	AppName[20];

int		MainX = 100;
int		MainY = 100;
int		MainWidth = 1000;
int		MainHeight = 80;

int		fTopmost = 0;		// 常に最上位位置の時にnot0
int		fDspWndQuit = 0;

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

// 読み込んだ各行についての情報 MemoInfo						//TAG_JUMP_MARK
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



// 設定ファイル ConfigInfo										//TAG_JUMP_MARK
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
	// バージョン
	double	DataVersion;
	double	ProgVersion;
	// 余白
	char	dummy[100];
};
struct SaveInfo {
	ConfigInfo	cd;
	char		dummy[500];
};



char	FilterString[] =
	"Text Files" "\0"	"*.txt" "\0"
	"All Files" "\0"	"*.*" "\0"
	"\0";



int GetCompileTime ( char * pStr )								//TAG_JUMP_MARK
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
// 改行コードまで移動する MoveToCRLF()							//TAG_JUMP_MARK
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
// 改行コードをスキップする SkipCRLF()							//TAG_JUMP_MARK
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



char * GetAppDataFolder( char * pBuf )							//TAG_JUMP_MARK
{

	if( pBuf == NULL )			return NULL;

	SHGetFolderPath(
		NULL,			//HWND   hwndOwner,
		CSIDL_APPDATA,	//int    nFolder,
		NULL,			//HANDLE hToken,
		0,				//DWORD  dwFlags,
		pBuf			//LPTSTR pszPath
	);
	lstrcat( pBuf, "\\MEMO1" );

	return pBuf;

}



//
// データファイルのフルパスを得る BCCGetDataPath0()				//TAG_JUMP_MARK
//
BOOL  BCCGetDataPath0(  char *pBuf,  char *pFileName  )
{
	if( pBuf == NULL )						return(  0  );
	if( pFileName == NULL )					return(  0  );
	if( *pFileName == 0 )					return(  0  );
	GetAppDataFolder( pBuf );
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
		wsprintf( fn, "MEMO1PastMemos%d.TXT", time(NULL) );
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
	else{
		// ファイルサイズを０に
		SetEndOfFile( hFile );	// 現在のファイルポインタ位置にEOFをセット
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



// WinMain()													//TAG_JUMP_MARK
int WinMain( HINSTANCE hInst, HINSTANCE hPrevInst,
												char * CmdLine, int CmdShow )
/*int main()*/
{

	//hInstance = hInst;
	hInstance = GetModuleHandle( NULL );

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
	wc.hInstance = hInstance;
	//wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	//wc.lpszMenuName = MAKEINTRESOURCE( IDR_MAINMENU );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ClassName;
	wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDR_ICON) );
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
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	//wc.lpszMenuName = MAKEINTRESOURCE( IDR_MAINMENU );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = DspWndCN;
	wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDR_ICON) );
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	if( RegisterClassEx( &wc ) == 0 ){
		MessageBox( NULL, "Error : register DspWndCN", AppName,
													MB_OK | MB_ICONERROR );
		return 0;
	}
	}// end


	// 設定データ読み込み										//TAG_JUMP_MARK
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

	// 前回のメモを取得											//TAG_JUMP_MARK
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

	// 残った過去メモを保存する
	SaveLines( 0, 1, pTopMemoData, nMaxRead );

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
	//MainHeight = 80;
	hwndMain = CreateWindowEx( WS_EX_CLIENTEDGE, ClassName, pt,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
		MainX, MainY, MainWidth, MainHeight,
		NULL, NULL, hInstance, NULL );
	//ShowWindow( hwnd, CmdShow );
	//UpdateWindow( hwnd );
	}


	// メッセージループ
	MSG	msg;
	while( GetMessage( &msg, NULL, 0, 0 ) ){
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}


	// 設定データ保存											//TAG_JUMP_MARK
	while(TRUE){

	SaveInfo	sd;
	ZeroMemory( &sd, sizeof(SaveInfo) );
	sd.cd.Magic[0] = 'M';
	sd.cd.Magic[1] = 'E';
	sd.cd.Magic[2] = 'M';
	sd.cd.Magic[3] = 'O';
	sd.cd.Magic[4] = '1';
	sd.cd.MainX = MainX;
	sd.cd.MainY = MainY;
	sd.cd.MainWidth = MainWidth;
	sd.cd.MainHeight = MainHeight;
	sd.cd.DspX = DspX;
	sd.cd.DspY = DspY;
	sd.cd.DspWidth = DspWidth;
	sd.cd.DspHeight = DspHeight;
	sd.cd.DataVersion = DataVersion;
	sd.cd.ProgVersion = ProgVersion;

	char	path[ MAX_PATH ];
	BCCGetConfigPath( path );
	HANDLE	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE,
		NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )	break;
	DWORD	bytesWrite;
	/* BOOL fOK = */
	WriteFile( hFile, &sd, sizeof(SaveInfo), &bytesWrite, NULL );
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



struct MenuInfo
{

	HWND	hWnd;		// メニューを表示する窓
	HMENU	hMenu;		// メニュー
	HMENU	hSubMenu;	// サブメニュー
	int		x;			// メニューを表示する位置
	int		y;
};



// ポップアップメニューの表示 DspPMenu()						//TAG_JUMP_MARK
void DspPMenu( MenuInfo * pMD )
{

	if( pMD == NULL )		return;

	POINT	pt;
	pt.x = pMD->x;
	pt.y = pMD->y;
	ClientToScreen( pMD->hWnd, &pt );

	TrackPopupMenu( pMD->hSubMenu, TPM_LEFTALIGN, pt.x, pt.y, 0,
															pMD->hWnd, NULL );
	DestroyMenu( pMD->hMenu );

}
//void DspPMenu( MenuInfo * pMD )



// メニュー項目の設定 SetMainMenuItem()							//TAG_JUMP_MARK
void SetMainMenuItem ( MenuInfo * pMD )
{

	if( pMD == NULL )		return;

	pMD->hMenu = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_MAINMENU ) );
	pMD->hSubMenu = GetSubMenu( pMD->hMenu, 0 );

	CheckMenuItem( pMD->hMenu, IDM_TOPMOST,
				MF_BYCOMMAND | ( fTopmost ? MF_CHECKED : MF_UNCHECKED )  );

}
//void SetMainMenuItem ()



// WndProc()													//TAG_JUMP_MARK
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_CREATE:
		{

		hwndEdit = CreateWindowEx( NULL, EditClass, NULL,
			WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL ,
			0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, hInstance, NULL );

		/*
		HDC	hdc = GetDC( hWnd );
		TEXTMETRIC	tm;
		GetTextMetrics( hdc, &tm );
		RECT	r;
		GetWindowRect( hwndEdit, &r );
		MoveWindow( hwndEdit, 0, 0, r.right, tm.tmHeight, TRUE );
		ReleaseDC( hWnd, hdc );
		*/

		// サブクラス化
		OldEditWndProc = (WNDPROC)SetWindowLong(
								hwndEdit, GWL_WNDPROC, (DWORD)EditWndProc );

		SetFocus( hwndEdit );

		}// WM_CREATE
		break;

	case WM_CTLCOLOREDIT:
		{

		SetTextColor( (HDC)wParam, RGB(255,255,100) );
		SetBkColor( (HDC)wParam, RGB(0,0,0) );
		return (LRESULT)GetStockObject( BLACK_BRUSH );

		}// WM_CTLCOLOREDIT
		break;

	case WM_SIZE:
		{

		//MoveWindow( hwndEdit, 0, 0, lParam&0xffff, lParam>>16, TRUE );

		HDC	hdc = GetDC( hWnd );
		TEXTMETRIC	tm;
		GetTextMetrics( hdc, &tm );
		ReleaseDC( hWnd, hdc );

		int		cw = lParam & 0xffff;
		int		ch = lParam >> 16;
		MoveWindow( hwndEdit,
						5, (ch-tm.tmHeight)/2, cw-10, tm.tmHeight, TRUE );

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

		MenuInfo	md;
		md.hWnd = hWnd;
		md.x = LOWORD( lParam );
		md.y = HIWORD( lParam );

		SetMainMenuItem( &md );

		DspPMenu( &md );

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

		case IDM_TEST:
			{
			}// IDM_TEST
			break;

		case IDM_TOPMOST:
			{

			SendMessage( hwndEdit, uMsg, wParam, lParam );

			}// IDM_TOPMOST
			break;

		case IDM_OPEN:
			{

			SendMessage( hwndEdit, uMsg, wParam, lParam );

			}// IDM_OPEN
			break;

		case IDM_DISP:
			{

			SendMessage( hwndEdit, uMsg, wParam, lParam );

			}// IDM_DISP
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

		case IDM_ABOUT:
			{

			SendMessage( hwndEdit, uMsg, wParam, lParam );

			}// IDM_ABOUT
			break;

		default:
			break;

		}// switch

		}// end WM_COMMAND
		break;

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
//	nl : 行番号(0〜)
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



// メニュー項目の設定 SetDspMenuItem()							//TAG_JUMP_MARK
void SetDspMenuItem ( MenuInfo * pMD )
{

	if( pMD == NULL )		return;

	pMD->hMenu = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_DSPMENU ) );
	pMD->hSubMenu = GetSubMenu( pMD->hMenu, 0 );

	//CheckMenuItem( pMD->hMenu, IDM_TOPMOST,
	//			MF_BYCOMMAND | ( fTopmost ? MF_CHECKED : MF_UNCHECKED )  );

}
//void SetDspMenuItem ()



// DspWndProc()													//TAG_JUMP_MARK
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

		SetTextColor( (HDC)wParam, RGB(255,255,100) );
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

		MenuInfo	md;
		md.hWnd = hWnd;
		md.x = LOWORD( lParam );
		md.y = HIWORD( lParam );

		SetDspMenuItem( &md );

		DspPMenu( &md );

		}// WM_RBUTTONDOWN
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

	case WM_KEYDOWN:
		{
		WORD	wScrollNotify = 0xFFFF;

		switch( wParam ){
		case VK_UP:			wScrollNotify = SB_LINEUP;		break;
		case VK_DOWN:		wScrollNotify = SB_LINEDOWN;	break;
		case VK_PRIOR:		wScrollNotify = SB_PAGEUP;		break;
		case VK_NEXT:		wScrollNotify = SB_PAGEDOWN;	break;
		case VK_HOME:		wScrollNotify = SB_TOP;			break;
		case VK_END:		wScrollNotify = SB_BOTTOM;		break;
		}// switch

		if( wScrollNotify != 0xffff )
			SendMessage( hWnd, WM_VSCROLL, MAKELONG(wScrollNotify,0), 0L );

		}// WM_KEYDOWN
		break;

	case WM_MOUSEWHEEL:
		// WORD GET_KEYSTATE_WPARAM(wParam)
		// short GET_WHEEL_DELTA_WPARAM(wParam)
		// short GET_X_LPARAM(lParam)
		// short GET_Y_LPARAM(lParam)
		{

		WORD	wScrollNotify = SB_LINEUP;
		if( GET_WHEEL_DELTA_WPARAM(wParam) < 0 )
			wScrollNotify = SB_LINEDOWN;
		SendMessage( hWnd, WM_VSCROLL, MAKELONG(wScrollNotify,0), 0L );

		//printf( "%d\n", GET_WHEEL_DELTA_WPARAM(wParam) );

		}// WM_MOUSEWHEEL
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



// EditWndProc()												//TAG_JUMP_MARK
LRESULT CALLBACK EditWndProc(
						HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_CREATE:
		{

		/*
		HDC	hdc = GetDC( hWnd );
		TEXTMETRIC	tm;
		GetTextMetrics( hdc, &tm );
		RECT	r;
		GetWindowRect( hWnd, &r );
		MoveWindow( hWnd, 0, 0, r.right, tm.tmHeight, TRUE );
		ReleaseDC( hWnd, hdc );
		*/

		return CallWindowProc( OldEditWndProc, hWnd, uMsg, wParam, lParam );

		}// WM_CREATE
		break;

	case WM_KEYDOWN:
		{

		if( (char)wParam == VK_RETURN )
			SendMessage( hwndMain, WM_RETURN, 0, 0 );

		return CallWindowProc( OldEditWndProc, hWnd, uMsg, wParam, lParam );

		}// WM_KEYDOWN
		break;

	case WM_RBUTTONDOWN:
		{

		MenuInfo	md;
		md.hWnd = hWnd;
		md.x = LOWORD( lParam );
		md.y = HIWORD( lParam );

		SetMainMenuItem( &md );

		DspPMenu( &md );

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

		case IDM_TOPMOST:
			{

			fTopmost ^= 1;

			HWND	hwndInsertAfter =
								fTopmost ? HWND_TOPMOST : HWND_NOTOPMOST ;

			SetWindowPos( hwndMain,
						hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			if( hwndDsp != NULL ){
				SetWindowPos( hwndDsp,
						hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			}

			}// IDM_TOPMOST
			break;

		case IDM_OPEN:
			{

			OPENFILENAME	ofn;
			ZeroMemory( &ofn, sizeof(OPENFILENAME) );
			char			dir[ MAX_PATH ];
			GetAppDataFolder( dir );
			char			buffer[ MAX_PATH ];
			buffer[0] = 0;	// これをしないと表示されないことあり
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.hInstance = hInstance;
			ofn.lpstrFilter = FilterString;
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
							OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY ;
			ofn.lpstrTitle = "past memos";
			ofn.lpstrInitialDir = dir;
			if( ! GetOpenFileName( &ofn ) )		break;
			ShellExecute( hWnd, "open", buffer, NULL, NULL, SW_SHOWNORMAL );

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

			if( fTopmost ){
				SetWindowPos( hwndDsp,
						HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			}

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

		case IDM_ABOUT:
			{
			char	buf[1000];
			sprintf( buf,
				"Memo1 : 1 line memo\n\n"
				"Program  Version : %g\n"
				"SaveData Version : %g\n"
				,
				ProgVersion,
				DataVersion
			);
			MessageBox( NULL, buf, AppName, MB_OK );
			}// IDM_ABOUT
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



// DEditWndProc()												//TAG_JUMP_MARK
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

		MenuInfo	md;
		md.hWnd = hWnd;
		md.x = LOWORD( lParam );
		md.y = HIWORD( lParam );

		SetDspMenuItem( &md );

		DspPMenu( &md );

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
