/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Модуль "Сканер блокчейн"                      */
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

/*----------------------------------------- Общесистемные переменные */

  struct Elem_pos_list_ {
                           int  elem ;
                           int  x ;
                           int  y ;
                           int  xs ;
                           int  ys ;
                        }  ;

 typedef struct Elem_pos_list_ Elem_pos_list ;

/*----------------------------------------------- Ссылки на картинки */

#ifndef  UNIX 

  static  HBITMAP  picMarkUn ;
  static  HBITMAP  picMarkWr ;
  static  HBITMAP  picMarkEr ;

#endif

/*-------------------------------------- Общие переменные управления */

/*------------------------------------ Обработчики элементов диалога */

#ifndef  UNIX 

  union WndProc_par {
                        long            par ;
                     LRESULT (CALLBACK *call)(HWND, UINT, WPARAM, LPARAM) ; 
                    } ;

  static union WndProc_par  Tmp_WndProc ;
//  static union WndProc_par  ConValues_WndProc ;

//     LRESULT CALLBACK  EMIRi_ConValues_WndProc(HWND, UINT, WPARAM, LPARAM) ;

#endif

/*------------------------------------------ Внутренние подпрограммы */

  void  EMIRi_sn_newblocks (HWND hDlg, char *prefix, SQL_link *db, char *regime) ;  /* Сканирование новых блоков блокчейн */
  void  EMIRi_sn_extcontrol(HWND hDlg, char *prefix, SQL_link *db) ;                /* Обработка команд внешнего управление */


/*********************************************************************/
/*								     */
/*	      Обработчик сообщений диалогового окна SCAN             */	

 INT_PTR  CALLBACK  EMIR_scan_dialog(  HWND  hDlg,     UINT  Msg, 
                                     WPARAM  wParam, LPARAM  lParam) 
{
#ifndef  UNIX 

       static  int  start_flag=1 ;  /* Флаг запуска */
              HWND  hPrn ;
              RECT  wr ;            /* Габарит окна */
	       int  x_screen ;      /* Габарит экрана */	
               int  y_screen ;
               int  x_shift ;       /* Центрующий сдвиг */	
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
               int  elm ;            /* Идентификатор элемента диалога */
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

     static  HFONT  font ;         /* Шрифт */
            TCITEM  tab_item ;
             NMHDR *hNM ;
              char  value[1024] ;
               int  row ;

/*------------------------------------------------- Большая разводка */

  switch(Msg) {

/*---------------------------------------------------- Инициализация */

    case WM_INITDIALOG: {
/*- - - - - - - - - - - - - - - - Размещение массива позиц.элементов */
             loc_pos=(Elem_pos_list *)calloc(1, sizeof(loc_pos_e)) ;
      memcpy(loc_pos, loc_pos_e, sizeof(loc_pos_e)) ;

          sprintf(loc_pos_ptr, "%p", loc_pos) ;                     /* Сохраняем массив в элементе окна */
             SETs(IDC_ELEM_LIST, loc_pos_ptr) ;
/*- - - - - - - - - - - - - - - - - - Тест координирования элементов */
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
/*- - - - - - - - - - - - -  Фиксация положения и размеров элементов */
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
/*- - - - - - - - - - - - - - - - - - - Подравнивание рамочного окна */
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
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Пропись шрифтов */
        if(font==NULL)
           font=CreateFont(14, 0, 0, 0, FW_THIN, 
                                 false, false, false,
                                  ANSI_CHARSET,
                                   OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                      VARIABLE_PITCH,
                                       "Courier New Cyr") ;
//         SendMessage(ITEM(IDC_LOG), WM_SETFONT, (WPARAM)font, 0) ;
/*- - - - - - - - - - - - - - - - Инициализация переключателя секций */
       for(i=0 ; i<__sections_cnt ; i++) {
                        tab_item.mask   =TCIF_TEXT ;
                        tab_item.pszText=__sections[i].title ;
                        tab_item.lParam =           i ;
         TabCtrl_InsertItem(ITEM(IDC_SECTIONS_SWITCH), i, &tab_item) ;
                                         }
/*- - - - - - - - - - - - - - - - -  Инициализация значеий элементов */
             SETs(IDC_NODE_URL, __node_url) ;

                        sprintf(value, "%s : %s", __db_name, __db_user) ;
             SETs(IDC_DB_NAME,  value) ;
             SETs(IDC_VERSION,  _VERSION  ) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - Доступ к элементам */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  			  return(FALSE) ;
  			     break ;
  			}
/*-------------------------------------- Отработка изменения размера */

    case WM_SIZE: {
                         GETs(IDC_ELEM_LIST, loc_pos_ptr) ;         /* Извлекаем массив позиционирования */ 
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
/*----------------------------------------------- Отработка статусов */

    case WM_NOTIFY:    {
                           elm=  LOWORD(wParam) ;
	                   hNM=(NMHDR *)lParam ;
                        status=hNM->code ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Выбор секции */
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
/*------------------------------------ Отработка внутренних сообений */

    case WM_USER:  {
/*- - - - - - - - - - - - - - - - - - - - - - - - Активизация секции */
        if(wParam==_USER_SECTION_ENABLE) {
                                             __dialog=hDlg ;

                                                 return(FALSE) ;
                                         }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			  return(FALSE) ;
  			     break ;
  		   }

/*------------------------------------------------ Отработка событий */

    case WM_COMMAND: {

                   sts=HIWORD(wParam) ;
	           elm=LOWORD(wParam) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - Раскрытие записи */
       if(elm==IDC_LOG    &&
          sts==LBN_DBLCLK   ) {

#pragma warning(disable : 4244)
                         row=LB_GET_ROW (IDC_LOG) ;
#pragma warning(default : 4244)
                      if(row==LB_ERR)  break ;

                             LB_GET_TEXT(IDC_LOG, row, value) ;
                                    SETs(IDC_DETAILS, value) ;

                                            return(FALSE) ;
                              } 
/*- - - - - - - - - - - - - - - - - - - - - - - - - Завершить работу */
       if(elm==IDC_TERMINATE) {

           reply=MessageBox(hDlg, "Вы действительно хотите завершить программу?",
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
/*--------------------------------------------------------- Закрытие */

    case WM_CLOSE:      {
/*- - - - - - - - - - - - - - - - - - - - - -  Освобождение ресурсов */
                             GETs(IDC_ELEM_LIST, loc_pos_ptr) ;     /* Извлекаем массив позиционирования */ 
                           sscanf(loc_pos_ptr, "%p", &loc_pos) ;
                             free(loc_pos) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			      return(FALSE) ;
			          break ;
			}
/*----------------------------------------------------------- Прочее */

//  case WM_PAINT:    break ;

    default :        {
			  return(FALSE) ;
			    break ;
		     }
/*-------------------------------------------------------------------*/
	      }
/*-------------------------------------------------------------------*/

    return(TRUE) ;

#endif

    return(NULL) ;
}


/********************************************************************/
/*                                                                  */
/*        THREAD - Фоновый поток модуля "Сканер блокчейн"           */

  DWORD WINAPI  Scan_Thread(LPVOID Pars)

{
#ifndef  UNIX 

   SQL_ODBC_link  DB ;   /* Соединение по-умолчанию для DCL-скрипта */
            HWND  hDlg ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;
          time_t  time_0 ;
             int  rows_cnt ;
             int  status ;
             int  i ;

/*---------------------------------------------------- Инициализация */

               time_0=0 ;

      if(__sn_request_period<=0)  __sn_request_period= 10 ;
      if(__sn_view_frame    <=0)  __sn_view_frame    =100 ;

              hDlg=hScan_Dialog ;

      if(__sn_records==NULL)  __sn_records=(struct Sn_record *)
                                             calloc(_SN_RECORDS_MAX, sizeof(*__sn_records)) ;
      if(__sn_events ==NULL)  __sn_events =(struct Sn_event *)
                                             calloc(_SN_RECORDS_MAX, sizeof(*__sn_events)) ;

/*----------------------------------------------------- Настройка БД */

/*------------------------------------------------------- Общий цикл */

   do {
          if(__exit_flag)  break ;                                  /* Если общий стоп */ 

                      Sleep(1000) ;

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#pragma warning(disable : 4244)
            rows_cnt=LB_GET_ROW (IDC_LOG) ;
#pragma warning(default : 4244)
         if(rows_cnt>__sn_view_frame) {

               for(i=0 ; i<rows_cnt-__sn_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
                                      }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Тайминг */
               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%02d.%02d %02d:%02d:%02d ",
                                    hhmmss->tm_mday,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec  ) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Соединение с БД */
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
/*- - - - - - - - - - - - - - - - - - - - - -  Сканирование блокчейн */
             EMIRi_sn_newblocks (hScan_Dialog, prefix, &DB, "SCAN") ;

      if(__external_control)
             EMIRi_sn_extcontrol(hScan_Dialog, prefix, &DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

                DB.Disconnect() ;                                   /* Отсоединение с БД */

      } while(1) ;

/*-------------------------------------------------------------------*/

#endif

  return(0) ;
}


/********************************************************************/
/*                                                                  */
/*     Обработчик фонового потока модуля "Сканер блокчейн"          */

  void  Scan_Process(SQL_link *DB, char *regime)

{
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;

#ifndef  UNIX 
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

#pragma warning(disable : 4244)

/*---------------------------------------------------- Инициализация */

      if(__sn_request_period<=0)  __sn_request_period= 10 ;
      if(__sn_view_frame    <=0)  __sn_view_frame    =100 ;

#ifndef  UNIX 
              hDlg=hScan_Dialog ;
#endif

      if(__sn_records==NULL)  __sn_records=(struct Sn_record *)
                                             calloc(_SN_RECORDS_MAX, sizeof(*__sn_records)) ;
      if(__sn_events ==NULL)  __sn_events =(struct Sn_event *)
                                             calloc(_SN_RECORDS_MAX, sizeof(*__sn_events)) ;

/*------------------------------------------------------- Общий цикл */

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#ifndef  UNIX 

            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__sn_view_frame) {

               for(i=0 ; i<rows_cnt-__sn_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
                                      }

                LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;

#endif
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Тайминг */
               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%02d.%02d %02d:%02d:%02d ",
                                    hhmmss->tm_mday,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec  ) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Сканирование блокчейн */
             EMIRi_sn_newblocks (hScan_Dialog, prefix, DB, regime) ;

      if(__external_control)
             EMIRi_sn_extcontrol(hScan_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

/*-------------------------------------------------------------------*/
                                    
  return ;
}


/********************************************************************/
/*                                                                  */
/*                Сканирование новых блоков блокчейн                */

   void  EMIRi_sn_newblocks(HWND hDlg, char *prefix, SQL_link *db, char *regime)

{
    static  time_t  last_commit ;
    static  time_t  last_clear ;
        SQL_cursor *Cursor ;
          Sn_block  block ;
               int  status ;
               int  events_delay ;
               int  rec_offset ;
               int  repeat ;
               int  cnt ;
     long long int  ts_nano ;
            time_t  block_time ;
         struct tm *time_ddmmyy ;
              char  ts[128] ;
              char *accounts[2*_SN_RECORDS_MAX] ;
               int  accounts_cnt ;
              char  hash[1024] ;
              char  value[1024] ;
              char  error[1024] ;
              char  text[4096] ;
              char *end ;
               int  exist_flag ;
               int  i ;
               int  j ;

#define  _COMMIT_DELAY     6                      // Пауза ожидания схлопывания форков
#define   _CLEAR_DELAY  3600                      // Периодичность очистки
#define   _CLEAR_DEEP   3600*24*__events_deep     // Глубина очистки

/*------------------------------------------------------- Подготовка */

         EMIR_log("(LVL.1) EMIRi_sn_newblocks") ;

        Cursor=db->LockCursor("EMIRi_sn_newblocks") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }

     if(__events_deep<=0)  __events_deep=10 ;
 
     if(!strcmp(__net_type, "QUORUM"))  events_delay=   0 ;
     else                               events_delay=_COMMIT_DELAY ;

/*------------------------------------------ Очистка таблицы событий */

  if(time(NULL)>last_clear+_CLEAR_DELAY) {

         block_time  =  time(NULL)-_CLEAR_DEEP ;
          time_ddmmyy=gmtime(&block_time) ;

       sprintf(ts, "%04d.%02d.%02d %02d:%02d", 
                    time_ddmmyy->tm_year+1900, time_ddmmyy->tm_mon+1, time_ddmmyy->tm_mday,
                    time_ddmmyy->tm_hour, time_ddmmyy->tm_min) ;

 sprintf(text, "Clear events to %s", ts) ;
EMIR_log(text) ;

                     sprintf(text, "delete from %s where \"TimeStamp\"<'%s' ",
                               __db_table_scan_events, ts) ;

    if(!stricmp(regime, "SCAN")) strcat(text, " and \"Address\"='0'") ;

       status=db->SqlExecute(Cursor, text, NULL, 0) ;
    if(status) {
                    sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Events clear : %s", prefix, db->error_text) ;
                 LB_ADD_ROW(IDC_LOG, text) ;
                       __db_errors_cnt++ ;
               }

                        db->Commit() ;

                    last_clear=time(NULL) ;

                                         }
/*-------------------------- Получение последнего сохранённого блока */

  if(__block_db==0) {

                      sprintf(text, "select \"BlockDb\", \"BlockDbHash\" from  %s ",
                                       __db_table_scan_state) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get last processed block : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
     if(status) {

         if(status==_SQL_NO_DATA)
                sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get last processed block : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get last processed block : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(text, (char *)Cursor->columns[0].value) ;
              __block_db=strtoul(text, &end, 10) ;
                         strncpy(__block_db_hash, (char *)Cursor->columns[1].value, sizeof(__block_db_hash)-1) ;

                 db->SelectClose (Cursor) ;
                    }
/*-------------------------------- Получение номера последнего блока */

   if(__block_last-__block_db > 10)  __scan_rush=1 ;
   else                              __scan_rush=0 ;

   if(__block_db>=__block_last-events_delay) {

     if(!stricmp(regime, "SCAN"))   Sleep(__mon_gen_period*1000) ;

        status=EMIR_node_lastblock(text, error) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get last blockchain block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                }

                 __block_last=strtoul(text, &end, 16) ;    

     if(__block_db>__block_last) {                                  /* Обработка "потери хвоста" */

                                     sprintf(text, "Rescan old context - %lx : %lx", __block_db, __block_last) ;
                                  LB_ADD_ROW(IDC_LOG, text) ;
#ifndef UNIX
                                     sprintf(text, "%lx : %lx", __block_db, __block_last) ;
                                        SETs(IDC_BLOCKS_NUMBERS, text) ;
#endif
                                           db->UnlockCursor(Cursor) ;
                                                       return ;
                                 }

                                              } 
/*------------------------------------ Стартовая точка режима EVENTS */

   if(!stricmp(regime, "EVENTS")) {

      if(__block_db==0) {
                             __block_db=__block_last-2*_COMMIT_DELAY ;
                            last_commit= 0 ;
                        }
                                  }
/*----------------------------------------- Получение и анализ блока */

   if(__block_db<__block_last-events_delay) {
/*- - - - - - - - - - - - - - - - -  Запрос данных предыдущего блока */
                           sprintf(text, "%lx", __block_db) ;
           cnt=EMIR_node_getblockh(text, &block,
                                          NULL, 0, 0, error) ;
        if(cnt<0) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get previous blockchain block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - Обработка форков */
    if(strcmp(__net_type, "QUORUM"))                                /* Кроме Quorum */
     if(       __block_db_hash[0]!=0       && 
        strcmp(__block_db_hash, block.hash)  ) {

                   sprintf(text, "Block fork detected for %lx : %s / %s", __block_db, __block_db_hash, block.hash) ;
                  EMIR_log(text) ;
               
                   sprintf(text, "Select \"BlockNum\",\"BlockHash\" "
                                 "From   %s "
                                 "Where  \"Address\"='0' "
                                 "Order by \"Id\" desc"   ,
                                     __db_table_scan_events) ;

          status=db->SelectOpen(Cursor, text, NULL, 0) ;
       if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_rs_synch - Get blocks history : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                        __db_errors_cnt++ ;
                              return ;
                  }

       do {
              status=db->SelectFetch(Cursor) ;
           if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
           if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_rs_synch - Get blocks history : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                        __db_errors_cnt++ ;
                              return ;
                      }

                           strncpy(text, (char *)Cursor->columns[0].value, sizeof(text)-1) ;
                           strncpy(hash, (char *)Cursor->columns[1].value, sizeof(hash)-1) ;

           cnt=EMIR_node_getblockh(text, &block,
                                          NULL, 0, 0, error) ;
        if(cnt<0) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get history blockchain block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                  }

        if(!stricmp(hash, block.hash)) {

                            __block_db=strtoul(text, &end, 16) ;
                                        strcpy(__block_db_hash, block.hash) ;

                   sprintf(text, "Scan rollback to block %lx : %s", __block_db, block.hash) ;
                  EMIR_log(text) ;

                                           break ;
                                       }

          } while(1) ;

                 db->SelectClose (Cursor) ;

                                               }       
/*- - - - - - - - - - - - - - - - - - - - Запрос данных нового блока */
                rec_offset=0 ;
     do {
                           sprintf(text, "%lx", __block_db+1) ;

        if(!stricmp(regime, "SCAN"))                                /* Для режима SCAN запрашиваем данные транзакций */
              cnt=EMIR_node_getblockh(text, &block,
                                          __sn_records, _SN_RECORDS_MAX, rec_offset, error) ;
        else  cnt=EMIR_node_getblockh(text, &block, NULL, 0, 0, error) ;

        if(cnt<0) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get blockchain block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                  }

     if(strlen(block.timestamp)>8) {                                /* Quorum - время создания блока в наносекундах */
                     ts_nano=strtoull(block.timestamp, &end, 16) ;
                  block_time= ts_nano/(long long)1000000000 ;
                                   }
     else                          {
                  block_time=strtoul(block.timestamp, &end, 16) ;
                                   }
 
                   time_ddmmyy=gmtime(&block_time) ;

                     sprintf(ts, "%04d.%02d.%02d %02d:%02d", 
                                    time_ddmmyy->tm_year+1900, time_ddmmyy->tm_mon+1, time_ddmmyy->tm_mday,
                                    time_ddmmyy->tm_hour, time_ddmmyy->tm_min) ;

        if(!stricmp(regime, "SCAN"))                                /* Только для режима SCAN */
           for(i=0 ; i<cnt ; i++) {

              if(strstr(__scan_exclude, __sn_records[i].from)!=NULL)  continue ;

             if(__sn_records[i].to[0]!=0)  
              if(strstr(__scan_exclude, __sn_records[i].to  )!=NULL)  continue ;

                                       last_commit=0 ;

                     sprintf(text, "%s  %8lx : %s -> %s  ", ts, __block_db+1, __sn_records[i].from, __sn_records[i].to) ;
                  LB_ADD_ROW(IDC_LOG, text) ;

                      sprintf(text, "insert into %s (\"Hash\",\"From\",\"To\",\"Value\",\"TimeStamp\") "
                                            "values ('%s', '%s', '%s', '%s', '%s')",
                                      __db_table_scan_transactions, __sn_records[i].hash, __sn_records[i].from,
                                                                    __sn_records[i].to  , __sn_records[i].value, ts) ;
                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                            sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Save transaction : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                      return ;
                        }
                                  }

#ifndef UNIX
             if(cnt>0)  LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif
             if(cnt!=_SN_RECORDS_MAX)  break ;

                rec_offset+=_SN_RECORDS_MAX ;

        } while(1) ;
/*- - - - - - - - - - - - - - - - - - - - - Построение списка счетов */
                           accounts_cnt=0 ;

         for(i=0 ; i<cnt ; i++) {

           for(j=0 ; j<accounts_cnt ; j++)
              if(!stricmp(accounts[j], __sn_records[i].from))  break ;

              if(j>=accounts_cnt) {
                                     accounts[accounts_cnt]=__sn_records[i].from ;
                                              accounts_cnt++ ;
                                  }

              if(__sn_records[i].to[0]   ==0 ||
                 __sn_records[i].value[0]==0   )  continue ;  

           for(j=0 ; j<accounts_cnt ; j++)
              if(!stricmp(accounts[j], __sn_records[i].to))  break ;

              if(j>=accounts_cnt) {
                                     accounts[accounts_cnt]=__sn_records[i].to ;
                                              accounts_cnt++ ;
                                  }

                                }
/*- - - - - - - - - - - - - - - - - - - -  Запрос остатков по счетам */
      if(!stricmp(regime, "SCAN"))                                  /* Только для режима SCAN */
         for(i=0 ; i<accounts_cnt ; i++) {

              status=EMIR_node_getbalance(accounts[i], "latest", value, error) ;
           if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get blockchain block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                      }

                            sprintf(text, "select count(*) from  %s where \"Account\"='%s'",
                                       __db_table_scan_accounts, accounts[i]) ;
              status=db->SelectOpen(Cursor, text, NULL, 0) ;
           if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Check account : %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                       }

              status=db->SelectFetch(Cursor) ;
           if(status) {
                            sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Check account : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                      }

                          strcpy(text, (char *)Cursor->columns[0].value) ;
              exist_flag=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;

           if(exist_flag)  sprintf(text, "update %s Set \"Balance\"='%s',\"TimeStamp\"='%s' Where \"Account\"='%s'",
                                            __db_table_scan_accounts, value, ts, accounts[i]) ;
           else            sprintf(text, "insert into %s (\"Account\",\"Balance\",\"TimeStamp\") "
                                         "values ('%s', '%s', '%s')",
                                            __db_table_scan_accounts, accounts[i], value, ts) ;
              status=db->SqlExecute(Cursor, text, NULL, 0) ;
           if(status) {
                            sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Save account balance : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                      return ;
                      }

                                         }
/*- - - - - - - - - - - - - - - - - - - - - - - Запрос событий блока */
                rec_offset=0 ;
                    repeat=0 ;

     do {
                           sprintf(text, "%lx", __block_db+1) ;
           cnt=EMIR_node_getevents(text, __sn_events, _SN_RECORDS_MAX, rec_offset, error) ;
        if(cnt<0) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Get blockchain events : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

            if(repeat<3) {  
               repeat++ ;
                          sprintf(text, "%s - Repeat request", prefix) ;
                       LB_ADD_ROW(IDC_LOG, text) ;
                            Sleep(1000) ; 
                              continue ;
                         }

                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                  }

                      repeat=0 ;

           for(i=0 ; i<cnt ; i++) {

             if(strstr(__scan_exclude, __sn_events[i].address)!=NULL)  continue ;

             if(!stricmp(__sn_events[i].topic,                      /* Исключаем события мониторинга */
                           "9202add79f68b8b508aaf800a65296e6d9260d4c25a1ac52b77f4697f621fbda"))  continue ;

                                       last_commit=0 ;

                     sprintf(text, " %s  %8lx : %s event %s  ", ts, __block_db+1, __sn_events[i].address, __sn_events[i].topic) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
 
                      sprintf(text, "insert into %s (\"Address\",\"Topic\",\"Transaction\",\"Data\",\"TimeStamp\",\"BlockNum\",\"BlockHash\") "
                                            "values ('%s', '%s', '%s', '%s', '%s', '%lx', '%s')",
                                      __db_table_scan_events, __sn_events[i].address,     __sn_events[i].topic,
                                                              __sn_events[i].transaction, __sn_events[i].data, ts,
                                                              __block_db+1, block.hash ) ;
                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                            sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Save event : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                      return ;
                        }
                                  }

#ifndef UNIX
             if(cnt>0)  LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif
             if(cnt!=_SN_RECORDS_MAX)  break ;	

                rec_offset+=_SN_RECORDS_MAX ;

        } while(1) ;
/*- - - - - - - - - - - - - - - - - - - - - - Регистрация хэша блока */
                      sprintf(text, "insert into %s (\"Address\",\"Topic\",\"TimeStamp\",\"BlockNum\",\"BlockHash\") "
                                            "values ('0', '0', '%s', '%lx', '%s')",
                                      __db_table_scan_events, ts, __block_db+1, block.hash ) ;
                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                            sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Save block info : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                      return ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                   __block_db++ ;
                            strcpy(__block_db_hash, block.hash) ;

        sprintf(text, "%lx : %lx", __block_db, __block_last) ;

#ifndef UNIX
           SETs(IDC_BLOCKS_NUMBERS, text) ;
#endif
                                             }
/*-------------------------------------- Регистрация обработки блока */

   if(__block_last-__block_db > 10)   __scan_rush=1 ;
   else                             {
                                      __scan_rush=0 ;
                                      last_commit=0 ;
                                    }

   if(last_commit+10<time(NULL)) {
                                       last_commit=time(NULL) ;

                      sprintf(text, "update %s set \"BlockDb\"=%ld, \"BlockDbHash\"='%s', \"BlockLast\"=%ld ",
                                      __db_table_scan_state, __block_db, __block_db_hash, __block_last) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_sn_newblocks - Save processed block : %s", prefix, db->error_text) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
                      db->Rollback() ;
                      db->UnlockCursor(Cursor) ;
                    __db_errors_cnt++ ;
                             return ;
                }

                 db->Commit() ;

                                 }
/*------------------------------------------------ Завершение работы */

                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*            Обработка команд внешнего управление                  */

   void  EMIRi_sn_extcontrol(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
           char  oper_id[128] ;
           char  oper_action[128] ;
           char  oper_data[2048] ;
           char  oper_time[128] ;
            int  status ;
            int  rec_offset ;
            int  cnt ;
           char  error[1024] ;
           char  text[8192] ;
            int  i ;

/*------------------------------------------- Резервирование курсора */

         EMIR_log("(LVL.1) EMIRi_sn_extcontrol") ;

        Cursor=db->LockCursor("EMIRi_sn_extcontrol") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_sn_extcontrol - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*---------------------------------------- Получение списка операций */

                      sprintf(text, "select \"Id\", \"Action\", \"Data\", \"Time\" "
                                    "from   %s "
                                    "where (\"Error\"='' or \"Error\" is null) "
                                    " and   \"Action\" in ('ScanBlock') "
                                    "order by \"Id\"",
                                       __db_table_ext_control) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_extcontrol - Get operation : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                                 db->SelectClose (Cursor) ;
                                 db->UnlockCursor(Cursor) ;
                                     return ;
                              }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_sn_extcontrol - Get operation: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(oper_id,     0, sizeof(oper_id)) ;
           strncpy(oper_id,      (char *)Cursor->columns[0].value, sizeof(oper_id)-1) ;
            memset(oper_action, 0, sizeof(oper_action)) ;
           strncpy(oper_action,  (char *)Cursor->columns[1].value, sizeof(oper_action)-1) ;
            memset(oper_data,   0, sizeof(oper_data)) ;
           strncpy(oper_data,    (char *)Cursor->columns[2].value, sizeof(oper_data)-1) ;
            memset(oper_time,   0, sizeof(oper_time)) ;
           strncpy(oper_time,    (char *)Cursor->columns[3].value, sizeof(oper_time)-1) ;

                 db->SelectClose (Cursor) ;

/*---------------------------------------------- Выполнение операций */

           sprintf(text, " %s  - ExternalControl(%s) : %s  %s", prefix, oper_id, oper_action, oper_data) ;
        LB_ADD_ROW(IDC_LOG, text) ;

            memset(error, 0, sizeof(error)) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Операция SCAN_BLOCK */
   if(!stricmp(oper_action, "ScanBlock")) {

                rec_offset=0 ;

     do {
           cnt=EMIR_node_getevents(oper_data, __sn_events, _SN_RECORDS_MAX, rec_offset, error) ;
        if(cnt<0) {
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   break ;
                  }

       for(i=0 ; i<cnt ; i++) {

                     sprintf(text, "  %s event %s  ", __sn_events[i].address, __sn_events[i].topic) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
 
                      sprintf(text, "insert into %s (\"Address\",\"Topic\",\"Transaction\",\"Data\",\"BlockNum\") "
                                            "values ('%s', '%s', '%s', '%s', '%s')",
                                      __db_table_scan_events, __sn_events[i].address,     __sn_events[i].topic,
                                                              __sn_events[i].transaction, __sn_events[i].data, 
                                                                oper_data ) ;
                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                             strcpy(error, db->error_text) ;
                                      break ;
                        }
                              }

             if(status)  break ;

             if(cnt!=_SN_RECORDS_MAX)  break ;

                rec_offset+=_SN_RECORDS_MAX ;

        } while(1) ;

                                          }
/*- - - - - - - - - - - - - - - - - - - -  Необрабатываемая операция */
   else                                   {

               db->UnlockCursor(Cursor) ;
                      return ;
                                          }
/*--------------------------------------------- Обработка результата */

     if(strstr(error, "TRANSPORT")!=NULL) {                         /* Если ошибка допускает повторное исполнение... */
                                             db->UnlockCursor(Cursor) ;
                                                   return ;
                                          }

     if(error[0]==0)  sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_ext_control, oper_id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_ext_control, error, oper_id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                      strcpy(error, db->error_text) ;
                    EMIR_log(error) ;

                         db->error_text[0]=0 ;
                         db->UnlockCursor(Cursor) ;
                       __db_errors_cnt++ ;
                               return ;
                }

                 db->Commit() ;

                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


