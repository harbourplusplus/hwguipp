/*
 *$Id: hcwindow.prg 2985 2021-05-05 16:02:52Z alkresin $
 *
 * HWGUI - Harbour Linux (GTK) GUI library source code:
 * HCustomWindow class
 *
 * Copyright 2004 Alexander S.Kresin <alex@kresin.ru>
 * www - http://www.kresin.ru
*/

#include "windows.ch"
#include "hbclass.ch"
#include "guilib.ch"
#include "error.ch"

   //ANNOUNCE HB_GTSYS
   REQUEST HB_GT_CGI_DEFAULT

CLASS HObject

   DATA cargo
   DATA objName

ENDCLASS

FUNCTION HB_GT_TRM
   RETURN NIL

FUNCTION HB_GT_TRM_DEFAULT
   RETURN NIL

INIT PROCEDURE HWGINIT

   hwg_gtk_init()
   Hwg_InitProc()
   hwg_ErrSys()
   SET( _SET_INSERT, .T. )

   RETURN

EXIT PROCEDURE Hwg_ExitProcedure
   Hwg_ExitProc()

   RETURN
