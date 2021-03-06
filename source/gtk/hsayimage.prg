/*
 * HWGUI - Harbour Linux (GTK) GUI library source code:
 * HSayImage class
 *
 * Copyright 2003 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#include "hbclass.ch"
#include "hwgui.ch"

   //- HSayImage

CLASS HSayImage INHERIT HControl

   CLASS VAR winclass   INIT "STATIC"
   DATA  oImage
   DATA bClick, bDblClick

   METHOD New( oWndParent, nId, nStyle, nLeft, nTop, nWidth, nHeight, bInit, ;
      bSize, ctoolt, bClick, bDblClick, bColor )
   METHOD Activate()
   METHOD END()  INLINE ( ::Super:END(), iif( ::oImage <> NIL,::oImage:Release(),::oImage := NIL ), ::oImage := NIL )

ENDCLASS

METHOD New( oWndParent, nId, nStyle, nLeft, nTop, nWidth, nHeight, bInit, ;
      bSize, ctoolt, bClick, bDblClick, bColor ) CLASS HSayImage

   ::Super:New( oWndParent, nId, nStyle, nLeft, nTop, ;
      Iif( nWidth != NIL, nWidth, 0 ), iif( nHeight != NIL, nHeight, 0 ),, ;
      bInit, bSize,, ctoolt,, bColor )

   ::title := ""

   ::bClick := bClick
   ::bDblClick := bDblClick

   RETURN Self

METHOD Activate() CLASS HSayImage

   IF !Empty(::oParent:handle)
      ::handle := hwg_Createstatic( ::oParent:handle, ::id, ;
         ::style, ::nLeft, ::nTop, ::nWidth, ::nHeight )
      ::Init()
   ENDIF

   RETURN NIL

   FUNCTION hwg_GetBitmapHeight( handle )
   LOCAL aBmpSize
   aBmpSize  := hwg_Getbitmapsize( handle )

   RETURN aBmpSize[2]

   FUNCTION hwg_GetBitmapWidth( handle )
   LOCAL aBmpSize
   aBmpSize  := hwg_Getbitmapsize( handle )

   RETURN aBmpSize[1]
