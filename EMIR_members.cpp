/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Модуль "Участники системы"                    */
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

#if defined(UNIX) || defined(_CONSOLE)
#else
#include "controls.h"
#include "resource.h"
#endif

#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

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

  static  char  members_wf_version[256] ;  /* Версия рабочего процесса по модулю Members */

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

  void  EMIRi_mb_synch              (HWND hDlg, char *prefix, SQL_link *db) ;               /* Процедура фоновой синхронизации объектов модуля "Участники" */
  void  EMIRi_mb_actions            (HWND hDlg, char *prefix, SQL_link *db) ;               /* Обработка очереди операций модуля "Участники" */
   int  EMIR_mb_action_AddMember    (Mb_action *action, SQL_link *db, char *error) ;        /* Операция ADD_MEMBER */
   int  EMIR_mb_action_PublMembers  (Mb_action *action, SQL_link *db, char *error) ;        /* Операция PUBLISH_MEMBERS */
   int  EMIR_mb_action_GetMember    (Mb_action *action, SQL_link *db, char *error) ;        /* Операция GET_MEMBER */
   int  EMIR_mb_action_GetMemberFile(Mb_action *action, SQL_link *db, char *error) ;        /* Операция GET_MEMBER_FILE */
   int  EMIR_mb_action_RaiseAlert   (Mb_action *action, SQL_link *db, char *error) ;        /* Операция RAISE_ALERT */
   int  EMIRi_mb_GetMember          (char *contract, char *key,                             /* Запрос GetMember смарт-контракта Members */
                                                   Member *member,
                                               MemberFile *files, int *files_cnt, int files_max,
                                                     char *error) ;
   int  EMIRi_mb_GetMembers         (char *contract, Member **members,                      /* Запрос GetMembers смарт-контракта Members */
                                                        int  *members_cnt, char *error) ; 
   int  EMIRi_mb_GetMemberFile      (char *contract, char *uuid,                            /* Запрос GetDocument смарт-контракта Member */
                                               MemberFile *file, char *error) ;
   int  EMIRi_mb_GetStatus          (char *contract, char  *state_id, char *error) ;        /* Запрос GetStatus смарт-контракта SystemAlert */

/*********************************************************************/
/*								     */
/*	      Обработчик сообщений диалогового окна MEMBERS          */	

 INT_PTR CALLBACK  EMIR_members_dialog(  HWND  hDlg,     UINT  Msg,
 		                       WPARAM  wParam, LPARAM  lParam) 
{
#if defined(UNIX) || defined(_CONSOLE)

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

}


/********************************************************************/
/*                                                                  */
/*            THREAD - Фоновый поток модуля "Участники"             */

  DWORD WINAPI  Members_Thread(LPVOID Pars)

{
#if defined(UNIX) || defined(_CONSOLE)
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

               time_0=0 ;

      if(__mb_request_period<=0)  __mb_request_period= 10 ;
      if(__mb_view_frame    <=0)  __mb_view_frame    =100 ;

              hDlg=hMembers_Dialog ;

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
         if(rows_cnt>__mb_view_frame) {

               for(i=0 ; i<rows_cnt-__mb_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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
          if(time_abs-time_0 > __mb_request_period) {               /* Если пауза не истекла... */

//             EMIRi_mb_synch(hMembers_Dialog, prefix, &DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_mb_actions(hMembers_Dialog, prefix, &DB) ;
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
/*           Обработчик фонового потока модуля "Участники"          */

   void  Members_Process(SQL_link *DB)

{
   static time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;
            char  error[1024] ;
            char  address[1024] ;
             int  status ;
            char *tmp ;

#if defined(UNIX) || defined(_CONSOLE)
#else
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

/*---------------------------------------------------- Инициализация */

      if(__net_locked)  return ;
      if( __db_locked)  return ;

      if(__mb_request_period<=0)  __mb_request_period= 10 ;
      if(__mb_view_frame    <=0)  __mb_view_frame    =100 ;

#if defined(UNIX) || defined(_CONSOLE)
#else
              hDlg=hMembers_Dialog ;
#endif

/*------------------------------------------------------- Общий цикл */

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#if defined(UNIX) || defined(_CONSOLE)
#else

            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
         if(rows_cnt>__mb_view_frame) {

               for(i=0 ; i<rows_cnt-__mb_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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

            sprintf(text, "Members> %s (LVL.1) Pulse start", prefix) ;

#if defined(UNIX) || defined(_CONSOLE)
             EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif
/*- - - - - - - - - - - - - - - Определение версии рабочего процесса */
     if(members_wf_version[0]==0) do {

                         memset(address, 0, sizeof(address)) ;
          status=EMIR_db_syspar(DB, "Members", address, error) ;
       if(status) {
                      snprintf(text, sizeof(text)-1, "%s - ERROR - Members_Process - Get Members address: %s", prefix, error) ;
                    LB_ADD_ROW(IDC_LOG, text) ;
                                 break ;
                  }

          status=EMIR_node_getversion(address, members_wf_version, error) ;
       if(status) {
                      snprintf(text, sizeof(text)-1, "%s - ERROR - Members_Process - Get Members version: %s", prefix, error) ;
                    LB_ADD_ROW(IDC_LOG, text) ;
                                 break ;
                  }

          tmp=strstr(members_wf_version, "Members.") ;
       if(tmp==NULL) {
                      snprintf(text, sizeof(text)-1, "%s - ERROR - Members_Process - Members version is invalid: %s", prefix, members_wf_version) ;
                    LB_ADD_ROW(IDC_LOG, text) ;
                         members_wf_version[0]=0 ;
                                 break ;
                     }

             memmove(members_wf_version, tmp, strlen(tmp)+1) ;

             sprintf(text, "%s - Members workflow version: %s", prefix, members_wf_version) ;
          LB_ADD_ROW(IDC_LOG, text) ;

                                     } while(0) ;

     if(members_wf_version[0]==0) {

          snprintf(text, sizeof(text)-1, "%s - ERROR - Members workflow version not defined - MEMBERS section is disabled", prefix) ;
        LB_ADD_ROW(IDC_LOG, text) ;

          EMIR_log("ATTENTION! Networking is LOCKED by MEMBERS") ;

                                     __net_locked=1 ;
                                          break ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_mb_actions(hMembers_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __mb_request_period) {               /* Если пауза не истекла... */

               EMIRi_mb_synch(hMembers_Dialog, prefix, DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

            sprintf(text, "Members> %s (LVL.1) Pulse done", prefix) ;

#if defined(UNIX) || defined(_CONSOLE)
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
/*                    модуля "Участники"                            */

   void  EMIRi_mb_synch(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
 static   Member *members ;
             int  members_cnt ;
            char  alert_flag[128] ;
 static     char  members_addr[128] ;
 static     char  box_addr[128] ;
             int  status ;
            char  text[1024] ;
            char  error[1024] ;
             int  i ;

/*--------------------------------------------------- Захват курсора */

        Cursor=db->LockCursor("EMIRi_mb_synch") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                          __db_errors_cnt++ ;
                                return ;
                      }
/*------------------------------------------------------- Подготовка */

   if(members_addr[0]==0) {
/*- - - - - - - - - - - - - - - Запрос адреса СК "Участники системы" */
                       memset(members_addr, 0, sizeof(members_addr)) ;
        status=EMIR_db_syspar(db, "Members", members_addr, error) ;
     if(status) {
                     snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get members address: %s", prefix, error) ;
                   LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

           sprintf(text, "%s - (LVL.1) Members address: %s", prefix, members_addr) ;
        LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - Запрос адреса СК "Почтовый ящик" */
                       memset(box_addr, 0, sizeof(box_addr)) ;
        status=EMIR_db_syspar(db, "MemberBox", box_addr, error) ;
     if(status) {
                     snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get box address: %s", prefix, error) ;
                   LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                }

           sprintf(text, "%s - (LVL.1) Box address: %s", prefix, box_addr) ;
        LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                          }
/*-------------------------------------- Мониторинг флага обновления */

                      sprintf(text, "select \"Flag\" "
                                    "from   %s "
                                    "where  \"Module\"='MEMBERS'", __db_table_system_alert) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get 'SystemAlert.Flag': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {

       if(status==_SQL_NO_DATA)  snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get 'SystemAlert.Flag': No record", prefix) ;
       else                      snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get 'SystemAlert.Flag': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(alert_flag, 0, sizeof(alert_flag)) ;
           strncpy(alert_flag, (char *)Cursor->columns[0].value, sizeof(alert_flag)-1) ;

            sprintf(text, "%s (LVL.1) Members> Alert Flag: %s", prefix, alert_flag) ;
         LB_ADD_ROW(IDC_LOG, text) ;

                 db->SelectClose(Cursor) ;

     if(!stricmp(alert_flag, "1"   ) ||
        !stricmp(alert_flag, "Y"   ) ||
        !stricmp(alert_flag, "TRUE")    ) {
                                             alert_flag[0]='Y' ;
                                         }
/*--------------------------- Обновление таблицы "Участники системы" */

   if(alert_flag[0]=='Y') {
/*- - - - - - - - - - - - - - - -  Получение списка участников из СК */
     if(members!=NULL)  free(members) ;
                             members=NULL ;

        status=EMIRi_mb_GetMembers(members_addr,                    /* Запрос GetMembers смарт-контракта Members */
                                       &members, &members_cnt, error) ; 
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get members list : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

       for(i=0 ; i<members_cnt ; i++) {
                         sprintf(text, "%s - Member : %32.32s - %s", prefix, members[i].key, members[i].version) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                      }
/*- - - - - - - - - - - - - Сравнение с локальным списком участников */
                      sprintf(text, "select \"Key\", \"Version\" "
                                    "from   %s "
                                    "where  \"DataUpdate\" in ('A','U')",
                                       __db_table_members) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get local members list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

        for(i=0 ; i<members_cnt ; i++)  members[i].flag='A' ;

   do { 

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Get local members list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

        for(i=0 ; i<members_cnt ; i++)
          if(!stricmp(members[i].key, 
                       (char *)Cursor->columns[0].value)) {
                                                             members[i].flag='U' ;
            if(!stricmp(members[i].version,
                         (char *)Cursor->columns[1].value))  members[i].flag= 0 ;
                                                                break ;       
                                                          }

          if(i>=members_cnt) {
                members=(Member *)realloc(members, (members_cnt+2)*sizeof(*members)) ;
                strcpy(members[members_cnt].key, (char *)Cursor->columns[0].value) ;
                       members[members_cnt].flag='D' ;
                               members_cnt++ ;
                             }

      } while(1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Удаление участников */
    for(i=0 ; i<members_cnt ; i++)
      if(members[i].flag=='D' ) {

                      sprintf(text, "%s - Member : %32.32s - delete", prefix, members[i].key) ;
                   LB_ADD_ROW(IDC_LOG, text) ;

                      sprintf(text, "update %s set \"DataUpdate\"='D' "
                                    "where  \"Key\"='%s'",
                                      __db_table_members, members[i].key) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Delete member: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                                      db->Commit() ;
                                }
/*- - - - - - - - - - - -  Операции добавления/обновления участников */
    for(i=0 ; i<members_cnt ; i++)
      if(members[i].flag=='A' ||
         members[i].flag=='U'   ) {

                      sprintf(text, "insert into %s (\"Action\", \"Object\", \"ObjectType\", \"Data\", \"Status\") "
                                    "values( 'GetMember', '%s', 'Members', '%s', 'NEW')",
                                      __db_table_members_actions, members_addr, members[i].key) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Queue add/update member: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                          }
/*------------------------------------------- Сброс флага оповещения */

   if(alert_flag[0]=='Y') {

                      sprintf(text, "update %s set \"Flag\"=0 where \"Module\"='MEMBERS'", __db_table_system_alert) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_synch - Clear alerting : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                            db->Rollback() ;
                          __db_errors_cnt++ ;
                                   return ;
                }

                          }
/*------------------------------------------------- Коммит изменений */

                     db->Commit() ;

           sprintf(text, "%s - Members Uploaded", prefix) ;
        LB_ADD_ROW(IDC_LOG, text) ;

/*--------------------------------------------- Освобождение курсора */

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/
}


/********************************************************************/
/*                                                                  */
/*    Обработка очереди операций модуля "Участники системы"         */

   void  EMIRi_mb_actions(HWND hDlg, char *prefix, SQL_link *db)

{
      SQL_cursor *Cursor ;
             int  status ;
            char  error_id[128] ;
            char  error[1024] ;
            char  text[1024] ;
             int  active_cnt ;
             int  i ;
             int  j ;

  static  time_t  last_purge ;
          time_t  block_time ;
       struct tm *time_ddmmyy ;
            char  ts[128] ;

#define   _PURGE_DELAY  3600                     // Периодичность очистки
#define   _PURGE_DEEP   3600*24*__purge_deep     // Глубина очистки

/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_MB_ACTIONS_MAX ; i++)
     if(__mb_actions[i]!=NULL) {
                                   free(__mb_actions[i]) ;
                                        __mb_actions[i]=NULL ;
                               }
/*------------------------------------------ Анализ очереди операций */

        Cursor=db->LockCursor("EMIRi_mb_actions") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_actions - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
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
                   snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_actions - Operations purge : %s", prefix, db->error_text) ;
                 LB_ADD_ROW(IDC_LOG, text) ;
                       __db_errors_cnt++ ;
                 }

                        db->Commit() ;

                    last_purge=time(NULL) ;

                  EMIR_log("Purge done") ;

                                           }
/*- - - - - - - - - - - - - - - - - - - -  Получение списка операций */
                      sprintf(text, "select \"Id\", \"Action\", \"Object\", \"ObjectType\", \"Executor\", \"Data\", \"Status\", \"Reply\", \"MasterId\", \"Error\" "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','HOLD') "
                                    "order by \"Id\"",
                                       __db_table_members_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_action - Get actions list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

   for(i=0 ; i<_MB_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_action - Get actions list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                   __mb_actions[i]=(struct Mb_action *)calloc(1, sizeof(*__mb_actions[i])) ;
           strncpy(__mb_actions[i]->id,          (char *)Cursor->columns[0].value, sizeof(__mb_actions[i]->id         )-1) ;
           strncpy(__mb_actions[i]->action,      (char *)Cursor->columns[1].value, sizeof(__mb_actions[i]->action     )-1) ;
           strncpy(__mb_actions[i]->object,      (char *)Cursor->columns[2].value, sizeof(__mb_actions[i]->object     )-1) ;
           strncpy(__mb_actions[i]->object_type, (char *)Cursor->columns[3].value, sizeof(__mb_actions[i]->object_type)-1) ;
           strncpy(__mb_actions[i]->executor,    (char *)Cursor->columns[4].value, sizeof(__mb_actions[i]->executor   )-1) ;
           strncpy(__mb_actions[i]->data,        (char *)Cursor->columns[5].value, sizeof(__mb_actions[i]->data       )-1) ;
           strncpy(__mb_actions[i]->status,      (char *)Cursor->columns[6].value, sizeof(__mb_actions[i]->status     )-1) ;
           strncpy(__mb_actions[i]->reply,       (char *)Cursor->columns[7].value, sizeof(__mb_actions[i]->reply      )-1) ;
           strncpy(__mb_actions[i]->master_id,   (char *)Cursor->columns[8].value, sizeof(__mb_actions[i]->master_id  )-1) ;
           strncpy(__mb_actions[i]->error,       (char *)Cursor->columns[9].value, sizeof(__mb_actions[i]->error      )-1) ;

                                      }

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - -  Получение "уровня" ошибок */
                      sprintf(text, "select min(\"Id\") "
                                    "from   %s "
                                    "where  \"Error\"<>'' and \"Error\" is not null",
                                       __db_table_members_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_mb_action - Get errors level : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

            memset(error_id, 0, sizeof(error_id)) ;
           strncpy(error_id, (char *)Cursor->columns[0].value, sizeof(error_id)-1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------ Проходы по списку */

    do {
               active_cnt=0 ;

/*------------------------------------------------- Перебор операций */

        for(i=0 ; i<_MB_ACTIONS_MAX ; i++) {

            if(__mb_actions[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Операция ADD_MEMBER */
            if(!stricmp(__mb_actions[i]->action, "ADDMEMBER")) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение */  
                  if(!stricmp(__mb_actions[j]->action, "ADDMEMBER") &&
                      stricmp(__mb_actions[j]->status, "DONE"     )   )  break ;

                  if(j<i)  continue ;

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : ADD_MEMBER : %s - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : ADD_MEMBER : %s/ERROR - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_AddMember(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_AddMember : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                               }
/*- - - - - - - - - - - - - - - - - - - - - - Операция UPDATE_MEMBER */
            if(!stricmp(__mb_actions[i]->action, "UPDATEMEMBER")) {

                for(j=0 ; j<i ; j++)                                /* Исключаем параллельное выполнение для одной и той же записи */  
                  if(!stricmp(__mb_actions[j]->action, "ADDMEMBER") &&
                     !stricmp(__mb_actions[j]->data,
                              __mb_actions[i]->data               ) &&
                      stricmp(__mb_actions[j]->status, "DONE"     )   )  break ;

                  if(j<i)  continue ;

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : UPDATE_MEMBER : %s - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : UPDATE_MEMBER : %s/ERROR - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_AddMember(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_AddMember : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - - - - Операция PUBLISH_MEMBERS */
            if(!stricmp(__mb_actions[i]->action, "PUBLISHMEMBERS")) {

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : PUBLISH_MEMBERS : %s - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : PUBLISH_MEMBERS : %s/ERROR - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_PublMembers(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_PublMembers : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Операция GET_MEMBER */
            if(!stricmp(__mb_actions[i]->action, "GETMEMBER")) {

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_MEMBER : %s - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_MEMBER : %s/ERROR - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_GetMember(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_GetMember : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                               }
/*- - - - - - - - - - - - - - - - - - - - - Операция GET_MEMBER_FILE */
            if(!stricmp(__mb_actions[i]->action, "GETMEMBERFILE")) {

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_MEMBER_FILE : %s - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_MEMBER_FILE : %s/ERROR - %s", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_GetMemberFile(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_GetMemberFile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                   }
/*- - - - - - - - - - - - - - - - - - - - - - - Операция RAISE_ALERT */
            else
            if(!stricmp(__mb_actions[i]->action, "RAISEALERT")) {

                for(j=0 ; j<i ; j++)                                /* В очереди выше не должно быть незавершённых операций */  
                  if( stricmp(__mb_actions[j]->action, "RAISEALERT") &&
                      stricmp(__mb_actions[j]->status, "DONE"      )   )  break ;

                  if(j<i)  continue ;

                if(error_id[0]!=0)
                  if(stricmp(__mb_actions[i]->id,                   /* Если операция ПОСЛЕ первой ошибки... */
                                        error_id )>0)  continue ;

                if(__mb_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : RAISEALERT : %s - %s ", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->object) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : RAISEALERT : %s/ERROR - %s ", prefix, __mb_actions[i]->id, __mb_actions[i]->status, __mb_actions[i]->object) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__mb_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_mb_action_RaiseAlert(__mb_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_mb_action_RaiseAlert : %s", prefix, error) ;
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
/*                                                                   */
/*                           Операция ADDMEMBER                      */

   int  EMIR_mb_action_AddMember(Mb_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  account[128] ;
        char *password ;
        char  data[8192] ;
        char  data_path[FILENAME_MAX] ;
        FILE *file ;
        char  result[128] ;
        char  key[128] ;
        char  key_hex[128] ;
        char  name[128] ;
        char  name_hex[128] ;
        char  lock[128] ;
        char  lock_hex[128] ;
        char  role[128] ;
        char  role_hex[128] ;
        char  bc_id[128] ;
        char  sign[128] ;
        char  sign_hex[128] ;
        char  cert_name[128] ;
        char  cert_type[128] ;
        char  cert_path[FILENAME_MAX] ;
        char  cert_type_hex[128] ;
        char  cert_link[256] ;
        char  cert_link_hex[512] ;
        char  addr[128] ;
        char  box[128] ;
        char  data_link[256] ;
        char  data_link_hex[512] ;
      Member  member ;
        char  gas[128] ;
        char  txn[128] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[16000] ;
        char *end ;

 static char  members_addr[128] ;
 static char *code ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

    if(code==NULL)  code=(char *)calloc(1, _CODE_SIZE) ;

       status=EMIR_db_nodepars(db, error) ;
    if(status)  return(-1) ;

    if(members_addr[0]==0) {
         status=EMIR_db_syspar(db, "Members", members_addr, error) ;
      if(status)  return(-1) ;
                           }

    if(action->object[0]==0)  strcpy(action->object, members_addr) ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;

      if(action->executor[0]!=0)
              strncpy(account,  action->executor, sizeof(account)-1) ;
      else    strncpy(account, __member_executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;                       /* Выделение пароля */
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*------------------------ Передача файла данных и файла сертификата */

   if(!stricmp(action->status, "NEW")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select \"Data\",\"Account\",\"Box\",\"CertFile\" "
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(data,       0, sizeof(data)) ;
           strncpy(data,      (char *)Cursor->columns[0].value, sizeof(data)-1) ;
            memset(addr,       0, sizeof(addr)) ;
           strncpy(addr,      (char *)Cursor->columns[1].value, sizeof(addr)-1) ;
            memset(box,        0, sizeof(box)) ;
           strncpy(box,       (char *)Cursor->columns[2].value, sizeof(box )-1) ;
            memset(cert_name,  0, sizeof(cert_name)) ;
           strncpy(cert_name, (char *)Cursor->columns[3].value, sizeof(cert_name)-1) ;

                 db->SelectClose(Cursor) ;

        if(strlen(addr)!=40 ||
           strlen(box )!=40   ) {
                                  strcpy(error, "Account or Box length must by exactly 40") ;
                                          status=-1 ;
                                             break ;
                                }
/*- - - - - - - - - - - - - - - - - - - -  Формирование файла данных */
                             data_path[0]=0 ;

     if(strlen(data)!=0) {

#ifdef  UNIX
//             snprintf(data_path, sizeof(data_path)-1, "%s/AddMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s/AddMember.%s", __work_folder, action->id) ;
#else
//             snprintf(data_path, sizeof(data_path)-1, "%s\\AddMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s\\AddMember.%s", __work_folder, action->id) ;
#endif
            file=fopen(data_path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, data_path) ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
                         }
/*- - - - - - - - - - - - - - - - - - -  Обработка файла сертификата */
                               cert_path[0]=0 ;

     if(strlen(cert_name)!=0) {

       if(__cert_storage[0]==0) {
                     strcpy(error, "Certs storage must be specified") ;
                                     status=-1 ;
                                        break ;
                                }

#ifdef  UNIX
               snprintf(cert_path, sizeof(cert_path)-1, "%s/%s",  __cert_storage, cert_name) ;
#else
               snprintf(cert_path, sizeof(cert_path)-1, "%s\\%s", __cert_storage, cert_name) ;
#endif

                              }
/*- - - - - - - - - - - - - - - - Занесение операций в FILES_ACTIONS */
     if(data_path[0]!=0) {

                        sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Status\", \"MasterId\") "
                                      "values( 'TransferFile', '%s', 'NEW', 'Member:%s')",
                                        __db_table_files_actions, data_path, action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                        break ;
                  }
                         }

     if(cert_path[0]!=0) {

                        sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Status\", \"MasterId\") "
                                      "values( 'TransferFile', '%s', 'NEW', 'Member:%s')",
                                        __db_table_files_actions, cert_path, action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                        break ;
                  }
                         }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s;%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, data_path, cert_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "FILE")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - Восстановление файловых операций */
            strcpy(data_path, action->reply) ;
        end=strchr(data_path, ';') ;
     if(end==NULL) {
                       sprintf(error, "Invalid data structure in operation") ;
                            break ;
                   }

                             *end=0 ;         
            strcpy(cert_path, end+1) ;

          strcpy(data_link,   "") ;
          strcpy(cert_link,   "") ;
          strcpy(result, "DONE") ;
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
     if(data_path[0]!=0) {

                      sprintf(text, "select \"Status\",\"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, data_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result,     0, sizeof(result)) ;
           strncpy(result,    (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(data_link,  0, sizeof(data_link)) ;
           strncpy(data_link, (char *)Cursor->columns[1].value, sizeof(data_link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                         }

     if(cert_path[0]!=0) {

                      sprintf(text, "select \"Status\",\"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, cert_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get cert: No record") ;
       else                      sprintf(error, "Get cert: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result,     0, sizeof(result)) ;
           strncpy(result,    (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(cert_link,  0, sizeof(cert_link)) ;
           strncpy(cert_link, (char *)Cursor->columns[1].value, sizeof(cert_link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
            sprintf(text, "select \"Name\",\"Lock\",\"Role\",\"Sign\",\"Account\",\"Box\",\"CertFile\""
                          "from   %s "
                          "where  \"Key\"='%s'", __db_table_members, action->data) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                           break ;
                }

            memset(key,        0, sizeof(key)) ;
           strncpy(key,        action->data, sizeof(key)-1) ;
            memset(name,       0, sizeof(name)) ;
           strncpy(name,      (char *)Cursor->columns[0].value, sizeof(name)-1) ;
            memset(lock,       0, sizeof(lock)) ;
           strncpy(lock,      (char *)Cursor->columns[1].value, sizeof(lock)-1) ;
            memset(role,       0, sizeof(role)) ;
           strncpy(role,      (char *)Cursor->columns[2].value, sizeof(role)-1) ;
            memset(sign,       0, sizeof(sign)) ;
           strncpy(sign,      (char *)Cursor->columns[3].value, sizeof(sign)-1) ;
            memset(addr,       0, sizeof(addr)) ;
           strncpy(addr,      (char *)Cursor->columns[4].value, sizeof(addr)-1) ;
            memset(box,        0, sizeof(box)) ;
           strncpy(box,       (char *)Cursor->columns[5].value, sizeof(box)-1) ;
            memset(cert_name,  0, sizeof(cert_name)) ;
           strncpy(cert_name, (char *)Cursor->columns[6].value, sizeof(cert_name)-1) ;

            memset(bc_id,      0, sizeof(bc_id)) ;
       if(Cursor->columns_cnt>7) 
           strncpy(bc_id,     (char *)Cursor->columns[7].value, sizeof(bc_id)-1) ;

                 db->SelectClose(Cursor) ;

            end=strrchr(cert_name, '.') ;
         if(end==NULL)  strcpy(cert_type, "") ;
         else           strcpy(cert_type, end+1) ;
/*- - - - - - - - - - - - - - - Формирование транзакции RecordMember */
              EMIR_txt2hex64(key,       key_hex,       strlen(key )) ;
              EMIR_txt2hex64(name,      name_hex,      strlen(name)) ;
              EMIR_txt2hex64(lock,      lock_hex,      strlen(lock)) ;
              EMIR_txt2hex64(role,      role_hex,      strlen(role)) ;
              EMIR_txt2hex64(sign,      sign_hex,      strlen(sign)) ;
              EMIR_txt2hex64(cert_type, cert_type_hex, strlen(cert_type)) ;
              EMIR_txt2hex64(cert_link, cert_link_hex, strlen(cert_link)) ;
              EMIR_txt2hex64(data_link, data_link_hex, strlen(data_link)) ;

             sprintf(code, "6e418270"                               /* Формируем блок данных транзакции */
                           "%s"
                           "%s"
                           "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "000000000000000000000000%s"
                           "%s"
                           "%s"
                           "0000000000000000000000000000000000000000000000000000000000000140"
                           "%064x"
                           "%064x"
                           "%s"   
                           "%064x"
                           "%s", 
                            key_hex, name_hex, lock_hex, role_hex, addr, box,
                            sign_hex, cert_type_hex, 
                             0x160+(int)strlen(cert_link_hex)/2, 
                            (int)strlen(cert_link), cert_link_hex, 
                            (int)strlen(data_link), data_link_hex) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Отправка RecordMember */
      do {
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

           if(status)  break ;
/*- - - - - - - - - - - - - - - - - Удаление записи из FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"MasterId\"='Member:%s'",
                                       __db_table_files_actions, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                       return(-1) ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) 
   if(!stricmp(result, "DONE")) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        return(-1) ;
                }

                                    db->Commit() ;
                                }

                                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                        }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                   memset(txn, 0, sizeof(txn)) ;
                  strncpy(txn, action->reply, sizeof(txn)-1) ;

              status =EMIR_node_checktxn(txn, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select \"Name\",\"Lock\",\"Role\",\"Sign\",\"Account\",\"Box\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                           break ;
                }

            memset(key,  0, sizeof(key)) ;
           strncpy(key,  action->data, sizeof(key)-1) ;
            memset(name, 0, sizeof(name)) ;
           strncpy(name, (char *)Cursor->columns[0].value, sizeof(name)-1) ;
            memset(lock, 0, sizeof(lock)) ;
           strncpy(lock, (char *)Cursor->columns[1].value, sizeof(lock)-1) ;
            memset(role, 0, sizeof(role)) ;
           strncpy(role, (char *)Cursor->columns[2].value, sizeof(role)-1) ;
            memset(sign, 0, sizeof(sign)) ;
           strncpy(sign, (char *)Cursor->columns[3].value, sizeof(sign)-1) ;
            memset(addr, 0, sizeof(addr)) ;
           strncpy(addr, (char *)Cursor->columns[4].value, sizeof(addr)-1) ;
            memset(box,  0, sizeof(box)) ;
           strncpy(box,  (char *)Cursor->columns[5].value, sizeof(addr)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  Сверка данных */
                                 memset(&member, 0, sizeof(member)) ;
              status=EMIRi_mb_GetMember(action->object, action->data, &member, NULL, NULL, 0, error) ;
           if(status)  break ;

           if( strcmp(member.name,    name) ||
               strcmp(member.lock,    lock) ||
               strcmp(member.role,    role) ||
              stricmp(member.account, addr) ||
              stricmp(member.box,     box ) ||
               strcmp(member.sign,    sign)   ) {
                                                  strcpy(error, "Values check fail") ;
                                                     status=-1 ;
                                                        break ;
                                                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
         } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
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
/*                                                                   */
/*                       Операция PUBLISH_MEMBERS                    */

   int  EMIR_mb_action_PublMembers(Mb_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  account[128] ;
        char *password ;
        char  tmpl[128] ;
        char  data[4096] ;
        char  data_path[FILENAME_MAX] ;
        FILE *file ;
        char  result[128] ;
        char  key[128] ;
        char  key_hex[128] ;
        char  name[128] ;
        char  name_hex[128] ;
        char  lock[128] ;
        char  lock_hex[128] ;
        char  role[128] ;
        char  role_hex[128] ;
        char  bc_id[128] ;
        char  sign[128] ;
        char  sign_hex[128] ;
        char  cert_name[128] ;
        char  cert_type[128] ;
        char  cert_path[FILENAME_MAX] ;
        char  cert_type_hex[128] ;
        char  cert_link[256] ;
        char  cert_link_hex[512] ;
        char  addr[128] ;
        char  box[128] ;
        char  data_link[256] ;
        char  data_link_hex[512] ;
      Member  member ;
        char  gas[128] ;
        char  txn[128] ;
        char  txn1[256] ;
        char  txn2[256] ;
        char  block[128] ;
        char  contract[128] ;
        char  reply[128] ;
        char  text[16000] ;
        char *end ;

 static char  members_addr[128] ;
 static char *code ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

    if(code==NULL)  code=(char *)calloc(1, _CODE_SIZE) ;

       status=EMIR_db_nodepars(db, error) ;
    if(status)  return(-1) ;

    if(members_addr[0]==0) {
         status=EMIR_db_syspar(db, "Members", members_addr, error) ;
      if(status)  return(-1) ;
                           }

    if(action->object[0]==0)  strcpy(action->object, members_addr) ;

/*---------------------------------------------- Разбор счета/пароля */

               memset(account, 0, sizeof(account)) ;

      if(action->executor[0]!=0)
              strncpy(account,  action->executor, sizeof(account)-1) ;
      else    strncpy(account, __member_executor, sizeof(account)-1) ;

              password=strchr(account, ':') ;                       /* Выделение пароля */
           if(password==NULL) {
                                   password="" ;
                              }
           else               {
                                  *password=0 ;  
                                   password++ ;  
                              }
/*--------------------------------- Создание смарт-контракта клиента */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - -  Обработка старого рабочего процесса */
     if(stricmp(members_wf_version, "Members.2019-02-19")<0) {      /* Для версий ниже "Members.2019-02-19" */

                    sprintf(text, "update %s "
                                  "set    \"Status\"='CARD', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        return(-1) ;
                  }

                    db->Commit() ;
                    db->UnlockCursor(Cursor) ;
                          return(0) ;
                                                             }
/*- - - - - - - - - - - - - - - - Извлечение шаблона смарт-контракта */
                       memset(tmpl, 0, sizeof(tmpl)) ;
        status=EMIR_db_syspar(db, "Member Template", tmpl, error) ;
     if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select \"Key\",\"BlockChainId\",\"Account\" "
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data for personal SC: No record") ;
       else                      sprintf(error, "Get data for personal SC: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;

                 db->SelectClose(Cursor) ;
                   break ;
                }

            memset(key,   0, sizeof(key)) ;
           strncpy(key,   (char *)Cursor->columns[0].value, sizeof(key)-1) ;
            memset(bc_id, 0, sizeof(bc_id)) ;
           strncpy(bc_id, (char *)Cursor->columns[1].value, sizeof(bc_id)-1) ;
            memset(addr,  0, sizeof(addr)) ;
           strncpy(addr,  (char *)Cursor->columns[2].value, sizeof(addr)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
    if(bc_id[0]==0) {                                               /* Если смарт-контракт клиента еще не создан... */

      do {

           if(strlen(addr)!=40) {
                 sprintf(error, "Member's account length must be 40 characters exectly") ;
                                    break ;
                                }

                EMIR_txt2hex64(key, key_hex, strlen(key )) ;        /* Преобразование Key в HEX */

              status=EMIR_node_getcode(tmpl,                        /* Извлечение кода контракта */
                                       code, _CODE_SIZE-1, error) ;
           if(status)  break ;

                        strcat(code, key_hex) ;                     /* Добавляем параметры конструктора смарт-контракта */
                        strcat(code, "000000000000000000000000") ;
                        strcat(code, addr) ;

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

           if(status)  break ;

                    }
    else            {
                               *txn=0 ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='CARD', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------ Передача файла данных и файла сертификата */

   if(!stricmp(action->status, "CARD")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
     if(action->reply[0]!=0) {

              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;

           if(!memicmp(contract, "0x", 2))  memmove(contract, contract+2, strlen(contract+2)+1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - Сохранение номера СК */
                    sprintf(text, "update %s "
                                  "set    \"BlockChainId\"='%s' "
                                  "where  \"Key\"='%s'",
                                       __db_table_members, contract, action->data) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }
                             } 
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select \"Data\",\"Account\",\"Box\",\"CertFile\" "
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(data,       0, sizeof(data)) ;
           strncpy(data,      (char *)Cursor->columns[0].value, sizeof(data)-1) ;
            memset(addr,       0, sizeof(addr)) ;
           strncpy(addr,      (char *)Cursor->columns[1].value, sizeof(addr)-1) ;
            memset(box,        0, sizeof(box)) ;
           strncpy(box,       (char *)Cursor->columns[2].value, sizeof(box )-1) ;
            memset(cert_name,  0, sizeof(cert_name)) ;
           strncpy(cert_name, (char *)Cursor->columns[3].value, sizeof(cert_name)-1) ;

                 db->SelectClose(Cursor) ;

        if(strlen(addr)!=40 ||
           strlen(box )!=40   ) {
                                  strcpy(error, "Account or Box length must by exactly 40") ;
                                          status=-1 ;
                                             break ;
                                }
/*- - - - - - - - - - - - - - - - - - - -  Формирование файла данных */
                             data_path[0]=0 ;

     if(strlen(data)!=0) {

#ifdef  UNIX
//             snprintf(data_path, sizeof(data_path)-1, "%s/AddMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s/AddMember.%s", __work_folder, action->id) ;
#else
//             snprintf(data_path, sizeof(data_path)-1, "%s\\AddMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s\\AddMember.%s", __work_folder, action->id) ;
#endif
            file=fopen(data_path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, data_path) ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
                         }
/*- - - - - - - - - - - - - - - - - - -  Обработка файла сертификата */
                               cert_path[0]=0 ;

     if(strlen(cert_name)!=0) {

       if(__cert_storage[0]==0) {
                     strcpy(error, "Certs storage must be specified") ;
                                     status=-1 ;
                                        break ;
                                }

#ifdef  UNIX
               snprintf(cert_path, sizeof(cert_path)-1, "%s/%s",  __cert_storage, cert_name) ;
#else
               snprintf(cert_path, sizeof(cert_path)-1, "%s\\%s", __cert_storage, cert_name) ;
#endif

                              }
/*- - - - - - - - - - - - - - - - Занесение операций в FILES_ACTIONS */
     if(data_path[0]!=0) {

                        sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Status\", \"MasterId\") "
                                      "values( 'TransferFile', '%s', 'NEW', 'Member:%s')",
                                        __db_table_files_actions, data_path, action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                        break ;
                  }
                         }

     if(cert_path[0]!=0) {

                        sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Status\", \"MasterId\") "
                                      "values( 'TransferFile', '%s', 'NEW', 'Member:%s')",
                                        __db_table_files_actions, cert_path, action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                        break ;
                  }
                         }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s;%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, data_path, cert_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                       db->UnlockCursor(Cursor) ;
                       db->Rollback() ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "FILE")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - Восстановление файловых операций */
            strcpy(data_path, action->reply) ;
        end=strchr(data_path, ';') ;
     if(end==NULL) {
                       sprintf(error, "Invalid data structure in operation") ;
                            break ;
                   }

                             *end=0 ;         
            strcpy(cert_path, end+1) ;

          strcpy(data_link,   "") ;
          strcpy(cert_link,   "") ;
          strcpy(result, "DONE") ;
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
     if(data_path[0]!=0) {

                      sprintf(text, "select \"Status\",\"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, data_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result,     0, sizeof(result)) ;
           strncpy(result,    (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(data_link,  0, sizeof(data_link)) ;
           strncpy(data_link, (char *)Cursor->columns[1].value, sizeof(data_link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                         }

     if(cert_path[0]!=0) {

                      sprintf(text, "select \"Status\",\"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, cert_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get cert: No record") ;
       else                      sprintf(error, "Get cert: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result,     0, sizeof(result)) ;
           strncpy(result,    (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(cert_link,  0, sizeof(cert_link)) ;
           strncpy(cert_link, (char *)Cursor->columns[1].value, sizeof(cert_link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
     if(stricmp(members_wf_version, "Members.2019-02-19")>=0) 
                      sprintf(text, "select \"Name\",\"Lock\",\"Role\",\"Sign\",\"Account\",\"Box\",\"CertFile\",\"BlockChainId\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
     else             sprintf(text, "select \"Name\",\"Lock\",\"Role\",\"Sign\",\"Account\",\"Box\",\"CertFile\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;


        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                           break ;
                }

            memset(key,        0, sizeof(key)) ;
           strncpy(key,        action->data, sizeof(key)-1) ;
            memset(name,       0, sizeof(name)) ;
           strncpy(name,      (char *)Cursor->columns[0].value, sizeof(name)-1) ;
            memset(lock,       0, sizeof(lock)) ;
           strncpy(lock,      (char *)Cursor->columns[1].value, sizeof(lock)-1) ;
            memset(role,       0, sizeof(role)) ;
           strncpy(role,      (char *)Cursor->columns[2].value, sizeof(role)-1) ;
            memset(sign,       0, sizeof(sign)) ;
           strncpy(sign,      (char *)Cursor->columns[3].value, sizeof(sign)-1) ;
            memset(addr,       0, sizeof(addr)) ;
           strncpy(addr,      (char *)Cursor->columns[4].value, sizeof(addr)-1) ;
            memset(box,        0, sizeof(box)) ;
           strncpy(box,       (char *)Cursor->columns[5].value, sizeof(box)-1) ;
            memset(cert_name,  0, sizeof(cert_name)) ;
           strncpy(cert_name, (char *)Cursor->columns[6].value, sizeof(cert_name)-1) ;

            memset(bc_id,      0, sizeof(bc_id)) ;
       if(Cursor->columns_cnt>7) 
           strncpy(bc_id,     (char *)Cursor->columns[7].value, sizeof(bc_id)-1) ;

                 db->SelectClose(Cursor) ;

            end=strrchr(cert_name, '.') ;
         if(end==NULL)  strcpy(cert_type, "") ;
         else           strcpy(cert_type, end+1) ;
/*- - - - - - - - - - - - - - - Формирование транзакции RecordMember */
              EMIR_txt2hex64(key,       key_hex,       strlen(key )) ;
              EMIR_txt2hex64(name,      name_hex,      strlen(name)) ;
              EMIR_txt2hex64(lock,      lock_hex,      strlen(lock)) ;
              EMIR_txt2hex64(role,      role_hex,      strlen(role)) ;
              EMIR_txt2hex64(sign,      sign_hex,      strlen(sign)) ;
              EMIR_txt2hex64(cert_type, cert_type_hex, strlen(cert_type)) ;
              EMIR_txt2hex64(cert_link, cert_link_hex, strlen(cert_link)) ;
              EMIR_txt2hex64(data_link, data_link_hex, strlen(data_link)) ;

             sprintf(code, "6e418270"                               /* Формируем блок данных транзакции */
                           "%s"
                           "%s"
                           "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "000000000000000000000000%s"
                           "%s"
                           "%s"
                           "0000000000000000000000000000000000000000000000000000000000000140"
                           "%064x"
                           "%064x"
                           "%s"   
                           "%064x"
                           "%s", 
                            key_hex, name_hex, lock_hex, role_hex, addr, box,
                            sign_hex, cert_type_hex, 
                             0x160+(int)strlen(cert_link_hex)/2, 
                            (int)strlen(cert_link), cert_link_hex, 
                            (int)strlen(data_link), data_link_hex) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Отправка RecordMember */
      do {
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        action->object, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                              action->object, code, gas, txn1, error) ;
           if(status)  break ;

         } while(0) ;

           if(status)  break ;
/*- - - - - - - - - - - -  Формирование транзакции SetMemberPersonal */
             sprintf(code, "9d981993"                               /* Формируем блок данных транзакции */
                           "%s"
                           "000000000000000000000000%s", 
                            key_hex, bc_id) ;
/*- - - - - - - - - - - - - - - - - - - - Отправка SetMemberPersonal */
                           txn2[0]=0 ;

     if(bc_id[0]!=0) {

      do {
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        action->object, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                              action->object, code, gas, txn2, error) ;
           if(status)  break ;

         } while(0) ;

           if(status)  break ;

                     }
/*- - - - - - - - - - - - - - - - - Удаление записи из FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"LocalPath\" in ('%s','%s') and \"MasterId\"='Member:%s'",
                                       __db_table_files_actions, data_path, cert_path, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                       return(-1) ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) 
   if(!stricmp(result, "DONE")) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s,%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, txn1, txn2, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        return(-1) ;
                }

                                    db->Commit() ;
                                }

                                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                        }
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_AddMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                   memset(txn1, 0, sizeof(txn1)) ;
                   memset(txn2, 0, sizeof(txn2)) ;

                  strncpy(txn1, action->reply, sizeof(txn1)-1) ;
              end= strchr(txn1, ',') ;
           if(end!=NULL) { 
                            strcpy(txn2, end+1) ;
                                  *end=0 ;
                         }

           if(txn1[0]!=0)  status =EMIR_node_checktxn(txn1, block, contract, error) ;
           if(txn2[0]!=0)  status|=EMIR_node_checktxn(txn2, block, contract, error) ;

           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select \"Name\",\"Lock\",\"Role\",\"Sign\",\"Account\",\"Box\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                        break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data: No record") ;
       else                      sprintf(error, "Get data: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                           break ;
                }

            memset(key,  0, sizeof(key)) ;
           strncpy(key,  action->data, sizeof(key)-1) ;
            memset(name, 0, sizeof(name)) ;
           strncpy(name, (char *)Cursor->columns[0].value, sizeof(name)-1) ;
            memset(lock, 0, sizeof(lock)) ;
           strncpy(lock, (char *)Cursor->columns[1].value, sizeof(lock)-1) ;
            memset(role, 0, sizeof(role)) ;
           strncpy(role, (char *)Cursor->columns[2].value, sizeof(role)-1) ;
            memset(sign, 0, sizeof(sign)) ;
           strncpy(sign, (char *)Cursor->columns[3].value, sizeof(sign)-1) ;
            memset(addr, 0, sizeof(addr)) ;
           strncpy(addr, (char *)Cursor->columns[4].value, sizeof(addr)-1) ;
            memset(box,  0, sizeof(box)) ;
           strncpy(box,  (char *)Cursor->columns[5].value, sizeof(addr)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  Сверка данных */
                                 memset(&member, 0, sizeof(member)) ;
              status=EMIRi_mb_GetMember(action->object, action->data, &member, NULL, NULL, 0, error) ;
           if(status)  break ;

           if( strcmp(member.name,    name) ||
               strcmp(member.lock,    lock) ||
               strcmp(member.role,    role) ||
              stricmp(member.account, addr) ||
              stricmp(member.box,     box ) ||
               strcmp(member.sign,    sign)   ) {
                                                  strcpy(error, "Values check fail") ;
                                                     status=-1 ;
                                                        break ;
                                                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
         } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
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
/*                           Операция GET_MEMBER                     */

   int  EMIR_mb_action_GetMember(Mb_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  data[4096] ;
        char  data_path[FILENAME_MAX] ;
        char  cert_path[FILENAME_MAX] ;
        char  cert_name[FILENAME_MAX] ;
        FILE *file ;
        char  result[128] ;
         int  add_flag ;
      Member  member ;
  MemberFile  files[_MB_FILES_MAX] ;
         int  files_cnt ;
        char  text[16000] ;
        char *end ;
         int  i ;  

 static char  members_addr[128] ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

    if(members_addr[0]==0) {
         status=EMIR_db_syspar(db, "Members", members_addr, error) ;
      if(status)  return(-1) ;
                           }

    if(action->object[0]==0)  strcpy(action->object, members_addr) ;

/*------------------------------------------ Извлечение данных из СК */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_GetMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - -  Получение данных из смарт-контракта */
                                 memset(&member, 0, sizeof(member)) ;
              status=EMIRi_mb_GetMember(action->object, action->data, &member, NULL, NULL, 0, error) ;
           if(status)  break ;
/*- - - - - - - - - - - - - - - - Занесение операций в FILES_ACTIONS */
               strcpy(data_path, "") ;
               strcpy(cert_path, "") ;

     if(member.data_link[0]!=0) {

#ifdef  UNIX
//             snprintf(data_path, sizeof(data_path)-1, "%s/GetMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s/GetMember.%s", __work_folder, action->id) ;
#else
//             snprintf(data_path, sizeof(data_path)-1, "%s\\GetMember_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(data_path, sizeof(data_path)-1, "%s\\GetMember.%s", __work_folder, action->id) ;
#endif


                      sprintf(text, "insert into %s (\"Action\", \"DfsPath\", \"LocalPath\", \"Status\", \"MasterId\") "
                                    "values( 'GetFile', '%s', '%s', 'NEW', 'Member:%s')",
                                      __db_table_files_actions, member.data_link, data_path, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                         break ;
                }
                                }

     if(member.cert_link[0]!=0) {

       if(__cert_storage[0]==0) {
                     strcpy(error, "Certs storage must be specified") ;
                                     status=-1 ;
                                        break ;
                                }

#ifdef  UNIX
               snprintf(cert_path, sizeof(cert_path)-1, "%s/%s.%s", __cert_storage, member.sign, member.cert_type) ;
#else
               snprintf(cert_path, sizeof(cert_path)-1, "%s\\%s.%s", __cert_storage, member.sign, member.cert_type) ;
#endif

                      sprintf(text, "insert into %s (\"Action\", \"DfsPath\", \"LocalPath\", \"Status\", \"MasterId\") "
                                    "values( 'GetFile', '%s', '%s', 'NEW', 'Member:%s')",
                                      __db_table_files_actions, member.cert_link, cert_path, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                         break ;
                }
                                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s;%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, data_path, cert_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Занесение данных в БД  */

   if(!stricmp(action->status, "FILE")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_GetMember") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - Восстановление файловых операций */
            strcpy(data_path, action->reply) ;
        end=strchr(data_path, ';') ;
     if(end==NULL) {
                       sprintf(error, "Invalid data structure in operation") ;
                            break ;
                   }

                             *end=0 ;         
            strcpy(cert_path, end+1) ;

            strcpy(result, "DONE") ;
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
    if(data_path[0]!=0) {

                      sprintf(text, "select \"Status\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, data_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get data file: No record") ;
       else                      sprintf(error, "Get data file: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                        }

    if(cert_path[0]!=0) {

                      sprintf(text, "select \"Status\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Member:%s'",
                                      __db_table_files_actions, cert_path, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get cert file: No record") ;
       else                      sprintf(error, "Get cert file: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - -  Считывание файла данных */
                           memset(data, 0, sizeof(data)) ;

    if(data_path[0]!=0) {

        file=fopen(data_path, "rb") ;
     if(file==NULL) {
                      sprintf(error, "Data file open error %d : %s", errno, data_path) ;
                             break ;
                    } 

             fread(data, 1, sizeof(data)-1, file) ;
            fclose(file) ;

                        }
/*- - - - - - - - - - - - - - - Формирование имени файла сертификата */
    if(cert_path[0]!=0) {

#ifdef  UNIX
                            end=strrchr(cert_path, '/') ;
#else
                            end=strrchr(cert_path, '\\') ;
#endif
                                 strcpy(cert_name, end+1) ;
                        }
    else                {
                                        cert_name[0]=0 ;
                        }
/*- - - - - - - - - - - - - - -  Получение данных из смарт-контракта */
                                          files_cnt=0 ;

                                 memset(&member, 0, sizeof(member)) ;
              status=EMIRi_mb_GetMember(action->object, action->data, &member,
                                           files, &files_cnt, _MB_FILES_MAX, error) ;
           if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - Проверка наличия участника */
                      sprintf(text, "select \"Name\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

             status=db->SelectFetch(Cursor) ;

                                    add_flag=0 ;
          if(status==_SQL_NO_DATA)  add_flag=1 ;
     else if(status) {
                 sprintf(error, "Check existance: %s", db->error_text) ;
                                                       db->error_text[0]=0 ;
                                                     __db_errors_cnt++ ;
                         break ;
                     }

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - -  Запись данных в таблицу */
     if(add_flag)  sprintf(text, "insert into %s (\"Key\",\"Name\",\"Lock\",\"Role\",\"Sign\",\"BlockChainId\",\"Account\",\"Box\",\"Data\",\"CertFile\",\"Version\",\"DataUpdate\") "
                                 "values         ( '%s',   '%s',    '%s',    '%s',    '%s',    '%s',             '%s',       '%s',   '%s',   '%s',        '%s',       'A')",
                                   __db_table_members, member.key, member.name, member.lock, member.role, member.sign,
                                                       member.bc_id, member.account, member.box, data, 
                                                       cert_name, member.version) ;
     else          sprintf(text, "update %s "
                                 "set    \"Name\"='%s',\"Lock\"='%s',\"Role\"='%s',\"Sign\"='%s',"
                                       " \"BlockChainId\"='%s',\"Account\"='%s',\"Box\"='%s',\"Data\"='%s',"
                                       " \"CertFile\"='%s', \"Version\"='%s',\"DataUpdate\"='U' "
                                 "where  \"Key\"='%s'",
                                   __db_table_members, member.name, member.lock, member.role, member.sign,
                                                       member.bc_id, member.account, member.box, data, 
                                                       cert_name, member.version, 
                                                       member.key) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                  EMIR_log(text) ; 
                   sprintf(error, "Add/update member record: %s", db->error_text) ;
                                                                  db->error_text[0]=0 ;
                                                                __db_errors_cnt++ ;
                               break ;
                }
/*- - - - - - - - - - - - - - - - - -  Регистрация файла сертификата */
    if(cert_path[0]!=0) 
     if(__crypto_cert[0]!=0) {                                       /* Если задан скрипт регистрации сертификата... */

              status=EMIR_crypto_cert(cert_path, member.sign, error) ;
           if(status)  break ;

                             }
/*- - - - - - - - - - - - - - - -  Удаление записей из FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"LocalPath\" in ('%s','%s') and \"MasterId\"='Member:%s'",
                                       __db_table_files_actions, data_path, cert_path, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                        break ;
                }
/*- - - - - - - - - - - - - - - Формирование операций запроса файлов */
              status=0 ;

     for(i=0 ; i<files_cnt ; i++) {
                        sprintf(text, "insert into %s (\"Action\",     \"Object\", \"Data\", \"Status\", \"MasterId\")"
                                      "         values('GetMemberFile', '%s',       '%s,%s',  'NEW',      'Member:%s')",
                                        __db_table_members_actions, member.bc_id, files[i].uid, member.key, action->id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                        sprintf(error, "Add GetMemberFile operation: %s", db->error_text) ;
                                 __db_errors_cnt++ ;
                         break ;
                  }
                                  }
       
       if(status)  break ; 
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL)
   if(!stricmp(result, "DONE")) {

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
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
/*                      Операция GET_MEMBER_FILE                     */

   int  EMIR_mb_action_GetMemberFile(Mb_action *action, SQL_link *db, char *error)
{
    SQL_cursor *Cursor ;
           int  status ;
          char  file_uuid[128] ;
          char *member_key ;
          char  member_id[128] ;
          char  hash[128] ;
          char  data_path[FILENAME_MAX] ;
          char  result[128] ;
           int  add_flag ;
    MemberFile  member_file ;
          char  text[8192] ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

       if(__members_storage[0]==0) {
                     strcpy(error, "Members documents storage must be specified") ;
                                      return(-1) ;
                                   }
/*--------------------------------------- Разбор реквизитов операции */

               memset(file_uuid, 0, sizeof(file_uuid)) ;
              strncpy(file_uuid,  action->data, sizeof(file_uuid)-1) ;

              member_key=strchr(file_uuid, ',') ;
           if(member_key==NULL) {
                           sprintf(error, "Operation Data invalid structure") ;
                                               return(-1) ;
                                }
           else                 {
                                  *member_key=0 ;  
                                   member_key++ ;  
                                }
/*------------------------------------------ Извлечение данных из СК */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_GetMemberFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - -  Получение данных из смарт-контракта */
                                     memset(&member_file, 0, sizeof(member_file)) ;
              status=EMIRi_mb_GetMemberFile(action->object, file_uuid, &member_file, error) ;
           if(status)  break ;
/*- - - - - - - - - - - - - - -  Создание папки документов участника */
#ifdef  UNIX
               snprintf(data_path, sizeof(data_path)-1, "%s/%s", __members_storage, member_key) ;
#else
               snprintf(data_path, sizeof(data_path)-1, "%s\\%s", __members_storage, member_key) ;
#endif

               status=EMIR_create_path(data_path) ;
            if(status) {
                     sprintf(error, "Member's folder creation error %d : %s", status, data_path) ;
                                         break ;
                       }
/*- - - - - - - - - - - - - - - - Занесение операции в FILES_ACTIONS */
#ifdef  UNIX
               snprintf(data_path, sizeof(data_path)-1, "%s/%s/%s.%s", __members_storage, member_key, file_uuid, member_file.file_ext) ;
#else
               snprintf(data_path, sizeof(data_path)-1, "%s\\%s\\%s.%s", __members_storage, member_key, file_uuid, member_file.file_ext) ;
#endif

                      sprintf(text, "insert into %s (\"Action\", \"DfsPath\", \"LocalPath\", \"Status\", \"MasterId\") "
                                    "values( 'GetFile', '%s', '%s', 'NEW', 'Member:%s')",
                                      __db_table_files_actions, member_file.dfs_path, data_path, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                         break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

                       db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status)  EMIR_text_subst(error, "'", "`", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, data_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                      return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------------- Занесение данных в БД  */

   if(!stricmp(action->status, "FILE")) {

    do {

        Cursor=db->LockCursor("EMIR_mb_action_GetMemberFile") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
                      sprintf(text, "select \"Status\" "
                                    "from   %s "
                                    "where  \"MasterId\"='Member:%s'", __db_table_files_actions, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                  EMIR_log(text) ;
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get member's file: No record") ;
       else                      sprintf(error, "Get member's file: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
/*- - - - - - - - - - - - - - -  Получение данных из смарт-контракта */
                               memset(&member_file, 0, sizeof(member_file)) ;
        status=EMIRi_mb_GetMemberFile(action->object, file_uuid, &member_file, error) ;
     if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - Проверка хэш-функции файла */
        status=EMIR_file_hash(action->reply, hash, text) ;
     if(status) {
                   sprintf(error, "File Hash calculation : File %s - %s", action->reply, text) ;
                             break ;
                }

     if(stricmp(member_file.hash, hash)) {
                      sprintf(error, "Invalid file Hash : File %s - %s<>%s", action->reply, member_file.hash, hash) ;
                                                 status=-1 ;
                                                    break ;
                                         }
/*- - - - - - - - - - - - - - - - -  Проверка наличия записи о файле */
                      sprintf(text, "select \"Uid\""
                                    "from   %s "
                                    "where  \"Member\"='%s' and \"Uid\"='%s'", 
                                     __db_table_members_files, member_key, file_uuid) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                  EMIR_log(text) ;
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

             status=db->SelectFetch(Cursor) ;

                                    add_flag=0 ;
          if(status==_SQL_NO_DATA)  add_flag=1 ;
     else if(status) {
                 sprintf(error, "Check file existance: %s", db->error_text) ;
                                                            db->error_text[0]=0 ;
                                                          __db_errors_cnt++ ;
                        break ;
                     }

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - Запрос идентификатора записи клиента */
    if(add_flag) {
                      sprintf(text, "select \"Id\""
                                    "from   %s "
                                    "where  \"Key\"='%s'", __db_table_members, member_key) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status) {
                 sprintf(error, "Get member Id: %s", db->error_text) ;
                                                     db->error_text[0]=0 ;
                                                   __db_errors_cnt++ ;
                         break ;
                }
  
            memset(member_id, 0, sizeof(member_id)) ;
           strncpy(member_id, (char *)Cursor->columns[0].value, sizeof(member_id)-1) ;

                     db->SelectClose(Cursor) ;
                 }
/*- - - - - - - - - - - - - - - - - - - - -  Запись данных в таблицу */
     if(add_flag)  sprintf(text, "insert into %s (\"Member\",\"MemberId\",\"Kind\",\"LocalPath\",\"DfsPath\",\"Hash\",\"Uid\",\"DataUpdate\") "
                                 "values         ( '%s',       %s,         '%s',    '%s',         '%s',       '%s',    '%s',   'A')",
                                   __db_table_members_files, member_key, member_id, 
                                              member_file.kind,     action->reply,
                                              member_file.dfs_path, member_file.hash, file_uuid) ;
     else          sprintf(text, "update %s "
                                 "set    \"Kind\"='%s',\"LocalPath\"='%s',"
                                       " \"DfsPath\"='%s',\"Hash\"='%s',\"DataUpdate\"='U' "
                                 "where  \"Member\"='%s' and \"Uid\"='%s'",
                                   __db_table_members_files,
                                              member_file.kind,     action->reply,
                                              member_file.dfs_path, member_file.hash,
                                              member_key,              file_uuid) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                EMIR_log(text) ;
                 sprintf(error, "Add/update member file record: %s", db->error_text) ;
                                                                     db->error_text[0]=0 ;
                                                                   __db_errors_cnt++ ;
                               break ;
                }
/*- - - - - - - - - - - - - - - -  Удаление записей из FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"MasterId\"='Member:%s'",
                                       __db_table_files_actions, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                        break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL)
   if(!stricmp(result, "DONE")) {

     if(status)  EMIR_text_subst(error, "'", "`", 0) ;

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
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
/*                       Операция RAISE_ALERT                        */

   int  EMIR_mb_action_RaiseAlert(Mb_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
        char  code[2048] ;
        char  account[128] ;
        char *password ;
        char  state_id[128] ;
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
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "NEW" ))  {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
      do {
                             memset(contract, 0, sizeof(contract)) ; 
              status=EMIR_db_syspar(db, "SystemAlert",              /* Запрос адреса СК SystemAlert */
                                             contract, error) ;
           if(status)  break ;

              status=EMIRi_mb_GetStatus(contract,                   /* Запрос текущего состояния смарт-контракта */
                                        state_id, error) ;
           if(status)  break ;

             sprintf(code, "66a5123c") ;                            /* Формируем блок данных транзакции */

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                            contract, code, gas, txn, error) ;
           if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
         } while(0) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

        Cursor=db->LockCursor("EMIR_mb_action_RaiseAlert") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Object\"='%s', \"Status\"='WAIT', \"Reply\"='%s', \"Data\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, contract, txn, state_id, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       return(-1) ;
                }

                    db->Commit() ;
                                                     }

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

              status=EMIRi_mb_GetStatus(action->object, state_id, error) ;
           if(status)  break ;

           if(!strcmp(state_id, action->data)) {                    /* Новое и предыдущее значения должны различаться */
                              strcpy(error, "Check value fail") ;
                                                  status=-1 ;
                                                     break ;
                                               }

         } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

        Cursor=db->LockCursor("EMIR_mb_action_RaiseAlert") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_members_actions, "", action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                   db->Rollback() ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
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
/*            Запрос GetMember смарт-контракта Members               */

   int  EMIRi_mb_GetMember(char *contract, char *key, 
                                         Member *member,
                                     MemberFile *files,
                                            int *files_cnt,
                                            int  files_max,
                                           char *error      )

{
   int  status ;
  char  key_hex[128] ;
  char  buff[4096] ;
  char  text[4096] ;
   int  size ;
  char *result ;
  char *end ;

 static char *request1="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xd452432e%s\"},\"latest\"],\"id\":1}" ;
 static char *request2="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x05bb4133%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------ Подготовка данных */

                     EMIR_txt2hex64(key, key_hex, strlen(key)) ;

/*--------------------------------------- Отправка запроса GetMember */

                          sprintf(buff, request1, contract, key_hex) ;
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

              strcpy(member->key,       key            ) ;          /* Распаковка данных */
              memcpy(member->name,      result+0*64, 64) ;
              memcpy(member->version,   result+1*64, 64) ;
              memcpy(member->lock,      result+2*64, 64) ;
              memcpy(member->role,      result+3*64, 64) ;
              memcpy(member->account,   result+4*64, 64) ;
              memcpy(member->box,       result+5*64, 64) ;

              memcpy(text, result+7*64, 64) ;
        size=strtoul(text, &end, 16) ;
     if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
     if(size>0) {
                   memset(text, 0, sizeof(text)) ;
                   memcpy(text, result+8*64, size*2) ;
             EMIR_hex2txt(text, text) ;
                  strncpy(member->data_link, text, sizeof(member->data_link)-1) ;
                }

        EMIR_hex2txt(member->name, member->name) ;                  /* Преобразование в текст */
        EMIR_hex2txt(member->lock, member->lock) ;
        EMIR_hex2txt(member->role, member->role) ;

              strcpy(member->account, member->account+24) ;
              strcpy(member->box,     member->box    +24) ;

#undef   _RESULT_PREFIX

/*----------------------------------- Отправка запроса GetMemberCert */

                          sprintf(buff, request2, contract, key_hex) ;
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

              memcpy(member->sign,      result+0*64, 64) ;
              memcpy(member->cert_type, result+1*64, 64) ;

              memcpy(text, result+3*64, 64) ;
        size=strtoul(text, &end, 16) ;
     if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
     if(size>0) {
                   memset(text, 0, sizeof(text)) ;
                   memcpy(text, result+4*64, size*2) ;
             EMIR_hex2txt(text, text) ;
                  strncpy(member->cert_link, text, sizeof(member->cert_link)-1) ;
                }

        EMIR_hex2txt(member->sign,      member->sign) ;
        EMIR_hex2txt(member->cert_type, member->cert_type) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetMembers смарт-контракта Members               */

   int  EMIRi_mb_GetMembers(char *contract, Member **members, int *members_cnt, char *error)

{
   int  status ;
  char *result ;
  char  value[128] ;
  char *buff ;
  char *end ;
   int  i ;

#define  _BUFF_SIZE    64000

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xee6e1840\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

              buff=(char *)calloc(1, _BUFF_SIZE) ;

                          sprintf(buff, request, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %128.128s", buff) ;
                          free(buff) ;
                           return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

#define  _RESULT_PREFIX   "\"result\":\""

       result=strstr(buff, _RESULT_PREFIX) ;                        /* Проверка наличия результата */
    if(result==NULL) {
                               result=strchr(buff, '{') ;
             if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                          free(buff) ;
                           return(-1) ;
                     }

                 result+=strlen(_RESULT_PREFIX)+2 ;

//            memcpy("Version", result+0*64, 64) ;            /* Распаковка данных */
//            memcpy("Offset",  result+1*64, 64) ;

              memset(value, 0, sizeof(value)) ;
              memcpy(value,     result+2*64, 64) ;

          *members_cnt=strtoul(value, &end, 16)/2 ;

                    result+=3*64 ;

          *members=(Member *)calloc(*members_cnt+1, sizeof(**members)) ;

        for(i=0 ; i<*members_cnt ; i++) {

                  memcpy((*members)[i].key,     result+(2*i+0)*64, 64) ;
                  memcpy((*members)[i].version, result+(2*i+1)*64, 64) ;

            EMIR_hex2txt((*members)[i].key,  (*members)[i].key) ;
                                        }

#undef   _RESULT_PREFIX

                          free(buff) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetDocument смарт-контракта Member              */

   int  EMIRi_mb_GetMemberFile(char *contract, char *uuid, MemberFile *file, char *error)

{
  char  version[128] ;
   int  status ;
  char  value[128] ;
  char  buff[4096] ;
   int  size ;
  char *result ;
  char *end ;

 static char *request1="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x44c89bb8%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------- Определение версии смарт-контракта */

      status=EMIR_node_getversion(contract, version, buff) ;
   if(status) {
                  sprintf(error, "TRANSPORT - %128.128s", buff) ;
                             return(-1) ;
              } 
/*--------------------------------------- Отправка запроса GetMember */

                          sprintf(buff, request1, contract, uuid) ;
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

              strcpy(file->uid,       uuid           ) ;            /* Распаковка данных */

    if(stricmp(version, "Member.2019-02-27")>=0) {                  /* Для версий смарта Member.2019-02-27 и выше */
              memcpy(file->actor_id,  result+0*64, 64) ;
              memcpy(file->kind,      result+1*64, 64) ;
              memcpy(file->file_ext,  result+2*64, 64) ;
              memcpy(file->hash,      result+3*64, 64) ;

                  memset(value, 0, sizeof(value)) ;
                  memcpy(value, result+5*64, 64) ;                  /* Извлекаем поле "Link" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(file->dfs_path, result+6*64, size*2) ;
                                                 }
    else                                         {
              memset(file->actor_id,  0,           64) ;
              memcpy(file->kind,      result+0*64, 64) ;
              memcpy(file->file_ext,  result+1*64, 64) ;
              memcpy(file->hash,      result+2*64, 64) ;

                  memset(value, 0, sizeof(value)) ;
                  memcpy(value, result+4*64, 64) ;                  /* Извлекаем поле "Link" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(file->dfs_path, result+5*64, size*2) ;
                                                 }

        EMIR_hex2txt(file->kind,     file->kind) ;                  /* Преобразование в текст */
        EMIR_hex2txt(file->file_ext, file->file_ext) ;
        EMIR_hex2txt(file->dfs_path, file->dfs_path) ;

             memmove(file->actor_id, file->actor_id+24, strlen(file->actor_id+24)+1) ;
             memmove(file->hash,     file->hash    +24, strlen(file->hash    +24)+1) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос GetStatus смарт-контракта SystemAlert           */

   int  EMIRi_mb_GetStatus(char *contract, char  *state_id, char *error)

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

//             strcpy(config, "0x") ;
//            strncpy(config+2, result+strlen(_RESULT_PREFIX)+64+24, 40) ;
//                    config[42]=0 ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}

