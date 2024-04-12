/*********************************************************************/
/*                                                                   */
/*      ????????????? ???????? ?? ? Ethereum ?? RPC-?????????        */
/*                                                                   */
/*                          ?????? "??????"                          */
/*                                                                   */
/*********************************************************************/

#ifdef UNIX
#include <unistd.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <windows.h>
#include <commctrl.h>
#include <io.h>
#include <direct.h>
#include <sys\timeb.h>
#include <sys\types.h>
#include <sys\stat.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#ifndef UNIX
#include "controls.h"
#include "resource.h"
#endif

#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4267)

/*----------------------------------------- ????????????? ?????????? */

  struct Elem_pos_list_ {
                           int  elem ;
                           int  x ;
                           int  y ;
                           int  xs ;
                           int  ys ;
                        }  ;

 typedef struct Elem_pos_list_ Elem_pos_list ;

/*----------------------------------------------- ?????? ?? ???????? */

#ifndef  UNIX 

  static  HBITMAP  picMarkUn ;
  static  HBITMAP  picMarkWr ;
  static  HBITMAP  picMarkEr ;

#endif

/*-------------------------------------- ????? ?????????? ?????????? */

/*------------------------------------ ??????????? ????????? ??????? */

#ifndef  UNIX 

  union WndProc_par {
                        long            par ;
                     LRESULT (CALLBACK *call)(HWND, UINT, WPARAM, LPARAM) ; 
                    } ;

  static union WndProc_par  Tmp_WndProc ;
//  static union WndProc_par  ConValues_WndProc ;

//     LRESULT CALLBACK  EMIRi_ConValues_WndProc(HWND, UINT, WPARAM, LPARAM) ;

#endif

/*------------------------------------------ ?????????? ???????????? */

  void  EMIRi_dl_synch          (HWND hDlg, char *prefix, SQL_link *db) ;               /* ????????? ??????? ????????????? ???????? ?????? "??????" */
  void  EMIRi_dl_actions        (HWND hDlg, char *prefix, SQL_link *db) ;               /* ????????? ??????? ???????? ?????? "??????" */
   int  EMIR_dl_action_AddDeal  (Dl_action *action, SQL_link *db, char *error) ;        /* ???????? ADD_DEAL */
   int  EMIR_dl_action_SetStatus(Dl_action *action, SQL_link *db, char *error) ;        /* ???????? SET_STATUS */
   int  EMIR_dl_action_GetDeal  (Dl_action *action, SQL_link *db, char *error) ;        /* ???????? GET_DEAL */
   int  EMIRi_dl_GetDeal        (char *contract, Deal *deal,                            /* ?????? ??????? Get... ?????-????????? Deal... */
                                            DealParty *parties,
                                                  int  parties_max,
                                                  int *parties_cnt,
                                             DealFile *files,
                                                  int  files_max,
                                                  int *files_cnt,
                                          DealHistory *history,
                                                  int  history_max,
                                                  int *history_cnt, char *error) ;
   int  EMIRi_dl_CheckBox       (char *box, char *contract, char *error) ;              /* ?????? ??????? CheckContract ?????-????????? Box */
   int  EMIRi_dl_GetBox         (char *contract, Deal **list, char *error) ;            /* ?????? ?????a GetContracts ?????-????????? Box */
   int  EMIRi_dl_AccessBox      (char *box, char *contract, char *error) ;              /* ?????? ??????? CheckBank ?????-????????? Box */

   int  EMIRi_dl_GetDealId      (Dl_action *action) ;                                   /* ?????????? ?????? ?????? ?? ???????? */


/*********************************************************************/
/*								     */
/*	      ?????????? ????????? ??????????? ???? DEALS            */	

   INT_PTR CALLBACK  EMIR_deals_dialog(  HWND  hDlg,     UINT  Msg,
 		                       WPARAM  wParam, LPARAM  lParam) 
{
#ifndef  UNIX 

       static  int  start_flag=1 ;  /* ???? ??????? */
              HWND  hPrn ;
              RECT  wr ;            /* ??????? ???? */
	       int  x_screen ;      /* ??????? ?????? */	
               int  y_screen ;
               int  x_shift ;       /* ?????????? ????? */	
               int  y_shift ;
              RECT  Rect_base ;
              RECT  Rect_real ;
              RECT  Rect ;
               int  x0_corr ;
               int  y0_corr ;
               int  x1_corr ;
               int  y1_corr ;
               int  dx ;
               int  dy ;
               int  x_size ;
               int  y_size ;
               int  x ;
               int  y ;
               int  xs ;
               int  ys ;
               int  elm ;            /* ????????????? ???????? ??????? */
               int  sts ;
               int  status ;
               int  reply ;
               int  i ; 

                   char  loc_pos_ptr[32] ;
          Elem_pos_list *loc_pos ;
   static Elem_pos_list  loc_pos_e[]={ {IDC_NODE_URL,        0, 0, 1, 0},
                                       {IDC_OPERATION,       0, 0, 1, 0},
                                       {IDC_LOG,             0, 0, 1, 1},
                                       {IDC_DETAILS,         0, 1, 1, 0},
                                       {IDC_SECTIONS_SWITCH, 0, 1, 1, 0},
                                       {IDC_VERSION,         1, 1, 0, 0},
                                       {0}                               } ;

     static  HFONT  font ;         /* ????? */
            TCITEM  tab_item ;
             NMHDR *hNM ;
              char  value[1024] ;
               int  row ;

#pragma warning(disable : 4244)

/*------------------------------------------------- ??????? ???????? */

  switch(Msg) {

/*---------------------------------------------------- ????????????? */

    case WM_INITDIALOG: {
/*- - - - - - - - - - - - - - - - ?????????? ??????? ?????.????????? */
             loc_pos=(Elem_pos_list *)calloc(1, sizeof(loc_pos_e)) ;
      memcpy(loc_pos, loc_pos_e, sizeof(loc_pos_e)) ;

          sprintf(loc_pos_ptr, "%p", loc_pos) ;                     /* ????????? ?????? ? ???????? ???? */
             SETs(IDC_ELEM_LIST, loc_pos_ptr) ;
/*- - - - - - - - - - - - - - - - - - ???? ??????????????? ????????? */
                GetWindowRect(          hDlg,    &Rect_base) ;
                GetWindowRect(ITEM(IDC_TESTPOS), &Rect     ) ;

                      dx     =-(Rect_base.right -Rect.left) ;
                      dy     =-(Rect_base.bottom-Rect.top ) ;
                       x_size=  Rect_base.right -Rect_base.left ;
                       y_size=  Rect_base.bottom-Rect_base.top ;

                 SetWindowPos(ITEM(IDC_TESTPOS), 0,
                               x_size+dx, y_size+dy, 0, 0, 
                                SWP_NOSIZE | SWP_NOZORDER) ;
                GetWindowRect(ITEM(IDC_TESTPOS), &Rect_real) ;

                        x1_corr=Rect.left-Rect_real.left ;
                        y1_corr=Rect.top -Rect_real.top ;

                             dx=Rect.left-Rect_base.left ;
                             dy=Rect.top -Rect_base.top  ;

                 SetWindowPos(ITEM(IDC_TESTPOS), 0,
                                      dx, dy, 0, 0, 
                                SWP_NOSIZE | SWP_NOZORDER) ;
                GetWindowRect(ITEM(IDC_TESTPOS), &Rect_real) ;
                        x0_corr=Rect.left-Rect_real.left ;
                        y0_corr=Rect.top -Rect_real.top ;
/*- - - - - - - - - - - - -  ???????? ????????? ? ???????? ????????? */
     for(i=0 ; loc_pos[i].elem ; i++) {

                GetWindowRect(ITEM(loc_pos[i].elem), &Rect) ;

        if(loc_pos[i].x )  loc_pos[i].x =Rect.left-Rect_base.right+x1_corr ;
        else               loc_pos[i].x =Rect.left-Rect_base.left +x0_corr ;

        if(loc_pos[i].y )  loc_pos[i].y =Rect.top-Rect_base.bottom+y1_corr ;
        else               loc_pos[i].y =Rect.top-Rect_base.top   +y0_corr ;

        if(loc_pos[i].xs)  loc_pos[i].xs= (     Rect.right-Rect.left     ) 
                                         -(Rect_base.right-Rect_base.left) ;
        else               loc_pos[i].xs=Rect.right-Rect.left ;

        if(loc_pos[i].ys)  loc_pos[i].ys= (     Rect.bottom-Rect.top     ) 
                                         -(Rect_base.bottom-Rect_base.top) ;
        else               loc_pos[i].ys=       Rect.bottom-Rect.top ;
                                      }
/*- - - - - - - - - - - - - - - - - - - ????????????? ????????? ???? */
       x_screen=GetSystemMetrics(SM_CXSCREEN) ;
       y_screen=GetSystemMetrics(SM_CYSCREEN) ;

           hPrn=GetParent( hDlg) ;
            GetWindowRect( hDlg, &wr) ;

        x_shift=(x_screen-(wr.right-wr.left+1))/2 ;
        y_shift=(y_screen-(wr.bottom-wr.top+1))/2 ;

         AdjustWindowRect(&wr, GetWindowLong(hPrn, GWL_STYLE), false) ;
               MoveWindow( hPrn,  x_shift,
                                  y_shift,
                                 wr.right-wr.left+1,
                                 wr.bottom-wr.top+1, true) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  ??????? ??????? */
        if(font==NULL)
           font=CreateFont(14, 0, 0, 0, FW_THIN, 
                                 false, false, false,
                                  ANSI_CHARSET,
                                   OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                      VARIABLE_PITCH,
                                       "Courier New Cyr") ;
//         SendMessage(ITEM(IDC_REQUEST), WM_SETFONT, (WPARAM)font, 0) ;
/*- - - - - - - - - - - - - - - - ????????????? ????????????? ?????? */
       for(i=0 ; i<__sections_cnt ; i++) {
                        tab_item.mask   =TCIF_TEXT ;
                        tab_item.pszText=__sections[i].title ;
                        tab_item.lParam =           i ;
         TabCtrl_InsertItem(ITEM(IDC_SECTIONS_SWITCH), i, &tab_item) ;
                                         }
/*- - - - - - - - - - - - - - - - -  ????????????? ??????? ????????? */
                    SETs(IDC_NODE_URL, __node_url) ;
                    SETs(IDC_VERSION,  _VERSION   ) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - ?????? ? ????????? */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  			  return(FALSE) ;
  			     break ;
  			}
/*-------------------------------------- ????????? ????????? ??????? */

    case WM_SIZE: {
                         GETs(IDC_ELEM_LIST, loc_pos_ptr) ;         /* ????????? ?????? ???????????????? */ 
                       sscanf(loc_pos_ptr, "%p", &loc_pos) ;

                GetWindowRect(hDlg, &Rect_base) ;

                       x_size=Rect_base.right -Rect_base.left ;
                       y_size=Rect_base.bottom-Rect_base.top ;

     for(i=0 ; loc_pos[i].elem ; i++) {

        if(loc_pos[i].x<0)  x =x_size+loc_pos[i].x ;
        else                x =       loc_pos[i].x ;
        
        if(loc_pos[i].y<0)  y =y_size+loc_pos[i].y ;
        else                y =       loc_pos[i].y ;

        if(loc_pos[i].xs<0) xs=x_size+loc_pos[i].xs ;
        else                xs=       loc_pos[i].xs ;

        if(loc_pos[i].ys<0) ys=y_size+loc_pos[i].ys ;
        else                ys=       loc_pos[i].ys ;

           SetWindowPos(ITEM(loc_pos[i].elem),  0,
                                x, y, xs, ys, 
                                  SWP_NOZORDER | SWP_NOCOPYBITS) ;
                                      }

			  return(FALSE) ;
  			     break ;
  		  }
/*----------------------------------------------- ????????? ???????? */

    case WM_NOTIFY:    {
                           elm=  LOWORD(wParam) ;
	                   hNM=(NMHDR *)lParam ;
                        status=hNM->code ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - ????? ?????? */
       if(   elm==IDC_SECTIONS_SWITCH &&
          status==TCN_SELCHANGE         ) {

                    __sec_change_time=time(NULL) ;

            row=TabCtrl_GetCurSel(ITEM(IDC_SECTIONS_SWITCH)) ;
         if(row!=__sec_work) {
                    ShowWindow(__sections[  row     ].hWnd, SW_RESTORE) ;
                    ShowWindow(__sections[__sec_work].hWnd, SW_HIDE) ;
                                          __sec_work=row ;
                   SendMessage(__sections[  row     ].hWnd, WM_USER,
                                (WPARAM)_USER_SECTION_ENABLE, NULL) ;
                             }

               TabCtrl_SetCurSel(GetDlgItem(__sections[__sec_work].hWnd, 
                                                   IDC_SECTIONS_SWITCH), __sec_work) ;

                                              return(FALSE) ;
                                          }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                              return(FALSE) ;
                       } 
/*------------------------------------ ????????? ?????????? ???????? */

    case WM_USER:  {
/*- - - - - - - - - - - - - - - - - - - - - - - - ??????????? ?????? */
        if(wParam==_USER_SECTION_ENABLE) {
                                             __dialog=hDlg ;

                                                 return(FALSE) ;
                                         }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			  return(FALSE) ;
  			     break ;
  		   }

/*------------------------------------------------ ????????? ??????? */

    case WM_COMMAND: {

                   sts=HIWORD(wParam) ;
	           elm=LOWORD(wParam) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - ????????? ?????? */
       if(elm==IDC_LOG    &&
          sts==LBN_DBLCLK   ) {

                         row=LB_GET_ROW (IDC_LOG) ;
                      if(row==LB_ERR)  break ;

                             LB_GET_TEXT(IDC_LOG, row, value) ;
                                    SETs(IDC_DETAILS, value) ;

                                            return(FALSE) ;
                              } 
/*- - - - - - - - - - - - - - - - - - - - - - - - - ????????? ?????? */
       if(elm==IDC_TERMINATE) {

           reply=MessageBox(hDlg, "?? ????????????? ?????? ????????? ??????????",
                                    "", MB_YESNO | MB_ICONQUESTION) ;
        if(reply==IDNO)  return(FALSE) ;

			                EndDialog(hDlg, TRUE) ;
  			          PostQuitMessage(0) ;  

                                   __exit_flag=1 ;
                              }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			  return(FALSE) ;
			     break ;
	             }
/*--------------------------------------------------------- ???????? */

    case WM_CLOSE:      {
/*- - - - - - - - - - - - - - - - - - - - - -  ???????????? ???????? */
                             GETs(IDC_ELEM_LIST, loc_pos_ptr) ;     /* ????????? ?????? ???????????????? */ 
                           sscanf(loc_pos_ptr, "%p", &loc_pos) ;
                             free(loc_pos) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			      return(FALSE) ;
			          break ;
			}
/*----------------------------------------------------------- ?????? */

//  case WM_PAINT:    break ;

    default :        {
			  return(FALSE) ;
			    break ;
		     }
/*-------------------------------------------------------------------*/
	      }
/*-------------------------------------------------------------------*/

    return(TRUE) ;

#pragma warning(default : 4244)

#endif
}


/********************************************************************/
/*                                                                  */
/*            THREAD - ??????? ????? ?????? "??????"                */

  DWORD WINAPI  Deals_Thread(LPVOID Pars)

{
#ifndef  UNIX 

   SQL_ODBC_link  DB ;   /* ?????????? ??-????????? ??? DCL-??????? */
            HWND  hDlg ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;
          time_t  time_0 ;
             int  rows_cnt ;
             int  status ;
             int  i ;

#pragma warning(disable : 4244)

/*---------------------------------------------------- ????????????? */

               time_0=0 ;

      if(__dl_request_period<=0)  __dl_request_period= 10 ;
      if(__dl_view_frame    <=0)  __dl_view_frame    =100 ;

              hDlg=hDeals_Dialog ;

/*----------------------------------------------------- ????????? ?? */

/*------------------------------------------------------- ????? ???? */

   do {
          if(__exit_flag)  break ;                                  /* ???? ????? ???? */ 

                      Sleep(1000) ;

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - ??????? ???? */
            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__dl_view_frame) {

               for(i=0 ; i<rows_cnt-__dl_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
                                      }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - -  ??????? */
               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%02d.%02d %02d:%02d:%02d ",
                                    hhmmss->tm_mday,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec  ) ;

            sprintf(text, "%s - Pulse start", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  ?????????? ? ?? */
     if(!DB.connected) {

         status=DB.Connect(__db_user, __db_password,
                           __db_name, "ODBC", NULL  ) ; 
      if(status) {
                      sprintf(text, "%s - ERROR - DB connection: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }

                DB.SetAutoCommit(0) ;

         status=DB.AllocCursors(3) ; 
      if(status) {
                      sprintf(text, "%s - ERROR - DB cursors allocation: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }
                       }
/*- - - - - - - - - - - - - - - - -  ????????? ??????? ????????????? */
          if(time_abs-time_0 > __dl_request_period) {               /* ???? ????? ?? ???????... */

//             EMIRi_dl_synch(hDeals_Dialog, prefix, &DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - ????????? ????? ???????? */
             EMIRi_dl_actions(hDeals_Dialog, prefix, &DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

                DB.Disconnect() ;                                   /* ???????????? ? ?? */

            sprintf(text, "%s - Pulse done", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;

      } while(1) ;

/*-------------------------------------------------------------------*/
                                    
  return(0) ;

#pragma warning(default : 4244)

#endif
}


/********************************************************************/
/*                                                                  */
/*           ?????????? ???????? ?????? ?????? "??????"             */

   void  Deals_Process(SQL_link *DB)

{
   static time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;

#ifndef  UNIX 
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

#pragma warning(disable : 4244)

/*---------------------------------------------------- ????????????? */

      if(__net_locked)  return ;

      if(__dl_request_period<=0)  __dl_request_period= 10 ;
      if(__dl_view_frame    <=0)  __dl_view_frame    =100 ;

#ifndef  UNIX 
              hDlg=hDeals_Dialog ;
#endif

/*------------------------------------------------------- ????? ???? */

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - ??????? ???? */
#ifndef  UNIX 

            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__dl_view_frame) {

               for(i=0 ; i<rows_cnt-__dl_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
                                      }

#endif
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - -  ??????? */
               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%02d.%02d %02d:%02d:%02d ",
                                    hhmmss->tm_mday,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec  ) ;

            sprintf(text, "  Deals> %s - Pulse start", prefix) ;
#ifdef UNIX
           EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
#endif

/*- - - - - - - - - - - - - - - - -  ????????? ??????? ????????????? */
          if(time_abs-time_0 > __dl_request_period) {               /* ???? ????? ?? ???????... */

               EMIRi_dl_synch(hDeals_Dialog, prefix, DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - ????????? ????? ???????? */
             EMIRi_dl_actions(hDeals_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

            sprintf(text, "  Deals> %s - Pulse stop", prefix) ;
#ifdef UNIX
           EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif

/*-------------------------------------------------------------------*/
                                    
  return ;

#pragma warning(default : 4244)
}


/********************************************************************/
/*                                                                  */
/*              ????????? ??????? ????????????? ????????            */
/*                     ?????? "??????"                              */

   void  EMIRi_dl_synch(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
            char  box_addr[128] ;
    static  Deal *box_list ;
    static   int  box_list_max ;
             int  box_list_cnt ;
    static  Deal *tbl_list ;
    static   int  tbl_list_max ;
             int  tbl_list_cnt ;
             int  clear_flag ;
             int  status ;
            char  event_id[128] ;
            char  event_topic[1024] ;
            char  event_data[1024] ;
            char  result[1024] ;
            char  text[1024] ;
            char  error[1024] ;
             int  i ;
             int  j ;

/*--------------------------------------------------- ?????? ??????? */

        Cursor=db->LockCursor("EMIRi_dl_synch") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_dl_synch - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                                return ;
                      }
/*------------------------------- ???????? ??????? ???????? ???????? */

                      sprintf(text, "select count(*) "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','ERROR','HOLD')",
                                     __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Check active operations: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Check active operations: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

     if(result[0]!='0') {                                           /* ???? ???? ???????? ????????... */
                                 sprintf(text, "Active operations is detected...") ;
                              LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                                   return ;
                        }
/*--------------------------------- ?????? ?????? ?? "???????? ????" */

                      sprintf(text, "select \"MemberBox\" "
                                    "from   %s", __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get 'MemberBox': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get 'MemberBox': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

            memset(box_addr, 0, sizeof(box_addr)) ;
           strncpy(box_addr, (char *)Cursor->columns[0].value, sizeof(box_addr)-1) ;

           sprintf(text, "%s - Box address: %s", prefix, box_addr) ;
        LB_ADD_ROW(IDC_LOG, text) ;

                 db->SelectClose(Cursor) ;

/*----------------------- ???????????? ?????? ?????????? ?? ???????? */

   if(EMIR_active_section("EVENTS")) {
/*- - - - - - - - - - - - - - - - - ?????? ??????? "????????? ?????" */
                      sprintf(text, "Select \"Id\", \"Topic\", \"Data\" "
                                    "From   %s "
                                    "Where  \"Address\"='%s' "
                                    "Order by \"Id\""   ,
                                     __db_table_scan_events, box_addr) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

                box_list_cnt=0 ;
                  clear_flag=0 ;

   for(i=0 ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

           strncpy(event_id,    (char *)Cursor->columns[0].value, sizeof(event_id   )-1) ;
           strncpy(event_topic, (char *)Cursor->columns[1].value, sizeof(event_topic)-1) ;
           strncpy(event_data , (char *)Cursor->columns[2].value, sizeof(event_data )-1) ;

            strcpy(event_data, event_data+24) ;

EMIR_log("Box event detected") ;
EMIR_log(event_topic) ;
EMIR_log(event_data) ;

      if(i>=box_list_max) {
                                    box_list_max+=10 ;
           box_list=(Deal *)realloc(box_list, box_list_max*sizeof(*box_list)) ;
                          } 

     if(!memicmp(event_topic, "a226", 4)) {                         /* ?????????? ????????? */

           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag= 1 ;
                                          }
     else
     if(!memicmp(event_topic, "745d", 4)) {                         /* ??????? ?/? ??? ???????? ????????? */

       if(!stricmp(event_data,                                      /* ??????? ?/? */ 
                    "0000000000000000000000000000000000000000")) {

                                          clear_flag=1 ;
                                             break ;
                                                                 }
       else                                                      {  /* ???????? ????????? */

           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag=-1 ;
                                                                 }
                                          }

                    }

                 db->SelectClose (Cursor) ;

         box_list_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - -  ??????? "????????? ?????" */
     if(clear_flag) {

EMIR_log("Box clear") ;

                        sprintf(text, "update %s set \"DataUpdate\"='D'",
                                        __db_table_deals) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Delete All Deals : %s", prefix, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                         return ;
                  }

                        sprintf(text, "Delete from %s "
                                      "Where  \"Address\"='%s' "
                                      " and   \"Id\"<=%s"   ,
                                     __db_table_scan_events, box_addr, event_id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Delete All Deals : %s", prefix, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                         return ;
                  }

                           return ;
                    }
/*- - - - - - - - - - - - - - - - - - - ????????? ??????? ?????????? */
                      sprintf(text, "Select e.\"Id\", \"Address\", \"Topic\" "
                                    "From   %s e, %s d "
                                    "Where  e.\"Address\"=d.\"BlockChainId\" "
                                    "Order by \"Id\"" ,
                                     __db_table_scan_events, __db_table_deals) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Contracts events : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

   for( ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

           strncpy(event_id,    (char *)Cursor->columns[0].value, sizeof(event_id   )-1) ;
           strncpy(event_data , (char *)Cursor->columns[1].value, sizeof(event_data )-1) ;
           strncpy(event_topic, (char *)Cursor->columns[2].value, sizeof(event_topic)-1) ;

EMIR_log("Contract event detected") ;
EMIR_log(event_topic) ;
EMIR_log(event_data) ;

      if(i>=box_list_max) {
                                    box_list_max+=10 ;
           box_list=(Deal *)realloc(box_list, box_list_max*sizeof(*box_list)) ;
                          } 

     if(!memicmp(event_topic, "0b27", 4)) {                         /* ????????? ????????? */

           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag= 1 ;
                                          }
                 }

                 db->SelectClose (Cursor) ;

         box_list_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*---- ???????????? ?????? ?????????? ?? ????????? "????????? ?????" */

   else                              {
/*- - - - - - - - - - - - - - - ?????? ??????????? "????????? ?????" */
        box_list_cnt=EMIRi_dl_GetBox(box_addr, &box_list, error) ;
     if(box_list_cnt<0) {
                            sprintf(text, "%s - ERROR - EMIRi_dl_synch - GetContracts : %s", prefix, error) ;
                         LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                        }

      for(i=0 ; i<box_list_cnt ; i++) {

            status=EMIRi_dl_GetDeal(box_list[i].address, &box_list[i], NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, error) ;
         if(status<0) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Contract status : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                    continue ;
                      }

                         sprintf(text, "Deal %s - %s : %s", box_list[i].address, box_list[i].status, box_list[i].version) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                      }
/*- - - - - - - - - - - - - - - - - - - - - -  ?????? ??????? ?????? */
                      sprintf(text, "Select \"Id\", \"BlockChainId\", \"Version\" "
                                    "From   %s "
                                    "Where  \"BlockChainId\" is not null and \"DataUpdate\"<>'D' ",
                                     __db_table_deals) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Deals list : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

   for(i=0 ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Get Deals list : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              return ;
                }

     if(i>=tbl_list_max) {
                                    tbl_list_max+=10 ;
           tbl_list=(Deal *)realloc(tbl_list, tbl_list_max*sizeof(*tbl_list)) ;
                         } 

           strncpy(tbl_list[i].id,      (char *)Cursor->columns[0].value, sizeof(tbl_list[i].id     )-1) ;
           strncpy(tbl_list[i].address, (char *)Cursor->columns[1].value, sizeof(tbl_list[i].address)-1) ;
           strncpy(tbl_list[i].version, (char *)Cursor->columns[2].value, sizeof(tbl_list[i].version)-1) ;
                    }

                 db->SelectClose (Cursor) ;

         tbl_list_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - ?????? ??????? ??????? */
     for(i=0 ; i<box_list_cnt ; i++)  box_list[i].flag=1 ;
     for(i=0 ; i<tbl_list_cnt ; i++)  tbl_list[i].flag=1 ;

   for(i=0 ; i<box_list_cnt ; i++) 
   for(j=0 ; j<tbl_list_cnt ; j++)
     if(!stricmp(box_list[i].address,
                 tbl_list[j].address )) {

                                            tbl_list[j].flag=0 ;
        if(!stricmp(box_list[i].version,
                    tbl_list[j].version ))  box_list[i].flag=0 ; 
                                                 break ;
                                         }
/*- - - - - - - - - - - - - - - - - - - - - ??????? ????????? ?????? */
   for(i=0 ; i<tbl_list_cnt ; i++)
     if(tbl_list[i].flag) {

                        sprintf(text, "Deal %s - DELETE", tbl_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                        sprintf(text, "update %s set \"DataUpdate\"='D' where \"Id\"=%s",
                                        __db_table_deals, tbl_list[i].id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Delete Deal (Id=%s) : %s", prefix, tbl_list[i].id, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                  }
                            }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*------------------------------ ?????? ??????????/?????????? ?????? */

   for(i=0 ; i<box_list_cnt ; i++)
     if(box_list[i].flag==1) {

                        sprintf(text, "Deal %s - ADD/UPDATE", box_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - - ???????? ???????????? ???????? */
                      sprintf(text, "select count(*) "
                                    "from   %s "
                                    "where  \"Object\"='%s'"
                                    " and   \"Status\" not in ('DONE','ERROR','HOLD')",
                                     __db_table_deals_actions, box_list[i].address) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Check operations for %s: %s", prefix, box_list[i].address, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_synch - Check operations for %s: %s", prefix, box_list[i].address, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

       if(result[0]!='0') {
                                 sprintf(text, "Active operations is detected...") ;
                              LB_ADD_ROW(IDC_LOG, text) ;
                                 continue ;
                          }
/*- - - - - - - - - - - - ??????????? ???????? ??????????/?????????? */
                        sprintf(text, "insert into %s (\"Action\",  \"Object\", \"Status\")"
                                      "         values('GetDeal', '%s',      'NEW')"  ,
                                        __db_table_deals_actions, box_list[i].address) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Registry Add/Update operation (Address=%s) : %s", prefix, box_list[i].address, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                             }
/*----------------------------------------- ??????? ???????? ?????? */

   for(i=0 ; i<box_list_cnt ; i++)
     if(box_list[i].flag==-1) {

                        sprintf(text, "Deal %s - DELETE", box_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                        sprintf(text, "update %s set \"DataUpdate\"='D' where \"BlockChainId\"=%s",
                                        __db_table_deals, box_list[i].address) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Delete Deal (BlockChainId=%s) : %s", prefix, box_list[i].address, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                  }
                              }
/*------------------------------------ ???????? ???????????? ??????? */

   if(EMIR_active_section("EVENTS")) {

      for(i=0 ; i<box_list_cnt ; i++) {

                        sprintf(text, "Delete from %s "
                                      "Where  \"Id\"=%s"   ,
                                     __db_table_scan_events, box_list[i].id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_dl_synch - Delete Event (Id=%s) : %s", prefix, box_list[i].id, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                         return ;
                  }
                                      }

                                     }
/*------------------------------------------------- ?????? ????????? */

                     db->Commit() ;

           sprintf(text, "%s - Deals Uploaded", prefix) ;
        LB_ADD_ROW(IDC_LOG, text) ;

/*--------------------------------------------- ???????????? ??????? */

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/
}


/********************************************************************/
/*                                                                  */
/*    ????????? ??????? ???????? ?????? "??????"                    */

   void  EMIRi_dl_actions(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
            int  status ;
           char  error_id[128] ;
           char  error[1024] ;
           char  text[2048] ;
            int  active_cnt ;
            int  i ;
            int  j ;

/*------------------------------------------ ??????? ?????? ???????? */

   for(i=0 ; i<_DL_ACTIONS_MAX ; i++)
     if(__dl_actions[i]!=NULL) {
                                   free(__dl_actions[i]) ;
                                        __dl_actions[i]=NULL ;
                               }
/*------------------------------------------ ?????? ??????? ???????? */

        Cursor=db->LockCursor("EMIRi_dl_actions") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_dl_actions - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                               return ;
                      }
/*- - - - - - - - - - - - - - - - - - - -  ????????? ?????? ???????? */
                      sprintf(text, "select \"Id\", \"Action\", \"Object\", \"ObjectType\", \"Executor\", \"Data\", \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','HOLD') "
                                    "order by \"Id\"",
                                       __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_action - Get actions list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

   for(i=0 ; i<_DL_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_action - Get actions list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

                   __dl_actions[i]=(struct Dl_action *)calloc(1, sizeof(*__dl_actions[i])) ;
           strncpy(__dl_actions[i]->id,          (char *)Cursor->columns[0].value, sizeof(__dl_actions[i]->id         )-1) ;
           strncpy(__dl_actions[i]->action,      (char *)Cursor->columns[1].value, sizeof(__dl_actions[i]->action     )-1) ;
           strncpy(__dl_actions[i]->object,      (char *)Cursor->columns[2].value, sizeof(__dl_actions[i]->object     )-1) ;
           strncpy(__dl_actions[i]->object_type, (char *)Cursor->columns[3].value, sizeof(__dl_actions[i]->object_type)-1) ;
           strncpy(__dl_actions[i]->executor,    (char *)Cursor->columns[4].value, sizeof(__dl_actions[i]->executor   )-1) ;
           strncpy(__dl_actions[i]->data,        (char *)Cursor->columns[5].value, sizeof(__dl_actions[i]->data       )-1) ;
           strncpy(__dl_actions[i]->status,      (char *)Cursor->columns[6].value, sizeof(__dl_actions[i]->status     )-1) ;
           strncpy(__dl_actions[i]->reply,       (char *)Cursor->columns[7].value, sizeof(__dl_actions[i]->reply      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - -  ????????? "??????" ?????? */
                      sprintf(text, "select min(\"Id\") "
                                    "from   %s "
                                    "where  \"Status\" in ('ERROR')",
                                       __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_dl_action - Get errors level : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

            memset(error_id, 0, sizeof(error_id)) ;
           strncpy(error_id, (char *)Cursor->columns[0].value, sizeof(error_id)-1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------ ??????? ?? ?????? */

    do {
               active_cnt=0 ;

/*------------------------------------------------- ??????? ???????? */

        for(i=0 ; i<_DL_ACTIONS_MAX ; i++) {

            if(__dl_actions[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  ???????? ADD_DEAL */
            if(!stricmp(__dl_actions[i]->action, "ADDDEAL")) {

                         sprintf(text, "%s - %s : ADD_DEAL : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_dl_action_AddDeal(__dl_actions[i], db, error) ;
                  if(status) {
                         sprintf(text, "%s - ERROR - EMIR_dl_action_AddDeal : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                             }
/*- - - - - - - - - - - - - - - - - - - - - - -  ???????? SET_STATUS */
            if(!stricmp(__dl_actions[i]->action, "SETSTATUS" )) {

                for(j=0 ; j<i ; j++)                                /* ??? ????? ? ??? ?? ?????? ???????? ??????????? ??????????????? */  
                  if((!stricmp(__dl_actions[j]->action, "ADDDEAL"   ) ||
                      !stricmp(__dl_actions[j]->action, "SETSTATUS" )   ) &&
                       EMIRi_dl_GetDealId(__dl_actions[j])==
                       EMIRi_dl_GetDealId(__dl_actions[i])                &&
                       stricmp(__dl_actions[j]->status, "DONE"      )        )  break ;

                  if(j<i)  continue ;

                         sprintf(text, "%s - %s : SET_STATUS : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_dl_action_SetStatus(__dl_actions[i], db, error) ;
                  if(status) {
                         sprintf(text, "%s - ERROR - EMIR_dl_action_SetStatus : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ???????? GET_DEAL */
            if(!stricmp(__dl_actions[i]->action, "GETDEAL")) {

                         sprintf(text, "%s - %s : GET_DEAL : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->object) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_dl_action_GetDeal(__dl_actions[i], db, error) ;
                  if(status) {
                         sprintf(text, "%s - ERROR - EMIR_dl_action_GetDeal : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           }
/*------------------------------------------------ ??????? ?? ?????? */

       } while(active_cnt) ;

/*-------------------------------------------------------------------*/

}


/*********************************************************************/
/*								     */
/*                           ???????? ADD_DEAL                       */

   int  EMIR_dl_action_AddDeal(Dl_action *action, SQL_link *db, char *error)
{
#define  _PARTY_MAX  20

  SQL_cursor *Cursor ;
         int  status ;
        char *code ;
        char  account[128] ;
        char *password ;
        char  json[128] ;
        char *r_id ;
        char  data[16200] ;
        char  sign_list[512] ;
        char  box_list[1024] ;
        char  path[FILENAME_MAX] ;
        FILE *file ;
        char  result[128] ;
        char  tmpl[128] ;
        char  kind[128] ;
        char  kind_hex[128] ;
        char  uuid[128] ;
        char  uuid_hex[128] ;
        char  role_hex[128] ;
        char  id_hex[128] ;
        char  link[128] ;
        char  id_parent[128] ;
        char  bc_parent[128] ;
        Deal  deal ;
   DealParty  party[_PARTY_MAX] ;
         int  party_cnt ;
   DealParty  party_bc[_PARTY_MAX] ;
         int  party_bc_cnt ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[2048] ;
        char *address ;
        char *end ;
         int  i ;
         int  j ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- ????????????? */

                           *error=0 ;

/*---------------------------------------------- ?????? ?????/?????? */

               memset(account, 0, sizeof(account)) ;                /* ????????? ?????? */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*--------------------------------------- ?????? ?????????? ???????? */

  do {
             memset(json, 0, sizeof(json)) ;
            strncpy(json, action->data, sizeof(json)-1) ;
       r_id= strstr(json, "\"Id\":\"") ;

    if(r_id==NULL) {  r_id=json ;  break ;  }

       r_id=strstr(r_id, "\":\"")+3 ;

       end=strchr(r_id, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

     } while(0) ;

/*----------------------------------- ???????????? ??????????? ????? */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - ?????????? ???????????? ?????? */
                      sprintf(text, "Select r.\"Kind\", r.\"Data\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"='%s'",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(kind,   0, sizeof(kind)) ;
           strncpy(kind,   (char *)Cursor->columns[0].value, sizeof(kind)-1) ;
            memset(data,   0, sizeof(data)) ;
           strncpy(data,   (char *)Cursor->columns[1].value, sizeof(data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  ???????? ??????? ??????? ?????? */
                      sprintf(text, "%s Template", kind) ;
                       memset(tmpl, 0, sizeof(tmpl)) ;
        status=EMIR_db_syspar(db, text, tmpl, error) ;
     if(status)  break ;
/*- - - - - - - - - - - - - - - - -  ?????? ?????? ?????????? ?????? */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign ,    (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - ???????? ??????? ?????????? ?????? */
       if(party_cnt==0) {
                           sprintf(error, "Empty party list") ;
                            status=-1 ;
                             break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - ???????????? ????? */
                   if(data[0]==0)  strcpy(data, " ") ;              /* ???? ?????? ??? - ?????? ???????? */

#ifdef  UNIX
               sprintf(path, "%s/AddDeal_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
#else
               sprintf(path, "%s\\AddDeal_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
#endif

            file=fopen(path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, path) ;
                            status=-1 ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
/*- - - - - - - - - - - - - - - - ????????? ???????? ? FILES_ACTIONS */
            strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

                      sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\") "
                                    "values( 'PutEncryptedFile', '%s', '%s', 'NEW')",
                                      __db_table_files_actions, path, sign_list) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                        break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------- ???????????? ?????????? ????????? */

   if(!stricmp(action->status, "FILE" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - -  ???????? ?????????? FILE-???????? */
                      sprintf(text, "select \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s'", __db_table_files_actions, action->reply) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "1: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Check data file transfer: No record") ;
       else                      sprintf(error, "Check data file transfer: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(link,   0, sizeof(link)) ;
           strncpy(link,   (char *)Cursor->columns[1].value, sizeof(link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
/*- - - - - - - - - - - - - - - - - - - ???????? ???????????? ?????? */
                      sprintf(text, "select r.\"Parent\", p.\"BlockChainId\" "
                                    "from   %s r left join %s p on p.\"Id\"=r.\"Parent\" "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals,
                                    __db_table_deals,
                                      r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "3: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get parent: No record") ;
       else                      sprintf(error, "Get parent: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                           break ;
                }

            memset(id_parent, 0, sizeof(id_parent)) ;
           strncpy(id_parent, (char *)Cursor->columns[0].value, sizeof(id_parent)-1) ;
            memset(bc_parent, 0, sizeof(bc_parent)) ;
           strncpy(bc_parent, (char *)Cursor->columns[1].value, sizeof(bc_parent)-1) ;

                 db->SelectClose(Cursor) ;

     if(id_parent[0]!=0) {

       if(!stricmp(id_parent, r_id)) {                              /* ???? ?????? ????????? ?? ?????? ????... */
                      sprintf(error, "Deal is the parent of itself") ;
                                        status=-1 ;
                                           break ;
                                     }

       if(bc_parent[0]==0) {                                        /* ???? ??????-???????? ??? ?? ???????????... */
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                           }
                         }
     else                {
                               strcpy(bc_parent, "0000000000000000000000000000000000000000") ;
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????????? ?????? */
                      sprintf(text, "select r.\"Kind\", r.\"DealsUUID\" "
                                    "from   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals,
                                      r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "3: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                           break ;
                }

            memset(kind,  0, sizeof(kind)) ;
           strncpy(kind, (char *)Cursor->columns[0].value, sizeof(kind)-1) ;
            memset(uuid,  0, sizeof(uuid)) ;
           strncpy(uuid, (char *)Cursor->columns[1].value, sizeof(uuid)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  ?????? ?????? ?????????? ?????? */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Account\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].account , (char *)Cursor->columns[2].value, sizeof(party[i].account )-1) ;

    if(strlen(party[i].account)!=40) {
                                        sprintf(error, "Invalid account for participant %s - <%s>", party[i].party_id, party[i].account) ;
                                                  status=-1 ;
                                                     break ;
                                    }
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - ?????????? ??????? ?????-????????? */
                      sprintf(text, "%s Template", kind) ;
                       memset(tmpl, 0, sizeof(tmpl)) ;
        status=EMIR_db_syspar(db, text, tmpl, error) ;
     if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - ?????????? ?????? ?????????? */
              EMIR_txt2hex64(uuid, uuid_hex, strlen(uuid)) ;
              EMIR_txt2hex64(kind, kind_hex, strlen(kind)) ;

             sprintf(text, "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "%s"
                           "00000000000000000000000000000000000000000000000000000000000000a0"
                           "%064x",
                            uuid_hex, kind_hex, bc_parent, link, party_cnt*3) ;

      for(i=0 ; i<party_cnt ; i++) {
              EMIR_txt2hex64(party[i].party_id,   id_hex, strlen(party[i].party_id)) ;
              EMIR_txt2hex64(party[i].role,     role_hex, strlen(party[i].role    )) ;

                 strcat(text, "000000000000000000000000") ;
                 strcat(text, party[i].account) ;
                 strcat(text, id_hex) ;
                 strcat(text, role_hex) ;
                                   }
/*- - - - - - - - - - - - - - - - - - - - - - -  ???????? ?????????? */
              code=(char *)calloc(1, _CODE_SIZE) ;

      do {
              status=EMIR_node_getcode(tmpl,                        /* ?????????? ???? ????????? */
                                       code, _CODE_SIZE-1, error) ;
           if(status)  break ;

                                 strcat(code, text) ;               /* ????????? ????????? ???????????? ?????-????????? */

              status=EMIR_node_checkgas(account,                    /* ??????? ???? ?? ???????? ????????? */
                                         NULL, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* ????????????? ????? */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_publcontract(account,                /* ???????? ?????????? */ 
                                              code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;

                        free(code) ;

           if(status)  break ;
/*- - - - - - - - - - - - - - - - - ???????? ?????? ?? FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"LocalPath\"='%s'",
                                       __db_table_files_actions, action->reply) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "2: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

              db->SelectClose(Cursor) ;

   if(!stricmp(result, "DONE")) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                    db->UnlockCursor(Cursor) ;
                                        return(-1) ;
                }

                                    db->Commit() ;
                                }

                                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- ???????? ????????????? */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  ????????? ????????? */
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;

           Sleep(20000) ;
           
                strcpy(contract, contract+2) ;                      /* ??????? ??????? 0x */
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????????? ?????? */
                      sprintf(text, "select r.\"Kind\", r.\"DealsUUID\" "
                                    "from   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals,
                                      r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "3: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                           break ;
                }

            memset(kind,  0, sizeof(kind)) ;
           strncpy(kind, (char *)Cursor->columns[0].value, sizeof(kind)-1) ;
            memset(uuid,  0, sizeof(uuid)) ;
           strncpy(uuid, (char *)Cursor->columns[1].value, sizeof(uuid)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  ?????? ?????? ?????????? ?????? */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Account\", m.\"Box\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].account,  (char *)Cursor->columns[2].value, sizeof(party[i].account )-1) ;
           strncpy(party[i].box,      (char *)Cursor->columns[3].value, sizeof(party[i].box     )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????? */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(contract, &deal, party_bc, _PARTY_MAX, &party_bc_cnt,
                                                       NULL,        0,        NULL,
                                                       NULL,        0,        NULL, error) ;
           if(status<0)  break ;

           if(strcmp(deal.id,   uuid) ||
              strcmp(deal.kind, kind)   ) {
EMIR_log("Values check fail") ;
EMIR_log(deal.id) ;
EMIR_log(   uuid) ;
EMIR_log(deal.kind) ;
EMIR_log(     kind) ;

                      strcpy(error, "Values check fail") ;
                              status=-1 ;
                                 break ;
                                         }

    for(i=0 ; i<party_cnt ; i++) {

      for(j=0 ; j<party_bc_cnt ; j++)
        if(!stricmp(party[i].party_id, party_bc[j].party_id) &&
           !stricmp(party[i].account,  party_bc[j].account ) &&
           !stricmp(party[i].role,     party_bc[j].role    )   )  break ;

        if(j>=party_bc_cnt) {
                      strcpy(error, "Parties check fail") ;
                              status=-1 ;
                                 break ;
                            }

                                 }

    if(status)  break ;        
/*- - - - - - - - - - - - - - - ???????? ??????????? ???????? ?????? */
     for(i=0 ; i<party_cnt ; i++) {

          status=EMIRi_dl_AccessBox(party[i].box, account, error) ;
       if(status <0)  break ;
       if(status==0) {
                        sprintf(error, "Party %s box %s is not accessible for address %s", party[i].party_id, party[i].box, account) ;
                              status=-1 ;
                                 break ;
                     }

          status=0 ;
                                  }

    if(status)  break ;        
/*- - - - - - - - - - - - - -  ???????? ?????????? ?? ???????? ????? */
                  strcpy(box_list, "") ;

     for(i=0 ; i<party_cnt ; i++) {

             sprintf(text, "552de325"                               /* ????????? ???? ?????? ?????????? */
                           "000000000000000000000000%s",
                            contract) ;

              status=EMIR_node_checkgas(account,                    /* ??????? ???? ?? ?????????? */
                                        party[i].box, text, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* ????????????? ????? */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* ???????? ?????????? */ 
                                            party[i].box, text, gas, txn, error) ;
           if(status)  break ;

                               strcat(box_list, txn) ;              /* ????????? ?????? ?????????? */
                               strcat(box_list, ",") ;
                          }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
         } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='BOXES', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, box_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                               db->UnlockCursor(Cursor) ;
                                         return(-1) ;
                }

                    db->Commit() ;

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- ???????? ????????????? */

   if(!stricmp(action->status, "BOXES")) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  ????????? ????????? */
      for(address=action->reply, end=address ;
                                 end!=NULL ; address=end+1, i++) {

              end=strchr(address, ',') ;
           if(end!=NULL)  *end=0 ;

           if(*address==0)  continue ;

              status=EMIR_node_checktxn(address, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }

           if(status < 0)   break ;
                                                                 }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - -  ?????? ?????? ?????????? ?????? */
                      sprintf(text, "Select p.\"PartyId\", m.\"Box\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].box,      (char *)Cursor->columns[1].value, sizeof(party[i].box     )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - -  ???????? ????????? ?????? ? ?/? */
     for(i=0 ; i<party_cnt ; i++) {

              status=EMIRi_dl_CheckBox(party[i].box, action->object, error) ;
           if(status==0) {
                           sprintf(error, "Box %s failed", party[i].box) ;
                               status=-1 ;
                                  break ;
                         }   
           if(status <0)    break ;

              status=0 ;
                                  }

           if(status <0)    break ;
/*- - - - - - - - - - - - - - - - - - - ????????? ?????? ?? ? ?????? */
                    sprintf(text, "update %s "
                                  "set    \"BlockChainId\"='%s' "
                                  "where  \"Id\"=%s",
                                    __db_table_deals, action->object, r_id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                         break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
         } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                    db->UnlockCursor(Cursor) ;
                                         return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                           ???????? SET_STATUS                     */

   int  EMIR_dl_action_SetStatus(Dl_action *action, SQL_link *db, char *error)
{
#define  _FILES_MAX  100

      SQL_cursor *Cursor ;
             int  status ;
            char *code ;
            char  account[128] ;
            char *password ;
            char  json[3048] ;
            char *r_id ;
            char *r_status ;
            char *r_remark ;
            char *r_data ;
            char  data[16200] ;
            char  path[FILENAME_MAX] ;
            FILE *file ;
            char  link[128] ;
            char  sign_list[512] ;
            Deal  deal ;
       DealParty  party[_PARTY_MAX] ;
             int  party_cnt ;
        DealFile  files[_FILES_MAX] ;
             int  files_cnt ;
        DealFile  files_bc[_FILES_MAX] ;
             int  files_bc_cnt ;
             int  done_flag ;
            char *oper ;
            char  status_hex[128] ;
            char  remark_hex[17000] ;
            char  recipients_hex[1024] ;
            char  uuid_hex[128] ;
            char  kind_hex[128] ;
            char  ext_hex[128] ;
            char  contract[128] ;
            char  gas[128] ;
            char  txn[128] ;
            char  txn_list[2048] ;
            char  block[128] ;
            char  result[128] ;
            char  reply[128] ;
            char  text[2048] ;
            char *address ;
            char *end ;
             int  i ;
             int  j ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- ????????????? */

                           *error=0 ;

/*---------------------------------------------- ?????? ?????/?????? */

               memset(account, 0, sizeof(account)) ;                /* ????????? ?????? */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*--------------------------------------- ?????? ?????????? ???????? */

       r_remark="" ;

  do {
                 memset(json, 0, sizeof(json)) ;
                strncpy(json, action->data, sizeof(json)-1) ;
       r_id    = strstr(json, "\"Id\":\"") ;
       r_status= strstr(json, "\"Status\":\"") ;
       r_remark= strstr(json, "\"Remark\":\"") ;
       r_data  = strstr(json, "\"Data\":\"") ;

    if(r_id    ==NULL ||
       r_status==NULL ||
       r_remark==NULL   ) {  r_id=NULL ;  break ;  }

       r_id    =strstr(r_id,      "\":\"")+3 ;
       r_status=strstr(r_status,  "\":\"")+3 ;
       r_remark=strstr(r_remark,  "\":\"")+3 ;

       end=strchr(r_id, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

       end=strchr(r_status, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

       end=strchr(r_remark, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

    if(strlen(r_remark      )> 1000 ||
       strchr(r_remark, '\'')!=NULL   ) {  r_id=NULL ;  break ;  }

    if(r_data==NULL)   r_data= "N" ;
    else            {
                       r_data=strstr(r_data,  "\":\"")+3 ;
                strupr(r_data) ;
                    }

     } while(0) ;

/*------------------------------------- ????????? ??????????? ?????? */

   if(!stricmp(action->status, "NEW" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - -  ?????????? ?????? ??? ?? ????????? ?????? */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Sign\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,           (char *)Cursor->columns[0].value, sizeof(files[i].id          )-1) ;
           strncpy(files[i].parent_id,    (char *)Cursor->columns[1].value, sizeof(files[i].parent_id   )-1) ;
           strncpy(files[i].relation,     (char *)Cursor->columns[2].value, sizeof(files[i].relation    )-1) ;
           strncpy(files[i].kind,         (char *)Cursor->columns[3].value, sizeof(files[i].kind        )-1) ;
           strncpy(files[i].local_path,   (char *)Cursor->columns[4].value, sizeof(files[i].local_path  )-1) ;
           strncpy(files[i].dfs_path,     (char *)Cursor->columns[5].value, sizeof(files[i].dfs_path    )-1) ;
           strncpy(files[i].sign,         (char *)Cursor->columns[6].value, sizeof(files[i].sign        )-1) ;
           strncpy(files[i].p_local_path, (char *)Cursor->columns[7].value, sizeof(files[i].p_local_path)-1) ;
           strncpy(files[i].p_dfs_path,   (char *)Cursor->columns[8].value, sizeof(files[i].p_dfs_path  )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - -  ???????????? ?????? */
     for(i=0 ; i<files_cnt ; i++) {

       if(!stricmp(files[i].relation, "SIGN")) {

                          sprintf(text, "insert into %s (\"Action\", \"ObjectPath\", \"LocalPath\", \"Receivers\", \"Status\") "
                                        "values( 'SignFile', '%s', '%s.sign', '%s', 'NEW')",
                                           __db_table_files_actions, files[i].p_local_path, files[i].p_local_path, files[i].sign) ;
            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Sign operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                    }
                                               }
       else                                    {
                        sprintf(error, "Unknown file relation %s", files[i].relation) ;
                            status=-1 ;
                              break ;
                                               }
                                  }

         if(status)  break ;        
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='SIGN', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- ????????? ??????????? ?????? */

   if(!stricmp(action->status, "SIGN"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - ???????? ?????????? ???????? ? ??????? */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"LocalPath\""
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'"
                                    " and   f2.\"LocalPath\"=a1.\"ObjectPath\"",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "%s", db->error_text) ;
                                         db->error_text[0]=0 ;
                         break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].local_path,(char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* ???? ??????... */
                    sprintf(error, "Error on SIGN operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* ???? ???????? ??? ?? ?????????... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* ???? ??????... */

     if(done_flag==0) {                                             /* ???? ???? ????????????? ???????? */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  ????????? ?????????? ???????? ? ??????? */
   for(i=0 ; i<files_cnt ; i++) {

                    sprintf(text, "update %s "
                                  "set    \"LocalPath\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_files, files[i].local_path, files[i].id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                                }
/*- - - - - - - - - - - - - - - - -  ????? ?????? ??? ???????? ? DFS */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"DfsPath\" is null ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,           (char *)Cursor->columns[0].value, sizeof(files[i].id          )-1) ;
           strncpy(files[i].parent_id,    (char *)Cursor->columns[1].value, sizeof(files[i].parent_id   )-1) ;
           strncpy(files[i].relation,     (char *)Cursor->columns[2].value, sizeof(files[i].relation    )-1) ;
           strncpy(files[i].kind,         (char *)Cursor->columns[3].value, sizeof(files[i].kind        )-1) ;
           strncpy(files[i].local_path,   (char *)Cursor->columns[4].value, sizeof(files[i].local_path  )-1) ;
           strncpy(files[i].dfs_path,     (char *)Cursor->columns[5].value, sizeof(files[i].dfs_path    )-1) ;
           strncpy(files[i].p_local_path, (char *)Cursor->columns[6].value, sizeof(files[i].p_local_path)-1) ;
           strncpy(files[i].p_dfs_path,   (char *)Cursor->columns[7].value, sizeof(files[i].p_dfs_path  )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - -  ???????? ????????????? ?????? */
     for(i=0 ; i<files_cnt ; i++) {

          if(access(files[i].local_path, 0x04)) {

                        sprintf(error, "File %s is absent or access denied : %s", files[i].id, files[i].local_path) ;
                            status=-1 ;
                              break ;              
                                                }
                                  }
/*- - - - - - - - - - - - - - - - ?????????? ???????????? ?????????? */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign,     (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - -  ???????? ?????? ? DFS */   
                               strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++) {
                        if(i)  strcat(sign_list, ",") ;
                               strcat(sign_list, party[i].sign) ;
                                  }


     for(i=0 ; i<files_cnt ; i++) {

       if(strstr(files[i].kind, "Sign")!=NULL)  oper="TransferFile" ;
       else                                     oper="PutEncryptedFile" ;

       if(!stricmp(files[i].relation, "COPY")) {

                   sprintf(text, "select count(*) from %s where \"LocalPath\"='%s'",
                                          __db_table_files_actions, files[i].p_local_path) ;

             status=db->SelectOpen(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                       break ;
                     }

             status=db->SelectFetch(Cursor) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                       break ;
                     }

              memset(result, 0, sizeof(result)) ;
             strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
           
                 db->SelectClose(Cursor) ;

          if(!stricmp(result, "1"))  continue ;

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\") "
                                 "values( '%s', '%s', '%s', 'NEW')",
                                          __db_table_files_actions, oper, files[i].p_local_path, sign_list) ;

                                               }
       else                                    { 

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\") "
                                 "values( '%s', '%s', '%s', 'NEW')",
                                          __db_table_files_actions, oper, files[i].local_path, sign_list) ;

                                               }

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                    }
                                  }

         if(status)  break ;        
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILES', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------- ???????? ?????? ?? ?? ?????? */

   if(!stricmp(action->status, "FILES"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - -  ???????? ?????????? ???????? ? COPY-??????? */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f2.\"LocalPath\" "
                                    "     , a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' and a1.\"LocalPath\"=f2.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                             db->error_text[0]=0 ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].parent_id,  (char *)Cursor->columns[1].value, sizeof(files[i].parent_id )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].status,     (char *)Cursor->columns[3].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[4].value, sizeof(files[i].dfs_path  )-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* ???? ??????... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* ???? ???????? ??? ?? ?????????... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* ???? ??????... */

     if(done_flag==0) {                                             /* ???? ???? ????????????? ???????? */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - ????????? ?????????? ???????? ? COPY-??????? */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"LocalPath\"='%s', \"DfsPath\"='%s' where \"Id\" in (%s, %s)",
                                     __db_table_deals_files,
                                       files[i].local_path, files[i].dfs_path,
                                        files[i].id, files[i].parent_id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - ???????? ????????? ???????? ? COPY-??????? */
                      sprintf(text, "Select f1.\"Id\", f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                                   break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[2].value, sizeof(files[i].dfs_path  )-1) ;

     if( stricmp(files[i].dfs_path, "" )) {                         /* ???? ???????? ??? ?? ?????????... */
                                                 done_flag=0 ;
                                          }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* ???? ??????... */

     if(done_flag==0) {                                             /* ???? ???? ????????????? ???????? */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - -  ????????? ????????? ???????? ? COPY-??????? */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"LocalPath\"='%s', \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, 
                                        files[i].local_path, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - ???????? ?????????? ???????? ? ??????? */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1, %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' "
                                    " and   f1.\"DfsPath\" is null "
                                    " and  (f1.\"Relation\"<>'COPY' or f1.\"Relation\" is null) "
                                    " and   a1.\"LocalPath\"=f1.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')",
                                     __db_table_deals_files, __db_table_files_actions,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,  (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* ???? ??????... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* ???? ???????? ??? ?? ?????????... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* ???? ??????... */

     if(done_flag==0) {                                             /* ???? ???? ????????????? ???????? */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  ????????? ?????????? ???????? ? ??????? */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - -  ????? ?????? ??? ???????? ? ??????? */
                      sprintf(text, "Select f1.\"Id\", f1.\"Kind\", f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Remark\", f1.\"Recipients\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' ",
                                     __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,           (char *)Cursor->columns[0].value, sizeof(files[i].id          )-1) ;
           strncpy(files[i].kind,         (char *)Cursor->columns[1].value, sizeof(files[i].kind        )-1) ;
           strncpy(files[i].local_path,   (char *)Cursor->columns[2].value, sizeof(files[i].local_path  )-1) ;
           strncpy(files[i].dfs_path,     (char *)Cursor->columns[3].value, sizeof(files[i].dfs_path    )-1) ;
           strncpy(files[i].remark,       (char *)Cursor->columns[4].value, sizeof(files[i].remark      )-1) ;
           strncpy(files[i].recipients,   (char *)Cursor->columns[5].value, sizeof(files[i].recipients  )-1) ;
           strncpy(files[i].file_uuid,    (char *)Cursor->columns[6].value, sizeof(files[i].file_uuid   )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - -  ???????? ????????? ?????? */
     for(i=0 ; i<files_cnt ; i++) {

        if(files[i].dfs_path[0]==0) {
                                         status=-1 ;
                      sprintf(error, "Check Transfer operation : File %s not transfered", files[i].id) ;
                     EMIR_log(error) ;
                                          break ;
                                    }

                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - ???????????? ????? ? ?????????? ?????? */
     for(i=0 ; i<files_cnt ; i++) {

                memset(files[i].file_ext, 0, sizeof(files[i].file_ext)) ;
           end=strrchr(files[i].local_path, '.') ;
        if(end!=NULL)  strncpy(files[i].file_ext, end+1, sizeof(files[i].file_ext)-1) ;

           status=EMIR_file_hash(files[i].local_path, files[i].hash, text) ;
        if(status) {
                      sprintf(error, "File Hash calculation : File %s - %s", files[i].id, text) ;
                     EMIR_log(error) ;
                                          break ;
                   }

                   sprintf(text, "update %s set \"Hash\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, files[i].hash, files[i].id) ;

           status=db->SqlExecute(Cursor, text, NULL, 0) ;
        if(status) {
                      sprintf(error, "File Hash calculation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                          break ;
                   }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - ?????????? ?????? ?????-????????? ?????? */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - -  ???????????? ? ???????? ???????? ?????????? */
           code=(char *)calloc(1, _CODE_SIZE) ;

             memset(txn_list, 0, sizeof(txn_list)) ;

     for(i=0 ; i<files_cnt ; i++) {

                 strcpy(uuid_hex, "0000000000000000000000000000000000000000000000000000000000000000") ;
                 strcat(uuid_hex, files[i].file_uuid) ;
                memmove(uuid_hex, uuid_hex+strlen(files[i].file_uuid), 64) ;
                        uuid_hex[64]=0 ;

         EMIR_txt2hex64(files[i].kind,       kind_hex,       strlen(files[i].kind)) ;
         EMIR_txt2hex64(files[i].file_ext,   ext_hex,        strlen(files[i].file_ext)) ;
         EMIR_txt2hex64(files[i].remark,     remark_hex,     strlen(files[i].remark)) ;
         EMIR_txt2hex64(files[i].recipients, recipients_hex, strlen(files[i].recipients)) ;

             sprintf(code, "fd2696ef"                               /* ????????? ???? ?????? ?????????? */
                           "%s"
                           "%s%s000000000000000000000000%s%64s"
                           "00000000000000000000000000000000000000000000000000000000000000e0"
                           "%064x"
                           "%064x" 
                           "%s"
                           "%064x"
                           "%s",
                            uuid_hex,
                            kind_hex, ext_hex, files[i].hash, files[i].dfs_path,
                            0x100+(int)strlen(remark_hex)/2,
                            (int)strlen(files[i].remark),     remark_hex,
                            (int)strlen(files[i].recipients), recipients_hex
                             ) ;

      do {
              status=EMIR_node_checkgas(account,                    /* ??????? ???? ?? ?????????? */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* ????????????? ????? */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* ???????? ?????????? */ 
                                             contract, code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;

           if(status)  break ;

                                      strcat(txn_list, txn) ;
                                      strcat(txn_list, ",") ;
                                  }

                 free(code) ;
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*-------------------------------------------- ???????? ????? ?????? */

   if(!stricmp(action->status, "DATA" )) do {
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  ????????? ????????? */
      for(address=action->reply, end=address ;
                                 end!=NULL ; address=end+1, i++) {

              end=strchr(address, ',') ;
           if(end!=NULL)  *end=0 ;

           if(*address==0)  continue ;

              status=EMIR_node_checktxn(address, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }

           if(status < 0)   break ;
                                                                 }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - ???? ???? ?????? ?? ????????? */
       if(r_data[0]=='N') {
                             strcpy(action->status, "STATUS") ;
                             strcpy(link, "0000000000000000000000000000000000000000000000000000000000000000") ;
                                 break ;
                          }
/*- - - - - - - - - - - - - - - - - - - - - - - ????????? ??????? ?? */
    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - ?????????? ???????????? ?????? */
                      sprintf(text, "Select r.\"Data\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"='%s'",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(data,   0, sizeof(data)) ;
           strncpy(data,   (char *)Cursor->columns[0].value, sizeof(data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  ?????? ?????? ?????????? ?????? */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign ,    (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - ???????? ??????? ?????????? ?????? */
       if(party_cnt==0) {
                           sprintf(error, "Empty party list") ;
                            status=-1 ;
                             break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - ???????????? ????? */
                   if(data[0]==0)  strcpy(data, " ") ;              /* ???? ?????? ??? - ?????? ???????? */

#ifdef  UNIX
               sprintf(path, "%s/SetStatus_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
#else
               sprintf(path, "%s\\SetStatus_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
#endif

            file=fopen(path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, path) ;
                            status=-1 ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
/*- - - - - - - - - - - - - - - - ????????? ???????? ? FILES_ACTIONS */
            strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

                      sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\") "
                                    "values( 'PutEncryptedFile', '%s', '%s', 'NEW')",
                                      __db_table_files_actions, path, sign_list) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                        break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA-WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           } while(0) ;



/*-------------------------------- ???????????? ?????????? ????????? */

   if(!stricmp(action->status, "DATA-WAIT" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - -  ???????? ?????????? FILE-???????? */
                      sprintf(text, "select \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s'", __db_table_files_actions, action->reply) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "1: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Check data file transfer: No record") ;
       else                      sprintf(error, "Check data file transfer: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(link,   0, sizeof(link)) ;
           strncpy(link,   (char *)Cursor->columns[1].value, sizeof(link)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
       } while(0) ;

                 db->UnlockCursor(Cursor) ;

     if(!stricmp(result, "DONE"))  strcpy(action->status, "STATUS") ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*----------------------------------------- ????????? ??????? ?????? */

   if(!stricmp(action->status, "STATUS"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - -  ????? ?????? ??? ???????? ? ??????? */
                      sprintf(text, "Select f1.\"Id\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' ",
                                     __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id       )-1) ;
           strncpy(files[i].file_uuid, (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - ?????????? ?????? ?????-????????? ?????? */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - -  ???????????? ?????????? ????????? */
           code=(char *)calloc(1, _CODE_SIZE) ;

         EMIR_txt2hex64(r_status, status_hex, strlen(r_status)) ;   /* ?????????????? Key ? HEX */
         EMIR_txt2hex64(r_remark, remark_hex, strlen(r_remark)) ;

             sprintf(code, "edd38017"                               /* ????????? ???? ?????? ?????????? */
                           "%s"
                           "0000000000000000000000000000000000000000000000000000000000000080"
                           "%s"
                           "%064x"
                           "%064x" 
                           "%s"
                           "%064x", 
                            status_hex, link, 0xa0+(int)strlen(remark_hex)/2, (int)strlen(r_remark), remark_hex, files_cnt) ;

     for(i=0 ; i<files_cnt ; i++) {

                 strcpy(uuid_hex, "0000000000000000000000000000000000000000000000000000000000000000") ;
                 strcat(uuid_hex, files[i].file_uuid) ;
                memmove(uuid_hex, uuid_hex+strlen(files[i].file_uuid), 65) ;

                sprintf(text, "%s", uuid_hex) ;
                 strcat(code, text) ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - - -  ???????? ?????????? */
      do {
              status=EMIR_node_checkgas(account,                    /* ??????? ???? ?? ?????????? */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* ????????????? ????? */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* ???????? ?????????? */ 
                                             contract, code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;

                 free(code) ;
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------------- ???????? ????????????? */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - ???????? ?????????? ???????? */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  ????????? ????????? */
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - - - - -  ?????? ??????? ?????? */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(action->object, &deal, 
                                            NULL,      0,          NULL,
                                            files_bc, _FILES_MAX, &files_bc_cnt,
                                            NULL,      0,          NULL, error) ;
           if(status)  break ;

           if(strcmp(deal.status, r_status)||
              strcmp(deal.remark, r_remark)  ) {

                      strcpy(error, "Values check fail") ;
                              status=-1 ;
                                 break ;
                                               }
/*- - - - - - - - - - - - - - - - - - - ?????????? ?????? ?????????? */
                      sprintf(text, "Select f1.\"Id\", f1.\"Kind\", f1.\"LocalPath\", f1.\"Hash\", f1.\"DfsPath\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"Status\"='%s' ",
                                     __db_table_deals_files,
                                       r_id, r_status ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

            strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
            strncpy(files[i].kind,       (char *)Cursor->columns[1].value, sizeof(files[i].kind      )-1) ;
            strncpy(files[i].local_path, (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;
            strncpy(files[i].hash,       (char *)Cursor->columns[3].value, sizeof(files[i].hash      )-1) ;
            strncpy(files[i].dfs_path,   (char *)Cursor->columns[4].value, sizeof(files[i].dfs_path  )-1) ;

             memset(files[i].file_ext, 0, sizeof(files[i].file_ext)) ;
        end=strrchr(files[i].local_path, '.') ;
     if(end!=NULL)  strncpy(files[i].file_ext, end+1, sizeof(files[i].file_ext)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - ?????? ?????? ?????????? */
     for(i=0 ; i<files_cnt ; i++) {

  EMIR_log("Files Check") ;
   sprintf(text, "1: %s : %s : %s : %s : %s", 
               r_status, files[i].kind, files[i].file_ext, files[i].hash, files[i].dfs_path) ;
  EMIR_log(text) ;

      for(j=0 ; j<files_bc_cnt ; j++) {

   sprintf(text, "2: %s : %s : %s : %s : %s", 
               files_bc[j].status, files_bc[j].kind, files_bc[j].file_ext, files_bc[j].hash, files_bc[j].dfs_path) ;
  EMIR_log(text) ;

        if(!stricmp(files_bc[j].status,   r_status         ) &&
           !stricmp(files_bc[j].kind,     files[i].kind    ) &&
           !stricmp(files_bc[j].file_ext, files[i].file_ext) &&
           !stricmp(files_bc[j].hash,     files[i].hash    ) &&
           !stricmp(files_bc[j].dfs_path, files[i].dfs_path)   )  break ;
                                      }

        if(j>=files_bc_cnt) {
                   sprintf(error, "Check fail for file %s", files[i].id) ;
                  EMIR_log(error) ;
                             status=-1 ;
                                break ;
                            }
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                                         return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                           ???????? GET_DEAL                       */

   int  EMIR_dl_action_GetDeal(Dl_action *action, SQL_link *db, char *error)
{
#define  _FILES_MAX  100

      SQL_cursor *Cursor ;
             int  status ;
            Deal  deal ;
       DealParty  party[_PARTY_MAX] ;
             int  party_cnt ;
       DealParty  party_bc[_PARTY_MAX] ;
             int  party_bc_cnt ;
        DealFile  files[_FILES_MAX] ;
             int  files_cnt ;
        DealFile  files_bc[_FILES_MAX] ;
             int  files_bc_cnt ;
     DealHistory  history[_FILES_MAX] ;
             int  history_cnt ;
     DealHistory  history_bc[_FILES_MAX] ;
             int  history_bc_cnt ;
            char  data[16400] ;
            char  data_path[FILENAME_MAX] ;
            FILE *file ;
            char *oper ;
            char  parent_id[128] ;
            char  result[128] ;
            char  text[5120] ;
             int  done_flag ;
             int  i ;
             int  j ;

/*---------------------------------------------------- ????????????? */

                           *error=0 ;

/*------------------------------------------ ????????? ?????? ?????? */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - ?????? ?????? ?????? ?? ?? */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(action->object, &deal,
                                       party_bc,   _PARTY_MAX, &party_bc_cnt,
                                       files_bc,   _FILES_MAX, &files_bc_cnt,
                                       history_bc, _FILES_MAX, &history_bc_cnt, error) ;
           if(status<0)  break ;
/*- - - - - - - - - - - - - - - - - - - ???????? ??????? ?????? ? ?? */
                      sprintf(text, "select count(*) from %s where  \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                    sprintf(error, "Check deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  ??????????? ???????????? ?????? */
   if(result[0]=='0') {

                strcpy(parent_id, "NULL") ;

      if(stricmp(deal.parent, 
                 "0000000000000000000000000000000000000000")) {

                         sprintf(text, "select \"Id\" from %s where  \"BlockChainId\"='%s'",
                                        __db_table_deals, deal.parent) ;
           status=db->SelectOpen(Cursor, text, NULL, 0) ;
        if(status) {
                     sprintf(error, "Check parent: %s", db->error_text) ;
                                     db->error_text[0]=0 ;
                                         break ;
                   }

           status=db->SelectFetch(Cursor) ;
        if(status==_SQL_NO_DATA) {
                                 }
        else
        if(status) {
                     sprintf(error, "Check deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                   }
        else       {
                       memset(parent_id, 0, sizeof(result)) ;
                      strncpy(parent_id, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
                   }

                       db->SelectClose(Cursor) ;
                                                              }

                      }
/*- - - - - - - - - - - - - - - -  ????????/?????????? ?????? ?????? */
       if(result[0]=='0')  sprintf(text, "insert into %s(\"DealsUUID\",\"Kind\",\"Status\",\"Remark\",\"Version\","
                                                       " \"BlockChainId\",\"Parent\",\"ParentBlockChainId\","
                                                       "\"DataUpdate\") "
                                         "values('%s','%s','%s','%s','%s','%s',%s,'%s','WA')",
                                           __db_table_deals,
                                               deal.id,      deal.kind,    deal.status, deal.remark,
                                               deal.version, deal.address,   parent_id, deal.parent ) ;
       else                sprintf(text, "update %s set \"Status\"='%s',\"Remark\"='%s',\"Version\"='%s',\"DataUpdate\"='WU' "
                                         "where  \"BlockChainId\"='%s'",
                                           __db_table_deals, deal.status, deal.remark, deal.version, deal.address) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Insert/Update deal record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - -  ??????????? ?????????????? ?????? */
                      sprintf(text, "select \"Id\" from %s where \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get deal Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                    sprintf(error, "Get deal Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }

            memset(action->data, 0, sizeof(action->data)) ;
           strncpy(action->data, (char *)Cursor->columns[0].value, sizeof(action->data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - ?????? ????? ????????? */
                            *data_path=0 ;

#ifdef  UNIX
                sprintf(data_path, "%s/%s", __file_storage, deal.link) ;
#else
                sprintf(data_path, "%s\\%s", __file_storage, deal.link) ;
#endif

                              oper="GetEncryptedFile" ;

                      sprintf(text, "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\") "
                                         "values('%s','%s','%s','%s','NEW')",
                                           __db_table_files_actions,
                                               oper, action->data, data_path, deal.link) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Insert/Update data file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                     }
/*- - - - - - - - - - - - - ????????? ?????? ?????????? ?????? ?? ?? */
                      sprintf(text, "Select p.\"Id\", p.\"PartyId\", p.\"Role\" "
                                    "From   %s p "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, action->data) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(party[i].id,       (char *)Cursor->columns[0].value, sizeof(party[i].id      )-1) ;
           strncpy(party[i].party_id, (char *)Cursor->columns[1].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[2].value, sizeof(party[i].role    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - ??????????? ????? ?????????? */
      for(i=0 ; i<party_bc_cnt ; i++)  party_bc[i].flag=1 ;

      for(i=0 ; i<party_bc_cnt ; i++)
      for(j=0 ; j<party_cnt    ; j++)
        if(!stricmp(party_bc[i].party_id, party[j].party_id) &&
           !stricmp(party_bc[i].role,     party[j].role    )   ) {
                party_bc[i].flag=0 ;   break ;
                                                                 }
/*- - - - - - - - - - - - - - - - - - - - ????????? ????? ?????????? */
      for(i=0 ; i<party_bc_cnt ; i++)
        if(party_bc[i].flag) {

                      sprintf(text, "insert into %s(\"DealId\",\"PartyId\",\"Role\",\"DataUpdate\") "
                                         "values(%s,'%s','%s','A')",
                                           __db_table_deals_parties,
                                               action->data, party_bc[i].party_id, party_bc[i].role) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                        sprintf(error, "Insert/Update party record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                     }
                             }

    if(status)  break ;
/*- - - - - - - - - - - - - - - ????????? ?????? ?????? ?????? ?? ?? */
                      sprintf(text, "Select \"Id\", \"DfsPath\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,       (char *)Cursor->columns[0].value, sizeof(files[i].id      )-1) ;
           strncpy(files[i].dfs_path, (char *)Cursor->columns[1].value, sizeof(files[i].dfs_path)-1) ;
                                  }

                 db->SelectClose(Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - ??????????? ????? ?????? */
      for(i=0 ; i<files_bc_cnt ; i++)  files_bc[i].flag=1 ;

      for(i=0 ; i<files_bc_cnt ; i++)
      for(j=0 ; j<files_cnt    ; j++)
        if(!stricmp(files_bc[i].dfs_path, files[j].dfs_path)) {
                files_bc[i].flag=0 ;   break ;
                                                              }
/*- - - - - - - - - - - - - - - - - - - - - - ????????? ????? ?????? */
      for(i=0 ; i<files_bc_cnt ; i++)
        if(files_bc[i].flag) {

#ifdef UNIX
                sprintf(files_bc[i].local_path, "%s/%s.%s",
                          __file_storage, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#else
                sprintf(files_bc[i].local_path, "%s\\%s.%s",
                          __file_storage, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#endif

                      sprintf(text, "insert into %s(\"DealId\",\"Version\",\"Status\",\"Kind\","
                                                  " \"Remark\",\"Recipients\",\"FileUUID\","
                                                  " \"LocalPath\",\"DfsPath\",\"Hash\",\"DataUpdate\") "
                                         "values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','A')",
                                           __db_table_deals_files,
                                               action->data, files_bc[i].version,    files_bc[i].status,     files_bc[i].kind,
                                                             files_bc[i].remark,     files_bc[i].recipients, files_bc[i].file_uuid,
                                                             files_bc[i].local_path, files_bc[i].dfs_path,   files_bc[i].hash) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Insert/Update file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                     }
      
          if(strstr(files_bc[i].kind, "Sign")!=NULL)  oper="GetFile" ;
          else                                        oper="GetEncryptedFile" ;

                      sprintf(text, "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\") "
                                         "values('%s','%s','%s','%s','NEW')",
                                           __db_table_files_actions,
                                               oper, action->data, files_bc[i].local_path, files_bc[i].dfs_path) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Insert/Update file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                     }

                             }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - -  ????????? ??????? ?? ?? */
                      sprintf(text, "Select \"Version\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s",
                                     __db_table_deals_history, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get history 1 : %s", db->error_text) ;
                  EMIR_log(text) ;
                  EMIR_log(error) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get history 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(history[i].version, (char *)Cursor->columns[0].value, sizeof(history[i].version)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many history steps") ;
                            status=-1 ;
                              break ;
                      }  

         history_cnt=i ;
/*- - - - - - - - - - - - - - - - -  ??????????? ????? ????? ??????? */
      for(i=0 ; i<history_bc_cnt ; i++)  history_bc[i].flag=1 ;

      for(i=0 ; i<history_bc_cnt ; i++)
      for(j=0 ; j<history_cnt    ; j++)
        if(!stricmp(history_bc[i].version, history[j].version)) {
                history_bc[i].flag=0 ;   break ;
                                                                }
/*- - - - - - - - - - - - - - - - - -  ????????? ????? ????? ??????? */
      for(i=0 ; i<history_bc_cnt ; i++)
        if(history_bc[i].flag) {

                      sprintf(text, "insert into %s(\"DealId\",\"Version\",\"Status\","
                                                  " \"Remark\",\"ActorId\",\"DataUpdate\") "
                                         "values(%s,'%s','%s','%s','%s','A')",
                                           __db_table_deals_history,
                                               action->data, history_bc[i].version, history_bc[i].status,
                                                             history_bc[i].remark,  history_bc[i].actor  ) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                        sprintf(error, "Insert/Update history record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                     }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Data\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->data, data_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- ????????? ??????????? ?????? */

   if(!stricmp(action->status, "FILE" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - ???????? ?????????? ???????? ? ??????? */
                      sprintf(text, "Select \"Id\", \"DfsPath\", \"Status\" "
                                    "From   %s  "
                                    "Where  \"Receivers\"='%s' ",
                                      __db_table_files_actions, action->data) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[2].value, sizeof(result)-1) ;

     if(!stricmp(result, "ERROR")) {                                /* ???? ??????... */
                    sprintf(error, "Error on DFS-transfer operation") ;
                                        status=-1 ;
                                           break ;
                                   }
     else
     if( stricmp(result, "DONE" )) {                                /* ???? ???????? ??? ?? ?????????... */
                                        done_flag=0 ;
                                   }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* ???? ??????... */

     if(done_flag==0) {                                             /* ???? ???? ????????????? ???????? */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }
/*- - - - - - - - - - - - - - - - - ????????? ????? ????????? ?????? */
     if(action->reply[0]!=0) {

             file=fopen(action->reply, "rb") ;
          if(file==NULL) {
                    sprintf(error, "Title file open error %d: %s", errno, action->reply) ;
                                       break ;
                         }

                       memset(data, 0, sizeof(data)) ;
                        fread(data, 1, sizeof(data)-1, file) ;
                       fclose(file) ;

                      sprintf(text, "update %s set \"Data\"='%s' "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals, data, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Title data insert: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }

                                 unlink(action->reply) ;
                             }
/*- - - - - - - - - - - - - - - - -  ?????? ??????? ????????? ?????? */
                      sprintf(text, "Select \"Id\",\"LocalPath\",\"Hash\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s and \"DataUpdate\"='A' ",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].hash,       (char *)Cursor->columns[2].value, sizeof(files[i].hash      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* ???? ??????? ??????? ???????... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - ???????? ???????????? ???-???? */
     for(i=0 ; i<files_cnt ; i++) {

           status=EMIR_file_hash(files[i].local_path, data, text) ;
        if(status) {
                      sprintf(error, "File Hash calculation : File %s - %s", files[i].id, text) ;
                     EMIR_log(error) ;
                                          break ;
                   }

        if(stricmp(files[i].hash, data)) {
                      sprintf(error, "Invalid file Hash : File %s - %s", files[i].id, files[i].hash) ;
                                                 status=-1 ;
                                                    break ;
                                         }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - - - - ????????????? ?????? ?????? ? ?? */
                      sprintf(text, "update %s set \"DataUpdate\"=substring(\"DataUpdate\",2,1) "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Unlock deal record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  ????????? ?????? ? ?? */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Status\"='ERROR', \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            ?????? ??????? Get... ?????-????????? Deal...          */

   int  EMIRi_dl_GetDeal(       char *contract,
                                Deal *deal, 
                           DealParty *parties,
                                 int  parties_max,
                                 int *parties_cnt,
                            DealFile *files, 
                                 int  files_max,
                                 int *files_cnt,
                         DealHistory *history, 
                                 int  history_max,
                                 int *history_cnt, char *error)

{
          int  status ;
         char *result ;
          int  size ;
         char  text[2048] ;
         char  value[2048] ;
         char *doc_rem ;
         char *doc_rcp ;
         char *end ;
          int  i  ;

  static char *buff ;

#define  _BUFF_MAX  64000

 static char *request_status ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xabd95b95\"},\"latest\"],\"id\":1}" ;
 static char *request_parties="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x3a9e9723\"},\"latest\"],\"id\":1}" ;
 static char *request_docs   ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x1efecdf6\"},\"latest\"],\"id\":1}" ;
 static char *request_doc    ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x44c89bb8%s\"},\"latest\"],\"id\":1}" ;
 static char *request_history="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x920cc179%064x\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- ???????? ??????? URL ???? */

   if(__node_url[0]==0) {
                           strcpy(error, "?? ????? URL ????") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- ????????? ???????? */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*--------------------------------------- ???????? ??????? GetStatus */

                          sprintf(buff, request_status, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memcpy(deal->id,             result+0*64, 64) ;
              memcpy(deal->kind,           result+1*64, 64) ;
              memcpy(deal->parent,         result+2*64, 64) ;
              memcpy(deal->link,           result+3*64, 64) ;
              memcpy(deal->version,        result+4*64, 64) ;
              memcpy(deal->status,         result+5*64, 64) ;

              memset(text, 0, sizeof(text)) ;                       /* ?????????? Remark(string) */
              memcpy(text, result+7*64, 64) ;
        size=strtoul(text, &end, 16) ;

              memset(deal->remark, 0, sizeof(deal->remark)) ;
              memcpy(deal->remark, result+8*64, size*2) ;

        EMIR_hex2txt(deal->id,     deal->id) ;                      /* ?????????????? ? ????? */
        EMIR_hex2txt(deal->kind,   deal->kind) ;
        EMIR_hex2txt(deal->status, deal->status) ;
        EMIR_hex2txt(deal->remark, deal->remark) ;       

              strcpy(deal->parent, deal->parent+24) ;

              strcpy(deal->address, contract) ;

#undef   _RESULT_PREFIX

/*-------------------------------------- ???????? ??????? GetParties */

  if(parties!=NULL) {

                          sprintf(buff, request_parties, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

                       memset(text, 0, sizeof(text)) ;  
                       memcpy(text, result+1*64, 64) ;
         *parties_cnt=strtoul(text, &end, 16)/3 ;
      if(*parties_cnt>parties_max) {
                                      sprintf(error, "Too many parties") ;
                                         return(-1) ;
                                   }

                 result+=2*64 ;

      for(i=0 ; i<*parties_cnt ; i++) {

              memset(&parties[i], 0, sizeof(parties[i])) ;

              memcpy(parties[i].account,  result+0*64, 64) ;
              memcpy(parties[i].party_id, result+1*64, 64) ;
              memcpy(parties[i].role,     result+2*64, 64) ;

        EMIR_hex2txt(parties[i].party_id, parties[i].party_id) ;    /* ?????????????? ? ????? */
        EMIR_hex2txt(parties[i].role,     parties[i].role    ) ;

              strcpy(parties[i].account,  parties[i].account+24) ;

                                         result+=3*64 ;
                                      }

#undef   _RESULT_PREFIX

                    }
/*------------------------------------ ???????? ??????? GetDocuments */

  if(files!=NULL) {

                          sprintf(buff, request_docs, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

                    memset(text, 0, sizeof(text)) ;  
                    memcpy(text, result+1*64, 64) ;
         *files_cnt=strtoul(text, &end, 16)/3 ;
      if(*files_cnt>files_max) {
                                sprintf(error, "Too many files") ;
                                    return(-1) ;
                               }

                 result+=2*64 ;

      for(i=0 ; i<*files_cnt ; i++) {

              memset(&files[i], 0, sizeof(files[i])) ;

              memcpy(files[i].file_uuid, result+0*64, 64) ;
              memcpy(files[i].version,   result+1*64, 64) ;
              memcpy(files[i].status,    result+2*64, 64) ;

        EMIR_hex2txt(files[i].status,   files[i].status  ) ;        /* ?????????????? ? ????? */

                                         result+=3*64 ;
                                    }

#undef   _RESULT_PREFIX

                  }
/*------------------------------------ ???????? ???????? GetDocument */

  if(files!=NULL) {

    for(i=0 ; i<*files_cnt ; i++) {

                          sprintf(buff, request_doc, contract, files[i].file_uuid) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memcpy(files[i].kind,     result+0*64, 64) ;          /* ?????????? '???????' ????????? */
              memcpy(files[i].file_ext, result+1*64, 64) ;
              memcpy(files[i].hash,     result+2*64, 64) ;
              memcpy(files[i].dfs_path, result+3*64, 64) ;

              memset(value, 0, sizeof(value)) ;                     /* ???????????????? '????????' ????????? */
              memcpy(value, result+5*64, 64) ;
             doc_rem=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+6*64, 64) ;
             doc_rcp=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, doc_rem, 64) ;                      /* ????????? ???? "Remark" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rem+64, size*2) ;

                  memcpy(value, doc_rcp, 64) ;                      /* ????????? ???? "Recepients" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rcp+64, size*2) ;

        EMIR_hex2txt(files[i].kind,       files[i].kind      ) ;    /* ?????????????? HEX->TEXT */
        EMIR_hex2txt(files[i].file_ext,   files[i].file_ext  ) ;
        EMIR_hex2txt(files[i].remark,     files[i].remark    ) ;
        EMIR_hex2txt(files[i].recipients, files[i].recipients) ;

              strcpy(files[i].hash, files[i].hash+24) ;

#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                  }
                  }
/*----------------------------- ????????? ??????? ????????? ???????? */

  if(history!=NULL) {
/*- - - - - - - - - - - - - - - - - - -  ??????????? ??????? ??????? */
         *history_cnt=strtoul(deal->version, &end, 16)+1 ;
      if(*history_cnt>files_max) {
                                sprintf(error, "Too many history steps") ;
                                    return(-1) ;
                                 }
/*- - - - - - - - - - - - - - - - - - - - ??????????? ?????? ??????? */
     for(i=0 ; i<*history_cnt ; i++) {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ?????? */
                          sprintf(buff, request_history, contract, i) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memset(&history[i], 0, sizeof(history[i])) ;

              memcpy(history[i].version, result+0*64, 64) ;
              memcpy(history[i].status,  result+1*64, 64) ;
//            memcpy(history[i].remark,  result+2*64, 64) ;         /* ?????????? ?????? ?? ?????? */
              memcpy(history[i].actor,   result+3*64, 64) ;

        EMIR_hex2txt(history[i].status, history[i].status) ;
              strcpy(history[i].actor , history[i].actor+24) ;

              memcpy(text, result+4*64, 64) ;
        size=strtoul(text, &end, 16) ;
     if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
     if(size<=0)  continue ;

              memset(text, 0, sizeof(text)) ;
              memcpy(text, result+5*64, size*2) ;
        EMIR_hex2txt(text, text) ;                                  /* ?????????????? ? ????? */
             strncpy(history[i].remark, text, sizeof(history[i].remark)-1) ;

#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - ??????????? ?????? ??????? */
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                    }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            ?????? ??????? CheckContract ?????-????????? Box       */

   int  EMIRi_dl_CheckBox(char *box, char *contract, char *error)

{
   int  status ;
  char *result ;
  char  buff[4096] ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x34d01d61000000000000000000000000%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- ???????? ??????? URL ???? */

   if(__node_url[0]==0) {
                           strcpy(error, "?? ????? URL ????") ;
                                   return(-1) ;
                        }
/*--------------------------------------- ???????? ??????? GetStatus */

                          sprintf(buff, request, box, contract) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                         result+=strlen(_RESULT_PREFIX)+2 ;
            EMIR_hex2txt(result, result) ;                          /* ?????????????? ? ????? */

      if(result[0]=='Y')  return(1) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           ?????? ?????a GetContracts ?????-????????? Box          */

   int  EMIRi_dl_GetBox(char *contract, Deal **list, char *error)

{
          int  status ;
         char *result ;
          int  list_cnt ;
         char  text[1024] ;
         char *end ;
          int  i  ;
          int  j  ;

  static char *buff ;

#define  _BUFF_MAX  64000

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x8040a5b8\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- ???????? ??????? URL ???? */

   if(__node_url[0]==0) {
                           strcpy(error, "?? ????? URL ????") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- ????????? ???????? */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*------------------------------------ ???????? ??????? GetContracts */

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ ?????? ?????????? */

#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

                   memset(text, 0, sizeof(text)) ;  
                   memcpy(text, result+2*64, 64) ;
         list_cnt=strtoul(text, &end, 16) ;
      if(list_cnt==0)  return(0) ;

                 result+=3*64 ;

          *list=(Deal *)realloc(*list, list_cnt*sizeof(Deal)) ;

      for(i=0 ; i<list_cnt ; i++) {
              memset(&(*list)[i], 0, sizeof(Deal)) ;
              memcpy( (*list)[i].address, result+i*64+24, 40) ;
                                  }

#undef   _RESULT_PREFIX

/*------------------------------------------- ???????? ?????? ?????? */

      for(i=0, j=0 ; i<list_cnt ; i++) {

        if(j!=i)  strcpy((*list)[j].address, (*list)[i].address) ;

        if(stricmp((*list)[j].address, 
                   "0000000000000000000000000000000000000000"))  j++ ;

                                       }

                  list_cnt=j ;

/*-------------------------------------------------------------------*/

   return(list_cnt) ;
}


/*********************************************************************/
/*								     */
/*              ?????????? ?????? ?????? ?? ????????                 */

   int  EMIRi_dl_GetDealId(Dl_action *action)

{
  char  data[1024] ;
  char *tmp ;


   if(!stricmp(action->action, "AddDeal"  )) {

#define  _KEY  "\"Id\":\""

                    strcpy(data, action->data) ;
                tmp=strstr(data, _KEY) ;
             if(tmp==NULL)  return(atoi(action->data)) ;

                    strcpy(data, tmp+strlen("\"Id\":\"")) ;
                tmp=strchr(data, '"') ;
             if(tmp==NULL)  return(0) ;
               *tmp=0 ;

                      return(atoi(data)) ;

#undef  _KEY  
                                             }

   else
   if(!stricmp(action->action, "SetStatus")) {

#define  _KEY  "\"Id\":\""

                    strcpy(data, action->data) ;
                tmp=strstr(data, _KEY) ;
             if(tmp==NULL)  return(0) ;

                    strcpy(data, tmp+strlen("\"Id\":\"")) ;
                tmp=strchr(data, '"') ;
             if(tmp==NULL)  return(0) ;
               *tmp=0 ;

                      return(atoi(data)) ;

#undef  _KEY  

                                             }

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            ?????? ??????? CheckBank ?????-????????? Box           */

   int  EMIRi_dl_AccessBox(char *box, char *contract, char *error)

{
   int  status ;
  char *result ;
  char  buff[4096] ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x376e6cc2000000000000000000000000%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- ???????? ??????? URL ???? */

   if(__node_url[0]==0) {
                           strcpy(error, "?? ????? URL ????") ;
                                   return(-1) ;
                        }
/*--------------------------------------- ???????? ??????? GetStatus */

                          sprintf(buff, request, box, contract) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  ?????? ?????????? */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* ???????? ??????? ?????????? */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                         result+=strlen(_RESULT_PREFIX)+2 ;
            EMIR_hex2txt(result, result) ;                          /* ?????????????? ? ????? */

      if(result[0]=='Y')  return(1) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


