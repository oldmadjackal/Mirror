/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Модуль системной конфигурации                 */
/*                                                                   */
/*********************************************************************/

#ifdef UNIX
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

#if defined UNIX || defined _CONSOLE
#else
#include "controls.h"
#include "resource.h"
#endif

#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)

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

  void  EMIRi_sc_synch           (HWND hDlg, char *prefix, SQL_link *db) ;                        /* Процедура фоновой синхронизации объектов модуля "Системная конфигурация" */
  void  EMIRi_sc_actions         (HWND hDlg, char *prefix, SQL_link *db) ;                        /* Обработка очереди операций модуля "Системная конфигурация" */
   int  EMIR_sc_action_Identify  (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция IDENTIFY */
   int  EMIR_sc_action_GetValue  (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция GETVALUE */
   int  EMIR_sc_action_GetStatus (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция GETSTATUS */
   int  EMIR_sc_action_Create    (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция CREATE */
   int  EMIR_sc_action_SetValue  (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция SETVALUE */
   int  EMIR_sc_action_AltConfig (Sc_action *action, SQL_link *db, char *error) ;                 /* Операция ALTER_CONFIGURATION */
   int  EMIR_sc_action_RaiseAlert(Sc_action *action, SQL_link *db, char *error) ;                 /* Операция RAISE_ALERT */
   int  EMIRi_sc_Identify        (char *contract, char  *reply, char *error) ;                    /* Запрос Identify на смарт контракт */
   int  EMIRi_sc_GetStatus       (char *contract, char  *state_id, char  *config, char *error) ;  /* Запрос GetStatus смарт-контракта SystemAlert */
   int  EMIRi_sc_GetKeys         (char *contract, SysPar **keys, int *keys_cnt, char *error) ;    /* Запрос GetKeys смарт-контракта Configuration */
   int  EMIRi_sc_GetValue        (char *contract, char  *key, char  *value, char *error) ;        /* Запрос GetValue смарт-контракта Configuration */


/*********************************************************************/
/*								     */
/*	      Обработчик сообщений диалогового окна SYS_CONFIG       */	

  INT_PTR CALLBACK  EMIR_sysconfig_dialog(  HWND  hDlg,     UINT  Msg,
                                          WPARAM  wParam, LPARAM  lParam) 
{
#if defined UNIX || defined _CONSOLE

    return(0) ;

#else

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
//         SendMessage(ITEM(IDC_REQUEST), WM_SETFONT, (WPARAM)font, 0) ;
/*- - - - - - - - - - - - - - - - Инициализация переключателя секций */
       for(i=0 ; i<__sections_cnt ; i++) {
                        tab_item.mask   =TCIF_TEXT ;
                        tab_item.pszText=__sections[i].title ;
                        tab_item.lParam =           i ;
         TabCtrl_InsertItem(ITEM(IDC_SECTIONS_SWITCH), i, &tab_item) ;
                                         }
/*- - - - - - - - - - - - - - - - -  Инициализация значеий элементов */
                    SETs(IDC_NODE_URL, __node_url) ;
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

                         row=LB_GET_ROW (IDC_LOG) ;
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

}


/********************************************************************/
/*                                                                  */
/*       THREAD - Фоновый поток модуля "Системная конфигурация"     */

  DWORD WINAPI  SysConfig_Thread(LPVOID Pars)

{
#if defined UNIX || defined _CONSOLE
#else

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

      if( __db_locked)  return ;

               time_0=0 ;

      if(__sc_request_period<=0)  __sc_request_period= 10 ;
      if(__sc_view_frame    <=0)  __sc_view_frame    =100 ;

              hDlg=hSysConfig_Dialog ;

/*------------------------------------------------------- Общий цикл */

   do {
          if(__exit_flag)  break ;                                  /* Если общий стоп */ 

                      Sleep(1000) ;

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__sc_view_frame) {

               for(i=0 ; i<rows_cnt-__sc_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __sc_request_period) {               /* Если пауза не истекла... */

             EMIRi_sc_synch(hSysConfig_Dialog, prefix, &DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_sc_actions(hSysConfig_Dialog, prefix, &DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

            sprintf(text, "%s (LVL.1) Pulse done", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;

      } while(1) ;

/*------------------------------------------------ Отсоединение с БД */

                DB.Disconnect() ;                                   /* Отсоединение с БД */

/*-------------------------------------------------------------------*/

#endif
                                    
  return(0) ;

}


/********************************************************************/
/*                                                                  */
/*     Обработчик фонового потока модуля "Системная конфигурация"   */

  void  SysConfig_Process(SQL_link *DB)

{
   static time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;

#if defined UNIX || defined _CONSOLE
#else
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

/*---------------------------------------------------- Инициализация */

      if( __db_locked)  return ;

      if(__sc_request_period<=0)  __sc_request_period= 10 ;
      if(__sc_view_frame    <=0)  __sc_view_frame    =100 ;

#if defined UNIX || defined _CONSOLE
#else
              hDlg=hSysConfig_Dialog ;
#endif

/*------------------------------------------------------- Общий цикл */

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#if defined UNIX || defined _CONSOLE
#else

            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__sc_view_frame) {

               for(i=0 ; i<rows_cnt-__sc_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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

            sprintf(text, "System>  %s (LVL.1) Pulse start\n", prefix) ;

#if defined UNIX || defined _CONSOLE
         EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
#endif
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __sc_request_period) {               /* Если пауза не истекла... */

             EMIRi_sc_synch(hSysConfig_Dialog, prefix, DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_sc_actions(hSysConfig_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

            sprintf(text, " System> %s (LVL.1) Pulse done", prefix) ;

#if defined UNIX || defined _CONSOLE
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
/*              Процедура фоновой синхронизации объектов            */
/*                    модуля "Системная конфигурация"               */

   void  EMIRi_sc_synch(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
 static  SysPar *keys ;
            int  keys_cnt ;
           char  addr_system_alert[128] ;
           char  addr_config[128] ;
           char  state_id[128] ;
           char  state_id_last[128] ;
           char  addr_config_last[128] ;
           char  value[1024] ;
            int  update_data ;
            int  status ;
           char  error[1024] ;
           char  text[2048] ;
            int  i ;

/*--------------------------- Запрос адреса СК системного оповещения */

        Cursor=db->LockCursor("EMIRi_sc_synch") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               return ;
                      }

                       memset(addr_system_alert, 0, sizeof(addr_system_alert)) ;
        status=EMIR_db_syspar(db, "SystemAlert", addr_system_alert, error) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

          snprintf(text, sizeof(text)-1, "%s (LVL.1) SystemAlert address: %s", prefix, addr_system_alert) ;
        LB_ADD_ROW(IDC_LOG, text) ;

                 db->SelectClose(Cursor) ;

/*---------------------------------- Запрос СК системного оповещения */

                         memset(state_id,    0, sizeof(state_id)) ;
                         memset(addr_config, 0, sizeof(addr_config)) ;
      status=EMIRi_sc_GetStatus(addr_system_alert, state_id, addr_config, error) ;
   if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get SystemAlert State': %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
              }

           sprintf(text, "%s (LVL.1) SystemAlert state : %s", prefix, state_id) ;
        LB_ADD_ROW(IDC_LOG, text) ;
           sprintf(text, "%s (LVL.1) SystemAlert config: %s", prefix, addr_config) ;
        LB_ADD_ROW(IDC_LOG, text) ;
/*-- Запрос последнего системного состояния и адреса СК конфигурации */
                      sprintf(text, "select \"SystemState\", \"Configuration\" "
                                    "from   %s", __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get 'SystemState': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get 'SystemState': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(state_id_last,    0, sizeof(state_id_last)) ;
           strncpy(state_id_last,    (char *)Cursor->columns[0].value, sizeof(state_id_last)-1) ;
            memset(addr_config_last, 0, sizeof(addr_config_last)) ;
           strncpy(addr_config_last, (char *)Cursor->columns[1].value, sizeof(addr_config_last)-1) ;

    if(!stricmp(state_id_last, state_id)) {
                                            db->UnlockCursor(Cursor) ;
                                                  return ;
                                          }

           sprintf(text, "%s - System configuration changed : %s != %s", prefix, state_id, state_id_last) ;
        LB_ADD_ROW(IDC_LOG, text) ;

                 db->SelectClose(Cursor) ;

/*----------------------------------- Проверка смены СК конфигурации */

    if(stricmp(addr_config_last, addr_config)) {

                     db->UnlockCursor(Cursor) ;

           sprintf(text, "%s - Configuration contract changed : %s != %s", prefix, addr_config, addr_config_last) ;
        LB_ADD_ROW(IDC_LOG, text) ;

                                                     return ;
                                               }
/*---------------------- Получение параметров системной конфигурации */

     if(keys!=NULL)  free(keys) ;
                          keys=NULL ;

        status=EMIRi_sc_GetKeys(addr_config_last,                   /* Запрос GetMembers смарт-контракта Members */
                                           &keys, &keys_cnt, error) ; 
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get configuration parameters list : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }
/*--------------------- Обновление параметров системной конфигурации */

    for(i=0 ; i<keys_cnt ; i++) {                                   /* LOOP - перебор параметров конфигурации */
/*- - - - - - - - - - - - - - - -  Запрос данных на СК Configuration */
                     memset(value, 0, sizeof(value)); 
          EMIRi_sc_GetValue(addr_config_last, keys[i].key, value, error) ;

      if(strstr(keys[i].key, "(s)"     )!=NULL)  EMIR_hex2txt(value, value) ;
      else
      if(strstr(keys[i].key, "Template")==NULL)  value[40]=0 ;
/*- - - - - - - - - -  Проверка наличия данных в таблице SYSTEM_PARS */
                       sprintf(text, "select count(*) "
                                     "from %s "
                                     "where \"Key\"='%s'", __db_table_system_configuration, keys[i].key) ;
         status=db->SelectOpen(Cursor, text, NULL, 0) ;
      if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Check key existance %s : %s", prefix, keys[i].key, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

         status=db->SelectFetch(Cursor) ;
//    if(status==_SQL_NO_DATA)   break ;
      if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Check key existance %s : %s", prefix, keys[i].key, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                 }

      if(stricmp((char *)Cursor->columns[0].value, "0"))  update_data=1 ;
      else                                                update_data=0 ;

                 db->SelectClose(Cursor) ; 
/*- - - - - - - - -  Занесение данных в таблицу SYSTEM_CONFIGURATION */
                       snprintf(text, sizeof(text)-1, "%s - Configuration parameters import : %s = %s", prefix, keys[i].key, value) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

      if(update_data)  sprintf(text, "update      %s set \"Value\"='%s' where \"Key\"='%s'",
                                        __db_table_system_configuration, value, keys[i].key) ;
      else             sprintf(text, "insert into %s(\"Key\", \"Value\") Values('%s','%s')",
                                        __db_table_system_configuration, keys[i].key, value) ;

         status=db->SqlExecute(Cursor, text, NULL, 0) ;
      if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Insert/update configuration parameter : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                }                                   /* END LOOP - перебор параметров конфигурации */
/*---------------------------------- Обновление системного состояния */

                      sprintf(text, "update %s set \"SystemState\"='%s'", __db_table_system_pars, state_id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get 'SystemState': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }
/*----------------------------------------------- Оповещение модулей */

                      sprintf(text, "update %s set \"Flag\"=1", __db_table_system_alert) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Modules alerting : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }
/*------------------------------------------------- Коммит изменений */

                     db->Commit() ;

           sprintf(text, "%s - System configuration Uploaded", prefix) ;
        LB_ADD_ROW(IDC_LOG, text) ;

/*--------------------------------------------- Освобождение курсора */

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------- Направление на перезагрузку */

                   __exit_flag=-1 ;

/*-------------------------------------------------------------------*/
}


/********************************************************************/
/*                                                                  */
/*    Обработка очереди операций модуля "Системная конфигурация"    */

   void  EMIRi_sc_actions(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
            int  status ;
           char  key[1024] ;
           char  error_id[128] ;
           char  error[1024] ;
           char  text[1024] ;
           char *end ;
            int  i ;
            int  j ;

/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_SC_ACTIONS_MAX ; i++)
     if(__sc_actions[i]!=NULL) {
                                   free(__sc_actions[i]) ;
                                        __sc_actions[i]=NULL ;
                               }
/*------------------------------------------ Анализ очереди операций */

        Cursor=db->LockCursor("EMIRi_sc_actions") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_actions - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*- - - - - - - - - - - - - - - - - - - -  Получение списка операций */
                      sprintf(text, "select \"Id\", \"Action\", \"Object\", \"ObjectType\", \"Executor\", \"Data\", \"Status\", \"Reply\", \"Error\" "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','HOLD') "
                                    "order by \"Id\"",
                                       __db_table_system_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get actions list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

   for(i=0 ; i<_SC_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_synch - Get actions list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                   __sc_actions[i]=(struct Sc_action *)calloc(1, sizeof(*__sc_actions[i])) ;
           strncpy(__sc_actions[i]->id,          (char *)Cursor->columns[0].value, sizeof(__sc_actions[i]->id         )-1) ;
           strncpy(__sc_actions[i]->action,      (char *)Cursor->columns[1].value, sizeof(__sc_actions[i]->action     )-1) ;
           strncpy(__sc_actions[i]->object,      (char *)Cursor->columns[2].value, sizeof(__sc_actions[i]->object     )-1) ;
           strncpy(__sc_actions[i]->object_type, (char *)Cursor->columns[3].value, sizeof(__sc_actions[i]->object_type)-1) ;
           strncpy(__sc_actions[i]->executor,    (char *)Cursor->columns[4].value, sizeof(__sc_actions[i]->executor   )-1) ;
           strncpy(__sc_actions[i]->data,        (char *)Cursor->columns[5].value, sizeof(__sc_actions[i]->data       )-1) ;
           strncpy(__sc_actions[i]->status,      (char *)Cursor->columns[6].value, sizeof(__sc_actions[i]->status     )-1) ;
           strncpy(__sc_actions[i]->reply,       (char *)Cursor->columns[7].value, sizeof(__sc_actions[i]->reply      )-1) ;
           strncpy(__sc_actions[i]->error,       (char *)Cursor->columns[8].value, sizeof(__sc_actions[i]->error      )-1) ;

                                      }

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - -  Получение "уровня" ошибок */
                      sprintf(text, "select min(\"Id\") "
                                    "from   %s "
                                    "where  \"Error\"<>'' and \"Error\" is not null",
                                       __db_table_system_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_sc_action - Get errors level : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(error_id, 0, sizeof(error_id)) ;
           strncpy(error_id, (char *)Cursor->columns[0].value, sizeof(error_id)-1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------- Перебор операций */

        for(i=0 ; i<_SC_ACTIONS_MAX ; i++) {

            if(__sc_actions[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Операция IDENTIFY */
            if(!stricmp(__sc_actions[i]->action, "IDENTIFY")) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__sc_actions[j]->action, "IDENTIFY") &&
                      stricmp(__sc_actions[j]->status, "DONE"    )   )  break ;

                  if(j<i)  continue ;

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : IDENTIFY : %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : IDENTIFY ERROR : %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_Identify(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_Identify : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - Операция GET_VALUE */
            else
            if(!stricmp(__sc_actions[i]->action, "GETVALUE")) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__sc_actions[j]->action, "GETVALUE") &&
                      stricmp(__sc_actions[j]->status, "DONE"    )   )  break ;

                  if(j<i)  continue ;

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GETVALUE : %s . %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object, __sc_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GETVALUE ERROR : %s . %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object, __sc_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_GetValue(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_GetValue : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                              }
/*- - - - - - - - - - - - - - - - - - - - - - -  Операция GET_STATUS */
            else
            if(!stricmp(__sc_actions[i]->action, "GETSTATUS")) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__sc_actions[j]->action, "GETSTATUS") &&
                      stricmp(__sc_actions[j]->status, "DONE"     )   )  break ;

                  if(j<i)  continue ;

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GETSTATUS : %s . %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object, __sc_actions[i]->object_type) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GETSTATUS ERROR: %s . %s", prefix, __sc_actions[i]->id, __sc_actions[i]->object, __sc_actions[i]->object_type) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_GetStatus(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_GetStatus : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                               }
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Операция CREATE */
            else
            if(!stricmp(__sc_actions[i]->action, "CREATE")) {

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : CREATE - %s : %s ", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object_type) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : CREATE - %s/ERROR : %s ", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object_type) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_Create(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_Create : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                            }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Операция SETVALUE */
            else
            if(!stricmp(__sc_actions[i]->action, "SETVALUE")) {

              if(!stricmp(__sc_actions[i]->status, "NEW")) {

                         memset(key, 0, sizeof(key)); 
                        strncpy(key, __sc_actions[i]->data, sizeof(key)-1); 
                     end=strchr(key, '=') ;
                  if(end!=NULL)   end[1]=0 ;

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__sc_actions[j]->action, "SETVALUE") &&
                      stricmp(__sc_actions[j]->status, "DONE"    ) &&
                     !stricmp(__sc_actions[j]->object,
                              __sc_actions[i]->object            ) &&
                     ! memcmp(__sc_actions[j]->data,
                                key, strlen(key)                 )   )  break ;

                                  if(j<i)  continue ; 
                                                           }

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : SETVALUE - %s : %s = %s", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object, __sc_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : SETVALUE - %s/ERROR : %s = %s", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object, __sc_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_SetValue(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_SetValue : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                              }
/*- - - - - - - - - - - - - - - - - - - Операция ALTER_CONFIGURATION */
            else
            if(!stricmp(__sc_actions[i]->action, 
                          "ALTERCONFIGURATION"  )) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__sc_actions[j]->action, 
                                         "ALTERCONFIGURATION") &&
                      stricmp(__sc_actions[j]->status, "DONE")   )  break ;

                  if(j<i)  continue ;

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : ALTERCONFIGURATION - %s : %s <- %s",       prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object, __sc_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : ALTERCONFIGURATION - %s/ERROR : %s <- %s", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object, __sc_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_AltConfig(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_AltConfig : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                   }
/*- - - - - - - - - - - - - - - - - - - - - - - Операция RAISE_ALERT */
            else
            if(!stricmp(__sc_actions[i]->action, "RAISEALERT")) {

                for(j=0 ; j<i ; j++)                                /* В очереди выше не должно быть незавершённых операций */  
                  if( stricmp(__sc_actions[j]->action, "RAISEALERT") &&
                      stricmp(__sc_actions[j]->status, "DONE"      )   )  break ;

                  if(j<i)  continue ;

                if(error_id[0]!=0)
                  if(stricmp(__sc_actions[i]->id,                   /* Если операция ПОСЛЕ первой ошибки... */
                                        error_id )>0)  continue ;

                if(__sc_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : RAISEALERT %s : %s ",       prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : RAISEALERT %s/ERROR : %s ", prefix, __sc_actions[i]->id, __sc_actions[i]->status, __sc_actions[i]->object) ;


                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__sc_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_sc_action_RaiseAlert(__sc_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_sc_action_RaiseAlert : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           }
/*-------------------------------------------------------------------*/

}


/*********************************************************************/
/*								     */
/*                        Операция IDENTIFY                          */

   int  EMIR_sc_action_Identify(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  text[4096] ;
        char  reply[2048] ;

/*----------------------------------------- Проверка смарт-контракта */

       status=EMIRi_sc_Identify(action->object, reply, error) ;
    if(status!=0)  return(-1) ;

/*---------------------------------------- Занесение результата в БД */

        Cursor=db->LockCursor("EMIR_sc_action_Identify") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                            __db_errors_cnt++ ;
                               return(-1) ;
                      }

                      sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_system_actions, reply, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
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
/*                        Операция GET_VALUE                         */

   int  EMIR_sc_action_GetValue(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  text[4096] ;
        char  buff[2048] ;
        char  out[2048] ;
        char *result ;
        char *end ;
         int  i ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xe0643e89%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

           EMIR_txt2hex(action->data, text, strlen(action->data)) ;
            
       for(i=strlen(text) ; i<64 ; i++)  text[ i]='0' ;
                                         text[64]= 0 ;

                          sprintf(buff, request, action->object, text) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\"0x"

              memset(out, 0, sizeof(out)) ;

   do {
           result=strstr(buff, _RESULT_PREFIX) ;                   /* Проверка наличия результата */
        if(result==NULL)  break ;

              strncpy(out, result+strlen(_RESULT_PREFIX), sizeof(out)-1) ;
           end=strchr(out, '"') ;
        if(end==NULL)  {  out[0]=0 ;  break ;  }

        if(strstr(action->data, "(s)"     )!=NULL)  EMIR_hex2txt(out, out) ;
        else
        if(strstr(action->data, "Template")==NULL)  out[40]=0 ;    /* Обрезание адресов смарт-контрактов */

      } while(0) ;

#undef   _RESULT_PREFIX

/*---------------------------------------- Занесение результата в БД */

        Cursor=db->LockCursor("EMIR_sc_action_GetValue") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

                              error[0]=0 ;

   do {
                      sprintf(text, "delete from %s "
                                    "where  \"Key\"='%s'",
                                       __db_table_system_configuration, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                         break ;
                }

                      sprintf(text, "insert into %s(\"Key\", \"Value\") "
                                    "values ('%s', '%s')",
                                       __db_table_system_configuration, action->data, out) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                         break ;
                }

                      sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_system_actions, out, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                         break ;
                }

      } while(0) ;

   if(error[0]==0)  db->Commit() ;
   else             db->Rollback() ;

                    db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Операция GET_STATUS                        */

   int  EMIR_sc_action_GetStatus(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  state_id[128] ;
        char  config[128] ;
        char  reply[2048] ;
        char  text[4096] ;

/*---------------------------------------- Обработка для SystemAlert */

   if(!stricmp(action->object_type, "SystemAlert")) {
/*- - - - - - - - - - - - - - - - - - -  Отправка запроса в Ethereum */
        status=EMIRi_sc_GetStatus(action->object, state_id, config, error) ;
     if(status)  return(-1) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
          sprintf(reply, "{\"StateID\":\"%s\", \"Configuration\":\"%s\"}", state_id, config) ;

        Cursor=db->LockCursor("EMIR_sc_action_GetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

                              error[0]=0 ;
                      sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_system_actions, reply, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                                    }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                           Операция CREATE                         */

   int  EMIR_sc_action_Create(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char *code ;
        char  account[128] ;
        char *password ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[1024] ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;                /* Выделение пароля */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*----------------------------------------------- Создание контракта */

   if(!stricmp(action->status, "NEW" ))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
              code=(char *)calloc(1, _CODE_SIZE) ;

      do {
              status=EMIR_node_getcode(action->data,             /*  Извлечение кода контракта */
                                         code, _CODE_SIZE-1, error) ;
           if(status)  break ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на создание контракта */
                                         NULL, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_publcontract(account,                /* Отправка транзакции */ 
                                              code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;

                        free(code) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_Create") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
      do {
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0)  return(0) ;
           if(status < 0)  break ;

                strcpy(contract, contract+2) ;                      /* Убираем префикс 0x */

              status=EMIRi_sc_Identify(contract, reply, error) ;
           if(status)  break ;

           if(*reply==0) {
                            strcpy(error, "Empty identify reply") ;
                                status=-1 ;
                                   break ;
                         }

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_Create") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, contract, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                           Операция SETVALUE                       */

   int  EMIR_sc_action_SetValue(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  code[2048] ;
        char  account[128] ;
        char *password ;
        char  key[128] ;
        char  key_hex[128] ;
        char *value ;
        char  value_hex[128] ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[1024] ;
         int  i ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;                /* Выделение пароля */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*------------------------------------------------- Парсинг значений */

               memset(key, 0, sizeof(key)) ;                        /* Разделение ключа и значения */
              strncpy(key, action->data, sizeof(key)-1) ;

         value=strchr(key, '=') ;
      if(value==NULL) {
                          value="" ;
                      }
      else            {
                         *value=0 ;  
                          value++ ;  
                      }

      if(value[1]=='x')  value+=2 ;                                 /* Если значение задано с префиксом 0x */

                      memset(key_hex, 0, sizeof(key_hex)) ;
              EMIR_txt2hex64(key, key_hex, strlen(key)) ;           /* Преобразование Key в HEX */

      if(strstr(key, "(s)")!=NULL) {
              EMIR_txt2hex64(value, value_hex, strlen(value)) ;     /* Преобразование Value в HEX */
                                   }
      else                         {
                    memset(value_hex, 0, sizeof(value_hex)) ;       /* Выравнивание Value bytes32 */
                   strncpy(value_hex, value, sizeof(value_hex)-1) ;
            
           for(i=strlen(value_hex) ; i<64 ; i++) value_hex[ i]='0' ;
                                                 value_hex[64]= 0 ;
                                   }
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "NEW" ))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
      do {
             sprintf(code, "926b2cda%s%s", key_hex, value_hex) ;    /* Формируем блок данных транзакции */

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        action->object, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                              action->object, code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_SetValue") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
      do {
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0)  return(0) ;
           if(status < 0)  break ;

              status=EMIRi_sc_GetValue(action->object, key, reply, error) ;
           if(status)  break ;

           if(stricmp(value_hex, reply)) {
                            strcpy(error, "Check value fail") ;
                                           status=-1 ;
                                              break ;
                                         }

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_SetValue") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                  Операция ALTER_CONFIGURATION                     */

   int  EMIR_sc_action_AltConfig(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  code[2048] ;
        char  account[128] ;
        char *password ;
        char *value ;
        char  state_id[128] ;
        char  config[128] ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[1024] ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;                /* Выделение пароля */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*------------------------------------------------- Парсинг значений */

              value=action->data ;
           if(value[1]=='x')  value+=2 ;                            /* Если значение задано с префиксом 0x */

/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "NEW" ))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
      do {
              status=EMIRi_sc_GetStatus(action->object,             /* Запрос текущего состояния смарт-контракта */
                                          state_id, config, error) ;
           if(status)  break ;

                                                                    /* Формируем блок данных транзакции */
             sprintf(code, "0c7ec864000000000000000000000000%s000000000000000000000000%s", config, value) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        action->object, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                              action->object, code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_AltConfig") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
      do {
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0)  return(0) ;
           if(status < 0)  break ;

              status=EMIRi_sc_GetStatus(action->object, state_id, config, error) ;
           if(status)  break ;

           if(stricmp(value, config)) {
                            strcpy(error, "Check value fail") ;
                                           status=-1 ;
                                              break ;
                                      }

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_AltConfig") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                       Операция RAISE_ALERT                        */

   int  EMIR_sc_action_RaiseAlert(Sc_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  code[2048] ;
        char  account[128] ;
        char *password ;
        char  state_id[128] ;
        char  config[128] ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[2048] ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;                /* Выделение пароля */
              strncpy(account, action->executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "NEW" ))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
      do {
              status=EMIRi_sc_GetStatus(action->object,             /* Запрос текущего состояния смарт-контракта */
                                          state_id, config, error) ;
           if(status)  break ;

             sprintf(code, "66a5123c") ;                            /* Формируем блок данных транзакции */

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        action->object, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                              action->object, code, gas, txn, error) ;
           if(status)  break ;

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_RaiseAlert") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s', \"Data\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, txn, state_id, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
      do {
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0)  return(0) ;
           if(status < 0)  break ;

              status=EMIRi_sc_GetStatus(action->object, state_id, config, error) ;
           if(status)  break ;

           if(!strcmp(state_id, action->data)) {                    /* Новое и предыдущее значения должны различаться */
                              strcpy(error, "Check value fail") ;
                                                  status=-1 ;
                                                     break ;
                                               }

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
        Cursor=db->LockCursor("EMIR_sc_action_RaiseAlert") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DONE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, action->reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_system_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

#undef  _CODE_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*              Запрос Identify на смарт контракт                    */

   int  EMIRi_sc_Identify(char *contract, char  *reply, char *error)

{
         int  status ;
        char  buff[2048] ;
        char  out[2048] ;
        char *result ;
        char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xd48b2e92\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------------------- Инициализация */

                      *reply=0 ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\"0x"

              memset(out, 0, sizeof(out)) ;

   do {
           result=strstr(buff, _RESULT_PREFIX) ;                   /* Проверка наличия результата */
        if(result==NULL)  break ;

              strncpy(out, result+strlen(_RESULT_PREFIX), sizeof(out)-1) ;
           end=strchr(out, '"') ;
        if(end==NULL)  {  out[0]=0 ;  break ;  }

      } while(0) ;

               EMIR_hex2txt(out, reply) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetStatus смарт-контракта SystemAlert           */

   int  EMIRi_sc_GetStatus(char *contract, char  *state_id, char  *config, char *error)

{
   int  status ;
  char *result ;
  char  buff[2048] ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xabd95b95\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\"0x"

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

              strncpy(state_id, result+strlen(_RESULT_PREFIX), 64) ;
                      state_id[64]=0 ;

              strncpy(config, result+strlen(_RESULT_PREFIX)+64+24, 40) ;
                      config[40]=0 ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetKeys смарт-контракта Configuration           */

   int  EMIRi_sc_GetKeys(char *contract, SysPar **keys, int *keys_cnt, char *error)

{
   int  status ;
  char *result ;
  char  value[256] ;
  char  buff[2048] ;
  char *end ;
   int  i ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xda31a2a5\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memset(value, 0, sizeof(value)) ;
              memcpy(value,     result+1*64, 64) ;

            *keys_cnt=strtoul(value, &end, 16) ;

                    result+=2*64 ;

            *keys=(SysPar *)calloc(*keys_cnt+1, sizeof(**keys)) ;

        for(i=0 ; i<*keys_cnt ; i++) {

                  memcpy((*keys)[i].key,  result+i*64, 64) ;
            EMIR_hex2txt((*keys)[i].key, (*keys)[i].key) ;
                                     }

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetValue смарт-контракта Configuration          */

   int  EMIRi_sc_GetValue(char *contract, char  *key, char  *value, char *error)

{
   int  status ;
  char *result ;
  char  key_hex[128] ;
  char  buff[2048] ;
   int  i ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xe0643e89%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

                           memset(key_hex, 0, sizeof(key_hex)) ;
                     EMIR_txt2hex(key, key_hex, strlen(key)) ;

           for(i=strlen(key_hex) ; i<64 ; i++) key_hex[ i]='0' ;
                                               key_hex[64]= 0 ;

                          sprintf(buff, request, contract, key_hex) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

              strncpy(value, result+strlen(_RESULT_PREFIX)+2, 64) ;
                      value[64]=0 ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}
