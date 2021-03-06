/*
 * HWGUI - Harbour Win32 GUI library source code:
 * C level windows functions
 *
 * Copyright 2001 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#define OEMRESOURCE
#include "hwingui.h"
#include <commctrl.h>

#include "hbapifs.h"
#include "hbapiitm.h"
#include "hbapicdp.h"
#include "hbvm.h"
#include "hbstack.h"
#include "hbapicls.h"

#include <math.h>
#include <float.h>
#include <limits.h>

/* Avoid warnings from GCC */
#include "warnings.h"
#include "incomp_pointer.h"

#define  FIRST_MDICHILD_ID     501
#define  WND_MDICHILD          3

static LRESULT CALLBACK s_MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK s_FrameWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK s_MDIChildWndProc(HWND, UINT, WPARAM, LPARAM);

static HHOOK s_KeybHook = nullptr;
HWND aWindows[2] = {0, 0};
PHB_DYNS pSym_onEvent = nullptr;
PHB_DYNS pSym_keylist = nullptr;
static LPCTSTR s_szChild = TEXT("MDICHILD");

void hwg_doEvents(void)
{
   MSG msg;

   while( PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) )
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   };
}

static void s_ClearKeyboard(void)
{
   MSG msg;

   // For keyboard
   while( PeekMessage(&msg, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) );
   // For Mouse
   while( PeekMessage(&msg, nullptr, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) );
}

/* Consume all queued events, useful to update all the controls... I split in 2 parts because I feel
 * that s_doEvents should be called internally by some other functions...
 */
HB_FUNC( HWG_DOEVENTS )
{
   hwg_doEvents();
}

/*  Creates main application window
    InitMainWindow(pObject, szAppName, cTitle, cMenu, hIcon, nBkColor, nStyle, nExclude, nLeft, nTop, nWidth, nHeight)
*/
HB_FUNC( HWG_INITMAINWINDOW )
{
   HWND hWnd = nullptr;
   WNDCLASS wndclass;
   HANDLE hInstance = GetModuleHandle(nullptr);
   DWORD ExStyle = 0;
   PHB_ITEM pObject = hb_param(1, Harbour::Item::OBJECT), temp;
   void * hAppName, * hTitle, * hMenu;
   LPCTSTR lpAppName = HB_PARSTR(2, &hAppName, nullptr);
   LPCTSTR lpTitle = HB_PARSTR(3, &hTitle, nullptr);
   LPCTSTR lpMenu = HB_PARSTR(4, &hMenu, nullptr);
   LONG nStyle = hb_parnl(7);
   LONG nExcl = hb_parnl(8);
   int x = hb_parnl(9);
   int y = hb_parnl(10);
   int width = hb_parnl(11);
   int height = hb_parnl(12);

   if( !aWindows[0] )
   {
      wndclass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
      wndclass.lpfnWndProc = s_MainWndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = static_cast<HINSTANCE>(hInstance);
      wndclass.hIcon = (hb_pcount() > 4 && !HB_ISNIL(5)) ? hwg_par_HICON(5) : LoadIcon(static_cast<HINSTANCE>(hInstance), TEXT(""));
      wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
      wndclass.hbrBackground = (hb_pcount() > 5 && !HB_ISNIL(6)) ? ((hb_parnl(6) == -1) ? nullptr :
         (HB_ISPOINTER(6) ? hwg_par_HBRUSH(6) : reinterpret_cast<HBRUSH>(hb_parnl(6)))) : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // TODO: ? realmente preciso checar o tipo ?
      wndclass.lpszMenuName  = lpMenu;
      wndclass.lpszClassName = lpAppName;

      if( RegisterClass(&wndclass) )
      {
         nStyle = (WS_OVERLAPPEDWINDOW & ~nExcl) | nStyle;
         hWnd = CreateWindowEx(ExStyle, lpAppName, lpTitle, nStyle, x, y,
            (!width) ? static_cast<LONG>(CW_USEDEFAULT) : width,
            (!height) ? static_cast<LONG>(CW_USEDEFAULT) : height,
            nullptr, nullptr, static_cast<HINSTANCE>(hInstance), nullptr);

         temp = hb_itemPutNL(nullptr, 1);
         SetObjectVar(pObject, "_NHOLDER", temp);
         hb_itemRelease(temp);
         SetWindowObject(hWnd, pObject);

         aWindows[0] = hWnd;
      }
   }
   hb_strfree(hAppName);
   hb_strfree(hTitle);
   hb_strfree(hMenu);

   HB_RETHANDLE(hWnd);
}

HB_FUNC( HWG_CENTERWINDOW )
{
   RECT rect, rectcli;
   int w, h, x, y;

   GetWindowRect(hwg_par_HWND(1), &rect);

   if( hb_parni(2) == WND_MDICHILD )
   {
      GetWindowRect(static_cast<HWND>(aWindows[1]), &rectcli);
      x = rectcli.right - rectcli.left;
      y = rectcli.bottom - rectcli.top;
      w = rect.right - rect.left;
      h = rect.bottom - rect.top;
   }
   else
   {
      w = rect.right - rect.left;
      h = rect.bottom - rect.top;
      x = GetSystemMetrics(SM_CXSCREEN);
      y = GetSystemMetrics(SM_CYSCREEN);
   }

   SetWindowPos(hwg_par_HWND(1), HWND_TOP, (x - w) / 2, (y - h) / 2, 0, 0, SWP_NOSIZE + SWP_NOACTIVATE + SWP_FRAMECHANGED + SWP_NOSENDCHANGING);
}

void ProcessMessage(MSG msg, HACCEL hAcceler, BOOL lMdi)
{
   int i;
   HWND hwndGoto;

   for( i = 0; i < iDialogs; i++ )
   {
      hwndGoto = aDialogs[i];
      if( IsWindow(hwndGoto) && IsDialogMessage(hwndGoto, &msg) )
      {
         break;
      }
   }

   if( i == iDialogs )
   {
      if( lMdi && TranslateMDISysAccel(aWindows[1], &msg) )
      {
         return;
      }

      if( !hAcceler || !TranslateAccelerator(aWindows[0], hAcceler, &msg) )
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
}

void ProcessMdiMessage(HWND hJanBase, HWND hJanClient, MSG msg, HACCEL hAcceler)
{
   if( !TranslateMDISysAccel(hJanClient, &msg) && !TranslateAccelerator(hJanBase, hAcceler, &msg) )
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

void hwg_ActivateMainWindow(BOOL bShow, HACCEL hAcceler, BOOL bMaximize, BOOL bMinimize)
{
   MSG msg;

   if( bShow )
   {
      ShowWindow(aWindows[0], bMaximize ? SW_SHOWMAXIMIZED : (bMinimize ? SW_SHOWMINIMIZED : SW_SHOWNORMAL));
   }

   while( GetMessage(&msg, nullptr, 0, 0) )
   {
      ProcessMessage(msg, hAcceler, 0);
   }
}

/*
HWG_ACTIVATEMAINWINDOW(lShow, hAccel|NIL, lMaximize, lMinimize)
*/
HB_FUNC( HWG_ACTIVATEMAINWINDOW )
{
   hwg_ActivateMainWindow(hb_parl(1), (HB_ISNIL(2) ? nullptr : static_cast<HACCEL>(HB_PARHANDLE(2))), ((HB_ISLOG(3) && hb_parl(3)) ? 1 : 0), ((HB_ISLOG(4) && hb_parl(4)) ? 1 : 0));
}

/*
HWG_PROCESSMESSAGE(lMdi|NIL, nSleep|NIL) --> .T.|.F.
*/
HB_FUNC( HWG_PROCESSMESSAGE )
{
   MSG msg;
   BOOL lMdi = (HB_ISNIL(1)) ? 0 : hb_parl(1);
   int nSleep = (HB_ISNIL(2)) ? 1 : hb_parni(2);

   if( PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) )
   {
      ProcessMessage(msg, 0, lMdi);
      hb_retl(true);
   }
   else
   {
      hb_retl(false);
   }

   SleepEx(nSleep, TRUE);
}

/* 22/09/2005 - <maurilio.longo@libero.it>
      It can be used to see if there are messages awaiting of a certain
      type, but it does not retrieve them
*/
/*
HWG_PEEKMESSAGE(hWnd, wMsgFilterMin, wMsgFilterMax) --> .T.|.F.
*/
HB_FUNC( HWG_PEEKMESSAGE )
{
   MSG msg;
   hb_retl(PeekMessage(&msg, hwg_par_HWND(1), hwg_par_UINT(2), hwg_par_UINT(3), PM_NOREMOVE));
}

HB_FUNC( HWG_INITCHILDWINDOW )
{
   HWND hWnd = nullptr;
   WNDCLASS wndclass;
   HMODULE /*HANDLE*/ hInstance = GetModuleHandle(nullptr);
   PHB_ITEM pObject = hb_param(1, Harbour::Item::OBJECT), temp;
   void * hAppName, * hTitle, * hMenu;
   LPCTSTR lpAppName = HB_PARSTR(2, &hAppName, nullptr);
   LPCTSTR lpTitle = HB_PARSTR(3, &hTitle, nullptr);
   LPCTSTR lpMenu = HB_PARSTR(4, &hMenu, nullptr);
   LONG nStyle = hb_parnl(7);
   int x = hb_parnl(8);
   int y = hb_parnl(9);
   int width = hb_parnl(10);
   int height = hb_parnl(11);
   HWND hParent = hwg_par_HWND(12);
   BOOL fRegistered = TRUE;

   if( !GetClassInfo( hInstance, lpAppName, &wndclass ) )
   {
      wndclass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
      wndclass.lpfnWndProc = s_MainWndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = static_cast<HINSTANCE>(hInstance);
      wndclass.hIcon = (hb_pcount() > 4 && !HB_ISNIL(5)) ? hwg_par_HICON(5) : LoadIcon(static_cast<HINSTANCE>(hInstance), TEXT(""));
      wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
      wndclass.hbrBackground = (((hb_pcount() > 5 && !HB_ISNIL(6)) ? ((hb_parnl(6) == -1) ? nullptr : hwg_par_HBRUSH(6)) : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)));
      /*
         wndclass.hbrBackground = ( ( (hb_pcount()>5 && !HB_ISNIL(6))?
         ( (hb_parnl(6)==-1)? static_cast<HBRUSH>(COLOR_WINDOW + 1) :
         CreateSolidBrush(hb_parnl(6)) )
         : static_cast<HBRUSH>(COLOR_WINDOW + 1) ) );
       */
      wndclass.lpszMenuName = lpMenu;
      wndclass.lpszClassName = lpAppName;

      //UnregisterClass(lpAppName, (HINSTANCE)hInstance);
      if( !RegisterClass(&wndclass) )
      {
         fRegistered = FALSE;
#ifdef __XHARBOUR__
         MessageBox(GetActiveWindow(), lpAppName, TEXT("Register Child Wnd Class"), MB_OK | MB_ICONSTOP);
#endif
      }
   }

   if( fRegistered )
   {
      hWnd = CreateWindowEx(WS_EX_MDICHILD, lpAppName, lpTitle,
            WS_OVERLAPPEDWINDOW | nStyle, x, y,
            ( !width ) ? static_cast<LONG>(CW_USEDEFAULT) : width,
            ( !height ) ? static_cast<LONG>(CW_USEDEFAULT) : height,
            hParent, nullptr, static_cast<HINSTANCE>(hInstance), nullptr);

      temp = hb_itemPutNL(nullptr, 1);
      SetObjectVar(pObject, "_NHOLDER", temp);
      hb_itemRelease(temp);
      SetWindowObject(hWnd, pObject);
   }

   HB_RETHANDLE(hWnd);

   hb_strfree(hAppName);
   hb_strfree(hTitle);
   hb_strfree(hMenu);
}

HB_FUNC( HWG_ACTIVATECHILDWINDOW )
{
   // ShowWindow(hwg_par_HWND(2), hb_parl(1) ? SW_SHOWNORMAL : SW_HIDE);
   ShowWindow(hwg_par_HWND(2), (HB_ISLOG(3) && hb_parl(3)) ? SW_SHOWMAXIMIZED : ((HB_ISLOG(4) &&  hb_parl(4)) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL));
}

/*  Creates frame MDI and client window
    InitMainWindow(cTitle, cMenu, cBitmap, hIcon, nBkColor, nStyle, nLeft, nTop, nWidth, nHeight)
*/
HB_FUNC( HWG_INITMDIWINDOW )
{
   HWND hWnd;
   WNDCLASS wndclass, wc;
   HANDLE hInstance = GetModuleHandle(nullptr);
   PHB_ITEM pObject = hb_param(1, Harbour::Item::OBJECT), temp;
   void * hAppName, * hTitle, * hMenu;
   LPCTSTR lpAppName = HB_PARSTR(2, &hAppName, nullptr);
   LPCTSTR lpTitle = HB_PARSTR(3, &hTitle, nullptr);
   LPCTSTR lpMenu = HB_PARSTR(4, &hMenu, nullptr);
   int x = hb_parnl(8);
   int y = hb_parnl(9);
   int width = hb_parnl(10);
   int height = hb_parnl(11);

   if( aWindows[0] )
   {
      hb_retni(-1);
   }
   else
   {
      // Register frame window
      wndclass.style = 0;
      wndclass.lpfnWndProc = s_FrameWndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = static_cast<HINSTANCE>(hInstance);
      wndclass.hIcon = ( hb_pcount() > 4 &&
            !HB_ISNIL(5) ) ? hwg_par_HICON(5) :
            LoadIcon(static_cast<HINSTANCE>(hInstance), TEXT(""));
      wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
      wndclass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
      wndclass.lpszMenuName = lpMenu;
      wndclass.lpszClassName = lpAppName;

      if( !RegisterClass(&wndclass) )
      {
         hb_retni(-2);
      }
      else
      {
         // Register client window
         wc.lpfnWndProc = ( WNDPROC ) s_MDIChildWndProc;
         wc.hIcon = (hb_pcount() > 4 && !HB_ISNIL(5)) ? hwg_par_HICON(5) : LoadIcon(static_cast<HINSTANCE>(hInstance), TEXT(""));
         wc.hbrBackground = (hb_pcount() > 5 && !HB_ISNIL(6)) ? hwg_par_HBRUSH(6) : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
         wc.lpszMenuName = nullptr;
         wc.cbWndExtra = 0;
         wc.lpszClassName = s_szChild;
         wc.cbClsExtra = 0;
         wc.hInstance = static_cast<HINSTANCE>(hInstance);
         wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
         wc.style = 0;

         if( !RegisterClass(&wc) )
         {
            hb_retni(-3);
         }
         else
         {
            // Create frame window
            hWnd = CreateWindow(lpAppName, lpTitle, WS_OVERLAPPEDWINDOW, x, y,
               (!width) ? static_cast<LONG>(CW_USEDEFAULT) : width,
               (!height) ? static_cast<LONG>(CW_USEDEFAULT) : height,
               nullptr, nullptr, static_cast<HINSTANCE>(hInstance), nullptr);
            if( !hWnd )
            {
               hb_retni(-4);
            }
            else
            {
               temp = hb_itemPutNL(nullptr, 1);
               SetObjectVar(pObject, "_NHOLDER", temp);
               hb_itemRelease(temp);
               SetWindowObject(hWnd, pObject);

               aWindows[0] = hWnd;
               HB_RETHANDLE(hWnd);
            }
         }
      }
   }
   hb_strfree(hAppName);
   hb_strfree(hTitle);
   hb_strfree(hMenu);
}

HB_FUNC( HWG_INITCLIENTWINDOW )
{
   CLIENTCREATESTRUCT ccs;
   HWND hWnd;
   int nPos = ( hb_pcount() > 1 && !HB_ISNIL(2) ) ? hb_parni(2) : 0;
   int x = hb_parnl(3);
   int y = hb_parnl(4);
   int width = hb_parnl(5);
   int height = hb_parnl(6);

   // Create client window
   ccs.hWindowMenu = GetSubMenu(GetMenu(aWindows[0]), nPos);
   ccs.idFirstChild = FIRST_MDICHILD_ID;

   hWnd = CreateWindow(TEXT("MDICLIENT"), nullptr, WS_CHILD | WS_CLIPCHILDREN | MDIS_ALLCHILDSTYLES, x, y, width, height,
      aWindows[0], nullptr, GetModuleHandle(nullptr), ( LPVOID ) &ccs);

   aWindows[1] = hWnd;
   HB_RETHANDLE(hWnd);
}

HB_FUNC( HWG_ACTIVATEMDIWINDOW )
{
   HACCEL hAcceler = (HB_ISNIL(2)) ? nullptr : static_cast<HACCEL>(HB_PARHANDLE(2));
   MSG msg;

   if( hb_parl(1) )
   {
      ShowWindow(aWindows[0], (HB_ISLOG(3) && hb_parl(3)) ? SW_SHOWMAXIMIZED : ((HB_ISLOG(4) && hb_parl(4)) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL));
      ShowWindow(aWindows[1], SW_SHOW);
   }

   while( GetMessage(&msg, nullptr, 0, 0) )
   {
      // ProcessMessage(msg, hAcceler, 0);
      ProcessMdiMessage(aWindows[0], aWindows[1], msg, hAcceler);
   }
}

/*  Creates child MDI window
    CreateMdiChildWindow(aChildWindow)
    aChildWindow = {cWindowTitle, Nil, aActions, Nil, nStatusWindowID, bStatusWrite}
    aActions = {{nMenuItemID, bAction}, ...}
*/

HB_FUNC( HWG_CREATEMDICHILDWINDOW )
{
   HWND hWnd = nullptr;
   PHB_ITEM pObj = hb_param(1, Harbour::Item::OBJECT);
   DWORD style = static_cast<DWORD>(hb_itemGetNL(GetObjectVar(pObj, "STYLE")));
   int y = ( int ) hb_itemGetNL(GetObjectVar(pObj, "NTOP"));
   int x = ( int ) hb_itemGetNL(GetObjectVar(pObj, "NLEFT"));
   int width = ( int ) hb_itemGetNL(GetObjectVar(pObj, "NWIDTH"));
   int height = ( int ) hb_itemGetNL(GetObjectVar(pObj, "NHEIGHT"));
   void * hTitle;
   LPCTSTR lpTitle = HB_ITEMGETSTR(GetObjectVar(pObj, "TITLE"), &hTitle, nullptr);

   if( !style )
   {
      style = WS_VISIBLE | WS_CHILD | WS_OVERLAPPEDWINDOW | ( int ) hb_parnl(2);   //WS_VISIBLE | WS_MAXIMIZE;
   }
   else
   {
      style = style | ( int ) hb_parnl(2);
   }

   if( aWindows[0] )
   {
      hWnd = CreateMDIWindow(
#if (((defined(_MSC_VER) && (_MSC_VER <= 1200))))
            ( LPSTR ) s_szChild,        // pointer to registered child class name
            ( LPSTR ) lpTitle,  // pointer to window name
#else
            s_szChild,          // pointer to registered child class name
            lpTitle,            // pointer to window name
#endif
            style,              // window style
            x,                  // horizontal position of window
            y,                  // vertical position of window
            width,              // width of window
            height,             // height of window
            static_cast<HWND>(aWindows[1]),       // handle to parent window (MDI client)
            GetModuleHandle(nullptr),    // handle to application instance
            ( LPARAM ) & pObj   // application-defined value
             );
   }
   HB_RETHANDLE(hWnd);
   hb_strfree(hTitle);
}

HB_FUNC( HWG_SENDMESSAGE )
{
   void * hText;
   LPCTSTR lpText = HB_PARSTR(4, &hText, nullptr);

   hb_retnl(static_cast<LONG>(SendMessage(hwg_par_HWND(1),  // handle of destination window
            hwg_par_UINT(2),  // message to send
            HB_ISPOINTER(3) ? ( WPARAM ) HB_PARHANDLE(3) : ( WPARAM ) hb_parnl(3),
            lpText ? ( LPARAM ) lpText : (HB_ISPOINTER(4) ? ( LPARAM ) HB_PARHANDLE(4) : hwg_par_LPARAM(4))
          )));
   hb_strfree(hText);
}

HB_FUNC( HWG_SENDMESSPTR )
{
   void * hText;
   LPCTSTR lpText = HB_PARSTR(4, &hText, nullptr);

   HB_RETHANDLE(SendMessage(hwg_par_HWND(1),  // handle of destination window
               hwg_par_UINT(2),  // message to send
               HB_ISPOINTER(3) ? ( WPARAM ) HB_PARHANDLE(3) : ( WPARAM ) hb_parnl(3),
               lpText ? ( LPARAM ) lpText : (HB_ISPOINTER(4) ? ( LPARAM ) HB_PARHANDLE(4) : hwg_par_LPARAM(4))
          ));
   hb_strfree(hText);
}

HB_FUNC( HWG_POSTMESSAGE )
{
   hb_retnl(static_cast<LONG>(PostMessage(hwg_par_HWND(1),  // handle of destination window
               hwg_par_UINT(2),  // message to send
               HB_ISPOINTER(3) ? ( WPARAM ) HB_PARHANDLE(3) : ( WPARAM ) hb_parnl(3),
               HB_ISPOINTER(4) ? ( LPARAM ) HB_PARHANDLE(4) : hwg_par_LPARAM(4)
          )));
}

HB_FUNC( HWG_SETFOCUS )
{
   HB_RETHANDLE(SetFocus(hwg_par_HWND(1)));
}

HB_FUNC( HWG_GETFOCUS )
{
   HB_RETHANDLE(GetFocus());
}

HB_FUNC( HWG_SELFFOCUS )
{
   HWND hWnd = HB_ISNIL(2) ? static_cast<HWND>(GetFocus()) : hwg_par_HWND(2);
   hb_retl(hwg_par_HWND(1) == hWnd);
}

HB_FUNC( HWG_SETWINDOWOBJECT )
{
   SetWindowObject(hwg_par_HWND(1), hb_param(2, Harbour::Item::OBJECT));
}

void SetWindowObject(HWND hWnd, PHB_ITEM pObject)
{
   SetWindowLongPtr(hWnd, GWLP_USERDATA, pObject ? ( LPARAM ) hb_itemNew(pObject) : 0);
}

HB_FUNC( HWG_GETWINDOWOBJECT )
{
   hb_itemReturn(( PHB_ITEM ) GetWindowLongPtr(hwg_par_HWND(1), GWLP_USERDATA));
}

HB_FUNC( HWG_SETWINDOWTEXT )
{
   void * hText;
   SetWindowText(hwg_par_HWND(1), HB_PARSTR(2, &hText, nullptr));
   hb_strfree(hText);
}

HB_FUNC( HWG_GETWINDOWTEXT )
{
   HWND hWnd = hwg_par_HWND(1);
   ULONG ulLen = static_cast<ULONG>(SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0));
   LPTSTR cText = ( TCHAR * ) hb_xgrab((ulLen + 1) * sizeof(TCHAR));

   ulLen = static_cast<ULONG>(SendMessage(hWnd, WM_GETTEXT, ( WPARAM ) (ulLen + 1), ( LPARAM ) cText));

   HB_RETSTRLEN(cText, ulLen);
   hb_xfree(cText);
}

HB_FUNC( HWG_SETWINDOWFONT )
{
   SendMessage(hwg_par_HWND(1), WM_SETFONT,
      HB_ISPOINTER(2) ? ( WPARAM ) HB_PARHANDLE(2) : ( WPARAM ) hb_parnl(2),
      MAKELPARAM(((HB_ISNIL(3)) ? 0 : hb_parl(3)), 0));
}

HB_FUNC( HWG_GETLASTERROR )
{
   hb_retnl(static_cast<LONG>(GetLastError()));
}

HB_FUNC( HWG_ENABLEWINDOW )
{
   // ShowWindow(hWnd, (lEnable) ? SW_SHOWNORMAL : SW_HIDE);
   EnableWindow(hwg_par_HWND(1), ( BOOL ) hb_parl(2));
}

HB_FUNC( HWG_DESTROYWINDOW )
{
   DestroyWindow(hwg_par_HWND(1));
}

HB_FUNC( HWG_HIDEWINDOW )
{
   ShowWindow(hwg_par_HWND(1), SW_HIDE);
}

HB_FUNC( HWG_SHOWWINDOW )
{
   ShowWindow(hwg_par_HWND(1), (HB_ISNIL(2)) ? SW_SHOW : hb_parni(2));
}

HB_FUNC( HWG_RESTOREWINDOW )
{
   ShowWindow(hwg_par_HWND(1), SW_RESTORE);
}

HB_FUNC( HWG_ISICONIC )
{
   hb_retl(IsIconic(hwg_par_HWND(1)));
}

HB_FUNC( HWG_ISWINDOWENABLED )
{
   hb_retl(IsWindowEnabled(hwg_par_HWND(1)));
}

HB_FUNC( HWG_ISWINDOWVISIBLE )
{
   hb_retl(IsWindowVisible(hwg_par_HWND(1)));
}

HB_FUNC( HWG_GETACTIVEWINDOW )
{
   HB_RETHANDLE(GetActiveWindow());
}

HB_FUNC( HWG_GETINSTANCE )
{
   hb_retnl(reinterpret_cast<LONG>(GetModuleHandle(nullptr)));
}

HB_FUNC( HWG_SETWINDOWSTYLE )
{
   hb_retnl(SetWindowLongPtr(hwg_par_HWND(1), GWL_STYLE, hb_parnl(2)));
}

HB_FUNC( HWG_GETWINDOWSTYLE )
{
   hb_retnl(GetWindowLongPtr(hwg_par_HWND(1), GWL_STYLE));
}

HB_FUNC( HWG_SETWINDOWEXSTYLE )
{
   hb_retnl(SetWindowLongPtr(hwg_par_HWND(1), GWL_EXSTYLE, hb_parnl(2)));
}

HB_FUNC( HWG_GETWINDOWEXSTYLE )
{
   hb_retnl(GetWindowLongPtr(hwg_par_HWND(1), GWL_EXSTYLE));
}

HB_FUNC( HWG_FINDWINDOW )
{
   void * hClassName;
   void * hWindowName;
   HB_RETHANDLE(FindWindow(HB_PARSTR(1, &hClassName, nullptr), HB_PARSTR(2, &hWindowName, nullptr)));
   hb_strfree(hClassName);
   hb_strfree(hWindowName);
}

HB_FUNC( HWG_SETFOREGROUNDWINDOW )
{
   hb_retl(SetForegroundWindow(hwg_par_HWND(1)));
}

//HB_FUNC( HWG_SETACTIVEWINDOW )
//{
//   hb_retnl(SetActiveWindow(hwg_par_HWND(1)));
//}

HB_FUNC( HWG_RESETWINDOWPOS )
{
   RECT rc;
   GetWindowRect(hwg_par_HWND(1), &rc);
   MoveWindow(hwg_par_HWND(1), rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top, 0);
}

/*
   s_MainWndProc alteradas na HWGUI. Agora as funcoes em hWindow.prg
   retornam 0 para indicar que deve ser usado o processamento default.
*/
static LRESULT CALLBACK s_MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   long int res;
   PHB_ITEM pObject = ( PHB_ITEM ) GetWindowLongPtr(hWnd, GWLP_USERDATA);

   if( !pSym_onEvent )
   {
      pSym_onEvent = hb_dynsymFindName("ONEVENT");
   }

   if( pSym_onEvent && pObject )
   {

      hb_vmPushSymbol(hb_dynsymSymbol(pSym_onEvent));
      hb_vmPush(pObject);
      hb_vmPushLong(static_cast<LONG>(message));
//      hb_vmPushLong(static_cast<LONG>(wParam));
//      hb_vmPushLong(static_cast<LONG>(lParam));
      HB_PUSHITEM(wParam);
      HB_PUSHITEM(lParam);
      hb_vmSend(3);
      if( HB_ISPOINTER(-1) )
      {
         return reinterpret_cast<LRESULT>(HB_PARHANDLE(-1));
      }
      else
      {
         res = hb_parnl(-1);
         if( res == -1 )
         {
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
         else
         {
            return res;
         }
      }
   }
   else
   {
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
}

static LRESULT CALLBACK s_FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   long int res;
   PHB_ITEM pObject = ( PHB_ITEM ) GetWindowLongPtr(hWnd, GWLP_USERDATA);

   if( !pSym_onEvent )
   {
      pSym_onEvent = hb_dynsymFindName("ONEVENT");
   }

   if( pSym_onEvent && pObject )
   {
      hb_vmPushSymbol(hb_dynsymSymbol(pSym_onEvent));
      hb_vmPush(pObject);
      hb_vmPushLong(static_cast<LONG>(message));
//      hb_vmPushLong(static_cast<LONG>(wParam));
//      hb_vmPushLong(static_cast<LONG>(lParam));
      HB_PUSHITEM(wParam);
      HB_PUSHITEM(lParam);
      hb_vmSend(3);
      if( HB_ISPOINTER(-1) )
      {
         return reinterpret_cast<LRESULT>(HB_PARHANDLE(-1));
      }
      else
      {
         res = hb_parnl(-1);
         if( res == -1 )
         {
            return DefFrameProc(hWnd, aWindows[1], message, wParam, lParam);
         }
         else
         {
            return res;
         }
      }
   }
   else
   {
      return DefFrameProc(hWnd, aWindows[1], message, wParam, lParam);
   }
}

static LRESULT CALLBACK s_MDIChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   long int res;
   PHB_ITEM pObject;

   if( message == WM_NCCREATE )
   {
      LPMDICREATESTRUCT cs = ( LPMDICREATESTRUCT ) ((( LPCREATESTRUCT ) lParam)->lpCreateParams);
      PHB_ITEM *pObj = ( PHB_ITEM * ) (cs->lParam);
      PHB_ITEM temp;

      temp = hb_itemPutNL(nullptr, 1);
      SetObjectVar(*pObj, "_NHOLDER", temp);
      hb_itemRelease(temp);

      temp = HB_PUTHANDLE(nullptr, hWnd);
      SetObjectVar(*pObj, "_HANDLE", temp);
      hb_itemRelease(temp);

      SetWindowObject(hWnd, *pObj);
   }

   pObject = ( PHB_ITEM ) GetWindowLongPtr(hWnd, GWLP_USERDATA);

   if( !pSym_onEvent )
   {
      pSym_onEvent = hb_dynsymFindName("ONEVENT");
   }

   if( pSym_onEvent && pObject )
   {
      hb_vmPushSymbol(hb_dynsymSymbol(pSym_onEvent));
      hb_vmPush(pObject);
      hb_vmPushLong(static_cast<LONG>(message));
//      hb_vmPushLong(static_cast<LONG>(wParam));
//      hb_vmPushLong(static_cast<LONG>(lParam));
      HB_PUSHITEM(wParam);
      HB_PUSHITEM(lParam);
      hb_vmSend(3);
      if( HB_ISPOINTER(-1) )
      {
         return reinterpret_cast<LRESULT>(HB_PARHANDLE(-1));
      }
      else
      {
         res = hb_parnl(-1);
         if( res == -1 )
         {
            return DefMDIChildProc(hWnd, message, wParam, lParam);
         }
         else
         {
            return res;
         }
      }
   }
   else
   {
      return DefMDIChildProc(hWnd, message, wParam, lParam);
   }

}

PHB_ITEM GetObjectVar(PHB_ITEM pObject, const char *varname)
{
   /* ( char * ) casting is a hack for old [x]Harbour versions
    * which used wrong hb_objSendMsg() declaration
    */
   return hb_objSendMsg(pObject, ( char * ) varname, 0);
}

void SetObjectVar(PHB_ITEM pObject, const char *varname, PHB_ITEM pValue)
{
   /* ( char * ) casting is a hack for old [x]Harbour versions
    * which used wrong hb_objSendMsg() declaration
    */
   hb_objSendMsg(pObject, ( char * ) varname, 1, pValue);
}

#if !defined(HB_HAS_STR_FUNC)

/* these are simple wrapper functions for xHarbour and older Harbour
 * versions which do not support automatic UNICODE conversions
 */

static const char s_szConstStr[1] = {0};

const char * hwg_strnull(const char * str)
{
   return str ? str : "";
}

const char * hwg_strget(PHB_ITEM pItem, void ** phStr, HB_SIZE * pnLen)
{
   const char * pStr;

   if( pItem && HB_IS_STRING(pItem) )
   {
      *phStr = ( void * ) s_szConstStr;
      pStr = hb_itemGetCPtr(pItem);
      if( pnLen )
      {
         *pnLen = hb_itemGetCLen(pItem);
      }
   }
   else
   {
      *phStr = nullptr;
      pStr = nullptr;
      if( pnLen )
      {
         *pnLen = 0;
      }
   }
   return pStr;
}

HB_SIZE hwg_strcopy(PHB_ITEM pItem, char *pStr, HB_SIZE nLen)
{
   if( pItem && HB_IS_STRING(pItem) )
   {
      HB_SIZE size = hb_itemGetCLen(pItem);

      if( pStr )
      {
         if( size > nLen )
         {
            size = nLen;
         }
         if( size )
         {
            memcpy(pStr, hb_itemGetCPtr(pItem), size);
         }
         if( size < nLen )
         {
            pStr[size] = '\0';
         }
      }
      else if( nLen && size > nLen )
      {
         size = nLen;
      }
      return size;
   }
   else if( pStr && nLen )
   {
      pStr[0] = '\0';
   }

   return 0;
}

char *hwg_strunshare(void **phStr, const char *pStr, HB_SIZE nLen)
{
   if( pStr == nullptr || phStr == nullptr || *phStr == nullptr )
   {
      return nullptr;
   }

   if( *phStr == ( void * ) s_szConstStr && nLen > 0 )
   {
      char *pszDest = ( char * ) hb_xgrab((nLen + 1) * sizeof(char));
      memcpy(pszDest, pStr, nLen * sizeof(char));
      pszDest[nLen] = 0;
      *phStr = ( void * ) pszDest;

      return pszDest;
   }

   return ( char * ) pStr;
}

void hwg_strfree(void * hString)
{
   if( hString && hString != ( void * ) s_szConstStr )
   {
      hb_xfree(hString);
   }
}
#endif /* !HB_HAS_STR_FUNC */

#if !defined(HB_EMULATE_STR_API)

static int s_iVM_CP = CP_ACP;   /* CP_OEMCP */

static const wchar_t s_wszConstStr[1] = {0};

const wchar_t *hwg_wstrnull(const wchar_t * str)
{
   return str ? str : L"";
}

const wchar_t *hwg_wstrget(PHB_ITEM pItem, void **phStr, HB_SIZE * pnLen)
{
   const wchar_t *pStr;

   if( pItem && HB_IS_STRING(pItem) )
   {
      HB_SIZE nLen = hb_itemGetCLen(pItem), nDest = 0;
      const char *pszText = hb_itemGetCPtr(pItem);

      if( nLen )
      {
         nDest = MultiByteToWideChar(s_iVM_CP, 0, pszText, nLen, nullptr, 0);
      }

      if( nDest == 0 )
      {
         *phStr = ( void * ) s_wszConstStr;
         pStr = s_wszConstStr;
      }
      else
      {
         wchar_t *pResult = ( wchar_t * ) hb_xgrab((nDest + 1) * sizeof(wchar_t));

         pResult[nDest] = 0;
         nDest = MultiByteToWideChar(s_iVM_CP, 0, pszText, nLen, pResult, nDest);
         *phStr = ( void * ) pResult;
         pStr = pResult;
      }
      if( pnLen )
      {
         *pnLen = nDest;
      }
   }
   else
   {
      *phStr = nullptr;
      pStr = nullptr;
      if( pnLen )
      {
         *pnLen = 0;
      }
   }
   return pStr;
}

void hwg_wstrlenset(PHB_ITEM pItem, const wchar_t * pStr, HB_SIZE nLen)
{
   if( pItem )
   {
      HB_SIZE nDest = 0;

      if( pStr != nullptr && nLen > 0 )
      {
         nDest = WideCharToMultiByte(s_iVM_CP, 0, pStr, nLen, nullptr, 0, nullptr, nullptr);
      }

      if( nDest )
      {
         char *pResult = ( char * ) hb_xgrab(nDest + 1);

         nDest = WideCharToMultiByte(s_iVM_CP, 0, pStr, nLen, pResult, nDest, nullptr, nullptr);
         hb_itemPutCLPtr(pItem, pResult, nDest);
      }
      else
      {
         hb_itemPutC(pItem, nullptr);
      }
   }
}

PHB_ITEM hwg_wstrlenput(PHB_ITEM pItem, const wchar_t * pStr, HB_SIZE nLen)
{
   if( pItem == nullptr )
   {
      pItem = hb_itemNew(nullptr);
   }

   hwg_wstrlenset(pItem, pStr, nLen);

   return pItem;
}

PHB_ITEM hwg_wstrput(PHB_ITEM pItem, const wchar_t * pStr)
{
   return hwg_wstrlenput(pItem, pStr, pStr ? wcslen(pStr) : 0);
}

void hwg_wstrset(PHB_ITEM pItem, const wchar_t * pStr)
{
   hwg_wstrlenset(pItem, pStr, pStr ? wcslen(pStr) : 0);
}

HB_SIZE hwg_wstrcopy(PHB_ITEM pItem, wchar_t * pStr, HB_SIZE nLen)
{
   if( pItem && HB_IS_STRING(pItem) )
   {
      const char *text = hb_itemGetCPtr(pItem);
      HB_SIZE size = hb_itemGetCLen(pItem);

      if( pStr )
      {
         size = MultiByteToWideChar(s_iVM_CP, 0, text, size, pStr, nLen);
         if( size < nLen )
         {
            pStr[size] = '\0';
         }
      }
      else
      {
         size = MultiByteToWideChar(s_iVM_CP, 0, text, size, nullptr, 0);
         if( nLen && size > nLen )
         {
            size = nLen;
         }
      }
      return size;
   }
   else if( pStr && nLen )
   {
      pStr[0] = '\0';
   }

   return 0;
}

wchar_t *hwg_wstrunshare(void **phStr, const wchar_t * pStr, HB_SIZE nLen)
{
   if( pStr == nullptr || phStr == nullptr || *phStr == nullptr )
   {
      return nullptr;
   }

   if( *phStr == ( void * ) s_wszConstStr && nLen > 0 )
   {
      wchar_t *pszDest = ( wchar_t * ) hb_xgrab((nLen + 1) * sizeof(wchar_t));
      memcpy(pszDest, pStr, nLen * sizeof(wchar_t));
      pszDest[nLen] = 0;
      *phStr = ( void * ) pszDest;

      return pszDest;
   }

   return ( wchar_t * ) pStr;
}

void hwg_wstrfree(void * hString)
{
   if( hString && hString != ( void * ) s_wszConstStr )
   {
      hb_xfree(hString);
   }
}

#endif /* HB_EMULATE_STR_API */

HB_FUNC( HWG_SETUTF8 )
{
#if defined(HB_EMULATE_STR_API)
   s_iVM_CP = CP_UTF8;
#elif !defined(__XHARBOUR__)
   PHB_CODEPAGE cdp = hb_cdpFindExt("UTF8");

   if( cdp )
   {
      hb_vmSetCDP(cdp);
   }
#endif
}

HB_FUNC( HWG_EXITPROCESS )
{
   ExitProcess(0);
}

HB_FUNC( HWG_DECREASEHOLDERS )
{
/*
   PHB_ITEM pObject = hb_param(1, Harbour::Item::OBJECT);
   #ifndef  UIHOLDERS
   if( pObject->item.asArray.value->ulHolders )
      pObject->item.asArray.value->ulHolders--;
   #else
   if( pObject->item.asArray.value->uiHolders )
      pObject->item.asArray.value->uiHolders--;
   #endif
*/
   HWND hWnd = hwg_par_HWND(1);
   PHB_ITEM pObject = ( PHB_ITEM ) GetWindowLongPtr(hWnd, GWLP_USERDATA);

   if( pObject )
   {
      hb_itemRelease(pObject);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
   }
}

HB_FUNC( HWG_SETTOPMOST )
{
   hb_retl(SetWindowPos(hwg_par_HWND(1), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
}

HB_FUNC( HWG_REMOVETOPMOST )
{
   hb_retl(SetWindowPos(hwg_par_HWND(1), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
}

HB_FUNC( HWG_CHILDWINDOWFROMPOINT )
{
   HWND hWnd = hwg_par_HWND(1);
   HWND child;
   POINT pt;

   pt.x = hb_parnl(2);
   pt.y = hb_parnl(3);
   child = ChildWindowFromPoint(hWnd, pt);

   HB_RETHANDLE(child);
}

HB_FUNC( HWG_WINDOWFROMPOINT )
{
   HWND hWnd = hwg_par_HWND(1);
   HWND child;
   POINT pt;

   pt.x = hb_parnl(2);
   pt.y = hb_parnl(3);
   ClientToScreen(hWnd, &pt);
   child = WindowFromPoint(pt);

   HB_RETHANDLE(child);
}

HB_FUNC( HWG_MAKEWPARAM )
{
   WPARAM p = MAKEWPARAM((static_cast<WORD>(hb_parnl(1))), (static_cast<WORD>(hb_parnl(2))));
   hb_retnl(static_cast<LONG>(p));
}

HB_FUNC( HWG_MAKELPARAM )
{
   LPARAM p = MAKELPARAM((static_cast<WORD>(hb_parnl(1))), (static_cast<WORD>(hb_parnl(2))));
   HB_RETHANDLE(p);
}

HB_FUNC( HWG_SETWINDOWPOS )
{
   BOOL res;
   HWND hWnd = (HB_ISNUM(1) || HB_ISPOINTER(1)) ? hwg_par_HWND(1) : nullptr;
   HWND hWndInsertAfter = (HB_ISNUM(2) || HB_ISPOINTER(2)) ? hwg_par_HWND(2) : nullptr;
   int X = hb_parni(3);
   int Y = hb_parni(4);
   int cx = hb_parni(5);
   int cy = hb_parni(6);
   UINT uFlags = hb_parni(7);

   res = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

   hb_retl(res);
}

HB_FUNC( HWG_SETASTYLE )
{
#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)
#define NMAP_STYLE(src, dest) if(!(dwStyle & (src))) dwText |= (dest)

   DWORD dwStyle = hb_parnl(1), dwText = 0;

   MAP_STYLE(SS_RIGHT, DT_RIGHT);
   MAP_STYLE(SS_CENTER, DT_CENTER);
   MAP_STYLE(SS_CENTERIMAGE, DT_VCENTER | DT_SINGLELINE);
   MAP_STYLE(SS_NOPREFIX, DT_NOPREFIX);
   MAP_STYLE(SS_WORDELLIPSIS, DT_WORD_ELLIPSIS);
   MAP_STYLE(SS_ENDELLIPSIS, DT_END_ELLIPSIS);
   MAP_STYLE(SS_PATHELLIPSIS, DT_PATH_ELLIPSIS);

   NMAP_STYLE(SS_LEFTNOWORDWRAP | SS_CENTERIMAGE | SS_WORDELLIPSIS | SS_ENDELLIPSIS | SS_PATHELLIPSIS, DT_WORDBREAK);

   hb_stornl(dwStyle, 1);
   hb_stornl(dwText, 2);
}

HB_FUNC( HWG_BRINGTOTOP )
{
   HWND hWnd = hwg_par_HWND(1);
   //DWORD ForegroundThreadID;
   //DWORD    ThisThreadID;
   //DWORD      timeout;
   //BOOL Res = FALSE;
   if( IsIconic(hWnd) )
   {
      ShowWindow(hWnd, SW_RESTORE);
      hb_retl(true);
      return;
   }

   //ForegroundThreadID = GetWindowThreadProcessID(GetForegroundWindow(),nullptr);
   //ThisThreadID = GetWindowThreadPRocessId(hWnd, nullptr);
   //   if( AttachThreadInput(ThisThreadID, ForegroundThreadID, TRUE) )
   //   {

   BringWindowToTop(hWnd);    // IE 5.5 related hack
   SetForegroundWindow(hWnd);
   //      AttachThreadInput(ThisThreadID, ForegroundThreadID,FALSE);
   //      Res = (GetForegroundWindow() == hWnd);
   //   }
   //hb_retl(Res);
}

HB_FUNC( HWG_UPDATEWINDOW )
{
   HWND hWnd = hwg_par_HWND(1);
   UpdateWindow(hWnd);
}

LONG GetFontDialogUnits(HWND h, HFONT f)
{
   HFONT hFont;
   HFONT hFontOld;
   LONG avgWidth;
   HDC hDc;
   LPCTSTR tmp = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
   SIZE sz;

   HB_SYMBOL_UNUSED(f);

   //get the hdc to the main window
   hDc = GetDC(h);

   //with the current font attributes, select the font
   //hFont = f;//GetStockObject(ANSI_VAR_FONT);
   hFont = static_cast<HFONT>(GetStockObject(ANSI_VAR_FONT));
   hFontOld = static_cast<HFONT>(SelectObject(hDc, hFont));

   //get its length, then calculate the average character width

   GetTextExtentPoint32( hDc, tmp, 52, &sz );
   avgWidth = ( sz.cx / 52 );

   //re-select the previous font & delete the hDc
   SelectObject(hDc, hFontOld);
   DeleteObject(hFont);
   ReleaseDC(h, hDc);

   return avgWidth;
}

HB_FUNC( HWG_GETFONTDIALOGUNITS )
{
   hb_retnl(GetFontDialogUnits(hwg_par_HWND(1), static_cast<HFONT>(HB_PARHANDLE(2))));
}

HB_FUNC( HWG_GETTOOLBARID )
{
   HWND hMytoolMenu = hwg_par_HWND(1);
   WPARAM wp = ( WPARAM ) hb_parnl(2);
   UINT uId;

   if( SendMessage(hMytoolMenu, TB_MAPACCELERATOR, ( WPARAM ) wp, ( LPARAM ) &uId) != 0 )
   {
      hb_retnl(uId);
   }
   else
   {
      hb_retnl(-1);
   }
}

HB_FUNC( HWG_ISWINDOW )
{
   hb_retl(IsWindow(hwg_par_HWND(1)));
}

HB_FUNC( HWG_MINMAXWINDOW )
{
   MINMAXINFO *lpMMI = ( MINMAXINFO * ) HB_PARHANDLE(2);
   DWORD m_fxMin;
   DWORD m_fyMin;
   DWORD m_fxMax;
   DWORD m_fyMax;

   m_fxMin = (HB_ISNIL(3)) ? lpMMI->ptMinTrackSize.x : hb_parni(3);
   m_fyMin = (HB_ISNIL(4)) ? lpMMI->ptMinTrackSize.y : hb_parni(4);
   m_fxMax = (HB_ISNIL(5)) ? lpMMI->ptMaxTrackSize.x : hb_parni(5);
   m_fyMax = (HB_ISNIL(6)) ? lpMMI->ptMaxTrackSize.y : hb_parni(6);
   lpMMI->ptMinTrackSize.x = m_fxMin;
   lpMMI->ptMinTrackSize.y = m_fyMin;
   lpMMI->ptMaxTrackSize.x = m_fxMax;
   lpMMI->ptMaxTrackSize.y = m_fyMax;

//   SendMessage(hwg_par_HWND(1), WM_GETMINMAXINFO, 0, ( LPARAM ) lpMMI);
}

HB_FUNC( HWG_GETWINDOWPLACEMENT )
{
   HWND hWnd = hwg_par_HWND(1);
   WINDOWPLACEMENT wp;

   wp.length = sizeof(WINDOWPLACEMENT);

   if( GetWindowPlacement(hWnd, &wp) )
   {
      hb_retnl(wp.showCmd);
   }
   else
   {
      hb_retnl(-1);
   }
}

HB_FUNC( HWG_FLASHWINDOW )
{
   FlashWindow(hwg_par_HWND(1), (HB_ISNIL(2)) ? 1 : hb_parni(2));
}

HB_FUNC( HWG_ANSITOUNICODE )
{
   void * hText = ( TCHAR * ) hb_xgrab((1024 + 1) * sizeof(TCHAR));
#if !defined(__XHARBOUR__)
   hb_parstr_u16( 1, HB_CDP_ENDIAN_NATIVE, &hText, nullptr );
#else
   hwg_wstrget(hb_param(1, Harbour::Item::ANY), &hText, nullptr);
#endif
   HB_RETSTRLEN(( TCHAR * )hText, 1024);
   hb_strfree(hText);
}

HB_FUNC( HWG_CLEARKEYBOARD )
{
   s_ClearKeyboard();
}

HB_FUNC( HWG_PAINTWINDOW )
{
   PAINTSTRUCT *pps = ( PAINTSTRUCT * ) hb_xgrab(sizeof(PAINTSTRUCT));
   HDC hDC = BeginPaint(hwg_par_HWND(1), pps);
   BOOL fErase = pps->fErase;
   RECT rc = pps->rcPaint;
   HBRUSH hBrush = (HB_ISNIL(2)) ? reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1) : hwg_par_HBRUSH(2);
   if( fErase == 1 )
   {
      FillRect(hDC, &rc, hBrush);
   }

   EndPaint(hwg_par_HWND(1), pps);
   hb_xfree(pps);
}

HB_FUNC( HWG_GETBACKBRUSH )
{
   HB_RETHANDLE(GetCurrentObject(GetDC(hwg_par_HWND(1)), OBJ_BRUSH));
}

HB_FUNC( HWG_WINDOWSETRESIZE )
{
   HWND handle = hwg_par_HWND(1);
   int iResizeable = (HB_ISNIL(2))? 0 : hb_parl(2);

   if( iResizeable )
   {
      SetWindowLong(handle, GWL_STYLE, GetWindowLong( handle, GWL_STYLE ) | (WS_SIZEBOX | WS_MAXIMIZEBOX));
   }
   else
   {
      SetWindowLong(handle, GWL_STYLE, GetWindowLong( handle, GWL_STYLE ) & ~(WS_SIZEBOX | WS_MAXIMIZEBOX));
   }
}

LRESULT CALLBACK KeybHook(int code, WPARAM wp, LPARAM lp)
{

   if( (code >= 0) && (lp & 0x80000000) )
   {
      HWND h = GetActiveWindow();
      PHB_ITEM pObject = ( PHB_ITEM ) GetWindowLongPtr(h, GWLP_USERDATA);

      if( !pSym_keylist )
      {
         pSym_keylist = hb_dynsymFindName("EVALKEYLIST");
      }

      if( pObject && pSym_keylist && hb_objHasMessage(pObject, pSym_keylist) )
      {
         hb_vmPushSymbol(hb_dynsymSymbol(pSym_keylist));
         hb_vmPush(pObject);
         hb_vmPushLong(static_cast<LONG>(wp));
         hb_vmSend(1);
      }
   }

   return CallNextHookEx(nullptr, code, wp, lp);
}

HB_FUNC( HWG__ISUNICODE )
{
#ifdef UNICODE
   hb_retl(true);
#else
   hb_retl(false);
#endif
}

HB_FUNC( HWG_INITPROC )
{
   s_KeybHook = SetWindowsHookEx(WH_KEYBOARD, KeybHook, 0, GetCurrentThreadId());
}

HB_FUNC( HWG_EXITPROC )
{
   if( aDialogs )
   {
      hb_xfree(aDialogs);
   }

   if( s_KeybHook )
   {
      UnhookWindowsHookEx(s_KeybHook);
      s_KeybHook = nullptr;
   }

}

/*
   hwg_SetApplocale()
   GTK only, for WinAPI empty function body
   for compatibility purpose
*/
HB_FUNC( HWG_SETAPPLOCALE )
{
}
