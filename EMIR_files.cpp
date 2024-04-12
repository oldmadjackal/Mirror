/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Модуль "Файловое хранилище"                   */
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

#ifndef UNIX

  static  HBITMAP  picMarkUn ;
  static  HBITMAP  picMarkWr ;
  static  HBITMAP  picMarkEr ;

#endif

/*-------------------------------------- Общие переменные управления */

/*------------------------------------ Обработчики элементов диалога */

#ifndef UNIX


  union WndProc_par {
                        long            par ;
                     LRESULT (CALLBACK *call)(HWND, UINT, WPARAM, LPARAM) ; 
                    } ;

  static union WndProc_par  Tmp_WndProc ;
//  static union WndProc_par  ConValues_WndProc ;

//     LRESULT CALLBACK  EMIRi_ConValues_WndProc(HWND, UINT, WPARAM, LPARAM) ;

#endif

/*------------------------------------------ Внутренние подпрограммы */

  void  EMIRi_fl_synch                 (HWND hDlg, char *prefix, SQL_link *db) ;              /* Процедура фоновой синхронизации объектов модуля "Файлы" */
  void  EMIRi_fl_actions               (HWND hDlg, char *prefix, SQL_link *db) ;              /* Обработка очереди операций модуля "Файлы" */
   int  EMIR_fl_action_TransferFile    (Fl_action *action, SQL_link *db, char *error) ;       /* Операция TRANSFER_FILE */
   int  EMIR_fl_action_PutEncryptedFile(Fl_action *action, SQL_link *db, char *error) ;       /* Операция PUT_ENCRYPTED_FILE */
   int  EMIR_fl_action_PutGammaFile    (Fl_action *action, SQL_link *db, char *error) ;       /* Операция PUT_GAMMA_FILE */
   int  EMIR_fl_action_SignFile        (Fl_action *action, SQL_link *db, char *error) ;       /* Операция SIGN_FILE */
   int  EMIR_fl_action_GetFile         (Fl_action *action, SQL_link *db, char *error) ;       /* Операция GET_FILE */
   int  EMIR_fl_action_GetEncryptedFile(Fl_action *action, SQL_link *db, char *error) ;       /* Операция GET_ENCRYPTED_FILE */
   int  EMIR_fl_action_GetGammaFile    (Fl_action *action, SQL_link *db, char *error) ;       /* Операция GET_GAMMA_FILE */
   int  EMIR_fl_action_DeliveryFile    (Fl_action *action, SQL_link *db, char *error) ;       /* Операция DELIVERY_FILE */


/*********************************************************************/
/*								     */
/*	      Обработчик сообщений диалогового окна FILES            */	

 INT_PTR CALLBACK  EMIR_files_dialog(  HWND  hDlg,     UINT  Msg, 
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
                    SETs(IDC_NODE_URL, __swarm_url) ;
                    SETs(IDC_REQUEST,  "") ;
                    SETs(IDC_VERSION,  _VERSION   ) ;
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
/*        THREAD - Фоновый поток модуля "Файловое хранилище"        */

  DWORD WINAPI  Files_Thread(LPVOID Pars)

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

      if(__fl_request_period<=0)  __fl_request_period= 10 ;
      if(__fl_view_frame    <=0)  __fl_view_frame    =100 ;

              hDlg=hFiles_Dialog ;

/*----------------------------------------------------- Настройка БД */

/*------------------------------------------------------- Общий цикл */

   do {
          if(__exit_flag)  break ;                                  /* Если общий стоп */ 

                      Sleep(1000) ;

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#pragma warning(disable : 4244)
            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
#pragma warning(default : 4244)
         if(rows_cnt>__fl_view_frame) {

               for(i=0 ; i<rows_cnt-__fl_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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

            sprintf(text, "%s (LVL.1) Pulse start", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Соединение с БД */
     if(!DB.connected) {

         status=DB.Connect(__db_user, __db_password,
                           __db_name, "ODBC", NULL  ) ; 
      if(status) {
                      snprintf(text, sizeof(text)-1, "%s - ERROR - DB connection: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }

                DB.SetAutoCommit(0) ;

         status=DB.AllocCursors(3) ; 
      if(status) {
                     snprintf(text, sizeof(text)-1, "%s - ERROR - DB cursors allocation: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }
                       }
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_fl_actions(hFiles_Dialog, prefix, &DB) ;
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __fl_request_period) {               /* Если пауза не истекла... */

               EMIRi_fl_synch(hMembers_Dialog, prefix, &DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

                DB.Disconnect() ;                                   /* Отсоединение с БД */

            sprintf(text, "%s (LVL.1) Pulse done", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;

      } while(1) ;

/*-------------------------------------------------------------------*/

#endif

  return(0) ;

}


/********************************************************************/
/*                                                                  */
/*     Обработчик фонового потока модуля "Файловое хранилище"       */

  void  Files_Process(SQL_link *DB)

{
  static  time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
             int  status ;
            char  prefix[512] ;
            char  text[1024] ;

#ifndef  UNIX
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

/*---------------------------------------------------- Инициализация */

      if(__net_locked)  return ;
      if( __db_locked)  return ;

      if(__fl_request_period<=0)  __fl_request_period= 10 ;
      if(__fl_view_frame    <=0)  __fl_view_frame    =100 ;

#ifndef  UNIX
              hDlg=hFiles_Dialog ;
#endif

/*------------------------------- Извлечение параметров конфигурации */

   if(time_0==0) {

        status=EMIR_db_nodepars(DB, text) ;
     if(status) {
                     EMIR_log(text) ;
                        return ;
                }

           EMIR_db_syspar(DB, "DfsDirect", __dfs_direct_box, text) ;

                }
/*------------------------------------------------------- Общий цикл */

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#ifndef  UNIX

#pragma warning(disable : 4244)
            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
#pragma warning(default : 4244)
         if(rows_cnt>__fl_view_frame) {

               for(i=0 ; i<rows_cnt-__fl_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
                                      }
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

            sprintf(text, "  FIles> %s (LVL.1) Pulse start", prefix) ;

#ifdef UNIX
             EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_fl_actions(hFiles_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __fl_request_period) {               /* Если пауза не истекла... */

               EMIRi_fl_synch(hMembers_Dialog, prefix, DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

            sprintf(text, "  Files> %s (LVL.1) Pulse done", prefix) ;

#ifdef UNIX
             EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif

/*-------------------------------------------------------------------*/
                                    
  return ;
}


/********************************************************************/
/*                                                                  */
/*    Обработка очереди операций модуля "Файловое хранилище"        */

   void  EMIRi_fl_actions(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
            int  status ;
           char  error[1024] ;
           char  text[1024] ;
            int  active_cnt ;
            int  i ;

  static  time_t  last_purge ;
          time_t  block_time ;
       struct tm *time_ddmmyy ;
            char  ts[128] ;

#define   _PURGE_DELAY  3600                     // Периодичность очистки
#define   _PURGE_DEEP   3600*24*__purge_deep     // Глубина очистки

/*------------------------------------------------------- Подготовка */

/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_FL_ACTIONS_MAX ; i++)
     if(__fl_actions[i]!=NULL) {
                                   free(__fl_actions[i]) ;
                                        __fl_actions[i]=NULL ;
                               }
/*---------------------------------------- Получение списка операций */

        Cursor=db->LockCursor("EMIRi_fl_actions") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_actions - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*- - - - - - - - - - - - - - - - - - - - -  Очистка списка операций */
   if(__purge_completed==0 &&
      __purge_deep     !=0   ) 
    if(time(NULL)>last_purge+_PURGE_DELAY) {

         block_time  =  time(NULL)-_PURGE_DEEP ;
          time_ddmmyy=gmtime(&block_time) ;

       sprintf(ts, "%04d.%02d.%02d %02d:%02d", 
                    time_ddmmyy->tm_year+1900, time_ddmmyy->tm_mon+1, time_ddmmyy->tm_mday,
                    time_ddmmyy->tm_hour, time_ddmmyy->tm_min) ;

       sprintf(text, "Purge operations to %s ...", ts) ;
      EMIR_log(text) ;

                       sprintf(text, "delete from %s where \"Status\"='DONE' and (\"MasterId\" is null or \"MasterId\"='') and \"CreateDate\"<'%s' ",
                                 __db_table_files_actions, ts) ;
         status=db->SqlExecute(Cursor, text, NULL, 0) ;
      if(status) {
                   snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_actions - Operations purge : %s", prefix, db->error_text) ;
                 LB_ADD_ROW(IDC_LOG, text) ;
                       __db_errors_cnt++ ;
                 }

                        db->Commit() ;

                    last_purge=time(NULL) ;

                  EMIR_log("Purge done") ;

                                           }
/*- - - - - - - - - - - - - - - - - - - -  Получение списка операций */
                      sprintf(text, "select \"Id\", \"Action\", \"ObjectPath\", \"LocalPath\", \"DfsPath\", "
                                    "       \"Executor\", \"Receivers\", \"Status\", \"Reply\", \"MasterId\", \"Error\" "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','HOLD') "
                                    "order by \"Id\"",
                                       __db_table_files_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_synch - Get actions list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

   for(i=0 ; i<_FL_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_synch - Get actions list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                   __fl_actions[i]=(struct Fl_action *)calloc(1, sizeof(*__fl_actions[i])) ;
           strncpy(__fl_actions[i]->id,          (char *)Cursor->columns[ 0].value, sizeof(__fl_actions[i]->id         )-1) ;
           strncpy(__fl_actions[i]->action,      (char *)Cursor->columns[ 1].value, sizeof(__fl_actions[i]->action     )-1) ;
           strncpy(__fl_actions[i]->object_path, (char *)Cursor->columns[ 2].value, sizeof(__fl_actions[i]->object_path)-1) ;
           strncpy(__fl_actions[i]->local_path,  (char *)Cursor->columns[ 3].value, sizeof(__fl_actions[i]->local_path )-1) ;
           strncpy(__fl_actions[i]->dfs_path,    (char *)Cursor->columns[ 4].value, sizeof(__fl_actions[i]->dfs_path   )-1) ;
           strncpy(__fl_actions[i]->executor,    (char *)Cursor->columns[ 5].value, sizeof(__fl_actions[i]->executor   )-1) ;
           strncpy(__fl_actions[i]->receivers,   (char *)Cursor->columns[ 6].value, sizeof(__fl_actions[i]->receivers  )-1) ;
           strncpy(__fl_actions[i]->status,      (char *)Cursor->columns[ 7].value, sizeof(__fl_actions[i]->status     )-1) ;
           strncpy(__fl_actions[i]->reply,       (char *)Cursor->columns[ 8].value, sizeof(__fl_actions[i]->reply      )-1) ;
           strncpy(__fl_actions[i]->master_id,   (char *)Cursor->columns[ 9].value, sizeof(__fl_actions[i]->master_id  )-1) ;
           strncpy(__fl_actions[i]->error,       (char *)Cursor->columns[10].value, sizeof(__fl_actions[i]->error      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

/*---------------- Получение имени сертификата приватного ключа узла */

                      sprintf(text, "select \"MemberSign\" "
                                    "from   %s", __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_actions - Get 'MemberSign': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_fl_actions - Get 'MemberSign': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(__member_sign, 0, sizeof(__member_sign)) ;
           strncpy(__member_sign, (char *)Cursor->columns[0].value, sizeof(__member_sign)-1) ;

                 db->SelectClose(Cursor) ;
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------ Проходы по списку */

    do {
               active_cnt=0 ;

/*------------------------------------------------- Перебор операций */

        for(i=0 ; i<_FL_ACTIONS_MAX ; i++) {

            if(__fl_actions[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - Операция TRANSFER_FILE */
            if(!stricmp(__fl_actions[i]->action, "TRANSFERFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : TRANSFER_FILE : %s - %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : TRANSFER_FILE : %s/ERROR - %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_fl_action_TransferFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_TransferFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - -  Операция PUT_ENCRYPTED_FILE */
            if(!stricmp(__fl_actions[i]->action, "PUTENCRYPTEDFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : PUT_ENCRYPTED_FILE : %s - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : PUT_ENCRYPTED_FILE : %s/ERROR - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_fl_action_PutEncryptedFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_PutEncryptedFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                      }
/*- - - - - - - - - - - - - - - - - - -  Операция PUT_GAMMA_FILE */
            if(!stricmp(__fl_actions[i]->action, "PUTGAMMAFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : PUT_GAMMA_FILE : %s - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : PUT_GAMMA_FILE : %s/ERROR - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_fl_action_PutGammaFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_PutGammaFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - Операция SIGN_FILE */
            if(!stricmp(__fl_actions[i]->action, "SIGNFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : SIGN_FILE : %s - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : SIGN_FILE : %s/ERROR - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->local_path, __fl_actions[i]->receivers) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_fl_action_SignFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_SignFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                              }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Операция GET_FILE */
            if(!stricmp(__fl_actions[i]->action, "GETFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_FILE : %s - %s -> %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->local_path) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_FILE : %s/ERROR - %s -> %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->local_path) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_fl_action_GetFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_GetFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                             }
/*- - - - - - - - - - - - - - - - - - -  Операция GET_ENCRYPTED_FILE */
            if(!stricmp(__fl_actions[i]->action, "GETENCRYPTEDFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_ENCRYPTED_FILE : %s - %s -> %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->local_path) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_ENCRYPTED_FILE : %s/ERROR - %s -> %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->local_path) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_fl_action_GetEncryptedFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_GetEncryptedFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                      }
/*- - - - - - - - - - - - - - - - - - -  Операция GET_GAMMA_FILE */
            if(!stricmp(__fl_actions[i]->action, "GETGAMMAFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_GAMMA_FILE : %s - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->receivers) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_GAMMA_FILE : %s/ERROR - %s for %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path, __fl_actions[i]->receivers) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                     status=EMIR_fl_action_GetGammaFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_GetGammaFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - - - - - Операция DELIVERY_FILE */
            if(!stricmp(__fl_actions[i]->action, "DELIVERYFILE")) {

                if(__fl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : DELIVERY_FILE : %s - %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : DELIVERY_FILE : %s/ERROR - %s", prefix, __fl_actions[i]->id, __fl_actions[i]->status, __fl_actions[i]->dfs_path) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__fl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_fl_action_DeliveryFile(__fl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_fl_action_DeliveryFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           }
/*------------------------------------------------ Проходы по списку */

       } while(active_cnt) ;

/*-------------------------------------------------------------------*/

}


/*********************************************************************/
/*								     */
/*                        Операция TRANSFER_FILE                     */

   int  EMIR_fl_action_TransferFile(Fl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
            char  text[4096] ;
            char  reply[2048] ;
            char  path[FILENAME_MAX] ;
   static time_t  next_file_time ;

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW"))  return(0) ;

   if(time(NULL)<next_file_time)   return(0) ;

/*------------------------------------------- Загрузка файла в SWARM */

           error[0]=0 ;

  do {

    if(action->local_path[0]==0) {
                 strcpy(error, "Source file is not defined") ;
                     break ;
                                 }
    if(access(action->local_path, 0x04)) {
                 strcpy(error, "Source file is absent or not accessible") ;
                     break ;
                                         }

       status=EMIR_dfs_putfile(action->local_path, reply, error) ;
    if(status!=0)  break ;

    if(__work_folder[0]==0) {                                       /* Получение файла из SWARM */
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//       snprintf(path, sizeof(path)-1, "%s/%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path, sizeof(path)-1, "%s/TransferFile.%s", __work_folder, action->id) ;
#else
//       snprintf(path, sizeof(path)-1, "%s\\%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path, sizeof(path)-1, "%s\\TransferFile.%s", __work_folder, action->id) ;
#endif

    if(!stricmp(__dfs_type, "SWARM"))                               /* Пауза */
     if(__swarm_pause>0)  next_file_time=time(NULL)+__swarm_pause ;

       status=EMIR_dfs_getfile(reply, path, error) ;
    if(status!=0)  break ;

       status=EMIR_compare_files(path, action->local_path) ;        /* Сравнение файлов */
    if(status) {
                   sprintf(error, "Invalid file replication on DFS: %s", path) ;
                      break ;
               } 

                                                       unlink(path) ;

    if(strstr(action->local_path, "KillAfter")!=NULL)  unlink(action->local_path) ;

     } while(0) ;

/*---------------------------------------- Занесение результата в БД */

   if(strstr(error, "TRANSPORT")==NULL) {

        Cursor=db->LockCursor("EMIR_fl_action_TransferFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                          __db_errors_cnt++ ;
                          __db_locked=1 ;
                }
     else       {
                       db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

                                        }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                   Операция PUT_ENCRYPTED_FILE                     */

   int  EMIR_fl_action_PutEncryptedFile(Fl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
            char  text[4096] ;
            char  reply[2048] ;
            char  path_e[FILENAME_MAX] ;
            char  path_g[FILENAME_MAX] ;
            char  path_d[FILENAME_MAX] ;
   static time_t  next_file_time ;

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW"))  return(0) ;

   if(time(NULL)<next_file_time)   return(0) ;

/*--------------------------------------------------- Загрузка файла */

                error[0]=0 ;

  do {

    if(action->local_path[0]==0) {
                 strcpy(error, "Source file is not defined") ;
                     break ;
                                 }
    if(access(action->local_path, 0x04)) {
                 strcpy(error, "Source file is absent or not accessible") ;
                     break ;
                                         }

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_crypto_inpack(action->local_path, path_e,        /* Шифрование файла */
                                   action->receivers, error) ;
    if(status)  break ;

       status=EMIR_dfs_putfile(path_e, reply, error);               /* Загрузка файла в DFS */
    if(status)  break ;

#ifdef  UNIX
//       snprintf(path_g, sizeof(path_g)-1, "%s/Get%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path_g, sizeof(path_g)-1, "%s/Get.%s", __work_folder, action->id) ;
#else
//       snprintf(path_g, sizeof(path_g)-1, "%s\\Get%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path_g, sizeof(path_g)-1, "%s\\Get.%s", __work_folder, action->id) ;
#endif

    if(!stricmp(__dfs_type, "SWARM"))                               /* Пауза */
     if(__swarm_pause>0)  next_file_time=time(NULL)+__swarm_pause ;

       status=EMIR_dfs_getfile(reply, path_g, error) ;              /* Выгрузка файла из DFS */
    if(status)  break ;

#ifdef  UNIX
//        snprintf(path_d, sizeof(path_d)-1, "%s/Decrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_d, sizeof(path_d)-1, "%s/Decrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_d, sizeof(path_d)-1, "%s\\Decrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_d, sizeof(path_d)-1, "%s\\Decrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_crypto_unpack(path_g, path_d,                    /* Дешифровывание файла */
                                    __member_sign, error) ;
    if(status)  break ;

       status=EMIR_compare_files(path_d, action->local_path) ;      /* Сравнение файлов */
    if(status) {
                   sprintf(error, "Invalid file replication on DFS: %s", path_d) ;
                      break ;
               } 

                                                       unlink(path_e) ;
                                                       unlink(path_g) ;
                                                       unlink(path_d) ;

    if(strstr(action->local_path, "KillAfter")!=NULL)  unlink(action->local_path) ;

     } while(0) ;

/*---------------------------------------- Занесение результата в БД */

   if(strstr(error, "TRANSPORT")==NULL) {

        Cursor=db->LockCursor("EMIR_fl_action_PutEncryptedFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                }
     else       {
                       db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

                                        }

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                   Операция PUT_GAMMA_FILE                         */

   int  EMIR_fl_action_PutGammaFile(Fl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
            char  text[4096] ;
            char  reply[2048] ;
            char  path_e[FILENAME_MAX] ;
            char  path_g[FILENAME_MAX] ;
            char  path_d[FILENAME_MAX] ;
   static time_t  next_file_time ;

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW"))  return(0) ;

   if(time(NULL)<next_file_time)   return(0) ;

/*--------------------------------------------------- Загрузка файла */

                error[0]=0 ;

  do {

    if(action->local_path[0]==0) {
                 strcpy(error, "Source file is not defined") ;
                     break ;
                                 }
    if(access(action->local_path, 0x04)) {
                 strcpy(error, "Source file is absent or not accessible") ;
                     break ;
                                         }

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_gamma_inpack(action->local_path, path_e,         /* Шифрование файла */
                                   action->receivers, error) ;
    if(status)  break ;

       status=EMIR_dfs_putfile(path_e, reply, error);               /* Загрузка файла в DFS */
    if(status)  break ;

#ifdef  UNIX
//       snprintf(path_g, sizeof(path_g)-1, "%s/Get%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path_g, sizeof(path_g)-1, "%s/Get.%s", __work_folder, action->id) ;
#else
//       snprintf(path_g, sizeof(path_g)-1, "%s\\Get%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
         snprintf(path_g, sizeof(path_g)-1, "%s\\Get.%s", __work_folder, action->id) ;
#endif

    if(!stricmp(__dfs_type, "SWARM"))                               /* Пауза */
     if(__swarm_pause>0)  next_file_time=time(NULL)+__swarm_pause ;

       status=EMIR_dfs_getfile(reply, path_g, error) ;              /* Выгрузка файла из DFS */
    if(status)  break ;

#ifdef  UNIX
//        snprintf(path_d, sizeof(path_d)-1, "%s/Decrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_d, sizeof(path_d)-1, "%s/Decrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_d, sizeof(path_d)-1, "%s\\Decrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_d, sizeof(path_d)-1, "%s\\Decrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_gamma_unpack(path_g, path_d,                    /* Дешифровывание файла */
                                    action->receivers, error) ;
    if(status)  break ;

       status=EMIR_compare_files(path_d, action->local_path) ;      /* Сравнение файлов */
    if(status) {
                   sprintf(error, "Invalid file replication on DFS: %s", path_d) ;
                      break ;
               } 

                                                       unlink(path_e) ;
                                                       unlink(path_g) ;
                                                       unlink(path_d) ;

    if(strstr(action->local_path, "KillAfter")!=NULL)  unlink(action->local_path) ;

     } while(0) ;

/*---------------------------------------- Занесение результата в БД */

   if(strstr(error, "TRANSPORT")==NULL) {

        Cursor=db->LockCursor("EMIR_fl_action_PutEncryptedFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                }
     else       {
                       db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

                                        }

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция SIGN_FILE                         */

   int  EMIR_fl_action_SignFile(Fl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  text[2048] ;

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW"))  return(0) ;

/*--------------------------------------------------- Загрузка файла */

                error[0]=0 ;

       status=EMIR_crypto_sign(action->object_path,                 /* Формирование подписи файла */
                               action->local_path, 
                               action->receivers, error) ;

/*---------------------------------------- Занесение результата в БД */

        Cursor=db->LockCursor("EMIR_fl_action_SignFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"=''"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                   db->Rollback() ;
                }
     else       {
                                   db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция GET_FILE                          */

   int  EMIR_fl_action_GetFile(Fl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
        char  path_e[FILENAME_MAX] ;
         int  status ;
        char  command[1024] ;
        char  text[2048] ;
        char  reply[128] ; 
      time_t  time_next ;

#ifdef  UNIX
#define     _TIME_FMT   "%lx"
#else
#define     _TIME_FMT   "%llx"
#endif

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW" ) &&
      stricmp(action->status, "WAIT")   )  return(0) ;

/*----------------------------------------- Отработка ожидания файла */

   if(!stricmp(action->status, "WAIT")) {                           /* Если мы запрашиваем файл и сразу его не получили -     */
                                                                    /*  - следующие попытки делаем с некоторой периодичностью */
                                            time_next=0 ;
          sscanf(action->reply, _TIME_FMT, &time_next) ;
      if(time_next>time(NULL))  return(0) ;

                                        }
/*-------------------------------------------- Выгрузка файла из DFS */

                error[0]=0 ;

  do {

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Transfer%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Transfer.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Transfer%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Transfer.%s", __work_folder, action->id) ;
#endif

       status=EMIR_dfs_getfile(action->dfs_path, path_e, error) ; /* Выгрузка файла из DFS */
    if(status)  break ;

#ifdef  UNIX
              errno=0 ;

             snprintf(command, sizeof(command)-1, "cp %s %s", path_e, action->local_path) ;
#else
          _set_errno(0) ;

             snprintf(command, sizeof(command)-1, "copy %s %s", path_e, action->local_path) ;
#endif

       status=system(command) ;
    if(status!=  0   ||
       errno ==ENOENT  ) {
                 sprintf(error, "TRANSPORT - Ошибка копирования файла (status=%d errno=%d) : %s", status, errno, command) ;
                             break ;                           
                         }

                          unlink(path_e) ;

     } while(0) ;

    if(strstr(error, "TRANSPORT")!=NULL) {                          /* При ошибке транспорта - отрабатываем WAIT */
                                             error[0]=0 ;
                                               status=1 ;  
                                         }
/*---------------------------------------- Занесение результата в БД */
                                       
        Cursor=db->LockCursor("EMIR_fl_action_GetFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==1)  sprintf(reply, _TIME_FMT, time(NULL)+60) ;
     else            strcpy(reply, "") ; 

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(status==1)    sprintf(text, "update %s "
                                    "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"=''"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция GET_ENCRYPTED_FILE                */

   int  EMIR_fl_action_GetEncryptedFile(Fl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
        char  path_e[FILENAME_MAX] ;
         int  status ;
        char  text[2048] ;
        char  reply[128] ; 
      time_t  time_next ;

#ifdef  UNIX
#define     _TIME_FMT   "%lx"
#else
#define     _TIME_FMT   "%llx"
#endif

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW" ) &&
      stricmp(action->status, "WAIT")   )  return(0) ;

/*----------------------------------------- Отработка ожидания файла */

   if(!stricmp(action->status, "WAIT")) {                           /* Если мы запрашиваем файл и сразу его не получили -     */
                                                                    /*  - следующие попытки делаем с некоторой периодичностью */
                                            time_next=0 ;
          sscanf(action->reply, _TIME_FMT, &time_next) ;

      if(time_next>time(NULL))  return(0) ;

                                        }
/*------------------------------------------ Выгрузка файла из SWARM */

                error[0]=0 ;

  do {

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_dfs_getfile(action->dfs_path, path_e, error) ; /* Выгрузка файла из SWARM */
    if(status)  break ;

       status=EMIR_crypto_unpack(path_e, action->local_path,        /* Дешифровывание файла */
                                              __member_sign, error) ;
    if(status)  break ;

                          unlink(path_e) ;

     } while(0) ;

    if(strstr(error, "TRANSPORT")!=NULL) {                          /* При ошибке транспорта - отрабатываем WAIT */
                                             error[0]=0 ;
                                               status=1 ;  
                                         }
/*---------------------------------------- Занесение результата в БД */

        Cursor=db->LockCursor("EMIR_fl_action_GetEncryptedFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==1)  sprintf(reply, _TIME_FMT, time(NULL)+60) ;
     else            strcpy(reply, "") ; 

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(status==1)    sprintf(text, "update %s "
                                    "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"=''"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                }
     else       {
                                   db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция GET_GAMMA_FILE                    */

   int  EMIR_fl_action_GetGammaFile(Fl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
        char  path_e[FILENAME_MAX] ;
         int  status ;
        char  text[2048] ;
        char  reply[128] ; 
      time_t  time_next ;

#ifdef  UNIX
#define     _TIME_FMT   "%lx"
#else
#define     _TIME_FMT   "%llx"
#endif

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW" ) &&
      stricmp(action->status, "WAIT")   )  return(0) ;

/*----------------------------------------- Отработка ожидания файла */

   if(!stricmp(action->status, "WAIT")) {                           /* Если мы запрашиваем файл и сразу его не получили -     */
                                                                    /*  - следующие попытки делаем с некоторой периодичностью */
                                            time_next=0 ;
          sscanf(action->reply, _TIME_FMT, &time_next) ;

      if(time_next>time(NULL))  return(0) ;

                                        }
/*------------------------------------------ Выгрузка файла из SWARM */

                error[0]=0 ;

  do {

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Encrypted.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Encrypted.%s", __work_folder, action->id) ;
#endif

       status=EMIR_dfs_getfile(action->dfs_path, path_e, error) ;   /* Выгрузка файла из SWARM */
    if(status)  break ;

       status=EMIR_gamma_unpack(path_e, action->local_path,         /* Дешифровывание файла */
                                        action->receivers, error) ;
    if(status)  break ;

                          unlink(path_e) ;

     } while(0) ;

    if(strstr(error, "TRANSPORT")!=NULL) {                          /* При ошибке транспорта - отрабатываем WAIT */
                                             error[0]=0 ;
                                               status=1 ;  
                                         }
/*---------------------------------------- Занесение результата в БД */

        Cursor=db->LockCursor("EMIR_fl_action_GetEncryptedFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==1)  sprintf(reply, _TIME_FMT, time(NULL)+60) ;
     else            strcpy(reply, "") ; 

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(status==1)    sprintf(text, "update %s "
                                    "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"=''"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                }
     else       {
                       db->Commit() ;
                }

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция DELIVERY_FILE                     */

   int  EMIR_fl_action_DeliveryFile(Fl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
        char  path_e[FILENAME_MAX] ;
         int  status ;
        char  text[2048] ;
        char  reply[128] ; 
      time_t  time_next ;

#ifdef  UNIX
#define     _TIME_FMT   "%lx"
#else
#define     _TIME_FMT   "%llx"
#endif

/*------------------------------------------------- Входной контроль */

   if(stricmp(action->status, "NEW" ) &&
      stricmp(action->status, "WAIT")   )  return(0) ;

/*----------------------------------------- Отработка ожидания файла */

   if(!stricmp(action->status, "WAIT")) {                           /* Если мы запрашиваем файл и сразу его не получили -     */
                                                                    /*  - следующие попытки делаем с некоторой периодичностью */
                                            time_next=0 ;
          sscanf(action->reply, _TIME_FMT, &time_next) ;
      if(time_next>time(NULL))  return(0) ;

                                        }
/*-------------------------------------------- Выгрузка файла из DFS */

                error[0]=0 ;

  do {

    if(__work_folder[0]==0) {
                               strcpy(error, "Work folder is not defined") ;
                                 break ;
                            }  

#ifdef  UNIX
//        snprintf(path_e, sizeof(path_e)-1, "%s/Delivery%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s/Delivery.%s", __work_folder, action->id) ;
#else
//        snprintf(path_e, sizeof(path_e)-1, "%s\\Delivery%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
          snprintf(path_e, sizeof(path_e)-1, "%s\\Delivery.%s", __work_folder, action->id) ;
#endif

       status=EMIR_dfs_getfile(action->dfs_path, path_e, error) ;   /* Выгрузка файла из DFS */
    if(status)  break ;

                        unlink(path_e) ;                            /* Удаляем принятый файл */

     } while(0) ;

    if(strstr(error, "TRANSPORT")!=NULL) {                          /* При ошибке транспорта - отрабатываем WAIT */
                                             error[0]=0 ;
                                               status=1 ;  
                                         }
/*---------------------------------------- Занесение результата в БД */
                                       
        Cursor=db->LockCursor("EMIR_fl_action_DeliveryFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==1)  sprintf(reply, _TIME_FMT, time(NULL)+60) ;
     else            strcpy(reply, "") ; 

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, error, action->id) ;
     else
     if(status==1)    sprintf(text, "update %s "
                                    "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, reply, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"=''"
                                    "where  \"Id\"=%s",
                                       __db_table_files_actions, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/********************************************************************/
/*                                                                  */
/*              Процедура фоновой синхронизации объектов            */
/*                           модуля "Файлы"                         */

   void  EMIRi_fl_synch(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
 static Sn_event *events ;
             int  events_cnt ;
            char  event_id[128] ;
            char  event_topic[1024] ;
            char  event_data[1024] ;
             int  status ;
            char  text[4096] ;
             int  i ;

/*------------------------------------------------- Входной контроль */

    if(!__files_delivery)  return ;

/*--------------------------------------------------- Захват курсора */

        Cursor=db->LockCursor("EMIRi_fl_synch") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_fl_synch - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                          __db_errors_cnt++ ;
                                return ;
                      }
/*------------------------------------------------------- Подготовка */

/*----------------------------------- Добавление операций по событию */

                      sprintf(text, "Select \"Id\", \"Topic\", \"Data\" "
                                    "From   %s "
                                    "Where  \"Topic\" in ('079c23bad522efe91172294142fad8c83e7c180220b72068a2d0c36b25e3c2a2', "
                                    "                     '6f3ea065f22c9e75ba8d5a3124cc788be17d14c2b11c3a7cfd67e0622127b301' ) "
                                    "Order by \"Id\""   ,
                                     __db_table_scan_events) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_fl_synch - Get Files events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                              return ;
                }

                events_cnt=0 ;
 
   for(i=0 ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_fl_synch - Get Files events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                              return ;
                }

           strncpy(event_id,    (char *)Cursor->columns[0].value, sizeof(event_id   )-1) ;
           strncpy(event_topic, (char *)Cursor->columns[1].value, sizeof(event_topic)-1) ;
           strncpy(event_data , (char *)Cursor->columns[2].value, sizeof(event_data )-1) ;

EMIR_log("Files event detected") ;
EMIR_log(event_topic) ;
EMIR_log(event_data) ;

           events=(Sn_event *)realloc(events, (events_cnt+1)*sizeof(*events)) ;

    if(__files_delivery)
     if(!memicmp(event_topic, "079c", 4) ||                         /* Распространение файла */
        !memicmp(event_topic, "6f3e", 4)   ) {

       if(!memicmp(event_topic, "079c", 4))                         
                    EMIR_hex2txt(event_data, event_data) ;          /* Извлечение данных bytes32,bytes32 */
       else         EMIR_hex2txt(event_data+2*64, event_data) ;     /* Извлечение данных string */

           strncpy(events[events_cnt].id,    event_id,    sizeof(events[events_cnt].id   )-1) ;
           strncpy(events[events_cnt].topic, event_topic, sizeof(events[events_cnt].topic)-1) ;
           strncpy(events[events_cnt].data,  event_data,  sizeof(events[events_cnt].data )-1) ;
                          events_cnt++ ;
                                             }
                    }

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - -  Запрос добавления/обновления клиентов */
   for(i=0 ; i<events_cnt ; i++) {

     if(!memicmp(events[i].topic, "079c", 4) ||                     /* Распространение файла */
        !memicmp(events[i].topic, "6f3e", 4)   ) {

           if(event_data[0]==0)  continue ;

                        sprintf(text, "File %s - DELIVERY", events[i].data) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                        sprintf(text, "insert into %s (\"Action\",    \"DfsPath\", \"Status\")"
                                      "         values('DeliveryFile', '%s',        'NEW')"  ,
                                        __db_table_files_actions, events[i].data) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_fl_synch - Files add delivery operation : %s", prefix, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                  }
                                                 }

                                 }
/*- - - - - - - - - - - - - - - - - -  Удаление обработанных событий */
      for(i=0 ; i<events_cnt ; i++) {

                        sprintf(text, "Delete from %s "
                                      "Where  \"Id\"=%s"   ,
                                     __db_table_scan_events, events[i].id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(text, "%s - ERROR - EMIRi_fl_synch - Delete Event (Id=%s) : %s", prefix, events[i].id, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                return ;
                  }
                                    }
/*------------------------------------------------- Коммит изменений */

                     db->Commit() ;

/*--------------------------------------------- Освобождение курсора */

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/
}


