/*
 * HWGUI - Harbour Win32 GUI library source code:
 * HFont class
 *
 * Copyright 2001 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#include "hbclass.ch"
#include "windows.ch"
#include "guilib.ch"

CLASS HFont INHERIT HObject

   CLASS VAR aFonts   INIT { }
   DATA handle
   DATA name, width, height , weight
   DATA charset, italic, Underline, StrikeOut
   DATA nCounter   INIT 1

   METHOD Add( fontName, nWidth, nHeight , fnWeight, fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut, nHandle )
   METHOD SELECT( oFont, nCharSet )
   METHOD RELEASE()
   METHOD SetFontStyle( lBold, nCharSet, lItalic, lUnder, lStrike, nHeight )
   METHOD PrintFont()
   METHOD Props2Arr()
   // METHOD AddC( fontName, nWidth, nHeight , fnWeight, fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut, nHandle )

ENDCLASS

METHOD Add( fontName, nWidth, nHeight , fnWeight, ;
      fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut, nHandle ) CLASS HFont

   LOCAL i, nlen := Len( ::aFonts )

   nHeight  := iif( nHeight == Nil, - 13, nHeight )
   fnWeight := iif( fnWeight == Nil, 0, fnWeight )
   fdwCharSet := iif( fdwCharSet == Nil, 0, fdwCharSet )
   fdwItalic := iif( fdwItalic == Nil, 0, fdwItalic )
   fdwUnderline := iif( fdwUnderline == Nil, 0, fdwUnderline )
   fdwStrikeOut := iif( fdwStrikeOut == Nil, 0, fdwStrikeOut )

   FOR i := 1 TO nlen
      IF ::aFonts[i]:name == fontName .AND.             ;
            ( ( Empty(::aFonts[i]:width) .AND. Empty(nWidth) ) ;
            .OR. ::aFonts[i]:width == nWidth ) .AND.    ;
            ::aFonts[i]:height == nHeight .AND.         ;
            ::aFonts[i]:weight == fnWeight .AND.        ;
            ::aFonts[i]:CharSet == fdwCharSet .AND.     ;
            ::aFonts[i]:Italic == fdwItalic .AND.       ;
            ::aFonts[i]:Underline == fdwUnderline .AND. ;
            ::aFonts[i]:StrikeOut == fdwStrikeOut

         ::aFonts[i]:nCounter ++
         IF nHandle != Nil
            hwg_Deleteobject( nHandle )
         ENDIF
         RETURN ::aFonts[i]
      ENDIF
   NEXT

   IF nHandle == Nil
      ::handle := hwg_Createfont( fontName, nWidth, nHeight , fnWeight, fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut )
   ELSE
      ::handle := nHandle
   ENDIF

   ::name      := fontName
   ::width     := nWidth
   ::height    := nHeight
   ::weight    := fnWeight
   ::CharSet   := fdwCharSet
   ::Italic    := fdwItalic
   ::Underline := fdwUnderline
   ::StrikeOut := fdwStrikeOut

   AAdd( ::aFonts, Self )

   RETURN Self

METHOD SELECT( oFont, nCharSet  ) CLASS HFont
   LOCAL af := hwg_Selectfont( oFont )

   IF af == Nil
      RETURN Nil
   ENDIF

   RETURN ::Add( af[2], af[3], af[4], af[5], iif( Empty( nCharSet ), af[6], nCharSet ), af[7], af[8], af[9], af[1] )

METHOD SetFontStyle( lBold, nCharSet, lItalic, lUnder, lStrike, nHeight ) CLASS HFont
   LOCAL  weight, Italic, Underline, StrikeOut

   IF lBold != Nil
      weight = iif( lBold, FW_BOLD, FW_REGULAR )
   ELSE
      weight := ::weight
   ENDIF
   Italic    := iif( lItalic = Nil, ::Italic, iif( lItalic, 1, 0 ) )
   Underline := iif( lUnder  = Nil, ::Underline, iif( lUnder , 1, 0 ) )
   StrikeOut := iif( lStrike = Nil, ::StrikeOut, iif( lStrike , 1, 0 ) )
   nheight   := iif( nheight = Nil, ::height, nheight )
   nCharSet  := iif( nCharSet = Nil, ::CharSet, nCharSet )

   RETURN HFont():Add( ::name, ::width, nheight, weight, ;
      nCharSet, Italic, Underline, StrikeOut ) // ::handle )

METHOD RELEASE() CLASS HFont
   LOCAL i, nlen := Len( ::aFonts )

   ::nCounter --
   IF ::nCounter == 0
#ifdef __XHARBOUR__
      FOR EACH i IN ::aFonts
         IF i:handle == ::handle
            hwg_Deleteobject( ::handle )
            ADel( ::aFonts, hb_enumindex() )
            ASize( ::aFonts, nlen - 1 )
            EXIT
         ENDIF
      NEXT
#else
      FOR i := 1 TO nlen
         IF ::aFonts[i]:handle == ::handle
            hwg_Deleteobject( ::handle )
            ADel( ::aFonts, i )
            ASize( ::aFonts, nlen - 1 )
            EXIT
         ENDIF
      NEXT
#endif
   ENDIF

   RETURN Nil

/* DF7BE: For debugging purposes */
METHOD PrintFont()  CLASS HFont
//        fontName, nWidth, nHeight , fnWeight, fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut
// Type:  C         N       N         N         N           N          N             N
// - 9999 means NIL

   LOCAL fontName , nWidth , nHeight , fnWeight , fdwCharSet , fdwItalic , fdwUnderline , fdwStrikeOut

   fontName     := iif( ::name == NIL , "<Empty>", ::name )
   nWidth       := iif( ::width == Nil, - 9999 , ::width )
   nHeight      := iif( ::height == NIL , - 9999 , ::height )
   fnWeight     := iif( ::weight == Nil, - 9999 , ::weight )
   fdwCharSet   := iif( ::CharSet == Nil, - 9999 , ::CharSet )
   fdwItalic    := iif( ::Italic == Nil, - 9999 , ::Italic )
   fdwUnderline := iif( ::Underline == Nil, - 9999 , ::Underline )
   fdwStrikeOut := iif( ::StrikeOut == Nil, - 9999 , ::StrikeOut )




RETURN "Font Name=" + fontName + " Width=" + ALLTRIM(STR(nWidth)) + " Height=" + ALLTRIM(STR(nHeight)) + ;
       " Weight=" + ALLTRIM(STR(fnWeight)) + " CharSet=" + ALLTRIM(STR(fdwCharSet)) + ;
       " Italic=" + ALLTRIM(STR(fdwItalic)) + " Underline=" + ALLTRIM(STR(fdwUnderline)) + ;
       " StrikeOut=" + ALLTRIM(STR(fdwStrikeOut))


/*
  Returns an array with font properties (for creating a copy of a font entry)
  Copy sample
   apffrarr := oFont1:Props2Arr()
   oFont2 := HFont():Add( apffrarr[1], apffrarr[2], apffrarr[3], apffrarr[4], apffrarr[5], ;
                apffrarr[6], apffrarr[7], apffrarr[8] )
 */
METHOD Props2Arr() CLASS HFont
//        fontName, nWidth, nHeight , fnWeight, fdwCharSet, fdwItalic, fdwUnderline, fdwStrikeOut
//        1         2       3         4         5           6          7             8
   LOCAL fontName , nWidth , nHeight , fnWeight , fdwCharSet , fdwItalic , fdwUnderline , fdwStrikeOut
   LOCAL aFontprops := {}

   fontName     := iif( ::name == NIL , "<Empty>", ::name )
   nWidth       := iif( ::width == Nil, - 9999 , ::width )
   nHeight      := iif( ::height == NIL , - 9999 , ::height )
   fnWeight     := iif( ::weight == Nil, - 9999 , ::weight )
   fdwCharSet   := iif( ::CharSet == Nil, - 9999 , ::CharSet )
   fdwItalic    := iif( ::Italic == Nil, - 9999 , ::Italic )
   fdwUnderline := iif( ::Underline == Nil, - 9999 , ::Underline )
   fdwStrikeOut := iif( ::StrikeOut == Nil, - 9999 , ::StrikeOut )

   AADD (aFontprops, fontName)  && C
   AADD (aFontprops, nWidth)    && all other of type N
   AADD (aFontprops, nHeight)
   AADD (aFontprops, fnWeight)
   AADD (aFontprops, fdwCharSet)
   AADD (aFontprops, fdwItalic)
   AADD (aFontprops, fdwUnderline)
   AADD (aFontprops, fdwStrikeOut)

 RETURN aFontprops