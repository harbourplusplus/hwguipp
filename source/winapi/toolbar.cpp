/*
 * HWGUI - Harbour Win32 GUI library source code:
 * C level controls functions
 *
 * Copyright 2001 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#define HB_OS_WIN_32_USED

#define OEMRESOURCE
#include "hwingui.h"
#include <commctrl.h>
#include <winuser.h>

#include "hbapiitm.h"
#include "hbvm.h"
#include "hbdate.h"
#include "hbtrace.h"

/* Suppress compiler warnings */
#include "incomp_pointer.h"
#include "warnings.h"

#ifndef CCM_SETVERSION
   #define CCM_SETVERSION (CCM_FIRST + 0x7)
#endif
#ifndef CCM_GETVERSION
   #define CCM_GETVERSION (CCM_FIRST + 0x8)
#endif
#ifndef TB_GETIMAGELIST
   #define TB_GETIMAGELIST         (WM_USER + 49)
#endif

#if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(LPNMTBGETINFOTIP)
typedef struct tagNMTBGETINFOTIPA
{
   NMHDR hdr;
   LPSTR pszText;
   int cchTextMax;
   int iItem;
   LPARAM lParam;
} NMTBGETINFOTIPA, *LPNMTBGETINFOTIPA;

typedef struct tagNMTBGETINFOTIPW
{
   NMHDR hdr;
   LPWSTR pszText;
   int cchTextMax;
   int iItem;
   LPARAM lParam;
} NMTBGETINFOTIPW, *LPNMTBGETINFOTIPW;

#ifdef UNICODE
#define LPNMTBGETINFOTIP        LPNMTBGETINFOTIPW
#else
#define LPNMTBGETINFOTIP        LPNMTBGETINFOTIPA
#endif

#endif

/*
HWG_CREATETOOLBAR(hWndParent, nID, nStyle, nX, nY, nWidth, nHeight, nExStyle) --> hToolBar
*/
HB_FUNC( HWG_CREATETOOLBAR )
{
   ULONG ulStyle = hb_parnl(3);
   ULONG ulExStyle = ((!HB_ISNIL(8)) ? hb_parnl(8) : 0) | ((ulStyle & WS_BORDER) ? WS_EX_CLIENTEDGE : 0);

   HWND hWndCtrl = CreateWindowEx(ulExStyle,   /* extended style */
         TOOLBARCLASSNAME,      /* predefined class  */
         nullptr,                  /* title   -   TBSTYLE_TRANSPARENT | */
         WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | TBSTYLE_ALTDRAG | TBSTYLE_TOOLTIPS |  TBSTYLE_WRAPABLE | CCS_TOP | CCS_NORESIZE | ulStyle, /* style  */
         hb_parni(4), hb_parni(5),  /* x, y       */
         hb_parni(6), hb_parni(7),  /* nWidth, nHeight */
         static_cast<HWND>(HB_PARHANDLE(1)),    /* parent window    */
         reinterpret_cast<HMENU>(static_cast<UINT_PTR>(hb_parni(2))),       /* control ID  */
         GetModuleHandle(nullptr),
         nullptr);

   HB_RETHANDLE(hWndCtrl);
}

/*
   hwg_Toolbaraddbuttons(handle, aItem, nLen)
   nLen : Set to Len(aItem )
*/

HB_FUNC( HWG_TOOLBARADDBUTTONS )
{
   HWND hWndCtrl = static_cast<HWND>(HB_PARHANDLE(1));
   /* HWND hToolTip = static_cast<HWND>(HB_PARHANDLE(4)); */
   PHB_ITEM pArray = hb_param(2, HB_IT_ARRAY);
   int iButtons = hb_parni(3);
   TBBUTTON * tb = static_cast<struct _TBBUTTON*>(hb_xgrab(iButtons * sizeof(TBBUTTON)));
   PHB_ITEM pTemp;

   ULONG ulID;
   DWORD style = GetWindowLong(hWndCtrl, GWL_STYLE);

   //SendMessage(hWndCtrl, CCM_SETVERSION, static_cast<WPARAM>(4), 0);

   SetWindowLongPtr(hWndCtrl, GWL_STYLE, style | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT);

   SendMessage(hWndCtrl, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0L);

   for( ULONG ulCount = 0; (ulCount < hb_arrayLen(pArray)); ulCount++ )
   {
      pTemp = hb_arrayGetItemPtr(pArray, ulCount + 1);
      ulID = hb_arrayGetNI(pTemp, 1);
      if( hb_arrayGetNI(pTemp, 4) == TBSTYLE_SEP )
      {
         tb[ulCount].iBitmap = 8;
      }
      else
      {
         tb[ulCount].iBitmap = ulID - 1; // ulID > 0 ? static_cast<int>(ulCount) : -1;
      }
      tb[ulCount].idCommand = hb_arrayGetNI(pTemp, 2);
      tb[ulCount].fsState = static_cast<BYTE>(hb_arrayGetNI(pTemp, 3));
      tb[ulCount].fsStyle = static_cast<BYTE>(hb_arrayGetNI(pTemp, 4));
      tb[ulCount].dwData = hb_arrayGetNI(pTemp, 5);
      tb[ulCount].iString = hb_arrayGetCLen(pTemp, 6) > 0 ? reinterpret_cast<INT_PTR>(hb_arrayGetCPtr(pTemp, 6)) : 0;
   }

   SendMessage(hWndCtrl, TB_ADDBUTTONS, static_cast<WPARAM>(iButtons), reinterpret_cast<LPARAM>(static_cast<LPTBBUTTON>(tb)));
   SendMessage(hWndCtrl, TB_AUTOSIZE, 0, 0);

   hb_xfree(tb);
}

HB_FUNC( HWG_TOOLBAR_SETBUTTONINFO )
{
   TBBUTTONINFO tb;
   void * hStr;

   tb.cbSize = sizeof(tb);
   tb.dwMask = TBIF_TEXT;
   tb.pszText = const_cast<LPTSTR>(HB_PARSTR(3, &hStr, nullptr));
   //tb.cchText = 1000;

   SendMessage(static_cast<HWND>(HB_PARHANDLE(1)), TB_SETBUTTONINFO, hb_parni(2), reinterpret_cast<LPARAM>(&tb));
}

HB_FUNC( HWG_TOOLBAR_LOADIMAGE )
{
   TBADDBITMAP tbab;

   tbab.hInst = nullptr;
   if( HB_ISPOINTER(2) )
   {
      tbab.nID = reinterpret_cast<UINT_PTR>(hb_parptr(2));
   }
   else
   {
      tbab.nID = static_cast<UINT_PTR>(hb_parni(2));
   }

   SendMessage(static_cast<HWND>(HB_PARHANDLE(1)), TB_ADDBITMAP, 0, reinterpret_cast<LPARAM>(&tbab));
}

HB_FUNC( HWG_TOOLBAR_LOADSTANDARTIMAGE )
{
   TBADDBITMAP tbab;
   HWND hWndCtrl = static_cast<HWND>(HB_PARHANDLE(1));
   int iIDB = hb_parni(2);
   HIMAGELIST himl;

   tbab.hInst = HINST_COMMCTRL;
   tbab.nID = iIDB; // IDB_HIST_SMALL_COLOR / IDB_VIEW_SMALL_COLOR / IDB_VIEW_SMALL_COLOR;

   SendMessage(hWndCtrl, TB_ADDBITMAP, 0, reinterpret_cast<LPARAM>(&tbab));
   himl = reinterpret_cast<HIMAGELIST>(SendMessage(hWndCtrl, TB_GETIMAGELIST, 0, 0));
   hb_retni(static_cast<int>(ImageList_GetImageCount(himl)));
}

HB_FUNC( HWG_IMAGELIST_GETIMAGECOUNT )
{
   hb_retni(ImageList_GetImageCount(static_cast<HIMAGELIST>(HB_PARHANDLE(1))));
}

HB_FUNC( HWG_TOOLBAR_SETDISPINFO )
{
   //LPTOOLTIPTEXT pDispInfo = static_cast<LPTOOLTIPTEXT>(HB_PARHANDLE(1));
   LPNMTTDISPINFO pDispInfo = static_cast<LPNMTTDISPINFO>(HB_PARHANDLE(1));

   if( pDispInfo )
   {
      HB_ITEMCOPYSTR(hb_param(2, HB_IT_ANY), pDispInfo->szText, HB_SIZEOFARRAY(pDispInfo->szText));
      pDispInfo->szText[HB_SIZEOFARRAY(pDispInfo->szText) - 1] = 0;
#if 0
      /* is it necessary? */
      if( !pDispInfo->hinst )
      {
         pDispInfo->lpszText = pDispInfo->szText;
      }
#endif
   }
}

HB_FUNC( HWG_TOOLBAR_GETDISPINFOID )
{
   //LPTOOLTIPTEXT pDispInfo = static_cast<LPTOOLTIPTEXT>(hb_parnl(1));
   LPNMTTDISPINFO pDispInfo = static_cast<LPNMTTDISPINFO>(HB_PARHANDLE(1));
   DWORD idButton = pDispInfo->hdr.idFrom;
   hb_retnl(idButton);
}

HB_FUNC( HWG_TOOLBAR_GETINFOTIP )
{
   LPNMTBGETINFOTIP pDispInfo = static_cast<LPNMTBGETINFOTIP>(HB_PARHANDLE(1));
   if( pDispInfo && pDispInfo->cchTextMax > 0 )
   {
      HB_ITEMCOPYSTR(hb_param(2, HB_IT_ANY), pDispInfo->pszText, pDispInfo->cchTextMax);
      pDispInfo->pszText[pDispInfo->cchTextMax - 1] = 0;
   }
}

HB_FUNC( HWG_TOOLBAR_GETINFOTIPID )
{
   LPNMTBGETINFOTIP pDispInfo = static_cast<LPNMTBGETINFOTIP>(HB_PARHANDLE(1));
   DWORD idButton = pDispInfo->iItem;
   hb_retnl(idButton);
}

HB_FUNC( HWG_TOOLBAR_IDCLICK )
{
   LPNMMOUSE pDispInfo = static_cast<LPNMMOUSE>(HB_PARHANDLE(1));
   DWORD idButton = pDispInfo->dwItemSpec;
   hb_retnl(idButton);
}

HB_FUNC( HWG_TOOLBAR_SUBMENU )
{
   LPNMTOOLBAR lpnmTB = static_cast<LPNMTOOLBAR>(HB_PARHANDLE(1));
   RECT rc = { 0, 0, 0, 0 };
   TPMPARAMS tpm;
   HMENU hPopupMenu;
   HMENU hMenuLoaded;
   HWND g_hwndMain = static_cast<HWND>(HB_PARHANDLE(3));
   HANDLE g_hinst = GetModuleHandle(0);

   SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, static_cast<WPARAM>(lpnmTB->iItem), reinterpret_cast<LPARAM>(&rc));

   MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, static_cast<LPPOINT>(static_cast<void*>(&rc)), 2);

   tpm.cbSize = sizeof(TPMPARAMS);
   // tpm.rcExclude = rc;
   tpm.rcExclude.left = rc.left;
   tpm.rcExclude.top = rc.top;
   tpm.rcExclude.bottom = rc.bottom;
   tpm.rcExclude.right = rc.right;
   hMenuLoaded = LoadMenu(static_cast<HINSTANCE>(g_hinst), MAKEINTRESOURCE(hb_parni(2)));
   hPopupMenu = GetSubMenu(LoadMenu(static_cast<HINSTANCE>(g_hinst), MAKEINTRESOURCE(hb_parni(2))), 0);

   TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, g_hwndMain, &tpm);
   //rc.left, rc.bottom, g_hwndMain, &tpm);

   DestroyMenu(hMenuLoaded);
}

HB_FUNC( HWG_TOOLBAR_SUBMENUEX )
{
   LPNMTOOLBAR lpnmTB = static_cast<LPNMTOOLBAR>(HB_PARHANDLE(1));
   RECT rc = { 0, 0, 0, 0 };
   TPMPARAMS tpm;
   HMENU hPopupMenu = static_cast<HMENU>(HB_PARHANDLE(2));
   HWND g_hwndMain = static_cast<HWND>(HB_PARHANDLE(3));

   SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, static_cast<WPARAM>(lpnmTB->iItem), reinterpret_cast<LPARAM>(&rc));

   MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, static_cast<LPPOINT>(static_cast<void*>(&rc)), 2);

   tpm.cbSize = sizeof(TPMPARAMS);
   //tpm.rcExclude = rc;
   tpm.rcExclude.left = rc.left;
   tpm.rcExclude.top = rc.top;
   tpm.rcExclude.bottom = rc.bottom;
   tpm.rcExclude.right = rc.right;
   TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, g_hwndMain, &tpm);
   //rc.left, rc.bottom, g_hwndMain, &tpm);
}

HB_FUNC( HWG_TOOLBAR_SUBMENUEXGETID )
{
   LPNMTOOLBAR lpnmTB = static_cast<LPNMTOOLBAR>(HB_PARHANDLE(1));
   hb_retnl(static_cast<LONG>(lpnmTB->iItem));
}
