/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*               Модуль "Централизованный мониториг"                 */
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

#pragma warning(disable : 4267)
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

   static char  sentry_wf_version[256] ;    /* Версия рабочего процесса по модулю Sentry */

   static  int  sentry_block_fail ;
   static char  sentry_block_last[128] ;

   static  int  sentry_file_fail ;
   static char  sentry_file_last[128] ;
   static char  sentry_file_mark[128] ;

   static  int  sentry_balance_fail ;
   static char  sentry_balance[128] ;

   static  int  sentry_mark_fail ;

   static  int  sentry_errors_cnt ;
   static  int  sentry_pending_cnt ;

  struct St_node {
                   char  node_id[96] ;
                   char  address[96] ;
                   char  node_ts[96] ;
                   char  block_last[96] ;
                   char  dfs_check[96] ;
                   char  balance[96] ;
                   char  errors[96] ;
                   char  pending[96] ;
                   char  alarm[8192] ;
                   char  details[8192] ;
                    int  update ;
                 time_t  ts ;
                    int  ts_error ;
                    int  nb_error ;
                    int  fl_error ;
                    int  bl_error ;
                    int  ac_error ;
                    int  pd_error ;
                    int  pd_prv ;
                    int  pd_deep ;
                 }  ;

#define  _NODES_MAX  1000

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

  void  EMIRi_st_newblocks   (HWND hDlg, char *prefix, SQL_link *db) ;              /* Сканирование новых блоков блокчейн */
  void  EMIRi_st_balance     (HWND hDlg, char *prefix, SQL_link *db) ;              /* Контроль остатка рабочего счёта */
  void  EMIRi_st_dfs_probe   (HWND hDlg, char *prefix, SQL_link *db) ;              /* "Запуск" контрольного файла в DFS */
  void  EMIRi_st_dfs_check   (HWND hDlg, char *prefix, SQL_link *db) ;              /* "Получение" контрольного файла в DFS */
  void  EMIRi_st_actions     (HWND hDlg, char *prefix, SQL_link *db) ;              /* Контроль операций в очереди */
  void  EMIRi_st_alive       (HWND hDlg, char *prefix, SQL_link *db) ;              /* Передача информации на Центральный Мониторинг */
  void  EMIRi_st_extended    (HWND hDlg, char *prefix, SQL_link *db, char *) ;      /* Передача расширенной информации на Центральный Мониторинг */
  void  EMIRi_st_nodes       (HWND hDlg, char *prefix, SQL_link *db) ;              /* Обработка информации узлов из Центрального Мониторинга */
  void  EMIRi_st_monitoring  (HWND hDlg, char *prefix, SQL_link *db) ;              /* формирование сигналов мониторинга */

   int  EMIRi_st_GetNodes    (char *contract, St_node **list, char *error) ;        /* Запрос методa GetNodesList смарт-контракта Monitoring */
   int  EMIRi_st_GetNode     (char *contract, St_node *node, char *error) ;         /* Запрос методa GetNode смарт-контракта Monitoring */
   int  EMIRi_st_GetCheckMark(char *contract, char  *reply, char *error) ;          /* Запрос GetCheckMark на смарт-контракт */
   int  EMIRi_st_GetCheckFile(char *contract, char  *reply, char *error) ;          /* Запрос GetCheckFile на смарт-контракт */

   int  EMIRi_st_StateFile   (char *open, char *block,                              /* Работа с файлом состояния мониторинга */
                                        time_t *block_ts,
                                          char *file_id,
                                          char *file_mark,
                                        time_t *file_ts,   char *error) ;


/*********************************************************************/
/*								             */
/*	      Обработчик сообщений диалогового окна SENTRY            */

 INT_PTR  CALLBACK  EMIR_sentry_dialog(  HWND  hDlg,     UINT  Msg, 
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
/*   THREAD - Фоновый поток модуля "Централизованный мониторинг"    */

  DWORD WINAPI  Sentry_Thread(LPVOID Pars)

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

      if(__st_request_period<=0)  __st_request_period= 10 ;
      if(__st_view_frame    <=0)  __st_view_frame    =100 ;

              hDlg=hSentry_Dialog ;

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
         if(rows_cnt>__st_view_frame) {

               for(i=0 ; i<rows_cnt-__st_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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
/*- - - - - - - - - - - - - - - - - - - - - Мониторинговые процедуры */
//             EMIRi_st_newblocks (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_dfs_check (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_balance   (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_actions   (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_alive     (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_extended  (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_dfs_probe (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_nodes     (hSentry_Dialog, prefix, &DB) ;
//             EMIRi_st_monitoring(hSentry_Dialog, prefix, &DB) ;
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
/* Обработчик фонового потока модуля "Централизованный мониторинг"  */

  void  Sentry_Process(SQL_link *DB)

{
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  address[1024] ;
             int  status ;
            char  text[4096] ;
            char  error[1024] ;
            char *tmp ;

#ifndef  UNIX 
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

#pragma warning(disable : 4244)

/*---------------------------------------------------- Инициализация */

      if(__st_request_period<=0)  __st_request_period= 10 ;
      if(__st_view_frame    <=0)  __st_view_frame    =100 ;

#ifndef  UNIX 
              hDlg=hSentry_Dialog ;
#endif

/*------------------------------------------------------- Общий цикл */

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#ifndef  UNIX 

            rows_cnt=LB_GET_ROW (IDC_LOG) ;
         if(rows_cnt>__st_view_frame) {

               for(i=0 ; i<rows_cnt-__st_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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
/*- - - - - - - - - - - - - - - Определение версии рабочего процесса */
     if(sentry_wf_version[0]==0) do {

                         memset(address, 0, sizeof(address)) ;
          status=EMIR_db_syspar(DB, "Monitoring", address, error) ;
       if(status) {
                       sprintf(text, "%s - ERROR - Sentry_Process - Get Monitoring address: %s", prefix, error) ;
                    LB_ADD_ROW(IDC_LOG, text) ;
                                 break ;
                  }

          status=EMIR_node_getversion(address, sentry_wf_version, error) ;
       if(status) {
                       sprintf(text, "%s - ERROR - Sentry_Process - Get Monitoring version: %s", prefix, error) ;
                    LB_ADD_ROW(IDC_LOG, text) ;
                                 break ;
                  }

          tmp=strstr(sentry_wf_version, "Monitoring.") ;
       if(tmp==NULL) {
                       sprintf(text, "%s - ERROR - Sentry_Process - Monitoring version is invalid: %s", prefix, sentry_wf_version) ;
                    LB_ADD_ROW(IDC_LOG, text) ;

                        strcpy(sentry_wf_version, "0000-00-00") ;
                                       break ;
                     }

             memmove(sentry_wf_version, tmp, strlen(tmp)+1) ;

             sprintf(text, "%s - Sentry workflow version: %s", prefix, sentry_wf_version) ;
          LB_ADD_ROW(IDC_LOG, text) ;

             EMIRi_st_extended  (hSentry_Dialog, prefix, DB, "DETAILS") ;

                                     } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - Мониторинговые процедуры */
             EMIRi_st_newblocks (hSentry_Dialog, prefix, DB) ;

    if(stricmp(__dfs_type, "DIRECT")) 
             EMIRi_st_dfs_check (hSentry_Dialog, prefix, DB) ;

             EMIRi_st_balance   (hSentry_Dialog, prefix, DB) ;
             EMIRi_st_actions   (hSentry_Dialog, prefix, DB) ;
             EMIRi_st_alive     (hSentry_Dialog, prefix, DB) ;
             EMIRi_st_extended  (hSentry_Dialog, prefix, DB, "ALARM") ;

    if(stricmp(__dfs_type, "DIRECT")) 
             EMIRi_st_dfs_probe (hSentry_Dialog, prefix, DB) ;

             EMIRi_st_nodes     (hSentry_Dialog, prefix, DB) ;
             EMIRi_st_monitoring(hSentry_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

/*-------------------------------------------------------------------*/
                                    
  return ;
}


/********************************************************************/
/*                                                                  */
/*                Сканирование новых блоков блокчейн                */

   void  EMIRi_st_newblocks(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            char  error[1024] ;
            char  text[4096] ;
            char *end ;

   static   char  block_cur[128] ;
   static   char  block_prv[128] ;
   static time_t  block_prv_ts ;
   static time_t  block_time ;
   static time_t  check_prv_ts ;

/*----------------------------------------- Контроль периода запроса */

   if(check_prv_ts!=0)
    if(check_prv_ts+block_time/2>time(NULL))  return ;
  
       check_prv_ts=time(NULL) ;

EMIR_log("EMIRi_st_newblock") ;

/*------------------------------------------------------- Подготовка */

   if(__mon_context_path[0]==0) {

        Cursor=db->LockCursor("EMIRi_st_newblocks") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
                                }
/*------------------------------------ Получение начальных установок */

  if(block_prv[0]==0) do {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
     if(__mon_context_path[0]!=0) {

                                     block_time=__mon_bh_pulse ;

          status=EMIRi_st_StateFile("READ", block_prv, &block_prv_ts, NULL, NULL, NULL, error) ;
       if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get saved state : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
//                                 return ;
                  }

                            strcpy(sentry_block_last, block_prv) ;

                                             break ;
                                  }
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"BlockCheckTime\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(text,(char *)Cursor->columns[0].value) ;
              block_time=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - -  Получение последнего сохранённого блока */
                      sprintf(text, "select \"BlockLast\",\"BlockLastTs\" from  %s ",
                                       __db_table_sentry_state) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get last block : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get last block : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get last block : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(block_prv, (char *)Cursor->columns[0].value) ;
                          strcpy(text,      (char *)Cursor->columns[1].value) ;
            block_prv_ts=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                         } while(0) ;

/*-------------------------------- Получение номера последнего блока */

                            strcpy(sentry_block_last, block_prv) ;

        status=EMIR_node_lastblock(block_cur, error) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get current block : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                   return ;
                }
/*------------------------------------- Сохранение изменённого блока */

   if(strcmp(block_cur, block_prv)) {

                       strcpy(block_prv, block_cur) ;
                              block_prv_ts=time(NULL) ;
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
     if(__mon_context_path[0]!=0) {

          status=EMIRi_st_StateFile("WRITE", block_prv, &block_prv_ts, NULL, NULL, NULL, error) ;
       if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Save state : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                    return ;
                  }
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Для настроек через БД */
     else                         {
 
                      sprintf(text, "update %s set \"BlockLast\"='%s', \"BlockLastTs\"=%ld ",
                                      __db_table_sentry_state, block_cur, (long)block_prv_ts) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Save last block : %s", prefix, db->error_text) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
                      db->Rollback() ;
                      db->UnlockCursor(Cursor) ;
                    __db_errors_cnt++ ;
                             return ;
                }

                 db->Commit() ;
                 db->UnlockCursor(Cursor) ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                    }
/*------------------------------------ Анализ просрочки нового блока */

   else                             {
                                               sentry_block_fail=0 ;
       if(block_prv_ts-time(NULL)>block_time)  sentry_block_fail=1 ;
                                    }
/*-------------------------------------------- Освобождение ресурсов */

   if(__mon_context_path[0]==0) {

                      db->UnlockCursor(Cursor) ;

                                }
/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*                Контроль остатка рабочего счёта                   */

   void  EMIRi_st_balance(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            char  error[1024] ;
            char  text[4096] ;
            char *end ;

   static   char  account[128] ;
   static   char  balance_limit[128] ;
   static time_t  check_time ;
   static time_t  check_prv_ts ;

/*----------------------------------------- Контроль периода запроса */

   if(check_prv_ts!=0)
    if(check_prv_ts+check_time>time(NULL))  return ;
  
       check_prv_ts=time(NULL) ;

EMIR_log("EMIRi_st_balance") ;

/*------------------------------------------------------- Подготовка */

          Cursor=NULL ;  

   if(__mon_context_path[0]==0) {

        Cursor=db->LockCursor("EMIRi_st_balance") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_balance - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
                                }
/*------------------------------------ Получение начальных установок */

  if(account[0]==0)  do {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
     if(__mon_context_path[0]!=0) {

                        check_time=__mon_bh_pulse ;

                strncpy(account,       __mon_basic_account, sizeof(account      )-1) ;
                strncpy(balance_limit, __mon_balance_limit, sizeof(balance_limit)-1) ;

                                             break ;
                                  }
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"BlockCheckTime\", \"BalanceLimit\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_balance - Get settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_balance - Get settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_balance - Get settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(text,(char *)Cursor->columns[0].value) ;
              check_time=strtoul(text, &end, 10) ;
                          strcpy(balance_limit,(char *)Cursor->columns[1].value) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - -  Получение настроек главного счёта */
                      sprintf(text, "select \"MemberAccount\" from  %s ",
                                       __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_balance - Get account settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_balance - Get account settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_balance - Get account settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strncpy(account, (char *)Cursor->columns[0].value, sizeof(account)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                        } while(0) ;

      if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;

/*--------------------------------- Получение баланса главного счёта */

        status=EMIR_node_getbalance(account, "latest", sentry_balance, error) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_balance - Get balance : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                   return ;
                }
/*-------------------------- Анализ достижения критического остатка  */

  if(strcmp(__net_type, "QUORUM")) {                                /* Для Quorum - не анализируем */

        if(strlen(sentry_balance)<strlen(balance_limit))  sentry_balance_fail=1 ;
   else if(strlen(sentry_balance)>strlen(balance_limit))  sentry_balance_fail=0 ;
   else if(strcmp(sentry_balance, balance_limit)<0     )  sentry_balance_fail=1 ;
   else                                                   sentry_balance_fail=0 ;
                                   }
  else                             {
                                                          sentry_balance_fail=0 ;
                                   }

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*                  Контроль операций в очереди                     */

   void  EMIRi_st_actions(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            char *table ;
            char  text[4096] ;
            char *end ;
             int  i  ;

   static time_t  check_time ;
   static time_t  check_prv_ts ;

/*------------------------------------------------- Входной контроль */

     if(__mon_context_path[0]!=0)  return ;                         /* Не проводится при отсутсвии БД */

/*------------------------------------------------------- Подготовка */

        Cursor=db->LockCursor("EMIRi_st_actions") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_actions - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*------------------------------------ Получение начальных установок */

  if(check_prv_ts==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"BlockCheckTime\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_actions - Get settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_balance - Get actions : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_balance - Get actions : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(text,(char *)Cursor->columns[0].value) ;
              check_time=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }
/*----------------------------------------- Контроль периода запроса */

    if(check_prv_ts+check_time>time(NULL)) {
                                     db->UnlockCursor(Cursor) ;
                                                   return ;
                                           } 
  
       check_prv_ts=time(NULL) ;

/*--------------------------------------- Получение счётчиков ошибок */

                            sentry_errors_cnt=0 ;

  for(i=0 ; i<20 ; i++) {
                                                      table=  NULL ;
         if(i==0 && EMIR_active_section("SYSTEM"  ))  table=__db_table_system_actions ;
    else if(i==1 && EMIR_active_section("MEMBERS" ))  table=__db_table_members_actions ;
    else if(i==2 && EMIR_active_section("DEALS"   ))  table=__db_table_deals_actions ;
    else if(i==3 && EMIR_active_section("FILES"   ))  table=__db_table_files_actions ;

         if(table==NULL)  continue ;

     sprintf(text, "select count(*) from  %s where (\"Error\"<>'' and \"Error\" is not null) and \"Status\" not in ('DONE', 'HOLD') ", table) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_actions - Get errors for table %s : %s", prefix, table, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_balance - Get errors for table %s : %s", prefix, table, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_balance - Get errors for table %s : %s", prefix, table, db->error_text) ;


                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                              strcpy(text,(char *)Cursor->columns[0].value) ;
          sentry_errors_cnt+=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;

                        }
/*----------------------- Получение счётчиков незавершённых операций */


                            sentry_pending_cnt=0 ;

  for(i=0 ; i<20 ; i++) {
                                                      table=  NULL ;
         if(i==0 && EMIR_active_section("SYSTEM"  ))  table=__db_table_system_actions ;
    else if(i==1 && EMIR_active_section("MEMBERS" ))  table=__db_table_members_actions ;
    else if(i==2 && EMIR_active_section("DEALS"   ))  table=__db_table_deals_actions ;
    else if(i==3 && EMIR_active_section("FILES"   ))  table=__db_table_files_actions ;

         if(table==NULL)  continue ;

     sprintf(text, "select count(*) from  %s where \"Status\" not in('HOLD','DONE')", table) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_actions - Get pending for table %s : %s", prefix, table, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_balance - Get pending for table %s : %s", prefix, table, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_balance - Get pending for table %s : %s", prefix, table, db->error_text) ;


                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                              strcpy(text,(char *)Cursor->columns[0].value) ;
          sentry_pending_cnt+=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;

                        }
/*------------------------------------------------ Завершение работы */

                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*            Передача информации на Центральный Мониторинг         */

   void  EMIRi_st_alive(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  mark[128] ;
          time_t  mark_ts ;
            char  code[2048] ;
            char  alive_ts_hex[128] ;
            char  file_last_hex[128] ;
            char  balance_hex[128] ;
            char  gas[128] ;
            char  contract[128] ;
            char  block[128] ;
            char  error[1024] ;
            char  reply[1024] ;
            char  text[4096] ;
            char *end ;
             int  i ;

   static   char  account[128] ;
   static   char  password[128] ;
   static time_t  alive_prv_ts ;
   static    int  alive_time ;
   static   char  alive_address[100] ;
   static   char  node_id[128] ;
   static   char  check_net[128] ;
   static   char  txn[128] ;

/*---------------------------------------- Контроль периода передачи */

   if(       __monitoring_rules[0]       !=0    &&
      strstr(__monitoring_rules, "ALIVE")==NULL   )  return ;

   if(alive_prv_ts!=0)
    if(alive_prv_ts+alive_time>time(NULL))  return ;
  
       alive_prv_ts=time(NULL) ;

EMIR_log("EMIRi_st_alive") ;

/*---------------------------------------------------- Инициализация */

   if(alive_address[0]==0) {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
    if(__mon_context_path[0]!=0) {

                       alive_time    =__mon_alive_pulse ;

                strcpy(text,          __mon_node_id) ;
        EMIR_txt2hex64(text, node_id, strlen(text)) ;

                strcpy(alive_address, __mon_alive_address) ;
                strcpy(account,       __mon_basic_account) ;
                strcpy(password,      __mon_basic_password) ;

                                 }
/*- - - - - - - - - - - - - - - - - - - - - -  Для настроек через БД */
    else                         {

        status=EMIR_db_syspar(db, "Monitoring", alive_address, error) ;
     if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_st_alive - Alive address: %s", prefix, error) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                }
                                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                           }
/*------------------------------------------------------- Подготовка */

              Cursor=NULL ;

   if(__mon_context_path[0]==0) {

        Cursor=db->LockCursor("EMIRi_st_alive") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_alive - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }

                                }
/*------------------------------------ Получение начальных установок */

  if(alive_time==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"NodeId\",\"AliveTime\",\"CheckNet\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_alive - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_alive - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_alive - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                       strncpy(text,(char *)Cursor->columns[0].value, 32) ;
                EMIR_txt2hex64(text, node_id, strlen(text)) ;
                        strcpy(text,(char *)Cursor->columns[1].value) ;
            alive_time=strtoul(text, &end, 10) ;
                       strncpy(check_net,(char *)Cursor->columns[2].value, 32) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - -  Получение настроек главного счёта */
                      sprintf(text, "select \"MemberAccount\", \"MemberPassword\" from  %s ",
                                       __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_alive - Get account settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_alive - Get account settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_alive - Get account settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strncpy(account, (char *)Cursor->columns[0].value, sizeof(account)-1) ;
                        strncpy(password,(char *)Cursor->columns[1].value, sizeof(password)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }

        if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;

/*--------------------------------------- Проверка контрольной метки */

                             memset(mark, 0, sizeof(mark)) ;
       status=EMIRi_st_GetCheckMark(alive_address, mark, error) ;
    if(status) {
                    sprintf(text, "%s - ERROR - EMIRi_st_dfs_alive - Check mark get error: %s", prefix, error) ;
                 LB_ADD_ROW(IDC_LOG, text) ;
                    if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;
                             return ;
               }

                  mark_ts=strtoul(mark, &end, 16) ;

                                         sentry_mark_fail=0 ;
    if(time(NULL)-mark_ts>2*alive_time)  sentry_mark_fail=1 ;

/*------------------ Проверка исполнения предыдущей alive-транзакции */

   if(txn[0]!=0) {

      do {
              status=EMIR_node_checktxn(txn, block, contract, error) ;
           if(status== 0)  return ;
           if(status < 0)  return ;

                         txn[0]=0 ;

         } while(0) ;

                 }
/*--------------------------------------- Отправка контрольной метки */

   if(check_net[0]=='Y' ||
      check_net[0]=='y'   ) {
/*- - - - - - - - - - - - - - - - - - - - Формирование пакета данных */
          time_abs=time(NULL) ;

                            reply[0]=0 ;

      do {

                sprintf(code, "ff4d9af2"
#ifdef  UNIX
                              "%064lx",
#else
                              "%064llx",
#endif
                               time_abs) ;
/*- - - - - - - - - - - - - - - - - - -  Передача данных на адрес ЦМ */
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        alive_address, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                            alive_address, code, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&     /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {  /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        continue ;
                                                                }
                             break ;
                      }

                         break ;
         } while(1) ;

    if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_st_alive - Send mark : %s", prefix, error) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
                      return ;
               }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                            }
/*--------------------------------- Формируем блок данных транзакции */
/*- - - - - - - - - - - - - - - - - - - - - - - -  Текущее время GMT */
               time_abs=  time( NULL) ;
                 hhmmss=gmtime(&time_abs) ;

             sprintf(text, "%04d.%02d.%02d %02d:%02d:%02d",
                                     hhmmss->tm_year+1900,
                                     hhmmss->tm_mon+1,
                                     hhmmss->tm_mday,
                                     hhmmss->tm_hour,
                                     hhmmss->tm_min,    
                                     hhmmss->tm_sec  ) ;
 
      EMIR_txt2hex64(text, alive_ts_hex, strlen(text)) ;
/*- - - - - - - - - - - - - - - - - - - - Последний контрольный файл */
              memset(file_last_hex, 0, sizeof(file_last_hex)) ;
              strcpy(file_last_hex, sentry_file_mark) ;

           for(i=strlen(file_last_hex) ; i<64 ; i++)  file_last_hex[ i]='0' ;
                                                      file_last_hex[64]= 0 ;
/*- - - - - - - - - - - - - - - - - - - - - -  Баланс главного счёта */
              memset(balance_hex,  0,  sizeof(balance_hex)) ;
              memset(balance_hex, '0',   64               ) ;
              strcpy(balance_hex+64-strlen(sentry_balance), sentry_balance) ;
/*- - - - - - - - - - - - - - - - - - - - Счётчики ошибок и ожиданий */
//                      sentry_errors_cnt=1 ;
//                    sentry_pendings_cnt=2 ;
/*- - - - - - - - - - - - - - - - - - - - Формирование пакета данных */
// SetAlive(bytes32  node_id_, bytes32  time_, uint256  last_block_, bytes32  last_file_, uint256  balance_, uint256  errors_, uint256  pendings_)

                            reply[0]=0 ;

      do {

             sprintf(code, "af152ca6"
                           "%s"
                           "%s"
                           "%064lx"
                           "%s"
                           "%s"
                           "%064x"
                           "%064x",
                            node_id, alive_ts_hex, 
                            strtoul(sentry_block_last, &end, 16),
                            file_last_hex,
                            balance_hex,
                            sentry_errors_cnt,
                            sentry_pending_cnt) ;

/*-------------------------------------- Передача данных на адрес ЦМ */

                  memset(txn, 0, sizeof(txn)) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        alive_address, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                            alive_address, code, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&     /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {  /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        continue ;
                                                                }
                             break ;
                      }

                         break ;
         } while(1) ;

    if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_st_alive - Send alive signal : %s", prefix, error) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
                      return ;
               }
/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*     Передача расширенной информации на Центральный Мониторинг    */

   void  EMIRi_st_extended(HWND hDlg, char *prefix, SQL_link *db, char *kind)

{
      SQL_cursor *Cursor ;
             int  status ;
            char  gas[128] ;
            char  txn[128] ;
            char  error[1024] ;
            char  reply[1024] ;
            char  text[4096] ;
            char  text_hex[8192] ;
            char  code[10240] ;
            char *end ;

   static    char  account[128] ;
   static    char  password[128] ;
   static     int  alive_time ;
   static    char  alive_address[100] ;
   static    char  node_id[128] ;
   static    char  alarm_prv[8192]="null" ;
   static    char  details_prv[8192]="null" ;
   static  time_t  time_prv ;

/*---------------------------------------- Контроль периода передачи */

   if(stricmp(sentry_wf_version,                                    /* Только для версий "Monitoring.2019-03-05" и выше */
               "Monitoring.2019-03-05")<0)  return ;

   if(time(NULL)-time_prv<alive_time)  return ;                     /* Контроль времени */
                 time_prv=time(NULL) ;

/*------------------------------------ Формирование текста сообщения */
 
         memset(text, 0, sizeof(text)) ;
/*- - - - - - - - - - - - - - -  Формирование текста ALARM-сообщения */
   if(!stricmp(kind, "ALARM")) {

    if(__db_errors_cnt)  sprintf(text, "SQL errors - %d", __db_errors_cnt) ;

          if(!stricmp(text, alarm_prv))  return ;                   /* Если текст ALARM-сообщения не поменялся - повторно не отправляем */

               strcpy(alarm_prv, text) ;

                               }
/*- - - - - - - - - - - - - -  Формирование текста DETAILS-сообщения */
   if(!stricmp(kind, "DETAILS")) {

              sprintf(text, "Version %s", _VERSION) ;

          if(!stricmp(text, details_prv))  return ;                 /* Если текст DETAILS-сообщения не поменялся - повторно не отправляем */

               strcpy(details_prv, text) ;

                                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
EMIR_log("EMIRi_st_extended") ;

/*---------------------------------------------------- Инициализация */

   if(alive_address[0]==0) {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
    if(__mon_context_path[0]!=0) {

                       alive_time    =__mon_alive_pulse ;

                strcpy(text,          __mon_node_id) ;
        EMIR_txt2hex64(text, node_id, strlen(text)) ;

                strcpy(alive_address, __mon_alive_address) ;
                strcpy(account,       __mon_basic_account) ;
                strcpy(password,      __mon_basic_password) ;

                                 }
/*- - - - - - - - - - - - - - - - - - - - - -  Для настроек через БД */
    else                         {

        status=EMIR_db_syspar(db, "Monitoring", alive_address, error) ;
     if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_st_alive - Alive address: %s", prefix, error) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                }
                                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                           }
/*--------------------------- Получение начальных установок через БД */

  if(alive_time==0) {
/*- - - - - - - - - - - - - - - - - - - - - - Резервирование курсора */
        Cursor=db->LockCursor("EMIRi_st_extended") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_extended - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"NodeId\",\"AliveTime\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_extended - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_extended - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_extended - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                       strncpy(text,(char *)Cursor->columns[0].value, 32) ;
                EMIR_txt2hex64(text, node_id, strlen(text)) ;
                        strcpy(text,(char *)Cursor->columns[1].value) ;
            alive_time=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - -  Получение настроек главного счёта */
                      sprintf(text, "select \"MemberAccount\", \"MemberPassword\" from  %s ",
                                       __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_extended - Get account settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_extended - Get account settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_extended - Get account settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strncpy(account, (char *)Cursor->columns[0].value, sizeof(account)-1) ;
                        strncpy(password,(char *)Cursor->columns[1].value, sizeof(password)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Освобождение ресурсов */
                           db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }
/*----------------------------------------------- Отправка сообщения */

        if(!stricmp(kind, "ALARM"  ))  strcpy(text, alarm_prv) ;
   else if(!stricmp(kind, "DETAILS"))  strcpy(text, details_prv) ;
   else                                  return ;
/*- - - - - - - - - - - - - - - - - - - - Формирование пакета данных */
         EMIR_txt2hex64(text, text_hex,  strlen(text)) ;            /* Преобразование строки в HEX */

                            reply[0]=0 ;

      do {

   if(!stricmp(kind, "ALARM"))
                sprintf(code, "2c55c1e0"
                              "%s"
                              "0000000000000000000000000000000000000000000000000000000000000040"
                              "%064x"
                              "%s",
                               node_id, (int)strlen(text), text_hex) ;
   else
   if(!stricmp(kind, "DETAILS"))
                sprintf(code, "a9425213"
                              "%s"
                              "0000000000000000000000000000000000000000000000000000000000000040"
                              "%064x"
                              "%s",
                               node_id, (int)strlen(text), text_hex) ;
   else     return ;
/*- - - - - - - - - - - - - - - - - - -  Передача данных на адрес ЦМ */
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        alive_address, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                            alive_address, code, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&     /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {  /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        continue ;
                                                                }
                             break ;
                      }

                         break ;
         } while(1) ;

    if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_st_extended - Send extended data : %s", prefix, error) ;
                  LB_ADD_ROW(IDC_LOG, text) ;

       if(!stricmp(kind, "ALARM"  ))  strcpy(  alarm_prv, "") ;    
       if(!stricmp(kind, "DETAILS"))  strcpy(details_prv, "") ;

                          return ;
               }
/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*                "Запуск" контрольного файла в DFS                 */

   void  EMIRi_st_dfs_probe(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            char  path_1[FILENAME_MAX] ;
            char  path_2[FILENAME_MAX] ;
            FILE *file ;
            char  reply[1024] ;
            char  error[1024] ;
            char  data[1024] ;
            char  id_hex[256] ;
            char  contract[128] ;
            char  block[128] ;
            char  text[8192] ;
            char  gas[1024] ;
            char *end ;

   static   char  account[128] ;
   static   char  password[128] ;
   static   char  check_net[128] ;
   static    int  alive_time ;
   static time_t  alive_prv_ts ;
   static   char  alive_address[100] ;
   static   char  id[128] ;
   static   char  txn[128] ;

/*------------------------------------------------- Входной контроль */

     if(__mon_context_path[0]!=0)  return ;                         /* Не проводится при отсутсвии БД */

/*--------------------------------------- Проверка передачи на СК ЦМ */

   if(txn[0]!=0) {

      do {
              status=EMIR_node_checktxn(txn, block, contract, error) ;
           if(status== 0)  return ;
           if(status < 0)  return ;

                         txn[0]=0 ;

              status=EMIRi_st_GetCheckFile(alive_address, text, error) ;
           if(status)  break ;

           if(stricmp(id, text)) {
EMIR_log(id) ;
EMIR_log(text) ;
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Check file delivery failure", prefix) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                                     return ;
                                 }

         } while(0) ;

                        return ;
                 }
/*------------------------------------------------- Входная проверка */

     if(__work_folder[0]==0) {
                               strcpy(error, "") ;
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Work folder is not defined", prefix) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                             }  

     if(check_net[0]=='N' ||
        check_net[0]=='n'   )  return ;

     if(alive_prv_ts+alive_time>time(NULL))  return ;

        alive_prv_ts=time(NULL) ;

/*---------------------------------------------------- Инициализация */

   if(alive_address[0]==0) {

        status=EMIR_db_syspar(db, "Monitoring", alive_address, error) ;
     if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Alive address: %s", prefix, error) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                }
                           }
/*------------------------------------------------------- Подготовка */

        Cursor=db->LockCursor("EMIRi_st_dfs_probe") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*------------------------------------ Получение начальных установок */

  if(check_net[0]==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"DfsCheckTime\",\"CheckNet\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strcpy(text,(char *)Cursor->columns[0].value) ;
            alive_time=strtoul(text, &end, 10) ;
                       strncpy(check_net,(char *)Cursor->columns[1].value, 32) ;

                 db->SelectClose (Cursor) ;

         if(check_net[0]!='Y' && 
            check_net[0]!='y'   )  check_net[0]='N' ;
/*- - - - - - - - - - - - - - - -  Получение настроек главного счёта */
                      sprintf(text, "select \"AddressMain\", \"MemberAccount\", \"MemberPassword\" from  %s ",
                                       __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get account settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get account settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Get account settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strncpy(text,    (char *)Cursor->columns[0].value, sizeof(text)-1) ;
                        strncpy(account, (char *)Cursor->columns[1].value, sizeof(account)-1) ;
                        strncpy(password,(char *)Cursor->columns[2].value, sizeof(password)-1) ;

                 db->SelectClose(Cursor) ;

//   if(stricmp(text, account))    check_net[0]='N' ;               /* Контрольный файл запускается только с административного узла */ 
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }

                                 db->UnlockCursor(Cursor) ;

     if(check_net[0]=='N' ||
        check_net[0]=='n'   ) {
                                        return ;
                              }
/*-------------------------------------- Создание контрольного файла */

#ifdef  UNIX
               snprintf(path_1, sizeof(path_1)-1, "%s/DfsCheck.1", __work_folder) ;
#else
               snprintf(path_1, sizeof(path_1)-1, "%s\\DfsCheck.1", __work_folder) ;
#endif

            file=fopen(path_1, "wb") ;
         if(file==NULL) {
                          sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - Data file creation error %d : %s", prefix, errno, path_1) ;
                       LB_ADD_ROW(IDC_LOG, text) ;
                           return ;
                        }

                 sprintf(text, "%lx", (long)time(NULL)) ;
                  fwrite(text, 1, strlen(text), file) ;
                  fclose(file) ;

/*-------------------------------- Отправка контрольного файла в DFS */

             error[0]=0 ;

  do {

       status=EMIR_dfs_putfile(path_1, id, error) ;
    if(status!=0)  break ;

#ifdef  UNIX
         snprintf(path_2, sizeof(path_2)-1, "%s/DfsCheck.2", __work_folder) ;
#else
         snprintf(path_2, sizeof(path_2)-1, "%s\\DfsCheck.2", __work_folder) ;
#endif

       status=EMIR_dfs_getfile(id, path_2, error) ;
    if(status!=0)  break ;

       status=EMIR_compare_files(path_1, path_2) ;                  /* Сравнение файлов */
    if(status) {
                   snprintf(error, sizeof(error)-1, "Invalid file replication on DFS: %s", path_1) ;
                      break ;
               } 

                          unlink(path_1) ;
                          unlink(path_2) ;

     } while(0) ;

    if(error[0]!=0) {
                          sprintf(text, "%s - ERROR - EMIRi_st_dfs_probe - %s", prefix, error) ;
                       LB_ADD_ROW(IDC_LOG, text) ;
                           return ;
                    }
/*--------------------------- Передача ID контрольного файла в СК ЦМ */

              EMIR_txt2hex64(id, id_hex, strlen(id)) ;

                            reply[0]=0 ;

      do {

             sprintf(data, "04a74a8c"                               /* Формируем блок данных транзакции */
                           "0000000000000000000000000000000000000000000000000000000000000020"
                           "%064x"
                           "%s",   
                            (int)strlen(id), id_hex) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        alive_address, data, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             alive_address, data, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&     /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {  /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        continue ;
                                                                }
                             break ;
                      }

                         break ;
         } while(1) ;

/*------------------------------------------------ Завершение работы */

                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*                Проверка контрольного файла в DFS                 */

   void  EMIRi_st_dfs_check(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            FILE *file ;
            char  path[FILENAME_MAX] ;
            char  id[1024] ;
            char  error[1024] ;
            char  text[4196] ;
            char *end ;

   static    int  alive_time ;
   static time_t  alive_prv_ts ;
   static   char  alive_address[100] ;
   static time_t  file_prv_ts ;

/*------------------------------------------------- Входная проверка */

   if(       __monitoring_rules[0]       !=0    &&
      strstr(__monitoring_rules, "DFS")==NULL   )  return ;

     if(__work_folder[0]==0) {
                               strcpy(error, "") ;
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Work folder is not defined", prefix) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                             }  
/*---------------------------------------- Контроль периода проверки */

   if(alive_prv_ts!=0)
     if(alive_prv_ts+alive_time>time(NULL))  return ;

        alive_prv_ts=time(NULL) ;

EMIR_log("EMIRi_st_dfs_check") ;

/*---------------------------------------------------- Инициализация */

                   error[0]=0 ;

   if(alive_address[0]==0) {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
    if(__mon_context_path[0]!=0) {

                       alive_time    =__mon_alive_pulse ;
                strcpy(alive_address, __mon_alive_address) ;

          status=EMIRi_st_StateFile("READ", NULL, NULL, sentry_file_last, sentry_file_mark, &file_prv_ts, error) ;
       if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_newblocks - Get saved state : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
//                                 return ;
                  }

                                 }
/*- - - - - - - - - - - - - - - - - - - - - -  Для настроек через БД */
    else                         {

        status=EMIR_db_syspar(db, "Monitoring", alive_address, error) ;
     if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Alive address: %s", prefix, error) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                }
                                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                           }
/*------------------------------------------------------- Подготовка */

               Cursor=NULL ;

   if(__mon_context_path[0]==0) {
   
        Cursor=db->LockCursor("EMIRi_st_dfs_check") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
                                }
/*------------------------------------ Получение начальных установок */

  if(alive_time==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"DfsCheckTime\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                        strcpy(text,(char *)Cursor->columns[0].value) ;
            alive_time=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - Получение последнего успешного файла */
                      sprintf(text, "select \"DfsCheck\",\"DfsMark\",\"DfsCheckTs\" from  %s ",
                                       __db_table_sentry_state) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get last dfs check : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get last dfs check : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Get last dfs check : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                          strcpy(sentry_file_last, (char *)Cursor->columns[0].value) ;
                          strcpy(sentry_file_mark, (char *)Cursor->columns[1].value) ;
                          strcpy(text,             (char *)Cursor->columns[2].value) ;
             file_prv_ts=strtoul(text, &end, 10) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                    }
/*-------------------------------------------- Определение просрочки */

                                            sentry_file_fail=0 ;
   if(time(NULL)-file_prv_ts>2*alive_time)  sentry_file_fail=1 ;

/*--------------------------- Получение данных со смарт-контракта ЦМ */

       status=EMIRi_st_GetCheckFile(alive_address, id, error) ;
    if(status) {
                    sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Check file id get error: %s", prefix, error) ;
                 LB_ADD_ROW(IDC_LOG, text) ;
                    if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;
                             return ;
               }

     if(id[0]==0) {
                    sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Empty file id get", prefix) ;
                 LB_ADD_ROW(IDC_LOG, text) ;

                      sentry_file_fail=0 ;

                    if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;
                             return ;
                  }
/*------------------------------ Получение контрольного файла из DFS */
            
  do {

#ifdef  UNIX
         snprintf(path, sizeof(path)-1, "%s/DfsCheck.0", __work_folder) ;
#else
         snprintf(path, sizeof(path)-1, "%s\\DfsCheck.0", __work_folder) ;
#endif

       status=EMIR_dfs_getfile(id, path, error) ;
    if(status!=0)  break ;

       file=fopen(path, "r") ;
    if(file==NULL) {
                     snprintf(error, sizeof(error)-1, "check file read error %d : %s", errno, path) ;
                        break ;
                   }

           memset(sentry_file_mark, 0, sizeof(sentry_file_mark)) ;
            fread(sentry_file_mark, 1, sizeof(sentry_file_mark)-1, file) ;
           fclose(file) ;
           unlink(path) ;

           strcpy(sentry_file_last, id) ;
                  sentry_file_fail=0 ;
                         file_prv_ts=time(NULL) ;

     } while(0) ;

    if(error[0]!=0) {
                          sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - %s", prefix, error) ;
                       LB_ADD_ROW(IDC_LOG, text) ;
                    if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;
                           return ;
                    }
/*------------------------------------- Сохранение изменённого блока */

   if(!stricmp(sentry_file_last, id)) {
/*- - - - - - - - - - - - - - - - - - - - -  Для настроек через файл */
     if(__mon_context_path[0]!=0) {

          status=EMIRi_st_StateFile("WRITE", NULL, NULL, sentry_file_last, sentry_file_mark, &file_prv_ts, error) ;
       if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Save state : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                    return ;
                  }
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Для настроек через БД */
     else                         {

                      sprintf(text, "update %s set \"DfsCheck\"='%s', \"DfsMark\"='%s', \"DfsCheckTs\"=%ld ",
                                      __db_table_sentry_state, sentry_file_last, sentry_file_mark, (long)file_prv_ts) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     sprintf(text, "%s - ERROR - EMIRi_st_dfs_check - Save last check file : %s", prefix, db->error_text) ;
                  LB_ADD_ROW(IDC_LOG, text) ;
                      db->Rollback() ;
                      db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                             return ;
                }

                 db->Commit() ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                      }
/*------------------------------------------------ Завершение работы */

     if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*      Обработка информации узлов из Центрального Мониторинга      */

   void  EMIRi_st_nodes(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
  static St_node *nodes ;
             int  cnt ;
            char  id[64] ;
            char  ts[64] ;
            char  error[1024] ;
            char  text[4096] ;
            char *end ;
             int  i ;

   static   char  check_net[128] ;
   static    int  alive_time ;
   static time_t  alive_prv_ts ;
   static   char  alive_address[100] ;
   static   char  node_id[128] ;

/*------------------------------------------------- Входной контроль */

     if(__mon_context_path[0]!=0)  return ;                         /* Не проводится при отсутсвии БД */

     if(check_net[0]=='N' ||
        check_net[0]=='n'   )  return ;

     if(alive_prv_ts+alive_time>time(NULL))  return ;

        alive_prv_ts=time(NULL) ;

/*---------------------------------------------------- Инициализация */

   if(alive_address[0]==0) {

        status=EMIR_db_syspar(db, "Monitoring", alive_address, error) ;
     if(status) {
                           sprintf(text, "%s - ERROR - EMIRi_st_nodes - Alive address: %s", prefix, error) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                               return ;
                }
                           }
/*------------------------------------------------------- Подготовка */

        Cursor=db->LockCursor("EMIRi_st_nodes") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_nodes - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
/*------------------------------------ Получение начальных установок */

  if(check_net[0]==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"NodeId\",\"AliveTime\",\"CheckNet\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                       strncpy(node_id,(char *)Cursor->columns[0].value, 32) ;
                        strcpy(text,(char *)Cursor->columns[1].value) ;
            alive_time=strtoul(text, &end, 10) ;
                       strncpy(check_net,(char *)Cursor->columns[2].value, 32) ;

                 db->SelectClose (Cursor) ;

         if(check_net[0]!='Y' && 
            check_net[0]!='y'   )  check_net[0]='N' ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }

     if(check_net[0]=='N' ||
        check_net[0]=='n'   ) {
                                 db->UnlockCursor(Cursor) ;
                                        return ;
                              }
/*------------------------------------- Получение списка узлов на ЦМ */

           cnt=EMIRi_st_GetNodes(alive_address, &nodes, error) ;
        if(cnt<0) {
                         sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get nodes list : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                                  return ;
                  }
/*---------------------------------------------- Запрос данных узлов */

    for(i=0 ; i<cnt ; i++) {

         status=EMIRi_st_GetNode(alive_address, &nodes[i], error) ;
      if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get node alive data : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return ;
                 }
                           }
/*------------------------ Получение списка обработанных ранее узлов */

                      sprintf(text, "Select \"NodeId\", \"NodeTimestamp\" "
                                    "From   %s ",
                                     __db_table_sentry_nodes) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_nodes - Get processed nodes list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                   return ;
                }

   do {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_nodes - Fetch processed nodes list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                   return ;
                }

           strncpy(id, (char *)Cursor->columns[0].value, sizeof(id)-1) ;
           strncpy(ts, (char *)Cursor->columns[1].value, sizeof(ts)-1) ;

       for(i=0 ; i<cnt ; i++)
         if(!stricmp(nodes[i].node_id, id))  break ;

         if(i<cnt) {
           if(strcmp(nodes[i].node_ts, ts)<0)  nodes[i].update=-1 ;
           else                                nodes[i].update= 1 ;
                   }

      } while(1) ;

                 db->SelectClose (Cursor) ;

/*----------------------------------------- Запись данных узлов в БД */

   for(i=0 ; i<cnt ; i++) {

     if(nodes[i].update==-1)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - Запись основных данных */
     if(nodes[i].update== 0)
              sprintf(text, "insert into %s (\"NodeId\",\"Address\",\"NodeTimestamp\",\"BlockLast\",\"DfsCheck\",\"Balance\",\"Errors\",\"Pending\") "
                                    "values ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
                       __db_table_sentry_nodes, nodes[i].node_id,   nodes[i].address, nodes[i].node_ts, nodes[i].block_last,
                                                nodes[i].dfs_check, nodes[i].balance, nodes[i].errors,  nodes[i].pending    ) ;
     else     sprintf(text, "update %s "
                            "set    \"Address\"      ='%s' "
                            "      ,\"NodeTimestamp\"='%s' "
                            "      ,\"BlockLast\"    ='%s' "
                            "      ,\"DfsCheck\"     ='%s' "
                            "      ,\"Balance\"      ='%s' "
                            "      ,\"Errors\"       ='%s' "
                            "      ,\"Pending\"      ='%s' "
                            "where  \"NodeId\"='%s' ",
                       __db_table_sentry_nodes, nodes[i].address,   nodes[i].node_ts, nodes[i].block_last,
                                                nodes[i].dfs_check, nodes[i].balance, nodes[i].errors,  nodes[i].pending,
                                                nodes[i].node_id    ) ;

                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                           EMIR_log(text) ;
                            sprintf(text, "%s - ERROR - EMIRi_st_nodes - Save node's basic data : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                     return ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - -  Запись Alarm-данных */
    if(stricmp(sentry_wf_version,                                   /* Только для версий "Monitoring.2019-03-05" и выше */
               "Monitoring.2019-03-05")>=0) {

             snprintf(text, sizeof(text)-1, 
                            "update %s "
                            "set    \"Alarm\"='%s' "
                            "where  \"NodeId\"='%s' ",
                       __db_table_sentry_nodes, nodes[i].alarm,
                                                nodes[i].node_id) ;

                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                           EMIR_log(text) ;
                            sprintf(text, "%s - ERROR - EMIRi_st_nodes - Save node's alarm data : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                     return ;
                        }

                                            }
/*- - - - - - - - - - - - - - - - - - - - - -  Запись Details-данных */
    if(stricmp(sentry_wf_version,                                   /* Только для версий "Monitoring.2019-03-05" и выше */
               "Monitoring.2019-03-05")>=0) {

             snprintf(text, sizeof(text)-1, 
                            "update %s "
                            "set    \"Details\"='%s' "
                            "where  \"NodeId\"='%s' ",
                       __db_table_sentry_nodes, nodes[i].details,
                                                nodes[i].node_id    ) ;

                status=db->SqlExecute(Cursor, text, NULL, 0) ;
             if(status) {
                           EMIR_log(text) ;
                            sprintf(text, "%s - ERROR - EMIRi_st_nodes - Save node's details data : %s", prefix, db->error_text) ;
                         LB_ADD_ROW(IDC_LOG, text) ;
                             db->UnlockCursor(Cursor) ;
                             db->Rollback() ;
                           __db_errors_cnt++ ;
                                     return ;
                        }

                                            }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                          }

                 db->Commit() ;

/*------------------------------------------------ Завершение работы */

                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*                 Формирование сигналов Мониторинга                */

   void  EMIRi_st_monitoring(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
          time_t  time_abs ;
          time_t  time_chk ;
          time_t  time_diff ;
       struct tm *hhmmss ;
       struct tm  hhmmss_n ;
            FILE *file ;
             int  status ;
  static St_node *nodes[_NODES_MAX] ;
             int  nodes_cnt ;
             int  errors_cnt ;
   unsigned long  block_max ;
   unsigned long  block_num ;
            char  text[1024] ;
            char *value ;
            char *end ;
             int  pending ;
             int  flag ;
             int  mon_raised ;
             int  i ;

   static   char  check_net[128] ;
   static    int  check_time ;
   static time_t  check_prv_ts ;
   static   char  balance_limit[128] ;
   static    int  alive_time ;
   static    int  files_time ;
   static   char  node_id[128] ;

/*------------------------------------------------- Входная проверка */

   if(__monitoring_path[0]==0)  return ;

    if(check_prv_ts) 
     if(check_prv_ts+check_time>time(NULL))  return ;

        check_prv_ts=time(NULL) ;

  EMIR_log("EMIRi_st_monitoring") ;
    
/*------------------------------------------------------- Подготовка */

				             Cursor=NULL ;

   if(__mon_context_path[0]!=0) {

                       check_time=__mon_bh_pulse ;
                       alive_time=__mon_alive_pulse ;
                strcpy(node_id,   __mon_node_id) ;
                strcpy(check_net,   "N"        ) ;

                                }
   else                         {

        Cursor=db->LockCursor("EMIRi_st_monitoring") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                               return ;
                      }
                                }
/*------------------------------------ Получение начальных установок */

  if(check_net[0]==0) {
/*- - - - - - - - - - - - - - - - - - -  Получение настроек проверки */
                      sprintf(text, "select \"NodeId\",\"BlockCheckTime\",\"AliveTime\",\"DfsCheckTime\",\"BalanceLimit\",\"CheckNet\" from  %s ",
                                       __db_table_sentry_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Get alive settings : %s", prefix, db->error_text) ;
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
                sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Get alive settings : %s", prefix, "Data is missed") ;
         else   sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Get alive settings : %s", prefix, db->error_text) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                       strncpy(node_id,      (char *)Cursor->columns[0].value, 32) ;
                        strcpy(text,         (char *)Cursor->columns[1].value) ;
            check_time=strtoul(text, &end, 10) ;
                        strcpy(text,         (char *)Cursor->columns[2].value) ;
            alive_time=strtoul(text, &end, 10) ;
                        strcpy(text,         (char *)Cursor->columns[3].value) ;
            files_time=strtoul(text, &end, 10) ;
                        strcpy(balance_limit,(char *)Cursor->columns[4].value) ;
                       strncpy(check_net,    (char *)Cursor->columns[5].value, 32) ;

                 db->SelectClose (Cursor) ;

         if(check_net[0]!='Y' && 
            check_net[0]!='y'   )  check_net[0]='N' ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                      }
/*---------------------------------------------- Запрос данных узлов */

                              nodes_cnt=0 ;

   if(check_net[0]=='Y' || 
      check_net[0]=='y'   ) {

                      sprintf(text, "select \"NodeId\", \"Address\", \"NodeTimestamp\", \"BlockLast\", \"DfsCheck\", \"Balance\" "
                                                     ", \"Errors\", \"Pending\", \"Alarm\", \"Details\" "
                                    "from   %s ",
                                       __db_table_sentry_nodes) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Get nodes data : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

   for(i=0 ; i<_NODES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_st_monitoring - Get nodes data: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

     if(nodes[i]==NULL)  nodes[i]=(struct St_node *)calloc(1, sizeof(*nodes[i])) ;

            memset(nodes[i], 0, sizeof(*nodes[i])) ;
           strncpy(nodes[i]->node_id,    (char *)Cursor->columns[0].value, sizeof(nodes[i]->node_id   )-1) ;
           strncpy(nodes[i]->address,    (char *)Cursor->columns[1].value, sizeof(nodes[i]->address   )-1) ;
           strncpy(nodes[i]->node_ts,    (char *)Cursor->columns[2].value, sizeof(nodes[i]->node_ts   )-1) ;
           strncpy(nodes[i]->block_last, (char *)Cursor->columns[3].value, sizeof(nodes[i]->block_last)-1) ;
           strncpy(nodes[i]->dfs_check,  (char *)Cursor->columns[4].value, sizeof(nodes[i]->dfs_check )-1) ;
           strncpy(nodes[i]->balance,    (char *)Cursor->columns[5].value, sizeof(nodes[i]->balance   )-1) ;
           strncpy(nodes[i]->errors,     (char *)Cursor->columns[6].value, sizeof(nodes[i]->errors    )-1) ;
           strncpy(nodes[i]->pending,    (char *)Cursor->columns[7].value, sizeof(nodes[i]->pending   )-1) ;
           strncpy(nodes[i]->alarm,      (char *)Cursor->columns[8].value, sizeof(nodes[i]->alarm     )-1) ;
           strncpy(nodes[i]->details,    (char *)Cursor->columns[9].value, sizeof(nodes[i]->details   )-1) ;

                              nodes_cnt++ ;

                                 }

                 db->SelectClose (Cursor) ;

                            }

   if(Cursor!=NULL)  db->UnlockCursor(Cursor) ;

/*--------------------------------------- Открытие файла мониторинга */

      file=fopen(__monitoring_path, "w") ;
   if(file==NULL) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_st_monitoring - Monitoring file oper error %d : %s", prefix, errno, __monitoring_path) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                            db->Rollback() ;
                                   return ;
                  }
/*------------------------------------------- Формирование заголовка */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;
 
   if(!stricmp(__monitoring_format, "XML")) {

     sprintf(text, "<Monitoring Timestamp=\"%04d.%02d.%02d %02d:%02d:%02d\" Name=\"%s\">\n",
                    hhmmss->tm_year+1900, hhmmss->tm_mon+1, hhmmss->tm_mday,
                    hhmmss->tm_hour,      hhmmss->tm_min,   hhmmss->tm_sec,
                    node_id                                                 ) ;
      fwrite(text, 1, strlen(text), file) ;
                                            }
   else                                     {

     sprintf(text, "%04d.%02d.%02d %02d:%02d:%02d\n",
                    hhmmss->tm_year+1900, hhmmss->tm_mon+1, hhmmss->tm_mday,
                    hhmmss->tm_hour,      hhmmss->tm_min,   hhmmss->tm_sec  ) ;
      fwrite(text, 1, strlen(text), file) ;

     sprintf(text, "%s\n", node_id) ;
      fwrite(text, 1, strlen(text), file) ;
                                            }
/*--------------------------------------- Определение сдвига времени */

                 hhmmss=gmtime(&time_abs) ;
              time_diff=time_abs-mktime( hhmmss)  ;

             mon_raised=0 ;

/*---------------------------------- Формирование локальных сигналов */


     if(!stricmp(__monitoring_format, "XML")) {
                             sprintf(text, "<Errors>\n") ;
                              fwrite(text, 1, strlen(text), file) ;
                                              }

    if(sentry_block_fail  ) {

         EMIR_log("Monitoring - New blocks missed") ;

      if(!stricmp(__monitoring_format, "XML"))
                              sprintf(text, "<Error Category=\"Local\" Name=\"%s\">New blocks missed</Error>\n", node_id) ;
      else                    sprintf(text, "ERROR Local - new blocks missed\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                        mon_raised=1 ;
                            }
    if(sentry_mark_fail   ) {

         EMIR_log("Monitoring - Central monitoring is not accessible") ;

      if(!stricmp(__monitoring_format, "XML"))
                              sprintf(text, "<Error Category=\"Local\" Name=\"%s\">Cental monitoring is not accessible</Error>\n", node_id) ;
      else                    sprintf(text, "ERROR Local - Cental monitoring is not accessible\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                        mon_raised=1 ;
                            }
    if(sentry_file_fail   ) {

         EMIR_log("Monitoring - Check file missed") ;

      if(!stricmp(__monitoring_format, "XML"))
                              sprintf(text, "<Error Category=\"Local\" Name=\"%s\">Check file missed</Error>\n", node_id) ;
      else                    sprintf(text, "ERROR Local - check file missed\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                        mon_raised=1 ;
                            }
    if(sentry_balance_fail) {

         EMIR_log("Monitoring - Balance is critical low") ;

      if(!stricmp(__monitoring_format, "XML"))
                              sprintf(text, "<Error Category=\"Local\" Name=\"%s\">Balance is critical low</Error>\n", node_id) ;
      else                    sprintf(text, "ERROR Local - balance is critical low\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                        mon_raised=1 ;
                            }
    if(sentry_errors_cnt  ) {
      if(!stricmp(__monitoring_format, "XML"))
                              sprintf(text, "<Error Category=\"Local\" Name=\"%s\">Errors in actions queue detected</Error>\n", node_id) ;
      else                    sprintf(text, "ERROR Local - errors in actions queue detected\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                        mon_raised=1 ;
                            }

       if(sentry_block_fail  ||                                     /* Подъем флага блокировки сети обмена */
          sentry_mark_fail   ||
          sentry_file_fail   ||
          sentry_balance_fail  ) {

           if(!__net_locked)  EMIR_log("ATTENTION! Networking is LOCKED by monitoring") ;

                                   __net_locked=1 ;
                                 }
       else                      {

           if( __net_locked)  EMIR_log("ATTENTION! Networking is UNLOCKED by monitoring") ;

                                   __net_locked=0 ;
                                 }

/*---------------------------- Формирование сигналов по другим узлам */

   if(check_net[0]=='Y' || 
      check_net[0]=='y'   ) {
                                 time_abs=time(NULL) ;
/*- - - - - - - - - - - - - - - - - - - - - - Анализ временной метки */
//   2018.08.31 11:44:11
//   01234567890123456789

                      errors_cnt=0 ;

        for(i=0 ; i<nodes_cnt ; i++) {

               memset(&hhmmss_n, 0, sizeof(hhmmss_n)) ;

                       hhmmss_n.tm_year=strtoul(nodes[i]->node_ts   , &end, 10)-1900 ;
                       hhmmss_n.tm_mon =strtoul(nodes[i]->node_ts+ 5, &end, 10)-   1 ;
                       hhmmss_n.tm_mday=strtoul(nodes[i]->node_ts+ 8, &end, 10) ;
                       hhmmss_n.tm_hour=strtoul(nodes[i]->node_ts+11, &end, 10) ;
                       hhmmss_n.tm_min =strtoul(nodes[i]->node_ts+14, &end, 10) ;
                       hhmmss_n.tm_sec =strtoul(nodes[i]->node_ts+17, &end, 10) ;

              nodes[i]->ts=mktime(&hhmmss_n)+time_diff ;
           if(nodes[i]->ts<time_abs-2*alive_time) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">Alive missed since %s</Error>\n", nodes[i]->node_id, nodes[i]->node_ts) ;
             else        sprintf(text, "ERROR Net - Node %s alive missed since %s\n", nodes[i]->node_id, nodes[i]->node_ts) ;
                          fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->ts_error=1 ;
                                                      errors_cnt++ ;
                                                      mon_raised=1 ;
                                                  }
                                     }

       if(errors_cnt==nodes_cnt) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Global\" >No new blocks generation or global pending</Error>\n") ;
             else                         sprintf(text, "ERROR Net - No new blocks generation or global pending\n") ;
                          fwrite(text, 1, strlen(text), file) ;
                                            mon_raised=1 ;
                                 }
/*- - - - - - - - - - - - - - - - - - - - - - Анализ концевых блоков */
              strcpy(text, "0000000000000000000000000000000000000000000000000000000000000000") ;
 
        for(i=0 ; i<nodes_cnt ; i++)
          if(strcmp(text, nodes[i]->block_last)<0) {
                             strcpy(text, nodes[i]->block_last) ;
                                         time_chk=nodes[i]->ts ;
                                                   }

             block_max=strtoul(text, &end, 16) ;

        for(i=0 ; i<nodes_cnt ; i++)
          if(nodes[i]->ts_error==0) {

                 block_num=strtoul(nodes[i]->block_last, &end, 16) ;

//          if(block_max-block_num > 
//              (time_chk-nodes[i]->ts+alive_time)/__mon_gen_period+3)
            if(block_max-block_num > __mon_blocks_delay) {

             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">New block proccesing delay</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s new block proccesing delay\n", nodes[i]->node_id) ;
                          fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->nb_error=1 ;
                                                      mon_raised=1 ;
                                                         }
                                    }
/*- - - - - - - - - - - - - - - - - - - -  Анализ контрольных файлов */
  if(stricmp(__dfs_type, "DIRECT")) {

                      errors_cnt=0 ;

        for(i=0 ; i<nodes_cnt ; i++)
          if(nodes[i]->ts_error==0) {

                           memcpy(text, nodes[i]->dfs_check, 8) ;
                                  text[8]=0 ;

                 time_chk=strtoul(text, &end, 16) ;

            if(nodes[i]->ts-time_chk>2*files_time) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">DFS failed</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s DFS failed\n", nodes[i]->node_id) ;
                          fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->fl_error=1 ;
                                                      errors_cnt++ ;
                                                      mon_raised=1 ;
                                                   }
                                    }

       if(errors_cnt==nodes_cnt) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Global\" >DFS crashed</Error>\n") ;
             else        sprintf(text, "ERROR Net - DFS crashed\n") ;
                          fwrite(text, 1, strlen(text), file) ;
                                            mon_raised=1 ;
                                 }

                                    }
/*- - - - - - - - - - - - - - - - - - - - - - Анализ балансов счетов */
  if(strcmp(__net_type, "QUORUM")) {                                /* Для Quorum - не анализируем */

        for(i=0 ; i<nodes_cnt ; i++) {

            for(value=nodes[i]->balance ; *value=='0' ; value++) ;

                                                         flag=0 ;
                if(strlen(value)<strlen(balance_limit))  flag=1 ;
           else if(strlen(value)>strlen(balance_limit))  flag=0 ;
           else if(strcmp(value, balance_limit)<0     )  flag=1 ;

           if(flag) {

             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">Account balance is critically low</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s account balance is critically low\n", nodes[i]->node_id) ;
                            fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->bl_error=1 ;
                                                      mon_raised=1 ;
                    }
                                     }

                                   }
/*- - - - - - - - - - - - - - - - - - - - - -  Анализ ошибок очереди */
        for(i=0 ; i<nodes_cnt ; i++) {

             block_num=strtoul(nodes[i]->errors, &end, 16) ;
          if(block_num) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">Errors in operation queues</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s has errors in operation queues\n", nodes[i]->node_id) ;
                            fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->ac_error=1 ;
                                                      mon_raised=1 ;
                        }
                                     }
/*- - - - - - - - - - - - - - - - - - - -  Анализ задержек в очереди */
        for(i=0 ; i<nodes_cnt ; i++) {

             pending=strtoul(nodes[i]->pending, &end, 16) ;

          if(pending<nodes[i]->pd_prv) {
                                            nodes[i]->pd_prv = 0 ;
                                            nodes[i]->pd_deep= 0 ;
                                       }
          if(pending>nodes[i]->pd_prv) {
                                            nodes[i]->pd_prv =pending;
                                            nodes[i]->pd_deep++ ;
                                       }

          if(nodes[i]->pd_deep>5) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">Pendings grow in operation queues</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s has pendings grow in operation queues\n", nodes[i]->node_id) ;
                            fwrite(text, 1, strlen(text), file) ;
                                              nodes[i]->pd_error=1 ;
                                                      mon_raised=1 ;
                                  }
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - Анализ ALARM-сообщений */
    if(stricmp(sentry_wf_version,                                   /* Только для версий "Monitoring.2019-03-05" и выше */
               "Monitoring.2019-03-05")>=0) {

        for(i=0 ; i<nodes_cnt ; i++) {

          if(nodes[i]->alarm[0]!=0) {
             if(!stricmp(__monitoring_format, "XML"))
                         sprintf(text, "<Error Category=\"Node\" Name=\"%s\">Alarm is sent</Error>\n", nodes[i]->node_id) ;
             else        sprintf(text, "ERROR Net - Node %s has alarm message\n", nodes[i]->node_id) ;
                            fwrite(text, 1, strlen(text), file) ;
                                                      mon_raised=1 ;
                                    }
                                     }

                                            }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                            }
/*------------------------------------------ Завершение блока ошибок */

      if(!stricmp(__monitoring_format, "XML")) {
                              sprintf(text, "</Errors>\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                               }

    if(mon_raised==0) {

      if(!stricmp(__monitoring_format, "XML")) {
                              sprintf(text, "<OverallState>Normal</OverallState>\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                               }
      else                                     {          
                        sprintf(text, "NORMAL\n") ;
                         fwrite(text, 1, strlen(text), file) ;
                                               }
                      }
    else              {

      if(!stricmp(__monitoring_format, "XML")) {
                              sprintf(text, "<OverallState>Error</OverallState>\n") ;
                               fwrite(text, 1, strlen(text), file) ;
                                               }
                      }
/*--------------------------------- Формирование информации по узлам */

   if(check_net[0]=='Y' || 
      check_net[0]=='y'   ) {

      if(!stricmp(__monitoring_format, "XML")) {
           sprintf(text, "<Nodes>\n");
            fwrite(text, 1, strlen(text), file) ;
                                               }

    for(i=0 ; i<nodes_cnt ; i++) {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - XML-формат */
      if(!stricmp(__monitoring_format, "XML")) {

           sprintf(text, "<Node Name=\"%s\" Account=\"%s\">\n", 
                          nodes[i]->node_id,    nodes[i]->address) ;
            fwrite(text, 1, strlen(text), file) ;

           sprintf(text, "<Info LastAlive=\"%s\" LastBlock=\"%s\" LastFile=\"%s\" Balance=\"%s\" Errors=\"%s\" Pending=\"%s\" />\n",
                          nodes[i]->node_ts,  nodes[i]->block_last, nodes[i]->dfs_check,
                          nodes[i]->balance,  nodes[i]->errors,     nodes[i]->pending   ) ;
            fwrite(text, 1, strlen(text), file) ;

        if(stricmp(sentry_wf_version,                               /* Только для версий "Monitoring.2019-03-05" и выше */
                      "Monitoring.2019-03-05")>=0) {
           sprintf(text, "<Alarm>") ;
            fwrite(text, 1, strlen(text), file) ;
            fwrite(nodes[i]->alarm, 1, strlen(nodes[i]->alarm), file) ;
           sprintf(text, "</Alarm>\n") ;
            fwrite(text, 1, strlen(text), file) ;

           sprintf(text, "<Details>") ;
            fwrite(text, 1, strlen(text), file) ;
            fwrite(nodes[i]->details, 1, strlen(nodes[i]->details), file) ;
           sprintf(text, "</Details>\n") ;
            fwrite(text, 1, strlen(text), file) ;
                                                   }

           sprintf(text, "<Status LostConnection=\"%d\" BlocksDelay=\"%d\" DfsFail=\"%d\" LowBalance=\"%d\" Errors=\"%d\" PendingsGrow=\"%d\" />\n",
                          nodes[i]->ts_error, nodes[i]->nb_error, nodes[i]->fl_error,
                          nodes[i]->bl_error, nodes[i]->ac_error, nodes[i]->pd_error ) ;
            fwrite(text, 1, strlen(text), file) ;

           sprintf(text, "</Node>\n");
            fwrite(text, 1, strlen(text), file) ;

                                               }
/*- - - - - - - - - - - - - - - - - - - - - - - - - Текстовой формат */
      else                                     {

           sprintf(text, "INFO %s;%s;%s;%s;%s;%s;%s;%s\n", 
                          nodes[i]->node_id,    nodes[i]->address,   nodes[i]->node_ts,
                          nodes[i]->block_last, nodes[i]->dfs_check, nodes[i]->balance,
                          nodes[i]->errors,     nodes[i]->pending                      ) ;
            fwrite(text, 1, strlen(text), file) ;

        if(stricmp(sentry_wf_version,                               /* Только для версий "Monitoring.2019-03-05" и выше */
                   "Monitoring.2019-03-05")>=0) {
           sprintf(text, "ALARM ") ;
            fwrite(text, 1, strlen(text), file) ;
            fwrite(nodes[i]->alarm, 1, strlen(nodes[i]->alarm), file) ;
            fwrite("\r\n", 1, strlen("\r\n"), file) ;
 
           sprintf(text, "DETAILS ") ;
            fwrite(text, 1, strlen(text), file) ;
            fwrite(nodes[i]->details, 1, strlen(nodes[i]->details), file) ;
            fwrite("\r\n", 1, strlen("\r\n"), file) ;
                                                }

           sprintf(text, "STATUS %s;%d;%d;%d;%d;%d;%d\n",
                          nodes[i]->node_id,
                          nodes[i]->ts_error, nodes[i]->nb_error, nodes[i]->fl_error,
                          nodes[i]->bl_error, nodes[i]->ac_error, nodes[i]->pd_error ) ;
            fwrite(text, 1, strlen(text), file) ;

                                               }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                 }

      if(!stricmp(__monitoring_format, "XML")) {
           sprintf(text, "</Nodes>\n");
            fwrite(text, 1, strlen(text), file) ;
                                               }

                            }
/*-------------------------------------------- Формирование концовки */

   if(!stricmp(__monitoring_format, "XML")) {

     sprintf(text, "</Monitoring>\n") ;
      fwrite(text, 1, strlen(text), file) ;
                                            }
   else                                     {
                                            }
/*--------------------------------------- Закрытие файла мониторинга */

                 fclose(file) ;

/*-------------------------------------------------------------------*/

}


/*********************************************************************/
/*								             */
/*        Запрос методa GetNodesList смарт-контракта Monitoring      */

   int  EMIRi_st_GetNodes(char *contract, St_node **list, char *error)

{
          int  status ;
         char *result ;
          int  list_cnt ;
         char  text[1024] ;
         char *end ;
          int  i  ;

  static char *buff ;

#define  _BUFF_MAX  64000

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x1ac8fc46\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*------------------------------------ Отправка запроса GetNodesList */

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

                   memset(text, 0, sizeof(text)) ;  
                   memcpy(text, result+1*64, 64) ;
         list_cnt=strtoul(text, &end, 16) ;
      if(list_cnt==0)  return(0) ;

                 result+=2*64 ;

          *list=(St_node *)realloc(*list, list_cnt*sizeof(St_node)) ;

      for(i=0 ; i<list_cnt ; i++) {
              memset(&(*list)[i], 0, sizeof(St_node)) ;
              memset( text, 0, sizeof(text)) ;
              memcpy( text, result+i*64, 64) ;
        EMIR_hex2txt( text, (*list)[i].node_id) ;
                                  }

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(list_cnt) ;
}


/*********************************************************************/
/*                                                                   */
/*          Запрос методa GetNode смарт-контракта Monitoring         */

   int  EMIRi_st_GetNode(char *contract, St_node *node, char *error)

{
   int  status ;
  char *result ;
  char  key_hex[128] ;
  char  buff[16000] ;
  char  value[128] ;
  char *a_ptr  ;
  char *d_ptr ;
   int  size ;
  char *end ;

 static char *request_1="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x825e0d82%s\"},\"latest\"],\"id\":1}" ;
 static char *request_2="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x988a5fc6%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------ Подготовка данных */

    EMIR_txt2hex64(node->node_id, key_hex, strlen(node->node_id)) ;

/*----------------------------------------- Отправка запроса GetNode */

                          sprintf(buff, request_1, contract, key_hex) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Разбор результата */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memcpy(node->address,    result+0*64, 64) ;
              memcpy(node->node_ts,    result+1*64, 64) ;
              memcpy(node->block_last, result+2*64, 64) ;
              memcpy(node->dfs_check,  result+3*64, 64) ;
              memcpy(node->balance,    result+4*64, 64) ;
              memcpy(node->errors,     result+5*64, 64) ;
              memcpy(node->pending,    result+6*64, 64) ;

        EMIR_hex2txt(node->node_ts, node->node_ts) ;                /* Преобразование в текст */

              strcpy(node->address, node->address+24) ;

#undef   _RESULT_PREFIX

/*---------------------------------------- Отправка запроса GetAlarm */

   if(stricmp(sentry_wf_version,                                    /* Только для версий "Monitoring.2019-03-05" и выше */
               "Monitoring.2019-03-05")>=0) {

                          sprintf(buff, request_2, contract, key_hex) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Разбор результата */
#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строчных' атрибутов */
              memcpy(value, result+0*64, 64) ;
               a_ptr=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+1*64, 64) ;
               d_ptr=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, a_ptr, 64) ;                        /* Извлекаем поле "Alarm" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(node->alarm, a_ptr+64, size*2) ;

                  memcpy(value, d_ptr, 64) ;                        /* Извлекаем поле "Details" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(node->details, d_ptr+64, size*2) ;

               EMIR_hex2txt(node->alarm,   node->alarm) ;
               EMIR_hex2txt(node->details, node->details) ;

#undef   _RESULT_PREFIX
                                            }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*              Запрос GetCheckMark на смарт-контракт                */

   int  EMIRi_st_GetCheckMark(char *contract, char  *reply, char *error)

{
         int  status ;
        char  buff[2048] ;
        char  out[2048] ;
        char *result ;
        char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x008a4b28\"},\"latest\"],\"id\":1}" ;

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

                             out[64]=0 ;
               strcpy(reply, out) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*              Запрос GetCheckFile на смарт-контракт                */

   int  EMIRi_st_GetCheckFile(char *contract, char  *reply, char *error)

{
         int  status ;
        char  buff[2048] ;
        char  text[2048] ;
        char *result ;
        char *end ;
         int  size ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x42fcbba2\"},\"latest\"],\"id\":1}" ;

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

              memset(text, 0, sizeof(text)) ;

           result=strstr(buff, _RESULT_PREFIX) ;                   /* Проверка наличия результата */
        if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                         }

                 result+=strlen(_RESULT_PREFIX) ;

                 memcpy(text, result+1*64, 64) ;
           size=strtoul(text, &end, 16) ;
        if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
        if(size>0) {
                     memset(text, 0, sizeof(text)) ;
                     memcpy(text, result+2*64, size*2) ;
               EMIR_hex2txt(text, text) ;
                     strcpy(reply, text) ;
                   }
 
#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*               Работа с файлом состояния мониторинга               */

   int  EMIRi_st_StateFile(char *oper, char *block,
                                     time_t *block_ts, 
                                       char *file_id,
                                       char *file_mark,
                                     time_t *file_ts,   char *error)
{
       FILE *file ;
       char  text[1024] ;
       char *end ;
        int  row ;

  static     int  s_init ;
  static    char  s_block[256] ;
  static  time_t  s_block_ts  ;
  static    char  s_file_id[256]  ;
  static    char  s_file_mark[256] ;
  static  time_t  s_file_ts ;

/*==================================================== Чтение данных */

   if(!stricmp(oper, "READ" )) {

/*--------------------------------------------------- Открытие файла */

   if(s_init==0) {
      s_init=1 ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Открытие файла */
        file=fopen(__mon_state_path, "rb") ;
     if(file==NULL) {
                        sprintf(error, "State file open error %d :%s", errno, __mon_state_path) ;
                          return(-1) ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - Считывание файла */
                    row=0 ;
     while(1) {                                                     /* CIRCLE.1 - Построчно читаем файл */
                    row++ ;
                      memset(text, 0, sizeof(text)) ;
                   end=fgets(text, sizeof(text)-1, file) ;          /* Считываем строку */
                if(end==NULL)  break ;

         if(text[0]==';')  continue ;                               /* Проходим комментарий */

            end=strchr(text, '\n') ;                                /* Удаляем символ конца строки */
         if(end!=NULL)  *end=0 ;
            end=strchr(text, '\r') ;
         if(end!=NULL)  *end=0 ;

       if(row==1) {	                                               /* Разборка значений */
                     strncpy(s_block, text, 20) ;
                             s_block[20]=0 ;
                  }
       else
       if(row==2) {
                     s_block_ts=strtoul(text, &end, 16) ;
                  }
       else
       if(row==3) {
                     strncpy(s_file_id, text, 64) ;
                             s_file_id[64]=0 ;
                  }
       else
       if(row==4) {
                     strncpy(s_file_mark, text, 64) ;
                             s_file_mark[64]=0 ;
                  }
       else
       if(row==5) {
                     s_file_ts= 0 ;
                     s_file_ts=strtoul(text, &end, 16) ;
                  }

              }                                                     /* CONTINUE.1 */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Закрытие файла */
                           fclose(file) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                 }

/*--------------------------------------------- Выдача данных наружу */

           if(block    !=NULL)  strcpy(block, s_block) ;
           if(block_ts !=NULL)        *block_ts=s_block_ts ;

           if(file_id  !=NULL)  strcpy(file_id,   s_file_id) ;
           if(file_mark!=NULL)  strcpy(file_mark, s_file_mark) ;
           if(file_ts  !=NULL)        *file_ts=s_file_ts ;

/*-------------------------------------------------------------------*/
                               }
/*==================================================== Запись данных */

   if(!stricmp(oper, "WRITE")) {

/*------------------------------------------- Получение данных сружи */

           if(block    !=NULL)  strcpy(s_block, block) ;
           if(block_ts !=NULL)         s_block_ts=*block_ts ;

           if(file_id  !=NULL)  strcpy(s_file_id,   file_id) ;
           if(file_mark!=NULL)  strcpy(s_file_mark, file_mark) ;
           if(file_ts  !=NULL)         s_file_ts=*file_ts ;

/*--------------------------------------------------- Открытие файла */

        file=fopen(__mon_state_path, "wb") ;
     if(file==NULL) {
                        sprintf(error, "State file open error %d :%s", errno, __mon_state_path) ;
                          return(-1) ;
                    }
/*-------------------------------------------------- Запись значений */

            sprintf(text, "%s\n", s_block) ;
             fwrite(text,  1, strlen(text), file) ;

            sprintf(text, "%lx\n", (long)s_block_ts) ;
             fwrite(text,  1, strlen(text), file) ;

            sprintf(text, "%s\n", s_file_id) ;
             fwrite(text,  1, strlen(text), file) ;

            sprintf(text, "%s\n", s_file_mark) ;
             fwrite(text,  1, strlen(text), file) ;

            sprintf(text, "%lx\n", (long)s_file_ts) ;
             fwrite(text,  1, strlen(text), file) ;

/*--------------------------------------------------- Закрытие файла */

                   fclose(file) ;

/*-------------------------------------------------------------------*/
                               }
/*===================================================================*/

  return(0) ;
}


