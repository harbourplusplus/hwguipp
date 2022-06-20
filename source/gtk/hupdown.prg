/*
 *$Id: hupdown.prg 2943 2021-02-20 17:32:38Z df7be $
 *
 * HWGUI - Harbour Linux (GTK) GUI library source code:
 * HUpDown class
 *
 * Copyright 2004 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#include "windows.ch"
#include "hbclass.ch"
#include "guilib.ch"

#ifndef UDS_SETBUDDYINT
#define UDS_SETBUDDYINT     2
#define UDS_ALIGNRIGHT      4
#endif

CLASS HUpDown INHERIT HControl

   CLASS VAR winclass   INIT "EDIT"
   DATA bSetGet
   DATA nValue
   DATA nLower INIT 0
   DATA nUpper INIT 999
   DATA nUpDownWidth INIT 12
   DATA lChanged    INIT .F.

   METHOD New( oWndParent, nId, vari, bSetGet, nStyle, nLeft, nTop, nWidth, nHeight, ;
      oFont, bInit, bSize, bPaint, bGfocus, bLfocus, ctoolt, tcolor, bcolor, nUpDWidth, nLower, nUpper )
   METHOD Activate()
   METHOD onEvent( msg, wParam, lParam )
   METHOD Refresh()
   METHOD Value( nValue ) SETGET
   METHOD SetRange( n1, n2 )  INLINE hwg_SetRangeUpdown( ::handle, n1, n2 )

ENDCLASS

METHOD New( oWndParent, nId, vari, bSetGet, nStyle, nLeft, nTop, nWidth, nHeight, ;
      oFont, bInit, bSize, bPaint, bGfocus, bLfocus, ctoolt, tcolor, bcolor,   ;
      nUpDWidth, nLower, nUpper ) CLASS HUpDown

   nStyle   := Hwg_BitOr( iif( nStyle == Nil,0,nStyle ), WS_TABSTOP )
   ::Super:New( oWndParent, nId, nStyle, nLeft, nTop, nWidth, nHeight, oFont, bInit, ;
      bSize, bPaint, ctoolt, tcolor, bcolor )

   IF Empty( vari )
      vari := 0
   ENDIF
   IF vari != Nil
      IF ValType( vari ) != "N"
         vari := 0
         Eval( bSetGet, vari )
      ENDIF
      ::title := Str( vari )
   ENDIF
   ::bSetGet := bSetGet

   IF nLower != Nil ; ::nLower := nLower ; ENDIF
   IF nUpper != Nil ; ::nUpper := nUpper ; ENDIF
   IF nUpDWidth != Nil ; ::nUpDownWidth := nUpDWidth ; ENDIF

   ::Activate()

   ::bGetFocus := bGFocus
   ::bLostFocus := bLFocus
   hwg_SetEvent( ::handle, "focus_in_event", WM_SETFOCUS, 0, 0 )
   hwg_SetEvent( ::handle, "focus_out_event", WM_KILLFOCUS, 0, 0 )

   RETURN Self

METHOD Activate() CLASS HUpDown

   IF !Empty( ::oParent:handle )
      ::handle := hwg_Createupdowncontrol( ::oParent:handle, ;
         ::nLeft, ::nTop, ::nWidth, ::nHeight, Val( ::title ), ::nLower, ::nUpper )
      hwg_Setwindowobject( ::handle, Self )
      ::Init()
   ENDIF

   RETURN Nil

METHOD onEvent( msg, wParam, lParam ) CLASS HUpDown

   * Variables not used 
   * LOCAL oParent := ::oParent
   * LOCAL nPos

   * Parameters not used
   HB_SYMBOL_UNUSED(wParam)
   HB_SYMBOL_UNUSED(lParam)

   //hwg_WriteLog( "UpDown: "+Str(msg,10)+"|"+Str(wParam,10)+"|"+Str(lParam,10) )
   IF msg == WM_SETFOCUS
      IF ::bSetGet == Nil
         IF ::bGetFocus != Nil
            Eval( ::bGetFocus, ::nValue := hwg_GetUpDown( ::handle ), Self )
         ENDIF
      ELSE
         __When( Self )
      ENDIF
   ELSEIF msg == WM_KILLFOCUS
      __Valid( Self )
   ENDIF
   RETURN 0

METHOD Refresh()  CLASS HUpDown

   * Variables not used
   * LOCAL vari

   IF ::bSetGet != Nil
      ::nValue := Eval( ::bSetGet )
      IF Str( ::nValue ) != ::title
         ::title := Str( ::nValue )
         hwg_SetUpDown( ::handle, ::nValue )
      ENDIF
   ELSE
      hwg_SetUpDown( ::handle, Val( ::title ) )
   ENDIF

   RETURN Nil

METHOD Value( nValue ) CLASS HUpDown

   IF nValue != Nil
      IF ValType( nValue ) == "N"
         hwg_SetUpdown( ::handle, nValue )
         ::nValue := nValue
         IF ::bSetGet != NIL
            Eval( ::bSetGet, nValue, Self )
         ENDIF
      ENDIF
   ELSE
      ::nValue := hwg_GetUpDown( ::handle )
   ENDIF

   RETURN ::nValue

STATIC FUNCTION __When( oCtrl )

   oCtrl:Refresh()
   IF oCtrl:bGetFocus != Nil
      RETURN Eval( oCtrl:bGetFocus, Eval( oCtrl:bSetGet ), oCtrl )
   ENDIF

   RETURN .T.

STATIC FUNCTION __Valid( oCtrl )

   oCtrl:nValue := hwg_GetUpDown( oCtrl:handle )
   IF oCtrl:bSetGet != Nil
      Eval( oCtrl:bSetGet, oCtrl:nValue )
   ENDIF
   IF oCtrl:bLostFocus != Nil .AND. !Eval( oCtrl:bLostFocus, oCtrl:nValue, oCtrl ) .OR. ;
         oCtrl:nValue > oCtrl:nUpper .OR. oCtrl:nValue < oCtrl:nLower
      hwg_Setfocus( oCtrl:handle )
   ENDIF

   RETURN .T.

* ============================ EOF of hupdown.prg ===============================
   