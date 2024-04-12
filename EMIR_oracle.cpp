/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Модуль "Оракул"                               */
/*                                                                   */
/*********************************************************************/

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
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

#ifdef UNIX
#include "../../dcl/lib/dcl.h"
#else
#include "..\..\dcl\lib\dcl.h"
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

/*--------------------------------------------------- DCL-библиотека */

                 Lang_DCL  DCL ;

          extern   double  dcl_errno ;            /* Системный указатель ошибок -> DCL_SLIB.CPP */
          extern     char  dcl_err_details[512] ; /* Системный указатель расширенного описания ошибок -> DCL_SLIB.CPP */
          extern SQL_link *dcl_sql_connect ;      /* Соединение по-умолчанию для DCL-скрипта */

    Dcl_decl *EMIR_dcl_CreateDeal   (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Создание сделки */
    Dcl_decl *EMIR_dcl_DeployDeal   (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Опубликование сделки */
    Dcl_decl *EMIR_dcl_SetDealState (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Изменение состояния сделки */
    Dcl_decl *EMIR_dcl_AddParty     (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Добавление участника сделки */
    Dcl_decl *EMIR_dcl_AddStatusMap (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Добавление элемента карты изменения статусов сделки */
    Dcl_decl *EMIR_dcl_AddAttribute (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Добавление атрибута сделки */
    Dcl_decl *EMIR_dcl_AddFile      (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Добавление файла к сделке */
    Dcl_decl *EMIR_dcl_GetDeal      (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Получение данных сделки */
    Dcl_decl *EMIR_dcl_GetAttributes(Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Получение списка атрибутов сделки */
    Dcl_decl *EMIR_dcl_GetFiles     (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Получение списка файлов сделки */
    Dcl_decl *EMIR_dcl_GetHistory   (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Получение истории статусов сделки */
    Dcl_decl *EMIR_dcl_GetParty     (Lang_DCL *, Dcl_decl *, Dcl_decl **, int) ;    /* Получение списка участников сделки */
//  Dcl_decl *ExtractDoc            (Lang_DCL *,             Dcl_decl **, int) ;    /*  */
    Dcl_decl  dcl_oracle_lib[]={

        {0, 0, 0, 0, "$PassiveData$", NULL, "emir", 0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "CreateDeal",       (void *)EMIR_dcl_CreateDeal,     "",      0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "DeployDeal",       (void *)EMIR_dcl_DeployDeal,     "",      0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "SetDealState",     (void *)EMIR_dcl_SetDealState,   "sss",   0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "AddParty",         (void *)EMIR_dcl_AddParty,       "ss",    0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "AddStatusMap",     (void *)EMIR_dcl_AddStatusMap,   "sss",   0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "AddAttribute",     (void *)EMIR_dcl_AddAttribute,   "ss",    0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "AddFile",          (void *)EMIR_dcl_AddFile,        "sssss", 0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "GetDeal",          (void *)EMIR_dcl_GetDeal,        "s",     0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "GetAttributes",    (void *)EMIR_dcl_GetAttributes,  "ss",    0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "GetFiles",         (void *)EMIR_dcl_GetFiles,       "ss",    0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "GetHistory",       (void *)EMIR_dcl_GetHistory,     "s",     0, 0},
	 {_DGT_VAL, _DCL_METHOD, 0, 0, "GetParty",         (void *)EMIR_dcl_GetParty,       "s",     0, 0},
/*
	 {_CHR_PTR, _DCL_CALL,   0, 0, "ExtractDoc",       (void *)ERPC_dcl_ExtractDoc,     "s",     0, 0},
*/
	 {0, 0, 0, 0, "", NULL, NULL, 0, 0}
                             } ;

             int EMIR_dcl_debug(void) ;

/*------------------------------------ Обработчики элементов диалога */

#define  _DEALS_MAX           5
#define  _DEAL_PARTIES_MAX   10 
#define  _DEAL_MAP_MAX      100
#define  _DEAL_ATTR_MAX      50
#define  _DEAL_FILES_MAX      10

                Deal  or_deals[_DEALS_MAX] ;
                 int  or_deals_cnt ;
           DealParty  or_parties[_DEALS_MAX*_DEAL_PARTIES_MAX] ;
                 int  or_parties_cnt ;
             DealMap  or_map[_DEALS_MAX*_DEAL_MAP_MAX] ;
                 int  or_map_cnt ;
            DealAttr  or_attr[_DEALS_MAX*_DEAL_ATTR_MAX] ;
                 int  or_attr_cnt ;
            DealFile  or_files[_DEALS_MAX*_DEAL_FILES_MAX] ;
                 int  or_files_cnt ;

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

  void  EMIRi_or_deals         (HWND hDlg, char *prefix, SQL_link *db) ;           /* Обработка интерфейса Deal модуля "Oracle" */
  void  EMIRi_or_extfiles      (HWND hDlg, char *prefix, SQL_link *db) ;           /* Обработка интерфейса ExtFile модуля "Oracle" */
   int  EMIR_or_DCL_deal       (Or_deal *action, SQL_link *db, char *error) ;      /* Обёртка DCL для Deal */
   int  EMIR_or_DCL_extfile    (Or_extfile *action, SQL_link *db, char *error) ;   /* Обёртка DCL для ExtFile */
   int  EMIR_or_DCL_exec       (char *processor,                                   /* Процессор DCL */
                                 Dcl_decl *pars, SQL_link *db, char *error) ;
   int  EMIRi_or_DeployDeals   (SQL_link *db, char *error) ;                       /* Создание новых сделок */
   int  EMIRi_or_ChangeDeals   (SQL_link *db, char *error, char *path) ;           /* Изменение состояния сделок */
   int  EMIRi_or_GetStoragePath(char *ext_path, char *path, char *error) ;         /* Формирование пути сохранения файла */


/*********************************************************************/
/*                                                                   */
/*	      Обработчик сообщений диалогового окна ORACLE            */	

 INT_PTR CALLBACK  EMIR_oracle_dialog(  HWND  hDlg,     UINT  Msg, 
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
}


/********************************************************************/
/*                                                                  */
/*        THREAD - Фоновый поток модуля "Oracle"                    */

  DWORD WINAPI  Oracle_Thread(LPVOID Pars)

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

      if(__or_request_period<=0)  __or_request_period= 10 ;
      if(__or_view_frame    <=0)  __or_view_frame    =100 ;

              hDlg=hOracle_Dialog ;

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
         if(rows_cnt>__or_view_frame) {

               for(i=0 ; i<rows_cnt-__or_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_or_deals   (hOracle_Dialog, prefix, &DB) ;
             EMIRi_or_extfiles(hOracle_Dialog, prefix, &DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

                DB.Disconnect() ;                                   /* Отсоединение с БД */

            sprintf(text, "%s (LVL.1) Pulse done", prefix) ;
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;

      } while(1) ;

/*-------------------------------------------------------------------*/

  return(0) ;

#endif
}


/********************************************************************/
/*                                                                  */
/*          Обработчик фонового потока модуля "Oracle"              */

  void  Oracle_Process(SQL_link *DB)

{
//static  time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;

#ifndef  UNIX
            HWND  hDlg ;
             int  rows_cnt ;
             int  i ;
#endif

/*---------------------------------------------------- Инициализация */

      if(__net_locked)  return ;

      if(__or_request_period<=0)  __or_request_period= 10 ;
      if(__or_view_frame    <=0)  __or_view_frame    =100 ;

#ifndef  UNIX
              hDlg=hOrcale_Dialog ;
#endif

/*------------------------------------------------------- Общий цикл */

     do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Очистка лога */
#ifndef  UNIX

#pragma warning(disable : 4244)
            rows_cnt=LB_GET_COUNT(IDC_LOG) ;
#pragma warning(default : 4244)
         if(rows_cnt>__or_view_frame) {

               for(i=0 ; i<rows_cnt-__or_view_frame ; i++)  LB_DEL_ROW(IDC_LOG, i) ;
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

            sprintf(text, " Oracle> %s (LVL.1) Pulse start", prefix) ;

#ifdef UNIX
             EMIR_log(text) ;
#else
         LB_ADD_ROW(IDC_LOG, text) ;
         LB_TOP_ROW(IDC_LOG, LB_GET_COUNT(IDC_LOG)-1) ;
#endif
/*- - - - - - - - - - - - - - - - - - - - - Обработка стека операций */
             EMIRi_or_deals   (hOracle_Dialog, prefix, DB) ;
             EMIRi_or_extfiles(hOracle_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        } while(0) ;

            sprintf(text, " Oracle> %s (LVL.1) Pulse done", prefix) ;

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
/*             Обработка интерфейса Deal модуля "Oracle"            */

   void  EMIRi_or_deals(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
            int  status ;
           long  txn_block ;
           char  error[1024] ;
           char  text[1024] ;
            int  i ;

/*------------------------------------------------------- Подготовка */

        status=EMIR_db_nodepars(db, error) ;
     if(status) {
                   LB_ADD_ROW(IDC_LOG, error) ;
                       return ;
                }
/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_OR_ACTIONS_MAX ; i++)
     if(__or_deals[i]!=NULL) {
                                   free(__or_deals[i]) ;
                                        __or_deals[i]=NULL ;
                             }
/*---------------------------------------- Получение списка операций */

        Cursor=db->LockCursor("EMIRi_or_deal") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_or_deal - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               return ;
                      }

                      sprintf(text, "select d.\"Id\", d.\"BlockChainId\", d.\"Kind\", d.\"Status\", d.\"OracleData\", "
                                    "       p.\"Processor\", p.\"Period\", "
                                    "       d.\"Version\", d.\"OracleVersion\", d.\"TxnBlock\" "
                                    "from   %s d inner join %s p on d.\"Kind\"=p.\"IFaceKey1\" and d.\"Status\"=p.\"IFaceKey2\" "
                                    "where  p.\"IFaceKind\"='Deal'"
                                    " and   d.\"DataUpdate\" in ('A','U') "
                                    " and   d.\"OracleNextTime\" < %ld or d.\"OracleNextTime\" is null "
                                    "order by d.\"OracleNextTime\"",
                                       __db_table_deals, __db_table_oracle_processor, (long)time(NULL)) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_or_deal - Get deals list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

   for(i=0 ; i<_OR_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_or_deal - Get deals list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                   __or_deals[i]=(struct Or_deal *)calloc(1, sizeof(*__or_deals[i])) ;
           strncpy(__or_deals[i]->id,          (char *)Cursor->columns[0].value, sizeof(__or_deals[i]->id         )-1) ;
           strncpy(__or_deals[i]->address,     (char *)Cursor->columns[1].value, sizeof(__or_deals[i]->address    )-1) ;
           strncpy(__or_deals[i]->kind,        (char *)Cursor->columns[2].value, sizeof(__or_deals[i]->kind       )-1) ;
           strncpy(__or_deals[i]->status,      (char *)Cursor->columns[3].value, sizeof(__or_deals[i]->status     )-1) ;
           strncpy(__or_deals[i]->data,        (char *)Cursor->columns[4].value, sizeof(__or_deals[i]->data       )-1) ;
           strncpy(__or_deals[i]->processor,   (char *)Cursor->columns[5].value, sizeof(__or_deals[i]->processor  )-1) ;
           strncpy(__or_deals[i]->period,      (char *)Cursor->columns[6].value, sizeof(__or_deals[i]->period     )-1) ;
           strncpy(__or_deals[i]->version,     (char *)Cursor->columns[7].value, sizeof(__or_deals[i]->version    )-1) ;
           strncpy(__or_deals[i]->pre_version, (char *)Cursor->columns[8].value, sizeof(__or_deals[i]->pre_version)-1) ;
           strncpy(__or_deals[i]->txn_block,   (char *)Cursor->columns[9].value, sizeof(__or_deals[i]->txn_block  )-1) ;
                                      }

                 db->SelectClose (Cursor) ;
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------- Перебор операций */

  for(i=0 ; i<_OR_ACTIONS_MAX ; i++) {

        if(__or_deals[i]==NULL)  continue ;
/*- - - - - - - - - - - - - Проверка исполнения транзакции изменения */
   if(!strcmp(__or_deals[i]->version,
              __or_deals[i]->pre_version)) {

           txn_block=atol(__or_deals[i]->txn_block);
        if(txn_block>=__block_db)  continue ;

          sprintf(text, "%s - WARNING - EMIRi_or_deal - status change missed : %s %s/%s - %s", prefix, __or_deals[i]->id, __or_deals[i]->kind, __or_deals[i]->status, __or_deals[i]->processor) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              continue ;
                                           } 
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  DCL-процессор */
#define  KEY  "DCL:"
   if(!memcmp(__or_deals[i]->processor, KEY, strlen(KEY))) {

          sprintf(text, "%s - DCL : %s %s/%s - %s", prefix, __or_deals[i]->id, __or_deals[i]->kind, __or_deals[i]->status, __or_deals[i]->processor) ;

               LB_ADD_ROW(IDC_LOG, text) ;

      status=EMIR_or_DCL_deal(__or_deals[i], db, error) ;
   if(status) {
     sprintf(text, "%s - ERROR - EMIR_or_deal : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
              }
                                                           }
#undef   KEY
/*- - - - - - - - - - - - - - - - - - - - - -  Неизвестный процессор */
   else                                                      {

          sprintf(text, "%s - ERROR - EMIRi_or_deal - unknown processor : %s %s/%s - %s", prefix, __or_deals[i]->id, __or_deals[i]->kind, __or_deals[i]->status, __or_deals[i]->processor) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                                                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*-------------------------------------------------------------------*/

}


/********************************************************************/
/*                                                                  */
/*          Обработка интерфейса ExtFile модуля "Oracle"            */

   void  EMIRi_or_extfiles(HWND hDlg, char *prefix, SQL_link *db)

{
     SQL_cursor *Cursor ;
     SQL_cursor *Cursor_u ;
            int  status ;
           char  iface_id[64] ;
           char  iface_folder_in[FILENAME_MAX] ;
           char  iface_folder_error[FILENAME_MAX] ;
           char  iface_key_1[64] ;
           char  iface_key_2[64] ;
           char  iface_processor[1024] ;
           char  iface_period[64] ;
         time_t  time_next ;
           char  error[1024] ;
           char  text[1024] ;
           char *end ;
            int  i ;

#ifdef UNIX
                 DIR *dir_hdr ;
       struct dirent *file_info ;
#else
                 int  dir_hdr ;
  struct _finddata_t  file_info ;
#endif

/*------------------------------------------------------- Подготовка */

        status=EMIR_db_nodepars(db, error) ;
     if(status) {
                   LB_ADD_ROW(IDC_LOG, error) ;
                       return ;
                }
/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_OR_ACTIONS_MAX ; i++)
     if(__or_extfiles[i]!=NULL) {
                                   free(__or_extfiles[i]) ;
                                        __or_extfiles[i]=NULL ;
                                }
/*---------------------------- Получение списка ExtFiles-интерфейсов */

        Cursor=db->LockCursor("EMIRi_or_extfile") ;
     if(Cursor==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_or_extfile - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               return ;
                      }

                      sprintf(text, "select p.\"Id\", p.\"IFacePars\", p.\"IFaceKey1\", p.\"IFaceKey2\", p.\"Processor\", p.\"Period\" "
                                    "from    %s p "
                                    "where  p.\"IFaceKind\"='ExtFile'"
                                    " and  (p.\"NextTime\" < %ld or p.\"NextTime\" is null) "
                                    "order by p.\"NextTime\"",
                                       __db_table_oracle_processor, (long)time(NULL)) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_or_extfile - Get ifaces list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

   for(i=0 ; i<_OR_ACTIONS_MAX ; ) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_or_extfile - Get ifaces list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

         strncpy(iface_id,        (char *)Cursor->columns[0].value, sizeof(iface_id       )-1) ;
         strncpy(iface_folder_in, (char *)Cursor->columns[1].value, sizeof(iface_folder_in)-1) ;
         strncpy(iface_key_1,     (char *)Cursor->columns[2].value, sizeof(iface_key_1    )-1) ;
         strncpy(iface_key_2,     (char *)Cursor->columns[3].value, sizeof(iface_key_2    )-1) ;
         strncpy(iface_processor, (char *)Cursor->columns[4].value, sizeof(iface_processor)-1) ;
         strncpy(iface_period,    (char *)Cursor->columns[5].value, sizeof(iface_period   )-1) ;
/*- - - - - - - - - - - - - Разбор управляющих параметров интерфейса */
        end=strchr(iface_folder_in, ';') ;
     if(end==NULL) {
                         sprintf(text, "%s - ERROR - EMIRi_or_extfile - Invalid ifaces specification: %s", prefix, iface_id) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                              continue ;
                   }

                                 *end=0 ;
       strcpy(iface_folder_error, end+1) ;
/*- - - - - - - - - - - - - - - -  Формирование списка файлов - *NIX */
#ifdef UNIX

        dir_hdr=opendir(iface_folder_in) ;
     if(dir_hdr!=NULL) { 

        for( ; i<_OR_ACTIONS_MAX ; ) {

            file_info=readdir(dir_hdr) ;
         if(file_info==NULL)  break ;

         if(file_info->d_name[0]=='.')  continue ;

                   __or_extfiles[i]=(struct Or_extfile *)calloc(1, sizeof(*__or_extfiles[i])) ;
           sprintf(__or_extfiles[i]->path,         "%s/%s", iface_folder_in, file_info->d_name) ;
            strcpy(__or_extfiles[i]->errors_folder, iface_folder_error) ;
           strncpy(__or_extfiles[i]->processor,     iface_processor, sizeof(__or_extfiles[i]->processor)-1) ;
           strncpy(__or_extfiles[i]->period,        iface_period,    sizeof(__or_extfiles[i]->period   )-1) ;
                                 i++ ; 

                                     } 

	                   closedir(dir_hdr) ;

                       }
/*- - - - - - - - - - - - -  Формирование списка файлов - MS Windows */
#else

        dir_hdr=_findfirst(iface_folder_in, &file_info) ;
     if(dir_hdr>=0) {

                find_next=0 ;

        for( ; i<_OR_ACTIONS_MAX ; ) {

             if(find_next) {
                     status=_findnext(dir_hdr, &file_info) ;
                  if(status)  break ; 
                           } 

                find_next++ ;

             if(file_info.name[0]=='.')  continue ;

                   __or_extfiles[i]=(struct Or_extfile *)calloc(1, sizeof(*__or_extfiles[i])) ;
           sprintf(__or_extfiles[i]->path,          "%s\\%s", iface_folder_in, file_info.name) ;
            strcpy(__or_extfiles[i]->errors_folder, iface_folder_error) ;
           strncpy(__or_extfiles[i]->processor,     iface_processor, sizeof(__or_extfiles[i]->processor)-1) ;
           strncpy(__or_extfiles[i]->period,        iface_period,    sizeof(__or_extfiles[i]->period   )-1) ;
                                 i++ ; 

                                     } 

                         _findclose(dir_hdr) ;

                    }
#endif
/*- - - - - - - - - - - - - - - - - - -  Простановка метки обработки */
        Cursor_u=db->LockCursor("EMIRi_or_extfile_updates") ;
     if(Cursor_u==NULL) {
                           sprintf(text, "%s - ERROR - EMIRi_or_extfile - Updates cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                               continue ;
                        }

                         time_next=time(NULL)+atoi(iface_period) ;

                      sprintf(text, "update %s "
                                    "set    \"NextTime\"='%ld'"
                                    "where  \"Id\"=%s",
                                       __db_table_oracle_processor, time_next, iface_id) ;

        status=db->SqlExecute(Cursor_u, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "%s - ERROR - EMIRi_or_extfile - Set iface next time error: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                          db->Rollback() ;
                        __db_errors_cnt++ ;
                }
     else       {
                        db->Commit() ;
                }

                        db->UnlockCursor(Cursor_u) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                   }

                 db->SelectClose (Cursor) ;
                 db->UnlockCursor(Cursor) ;

/*------------------------------------------------- Перебор операций */

  for(i=0 ; i<_OR_ACTIONS_MAX ; i++) {

        if(__or_extfiles[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  DCL-процессор */
#define  KEY  "DCL:"

   if(!memcmp(__or_extfiles[i]->processor, KEY, strlen(KEY))) {

          sprintf(text, "%s - DCL : %s - %s", prefix, __or_extfiles[i]->path, __or_extfiles[i]->processor) ;

               LB_ADD_ROW(IDC_LOG, text) ;

      status=EMIR_or_DCL_extfile(__or_extfiles[i], db, error) ;
   if(status) {
     sprintf(text, "%s - ERROR - EMIR_or_extfile : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
              }
                                                              }
#undef   KEY
/*- - - - - - - - - - - - - - - - - - - - - -  Неизвестный процессор */
   else                                                      {

          sprintf(text, "%s - ERROR - EMIRi_or_extfile - unknown processor : %s - %s", prefix, __or_extfiles[i]->path, __or_extfiles[i]->processor) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                                                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*-------------------------------------------------------------------*/

}


/*********************************************************************/
/*                                                                   */
/*                     Обёртка DCL для Deal                          */

   int  EMIR_or_DCL_deal(Or_deal *action, SQL_link *db, char *error)

{
#define  _SYSVARS_MAX    100

   SQL_cursor *Cursor ;
     Dcl_decl  sys_vars[_SYSVARS_MAX] ;
         char  prc_status_next[1024] ;
         char  prc_remark[1024] ;
       time_t  time_next ;
         char  text[1024] ;
          int  status ;
          int  err_flag ;
          int  i ;

/*------------------------------------------------------- Подготовка */

/*----------------------------------------------- Внешние переменные */

       memset(prc_status_next, 0, sizeof(prc_status_next)) ;
       memset(prc_remark     , 0, sizeof(prc_remark     )) ;

       memset(sys_vars, 0, sizeof(sys_vars)) ;

                        i=0 ;
       strcpy( sys_vars[i].name, "$address") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= action->address ;
               sys_vars[i].size=strlen(action->address) ;
               sys_vars[i].buff= -1 ;
                        i++ ;
       strcpy( sys_vars[i].name, "$kind") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= action->kind ;
               sys_vars[i].size=strlen(action->kind) ;
               sys_vars[i].buff= -1 ;
                        i++ ;
       strcpy( sys_vars[i].name, "$status") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= action->status ;
               sys_vars[i].size=strlen(action->status) ;
               sys_vars[i].buff= -1 ;
                        i++ ;
       strcpy( sys_vars[i].name, "$status_next") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= prc_status_next ;
               sys_vars[i].size=  0 ;
               sys_vars[i].buff=sizeof(prc_status_next) ;
                        i++ ;
       strcpy( sys_vars[i].name, "$remark") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= prc_remark ;
               sys_vars[i].size=  0 ;
               sys_vars[i].buff=sizeof(prc_remark) ;
                        i++ ;

/*-------------------------------- Выполнение операционной процедуры */

     err_flag=EMIR_or_DCL_exec(action->processor, sys_vars, db, error) ;

/*----------------------- Обработка результатов исполнения процедуры */

            time_next=time(NULL)+atoi(action->period) ;

        Cursor=db->LockCursor("EMIR_or_DCL_deal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - Исполнение смены статуса */
   if(prc_status_next[0]!=0) {

                      sprintf(text, "Insert into %s (\"Action\",\"Data\",\"Executor\",\"Status\")"
                                    "Values('SetStatus',"
                                           "'{\"Id\":\"%s\",\"Status\":\"%s\",\"Remark\":\"%s\"}',"
                                           "'%s',"
                                           "'NEW')",
                                       __db_table_deals_actions, action->id, prc_status_next, prc_remark, __member_executor) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                return(-1) ;
                }

                      sprintf(text, "update %s "
                                    "set    \"OracleNextTime\"='%ld', \"OracleVersion\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals, time_next, action->version, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                       return(-1) ;
                }

                             }
/*- - - - - - - - - - - - - - - - - - - - - -  Без изменения статуса */
   else                      {

                      sprintf(text, "update %s "
                                    "set    \"OracleNextTime\"='%ld'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals, time_next, action->id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                    return(-1) ;
                }

                             }
/*- - - - - - - - - - - - -  Пропись следующего контрольного времени */

                       db->Commit() ;
                   db->UnlockCursor(Cursor) ;


/*------------------------------------------------- Обработка ошибок */

    if(err_flag)  return(-1) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                     Обёртка DCL для ExtFile                       */

   int  EMIR_or_DCL_extfile(Or_extfile *action, SQL_link *db, char *error)

{
#define  _SYSVARS_MAX    100

     Dcl_decl  sys_vars[_SYSVARS_MAX] ;
         char  command[1024] ;
         char *name ;
       time_t  time_abs ;
         FILE *file ;
          int  status ;
          int  err_flag ;
          int  i ;

/*------------------------------------------------------- Подготовка */

/*----------------------------------------------- Внешние переменные */

       memset(sys_vars, 0, sizeof(sys_vars)) ;

                        i=0 ;
       strcpy( sys_vars[i].name, "$path") ;
               sys_vars[i].type=_DCL_CHAR_AREA ;
               sys_vars[i].addr= action->path ;
               sys_vars[i].size=strlen(action->path) ;
               sys_vars[i].buff= -1 ;
                        i++ ;

/*-------------------------------- Выполнение операционной процедуры */

      err_flag=EMIR_or_DCL_exec(action->processor, sys_vars, db, error) ;

/*-------------------------------------- Исполнение очереди действий */

  if(!err_flag) {

   if(!err_flag)  err_flag=EMIRi_or_DeployDeals(db, error) ;        /* Создание новых сделок */
   if(!err_flag)  err_flag=EMIRi_or_ChangeDeals(db, error,          /* Изменение состояния сделок */
                                                  action->path) ;

   if(!err_flag)            db->Commit() ;
                }
/*----------------------- Обработка результатов исполнения процедуры */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - -  Если ошибка */
    if(err_flag) {                                                  /* Если ошибка - перемещаем файл в папку ошибочных... */
#ifdef UNIX
                     time_abs=time(NULL) ;

            name=strrchr(action->path, '/') ;
         if(name!=NULL)  name++ ;
         else            name=action->path ;

                  sprintf(command, "mv \"%s\" \"%s/%s.%lx\"", action->path, action->errors_folder, name, time_abs) ;
            status=system(command) ;
         if(status) {
                        sprintf(error, "File move to errors folder error (status=%d errno=%d)", status, errno) ;
                                   return(-1) ;
                    }
#else
                        sprintf(error, "File move to errors folder not implemented for Windows") ;
                                   return(-1) ;
#endif

              sprintf(command, "%s/%s.%lx.error", action->errors_folder, name, time_abs) ;
           file=fopen(command, "wb") ;
        if(file==NULL) {
                         sprintf(error, "Error description file creation error %d : %s", errno, command) ;
                                   return(-1) ;
                       }

               fwrite(error, 1, strlen(error), file) ;
               fclose(file) ;

                 }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Если нормально */
    else         {                                                  /* Если нормально - удаляем файл сообщения... */

            status=unlink(action->path) ;
         if(status) {
               sprintf(error, "Iface file delete error %d : %s", errno, action->path) ;
                         return(-1) ;
                    }
                 }
/*------------------------------------------------- Обработка ошибок */

    if(err_flag)  return(-1) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                        Процессор DCL                              */

   int  EMIR_or_DCL_exec(char *processor, Dcl_decl *pars, SQL_link *db, char *error)

{
#define  _SYSVARS_MAX    100

     Dcl_decl *vars[10] ;
         char *libs[10] ;
     Dcl_decl  sys_vars[_SYSVARS_MAX] ;
         char  dcl_name[512] ;
         char  dcl_path[512] ;
         char  error_msg[1024] ;
         char *dcl_text ;
         char *end ;
          int  i ;

 extern Dcl_decl  dcl_debug_lib[] ;  
 extern Dcl_decl  dcl_std_lib[] ;    
 extern Dcl_decl  dcl_file_lib[] ;   
 extern Dcl_decl  dcl_sql_lib[] ;    
 extern Dcl_decl  dcl_office_lib[] ; 

/*---------------------------------------------- Загрузка DCL-файла  */

    if(__dcl_storage[0]==0) {
                              strcpy(error, "Dcl-objects folder is not defined") ;
                                return(-1) ;
                            }

           strcpy(dcl_name, processor+strlen("DCL:")) ;
       end=strchr(dcl_name, '(') ;
    if(end!=NULL) {
                      *end=0 ;
                  }

#ifdef  UNIX
                      sprintf(dcl_path, "%s/%s.dcl", __dcl_storage, dcl_name) ;
#else
                      sprintf(dcl_path, "%s\\%s.dcl", __dcl_storage, dcl_name) ;
#endif

       dcl_text=EMIR_loadfile(dcl_path, error) ;
    if(dcl_text==NULL) {
                          return(-1) ;
                       }
/*------------------------------------------------------- Подготовка */

          dcl_sql_connect=db ;
//                        db->SetAutoCommit(0) ;                    /* Отключаем автокоммит */

/*-------------------------------------------- Подключение библиотек */

       memset(vars, 0, sizeof(vars)) ;

	   vars[0]=sys_vars ;                                       /* Список переменных и функций */
	   vars[1]=dcl_debug_lib ;
	   vars[2]=dcl_std_lib ;
	   vars[3]=dcl_file_lib ;
	   vars[4]=dcl_sql_lib ;
	   vars[5]=dcl_office_lib ;
	   vars[6]=dcl_oracle_lib ;

       memset(libs, 0, sizeof(libs)) ;

            DCL.mDebug=EMIR_dcl_debug ;

/*----------------------------------------------- Внешние переменные */

       memset(sys_vars, 0, sizeof(sys_vars)) ;

     for(i=0 ; pars[i].name[0]!=0 ; i++) sys_vars[i]=pars[i] ;

             or_deals_cnt=0 ;
           or_parties_cnt=0 ;
               or_map_cnt=0 ;
              or_attr_cnt=0 ;
             or_files_cnt=0 ;


/*-------------------------------- Выполнение операционной процедуры */

                     DCL.mWorkDir    =__work_folder ;
                     DCL.mLibDirs    =libs ;
                     DCL.mVars       =vars ;

                     DCL.mProgramMem =dcl_text ;
                     DCL.mProgramFile=  NULL ;

                     DCL.vProcess() ;

/*------------------------------------------------- Обработка ошибок */

                               error[0]=0 ;

  if(DCL.mError_code) {

            strcpy(error_msg, DCL.vDecodeError(DCL.mError_code)) ;
//       CharToOem(error_msg, error_msg) ;

           sprintf(error, "File   :%s  Row:%d  Bad:<%s>\r\n"
                          "Error  :%d  %s\r\n"
                          "Details:%s\r\n",
                          (DCL.mError_file!=NULL)?DCL.mError_file:"NULL", 
                           DCL.mRow, 
                          (DCL.mError_position!=NULL)?DCL.mError_position:"NULL", 
                           DCL.mError_code, error_msg, DCL.mError_details) ;

                                  return(-1) ;
                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                        DCL-БИБЛИОТЕКА                             */
/*                                                                   */
/*********************************************************************/


/*********************************************************************/
/*                                                                   */
/*                         Создание сделки                           */

   Dcl_decl *EMIR_dcl_CreateDeal(Lang_DCL  *dcl_kernel,
                                 Dcl_decl  *source, 
                                 Dcl_decl **pars, 
                                      int   pars_cnt)

{
   char  rec_type[32] ;
   char  dummy[64] ;
    int  status ;

          Dcl_decl  rec_data[4] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)  0, rec_type,    32,    32},
                                  {_CHR_AREA, 0, 0, 0, "Kind",    (void *) 32, dummy,       64,    64},
                                  {_CHR_AREA, 0, 0, 0, "UUID",    (void *) 96, dummy,       64,    64},
                                  {_CHR_AREA, 0, 0, 0, "Data",    (void *)160, dummy,    16008, 16008}
                                 } ;
  Dcl_complex_type  rec_template={ "deal", 16168, rec_data, 4} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt!= 0) {                                               /* Проверяем число параметров */
                        dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                              return(&dgt_return) ; 
                    }
/*-------------------------------------------------- Создание сделки */

                            strcpy(rec_type, "deal") ;
                            strcpy(dummy,    ""    ) ;

           rec_data[0].size=strlen(rec_data[0].prototype) ;
           rec_data[1].size=strlen(rec_data[1].prototype) ;
           rec_data[2].size=strlen(rec_data[2].prototype) ;
           rec_data[3].size=strlen(rec_data[3].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status)  dgt_value=-1 ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                Добавление участника сделки                        */

   Dcl_decl *EMIR_dcl_AddParty(Lang_DCL  *dcl_kernel,
                               Dcl_decl  *source, 
                               Dcl_decl **pars, 
                                    int   pars_cnt)

{
   char  rec_type[32] ;
   char  party_id[64] ;
   char  party_role[64] ;
    int  status ;

          Dcl_decl  rec_data[3] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)  0, rec_type,   32, 32},
                                  {_CHR_AREA, 0, 0, 0, "PartyId", (void *) 32, party_id,   64, 64},
                                  {_CHR_AREA, 0, 0, 0, "Role",    (void *) 96, party_role, 64, 64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_party", 160, rec_data, 3} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt      <  2  ||                                         /* Проверяем число параметров */
      pars[0]->addr==NULL ||
      pars[1]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                              }

                  memset(party_id,   0, sizeof(party_id  )) ;
                  memset(party_role, 0, sizeof(party_role)) ;

    if(pars[0]->size>=sizeof(party_id))
                  memcpy(party_id, pars[0]->addr, sizeof(party_id)-1) ;
    else          memcpy(party_id, pars[0]->addr, pars[0]->size) ;

    if(pars[1]->size>=sizeof(party_role))
                  memcpy(party_role, pars[1]->addr, sizeof(party_role)-1) ;
    else          memcpy(party_role, pars[1]->addr, pars[1]->size) ;

/*-------------------------------------------------- Создание сделки */

                            strcpy(rec_type, "deal_party") ;

           rec_data[0].size=strlen(rec_data[0].prototype) ;
           rec_data[1].size=strlen(rec_data[1].prototype) ;
           rec_data[2].size=strlen(rec_data[2].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status)  dgt_value=-1 ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*       Добавление элемента карты изменения статусов сделки         */

   Dcl_decl *EMIR_dcl_AddStatusMap(Lang_DCL  *dcl_kernel,
                                   Dcl_decl  *source, 
                                   Dcl_decl **pars, 
                                        int   pars_cnt)

{
   char  rec_type[32] ;
   char  map_prev[64] ;
   char  map_next[64] ;
   char  map_role[64] ;
    int  status ;

          Dcl_decl  rec_data[4] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",     (void *)  0, rec_type, 32, 32},
                                  {_CHR_AREA, 0, 0, 0, "Status",     (void *) 32, map_prev, 64, 64},
                                  {_CHR_AREA, 0, 0, 0, "StatusNext", (void *) 96, map_next, 64, 64},
                                  {_CHR_AREA, 0, 0, 0, "Role",       (void *)160, map_role, 64, 64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_map", 224, rec_data, 4} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt      <  3  ||                                         /* Проверяем число параметров */
      pars[0]->addr==NULL ||
      pars[1]->addr==NULL ||
      pars[2]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                              }

                  memset(map_prev, 0, sizeof(map_prev)) ;
                  memset(map_next, 0, sizeof(map_next)) ;
                  memset(map_role, 0, sizeof(map_role)) ;

    if(pars[0]->size>=sizeof(map_prev))
                  memcpy(map_prev, pars[0]->addr, sizeof(map_prev)-1) ;
    else          memcpy(map_prev, pars[0]->addr, pars[0]->size) ;

    if(pars[1]->size>=sizeof(map_next))
                  memcpy(map_next, pars[1]->addr, sizeof(map_next)-1) ;
    else          memcpy(map_next, pars[1]->addr, pars[1]->size) ;

    if(pars[2]->size>=sizeof(map_role))
                  memcpy(map_role, pars[2]->addr, sizeof(map_role)-1) ;
    else          memcpy(map_role, pars[2]->addr, pars[2]->size) ;

/*-------------------------------------------------- Создание сделки */

                            strcpy(rec_type, "deal_map") ;

           rec_data[0].size=strlen(rec_data[0].prototype) ;
           rec_data[1].size=strlen(rec_data[1].prototype) ;
           rec_data[2].size=strlen(rec_data[2].prototype) ;
           rec_data[3].size=strlen(rec_data[3].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status)  dgt_value=-1 ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                Добавление атрибута сделки                         */

   Dcl_decl *EMIR_dcl_AddAttribute(Lang_DCL  *dcl_kernel,
                                   Dcl_decl  *source, 
                                   Dcl_decl **pars, 
                                        int   pars_cnt)

{
   char  rec_type[32] ;
   char  attr_name[64] ;
   char  attr_value[64] ;
    int  status ;

          Dcl_decl  rec_data[3] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$", (void *)  0, rec_type,   32, 32},
                                  {_CHR_AREA, 0, 0, 0, "Name",   (void *) 32, attr_name,  64, 64},
                                  {_CHR_AREA, 0, 0, 0, "Value",  (void *) 96, attr_value, 64, 64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_attribute", 160, rec_data, 3} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt      <  2  ||                                         /* Проверяем число параметров */
      pars[0]->addr==NULL ||
      pars[1]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                              }

                  memset(attr_name,  0, sizeof(attr_name )) ;
                  memset(attr_value, 0, sizeof(attr_value)) ;

    if(pars[0]->size>=sizeof(attr_name))
                  memcpy(attr_name, pars[0]->addr, sizeof(attr_name)-1) ;
    else          memcpy(attr_name, pars[0]->addr, pars[0]->size) ;

    if(pars[1]->size>=sizeof(attr_value))
                  memcpy(attr_value, pars[1]->addr, sizeof(attr_value)-1) ;
    else          memcpy(attr_value, pars[1]->addr, pars[1]->size) ;

/*-------------------------------------------------- Создание сделки */

                            strcpy(rec_type, "deal_attribute") ;

           rec_data[0].size=strlen(rec_data[0].prototype) ;
           rec_data[1].size=strlen(rec_data[1].prototype) ;
           rec_data[2].size=strlen(rec_data[2].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status)  dgt_value=-1 ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                      Добавление файла сделки                      */

   Dcl_decl *EMIR_dcl_AddFile(Lang_DCL  *dcl_kernel,
                              Dcl_decl  *source, 
                              Dcl_decl **pars, 
                                   int   pars_cnt)

{
   char  rec_type[32] ;
   char  file_path[512] ;
   char  file_kind[64] ;
   char  file_remark[1024] ;
   char  file_recipients[1024] ;
   char  file_uuid[64] ;
    int  status ;

          Dcl_decl  rec_data[6] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",     (void *)   0, rec_type,          32,   32},
                                  {_CHR_AREA, 0, 0, 0, "Path",       (void *)  32, file_path,        512,  512},
                                  {_CHR_AREA, 0, 0, 0, "Kind",       (void *) 544, file_kind,         32,   32},
                                  {_CHR_AREA, 0, 0, 0, "UUID",       (void *) 576, file_uuid,         32,   32}, 
                                  {_CHR_AREA, 0, 0, 0, "Remark",     (void *) 608, file_remark,     1024, 1024}, 
                                  {_CHR_AREA, 0, 0, 0, "Recipients", (void *)1632, file_recipients, 1024, 1024} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_file", 2656, rec_data, 6} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt      <  5  ||                                         /* Проверяем число параметров */
      pars[0]->addr==NULL ||
      pars[1]->addr==NULL ||
      pars[2]->addr==NULL ||
      pars[3]->addr==NULL ||
      pars[4]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                              }

                  memset(file_path,       0, sizeof(file_path      )) ;
                  memset(file_kind,       0, sizeof(file_kind      )) ;
                  memset(file_remark,     0, sizeof(file_remark    )) ;
                  memset(file_recipients, 0, sizeof(file_recipients)) ;
                  memset(file_uuid,       0, sizeof(file_uuid      )) ;

    if(pars[0]->size>=sizeof(file_path))
                  memcpy(file_path, pars[0]->addr, sizeof(file_path)-1) ;
    else          memcpy(file_path, pars[0]->addr, pars[0]->size) ;

    if(pars[1]->size>=sizeof(file_kind))
                  memcpy(file_kind, pars[1]->addr, sizeof(file_kind)-1) ;
    else          memcpy(file_kind, pars[1]->addr, pars[1]->size) ;

    if(pars[2]->size>=sizeof(file_remark))
                  memcpy(file_remark, pars[2]->addr, sizeof(file_remark)-1) ;
    else          memcpy(file_remark, pars[2]->addr, pars[2]->size) ;

    if(pars[3]->size>=sizeof(file_recipients))
                  memcpy(file_recipients, pars[3]->addr, sizeof(file_recipients)-1) ;
    else          memcpy(file_recipients, pars[3]->addr, pars[3]->size) ;

    if(pars[4]->size>=sizeof(file_uuid))
                  memcpy(file_uuid, pars[4]->addr, sizeof(file_uuid)-1) ;
    else          memcpy(file_uuid, pars[4]->addr, pars[4]->size) ;

/*-------------------------------------------------- Создание сделки */

                            strcpy(rec_type, "deal_file") ;

           rec_data[0].size=strlen(rec_data[0].prototype) ;
           rec_data[1].size=strlen(rec_data[1].prototype) ;
           rec_data[2].size=strlen(rec_data[2].prototype) ;
           rec_data[3].size=strlen(rec_data[3].prototype) ;
           rec_data[4].size=strlen(rec_data[4].prototype) ;
           rec_data[5].size=strlen(rec_data[5].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status)  dgt_value=-1 ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                      Опубликование сделки                         */

   Dcl_decl *EMIR_dcl_DeployDeal(Lang_DCL  *dcl_kernel,
                                 Dcl_decl  *source, 
                                 Dcl_decl **pars, 
                                      int   pars_cnt)

{
  Dcl_complex_record *record ;	
            Dcl_decl *uuid ;
                char  rec_type[32] ;
                char *deal_kind ;
                char *deal_uuid ;
                char *deal_data ;
           DealParty  parties[_DEAL_PARTIES_MAX] ;
                 int  parties_cnt ;
             DealMap  map[_DEAL_MAP_MAX] ;
                 int  map_cnt ;
            DealAttr  attr[_DEAL_ATTR_MAX] ;
                 int  attr_cnt ;
                 int  i ;
                 int  k ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

#define  ELEM   record->elems

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt!= 0) {                                               /* Проверяем число параметров */
                        dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                              return(&dgt_return) ; 
                    }
/*---------------------------------- Инициализация параметров сделки */

                 deal_kind =NULL ;
                 deal_uuid =NULL ;
                 deal_data =NULL ;

                parties_cnt= 0 ;
                    map_cnt= 0 ;
                   attr_cnt= 0 ;

/*------------------------------------- Извлечение параметров сделки */

     for(record=(Dcl_complex_record *)source->addr,                 /* LOOP - Перебираем записи -        */
                        i=0 ; i<source->buff ; i++,                 /*         ищем запись с типом EMAIL */
         record=(Dcl_complex_record *)record->next_record) {
/*- - - - - - - - - - - - - - - - - - - - - - - Идентификация записи */
       for(k=0 ; k<record->elems_cnt ; k++)                         /* Ищем поле $type$ */
         if(!stricmp(ELEM[k].name, "$type$"))  break ;

         if(k>=record->elems_cnt)  continue ;                       /* Если такое не найдено... */

             memset(rec_type, 0, sizeof(rec_type)) ;
             memcpy(rec_type, ELEM[k].addr, ELEM[k].size) ;       
/*- - - - - - - - - - - - - - - - - - - - - - Базовые данные сделки */
     if(!stricmp(rec_type, "deal")) {

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Kind" )) {
            if(ELEM[k].size>=ELEM[k].buff)  ELEM[k].size=ELEM[k].buff-1 ;
                 deal_kind              =(char *)ELEM[k].addr ;
                 deal_kind[ELEM[k].size]=         0 ;
                                             }
         else
         if(!stricmp(ELEM[k].name, "UUID" )) {
            if(ELEM[k].size>=ELEM[k].buff)  ELEM[k].size=ELEM[k].buff-1 ;
                 deal_uuid              =(char *)ELEM[k].addr ;
                 deal_uuid[ELEM[k].size]=         0 ;
                      uuid              =&ELEM[k] ;
                                             }
         else
         if(!stricmp(ELEM[k].name, "Data" )) {
            if(ELEM[k].size>=ELEM[k].buff)  ELEM[k].size=ELEM[k].buff-1 ;
                 deal_data              =(char *)ELEM[k].addr ;
                 deal_data[ELEM[k].size]=         0 ;
                                             }

              if(deal_kind==NULL ||
                 deal_uuid==NULL   ) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                   strcpy(dcl_err_details, "Missed fields 'Kind' or 'UUID' in deal context") ; 
                                         dgt_value=-1 ;
                                 return(&dgt_return) ;
                                     }

                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Участник сделки */
     if(!stricmp(rec_type, "deal_party")) {

      if(parties_cnt>=_DEAL_PARTIES_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many parties of deal (>%d)", _DEAL_PARTIES_MAX) ;
                                         dgt_value=-1 ;
                                 return(&dgt_return) ;
                                         }

#define  P  parties[parties_cnt]

              memset(P.party_id, 0, sizeof(P.party_id)) ;
              memset(P.role,     0, sizeof(P.role    )) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "PartyId")) {
              memcpy(P.party_id, ELEM[k].addr, ELEM[k].size) ;
                                               }
         else
         if(!stricmp(ELEM[k].name, "Role"   )) {
              memcpy(P.role, ELEM[k].addr, ELEM[k].size) ;
                                               }

                          parties_cnt++ ;

#undef  P
                                          }
/*- - - - - - - - - - - - - - - - - - - - - -  Карта статусов сделки */
     if(!stricmp(rec_type, "deal_map"  )) {

      if(map_cnt>=_DEAL_MAP_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many elements at deal's status map (>%d)", _DEAL_MAP_MAX) ;
                                           dgt_value=-1 ;
                                   return(&dgt_return) ;
                                 }

#define  M  map[map_cnt]

              memset(M.status,      0, sizeof(M.status     )) ;
              memset(M.status_next, 0, sizeof(M.status_next)) ;
              memset(M.role,        0, sizeof(M.role       )) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Status"    )) {
              memcpy(M.status, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "StatusNext")) {
              memcpy(M.status_next, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "Role"      )) {
            if(ELEM[k].size>=ELEM[k].buff)  ELEM[k].size=ELEM[k].buff-1 ;

              memcpy(M.role, ELEM[k].addr, ELEM[k].size) ;
                                                  }

                          map_cnt++ ;

#undef  M
                                          }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Атрибут сделки */
     if(!stricmp(rec_type, "deal_attribute")) {

      if(attr_cnt>=_DEAL_ATTR_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many attributes of deal (>%d)", _DEAL_ATTR_MAX) ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                                   }

#define  A  attr[attr_cnt]

              memset(A.key,   0, sizeof(A.key  )) ;
              memset(A.value, 0, sizeof(A.value)) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Name" )) {
              memcpy(A.key, ELEM[k].addr, ELEM[k].size) ;
                                             }
         else
         if(!stricmp(ELEM[k].name, "Value")) {
              memcpy(A.value, ELEM[k].addr, ELEM[k].size) ;
                                             }

                          attr_cnt++ ;

#undef  A
                                              }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                                           }
/*-------------------------------------- Проверка обязательных полей */

       if( deal_kind==NULL ||
          *deal_kind==  0    ) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                   strcpy(dcl_err_details, "Property 'Kind' must be specified") ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                               }

       if(parties_cnt==0) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                   strcpy(dcl_err_details, "Parties not specified for deal") ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                          }
/*-------------------------------- Подстановка значений по-умолчанию */

       if(*deal_uuid==  0 ) {
                                  EMIR_uuid_generation(deal_uuid) ;

                                     uuid->size=strlen(deal_uuid) ;
                            }

       if( deal_data==NULL) {
                               deal_data="" ;
                            }
/*----------------- Регистрация публикации сделки в очереди операций */

      if(or_deals_cnt>=_DEALS_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many deals in queue (>%d)", _DEALS_MAX) ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                                   }
/*- - - - - - - - - - - - - - - - - -  Регистрация собственно сделки */
#define  D  or_deals[or_deals_cnt]

              memset(&D, 0, sizeof(D)) ;

             strncpy(D.kind, deal_kind, sizeof(D.kind)-1) ;
             strncpy(D.uuid, deal_uuid, sizeof(D.uuid)-1) ;
             strncpy(D.data, deal_data, sizeof(D.data)-1) ;

                       or_deals_cnt++ ;

#undef   D
/*- - - - - - - - - - - - - - - - - - - - - - Регистрация участников */
#define  P  or_parties[or_parties_cnt]

   for(i=0 ; i<parties_cnt ; i++) {
                                      P     =parties[i] ;
                                      P.flag=or_deals_cnt-1 ;
                                             or_parties_cnt++ ;
                                  }
#undef   P
/*- - - - - - - - - - - - - - - - - - - - Регистрация карты статусов */
#define  M  or_map[or_map_cnt]

   for(i=0 ; i<map_cnt ; i++) {
                                  M     =map[i] ;
                                  M.flag=or_deals_cnt-1 ;
                                         or_map_cnt++ ;
                              }

#undef   M
/*- - - - - - - - - - - - - - - - - - - - - - Регистрация аттрибутов */
#define  A  or_attr[or_attr_cnt]

   for(i=0 ; i<attr_cnt ; i++) {
                                   A     =attr[i] ;
                                   A.flag=or_deals_cnt-1 ;
                                          or_attr_cnt++ ;
                               }

#undef   A
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*-------------------------------------------------------------------*/

#undef  ELEM

  return(&dgt_return) ;

}


/*********************************************************************/
/*                                                                   */
/*                 Изменение состояния сделки                        */

   Dcl_decl *EMIR_dcl_SetDealState(Lang_DCL  *dcl_kernel,
                                   Dcl_decl  *source, 
                                   Dcl_decl **pars, 
                                        int   pars_cnt)

{
  Dcl_complex_record *record ;
                char  rec_type[32] ;
                char  deal_status[64] ;
                char  deal_remark[1024] ;
                char  deal_data[16008] ;
                Deal  deal ;
            DealFile  files[_DEAL_FILES_MAX] ;
                 int  files_cnt ;
            DealAttr  attr[_DEAL_ATTR_MAX] ;
                 int  attr_cnt ;
                 int  i ;
                 int  k ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

#define  ELEM   record->elems

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

   if(pars_cnt      <  3  ||                                         /* Проверяем число параметров */
      pars[0]->addr==NULL ||
      pars[1]->addr==NULL ||
      pars[2]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                              }

                  memset(deal_status, 0, sizeof(deal_status)) ;
                  memset(deal_remark, 0, sizeof(deal_remark)) ;
                  memset(deal_data,   0, sizeof(deal_data  )) ;

    if(pars[0]->size>=sizeof(deal_status))
                  memcpy(deal_status, pars[0]->addr, sizeof(deal_status)-1) ;
    else          memcpy(deal_status, pars[0]->addr, pars[0]->size) ;

    if(pars[1]->size>=sizeof(deal_remark))
                  memcpy(deal_remark, pars[1]->addr, sizeof(deal_remark)-1) ;
    else          memcpy(deal_remark, pars[1]->addr, pars[1]->size) ;

    if(pars[2]->size>=sizeof(deal_data))
                  memcpy(deal_data, pars[2]->addr, sizeof(deal_data)-1) ;
    else          memcpy(deal_data, pars[2]->addr, pars[2]->size) ;

/*---------------------------------- Инициализация параметров сделки */

                  files_cnt= 0 ;
                   attr_cnt= 0 ;

/*------------------------------------- Извлечение параметров сделки */

     for(record=(Dcl_complex_record *)source->addr,                 /* LOOP - Перебираем записи -        */
                        i=0 ; i<source->buff ; i++,                 /*         ищем запись с типом EMAIL */
         record=(Dcl_complex_record *)record->next_record) {
/*- - - - - - - - - - - - - - - - - - - - - - - Идентификация записи */
       for(k=0 ; k<record->elems_cnt ; k++)                         /* Ищем поле $type$ */
         if(!stricmp(ELEM[k].name, "$type$"))  break ;

         if(k>=record->elems_cnt)  continue ;                       /* Если такое не найдено... */

             memset(rec_type, 0, sizeof(rec_type)) ;
             memcpy(rec_type, ELEM[k].addr, ELEM[k].size) ;       
/*- - - - - - - - - - - - - - - - - - - - - - Базовые данные сделки */
     if(!stricmp(rec_type, "deal")) {

              memset(&deal, 0, sizeof(deal)) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Address")) {
              memcpy(deal.address, ELEM[k].addr, ELEM[k].size) ;
                                               }
         else
         if(!stricmp(ELEM[k].name, "UUID")) {
              memcpy(deal.uuid, ELEM[k].addr, ELEM[k].size) ;
                                            }
                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Файл сделки */
     if(!stricmp(rec_type, "deal_file")) {

      if(files_cnt>=_DEAL_PARTIES_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many files of deal (>%d)", _DEAL_FILES_MAX) ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                                      }

#define  F  files[files_cnt]

              memset(&F, 0, sizeof(F)) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Path"      )) {
              memcpy(F.local_path, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "Kind"      )) {
              memcpy(F.kind, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "UUID"      )) {
              memcpy(F.file_uuid, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "Remark"    )) {
              memcpy(F.remark, ELEM[k].addr, ELEM[k].size) ;
                                                  }
         else
         if(!stricmp(ELEM[k].name, "Recipients")) {
              memcpy(F.recipients, ELEM[k].addr, ELEM[k].size) ;
                                                  }

       if(*F.file_uuid==0)  EMIR_uuid_generation(F.file_uuid) ;      /* Подстановка значений по-умолчанию */

                          files_cnt++ ;

#undef  F
                                          }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Атрибут сделки */
     if(!stricmp(rec_type, "deal_attribute")) {

      if(attr_cnt>=_DEAL_ATTR_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many attributes of deal (>%d)", _DEAL_ATTR_MAX) ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                                   }

#define  A  attr[attr_cnt]

              memset(A.key,   0, sizeof(A.key  )) ;
              memset(A.value, 0, sizeof(A.value)) ;

       for(k=0 ; k<record->elems_cnt ; k++)
         if(!stricmp(ELEM[k].name, "Name" )) {
              memcpy(A.key, ELEM[k].addr, ELEM[k].size) ;
                                             }
         else
         if(!stricmp(ELEM[k].name, "Value")) {
              memcpy(A.value, ELEM[k].addr, ELEM[k].size) ;
                                             }

                          attr_cnt++ ;

#undef  A
                                              }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                                           }
/*------------------ Регистрация изменения сделки в очереди операций */

      if(or_deals_cnt>=_DEALS_MAX) {
                          dcl_errno=_DCLE_USER_DEFINED ;
                  sprintf(dcl_err_details, "Too many deals in queue (>%d)", _DEALS_MAX) ;
                                             dgt_value=-1 ;
                                     return(&dgt_return) ;
                                   }
/*- - - - - - - - - - - - - - - - - -  Регистрация собственно сделки */
#define  D  or_deals[or_deals_cnt]

                                 D=deal ;

                         strncpy(D.status,  deal_status, sizeof(D.status)-1) ;
                         strncpy(D.remark,  deal_remark, sizeof(D.remark)-1) ;
                         strncpy(D.data,    deal_data,   sizeof(D.data  )-1) ;

     if(D.address[0]==0)  strcpy(D.address, "N") ;

                       or_deals_cnt++ ;

#undef   D
/*- - - - - - - - - - - - - - - - - - - - - - Регистрация участников */
#define  F  or_files[or_files_cnt]

   for(i=0 ; i<files_cnt ; i++) {
                                      F     =files[i] ;
                                      F.flag=or_deals_cnt-1 ;
                                             or_files_cnt++ ;
                                }
#undef   F
/*- - - - - - - - - - - - - - - - - - - - - - Регистрация аттрибутов */
#define  A  or_attr[or_attr_cnt]

   for(i=0 ; i<attr_cnt ; i++) {
                                   A     =attr[i] ;
                                   A.flag=or_deals_cnt-1 ;
                                          or_attr_cnt++ ;
                               }

#undef   A
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*-------------------------------------------------------------------*/

#undef  ELEM

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                Получение параметров сделки                        */

   Dcl_decl *EMIR_dcl_GetDeal(Lang_DCL  *dcl_kernel,
                              Dcl_decl  *source, 
                              Dcl_decl **pars, 
                                   int   pars_cnt)

{
   SQL_cursor *Cursor ;
         char  address[128] ;
         char  rec_type[32] ;
         char  deal_kind[64] ;
         char  deal_uuid[64] ;
         char  deal_data[16008] ;
         char  text[1024] ;
          int  status ;

          Dcl_decl  rec_data[5] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)  0, rec_type,     32,    32},
                                  {_CHR_AREA, 0, 0, 0, "Address", (void *) 32, address,      64,    64},
                                  {_CHR_AREA, 0, 0, 0, "Kind",    (void *) 96, deal_kind,    64,    64},
                                  {_CHR_AREA, 0, 0, 0, "UUID",    (void *)160, deal_uuid,    64,    64},
                                  {_CHR_AREA, 0, 0, 0, "Data",    (void *)224, deal_data, 16008, 16008}
                                 } ;
  Dcl_complex_type  rec_template={ "deal", 16232, rec_data, 5} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

       if(pars_cnt     !=1   ||                                     /* Проверяем число параметров */
	  pars[0]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                                 }

                    memset(address, 0, sizeof(address)) ;           /* Извлекаем адрес контракта сделки */
        if(pars[0]->size>=sizeof(address))
                    memcpy(address, pars[0]->addr, sizeof(address)-1) ;
        else        memcpy(address, pars[0]->addr, pars[0]->size) ;

/*---------------------------------------------------- Запрос данных */

        Cursor=dcl_sql_connect->LockCursor("EMIR_dcl_GetDeal") ;
     if(Cursor==NULL) {
                        sprintf(dcl_err_details, "GetDeal - Cursor allocation error") ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;
                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                      }

                      sprintf(text, "select d.\"Kind\", d.\"DealsUUID\" , d.\"Data\" "
                                    "from   %s d "
                                    "where  d.\"BlockChainId\"='%s' ",
                                       __db_table_deals, address) ;

        status=dcl_sql_connect->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(dcl_err_details, "GetDeal - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

   do {

        status=dcl_sql_connect->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(dcl_err_details, "GetAttributes - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

           strcpy( rec_type, "deal_attribute") ;

           strncpy(deal_kind, (char *)Cursor->columns[0].value, sizeof(deal_kind)-1) ;
           strncpy(deal_uuid, (char *)Cursor->columns[1].value, sizeof(deal_uuid)-1) ;
           strncpy(deal_data, (char *)Cursor->columns[2].value, sizeof(deal_data)-1) ;

                    rec_data[0].size=strlen(rec_data[0].prototype) ;
                    rec_data[1].size=strlen(rec_data[1].prototype) ;
                    rec_data[2].size=strlen(rec_data[2].prototype) ;
                    rec_data[3].size=strlen(rec_data[3].prototype) ;
                    rec_data[3].size=strlen(rec_data[4].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status) break ;

                                 dgt_value++ ;

      } while(1) ;

                  dcl_sql_connect->SelectClose (Cursor) ;
                  dcl_sql_connect->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*              Получение списка атрибутов сделки                    */

   Dcl_decl *EMIR_dcl_GetAttributes(Lang_DCL  *dcl_kernel,
                                    Dcl_decl  *source, 
                                    Dcl_decl **pars, 
                                         int   pars_cnt)

{
   SQL_cursor *Cursor ;
         char  address[128] ;
         char  key_mask[128] ;
         char  rec_type[32] ;
         char  attr_key[64] ;
         char  attr_value[64] ;
         char  text[1024] ;
          int  status ;

          Dcl_decl  rec_data[3] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$", (void *)  0, rec_type,   32, 32},
                                  {_CHR_AREA, 0, 0, 0, "Name",   (void *) 32, attr_key,   64, 64},
                                  {_CHR_AREA, 0, 0, 0, "Value",  (void *) 96, attr_value, 64, 64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_attribute", 160, rec_data, 3} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

       if(pars_cnt     !=2   ||                                     /* Проверяем число параметров */
	  pars[0]->addr==NULL ||
	  pars[1]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                                 }

                    memset(address, 0, sizeof(address)) ;           /* Извлекаем адрес контракта сделки */
        if(pars[0]->size>=sizeof(address))
                    memcpy(address, pars[0]->addr, sizeof(address)-1) ;
        else        memcpy(address, pars[0]->addr, pars[0]->size) ;

                    memset(key_mask, 0, sizeof(key_mask)) ;         /* Извлекаем маску имен аттрибутов */
        if(pars[1]->size>=sizeof(key_mask))
                    memcpy(key_mask, pars[1]->addr, sizeof(key_mask)-1) ;
        else        memcpy(key_mask, pars[1]->addr, pars[1]->size) ;


/*---------------------------------------------------- Запрос данных */

        Cursor=dcl_sql_connect->LockCursor("EMIR_dcl_GetAttributes") ;
     if(Cursor==NULL) {
                        sprintf(dcl_err_details, "GetAttributes - Cursor allocation error") ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;
                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                      }

                      sprintf(text, "select a.\"Key\", a.\"Value\" "
                                    "from   %s a left join %s d on a.\"DealId\"=d.\"Id\" "
                                    "where  d.\"BlockChainId\"='%s' " 
                                    " and   a.\"Key\" like '%s'",
                                       __db_table_deals_attributes, __db_table_deals, address, key_mask) ;

        status=dcl_sql_connect->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(dcl_err_details, "GetAttributes - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

   do {

        status=dcl_sql_connect->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(dcl_err_details, "GetAttributes - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

            strcpy(rec_type,   "deal_attribute") ;
           strncpy(attr_key,   (char *)Cursor->columns[0].value, sizeof(attr_key  )-1) ;
           strncpy(attr_value, (char *)Cursor->columns[1].value, sizeof(attr_value)-1) ;

                    rec_data[0].size=strlen(rec_data[0].prototype) ;
                    rec_data[1].size=strlen(rec_data[1].prototype) ;
                    rec_data[2].size=strlen(rec_data[2].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status) break ;

                                 dgt_value++ ;

      } while(1) ;

                  dcl_sql_connect->SelectClose (Cursor) ;
                  dcl_sql_connect->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*              Получение списка файлов сделки                       */

   Dcl_decl *EMIR_dcl_GetFiles(Lang_DCL  *dcl_kernel,
                               Dcl_decl  *source, 
                               Dcl_decl **pars, 
                                    int   pars_cnt)

{
   SQL_cursor *Cursor ;
         char  address[128] ;
         char  vs_mask[128] ;
         char  rec_type[32] ;
         char  file_path[FILENAME_MAX] ;
         char  file_kind[64] ;
         char  file_remark[1024] ;
         char  file_status[64] ;
         char  file_version[128] ;
         char  file_uuid[64] ;
         char  text[1024] ;
         char  where[1024] ;
          int  status ;

          Dcl_decl  rec_data[7] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)   0, rec_type,      32,   32},
                                  {_CHR_AREA, 0, 0, 0, "Path",    (void *)  32, file_path,    512,  512},
                                  {_CHR_AREA, 0, 0, 0, "Kind",    (void *) 544, file_kind,     64,   64},
                                  {_CHR_AREA, 0, 0, 0, "Remark",  (void *) 608, file_remark, 1024, 1024},
                                  {_CHR_AREA, 0, 0, 0, "Status",  (void *)1632, file_status,   64,   64},
                                  {_CHR_AREA, 0, 0, 0, "Version", (void *)1696, file_version, 128,  128},
                                  {_CHR_AREA, 0, 0, 0, "UUID",    (void *)1824, file_uuid,     64,   64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_file", 1888, rec_data, 7} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

       if(pars_cnt     !=2   ||                                     /* Проверяем число параметров */
	  pars[0]->addr==NULL ||
	  pars[1]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                                 }

                    memset(address, 0, sizeof(address)) ;           /* Извлекаем адрес контракта сделки */
        if(pars[0]->size>=sizeof(address))
                    memcpy(address, pars[0]->addr, sizeof(address)-1) ;
        else        memcpy(address, pars[0]->addr, pars[0]->size) ;

                    memset(vs_mask, 0, sizeof(vs_mask)) ;           /* Извлекаем маску отбора версий/статусов */
        if(pars[1]->size>=sizeof(vs_mask))
                    memcpy(vs_mask, pars[1]->addr, sizeof(vs_mask)-1) ;
        else        memcpy(vs_mask, pars[1]->addr, pars[1]->size) ;

/*---------------------------------------------------- Запрос данных */

        Cursor=dcl_sql_connect->LockCursor("EMIR_dcl_GetFiles") ;
     if(Cursor==NULL) {
                        sprintf(dcl_err_details, "GetFiles - Cursor allocation error") ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;
                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                      }

                         memset(where, 0, sizeof(where)) ;
     if(vs_mask[0]!=0)  sprintf(where, " and  (f.\"Status\"='%s' or f.\"Version\"='%s')", vs_mask, vs_mask) ;

                        sprintf(text, "select f.\"LocalPath\", f.\"Kind\", f.\"Remark\", f.\"Status\", f.\"Version\", f.\"FileUUID\" "
                                      "from   %s f left join %s d on f.\"DealId\"=d.\"Id\" "
                                      "where  d.\"BlockChainId\"='%s' %s",
                                         __db_table_deals_files, __db_table_deals, address, where) ;

        status=dcl_sql_connect->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(dcl_err_details, "GetFiles - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

   do {

        status=dcl_sql_connect->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(dcl_err_details, "GetFiles - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

            strcpy(rec_type,      "deal_file") ;
           strncpy(file_path,    (char *)Cursor->columns[0].value, sizeof(file_path   )-1) ;
           strncpy(file_kind,    (char *)Cursor->columns[1].value, sizeof(file_kind   )-1) ;
           strncpy(file_remark,  (char *)Cursor->columns[2].value, sizeof(file_remark )-1) ;
           strncpy(file_status,  (char *)Cursor->columns[3].value, sizeof(file_status )-1) ;
           strncpy(file_version, (char *)Cursor->columns[4].value, sizeof(file_version)-1) ;
           strncpy(file_uuid,    (char *)Cursor->columns[5].value, sizeof(file_uuid   )-1) ;

                    rec_data[0].size=strlen(rec_data[0].prototype) ;
                    rec_data[1].size=strlen(rec_data[1].prototype) ;
                    rec_data[2].size=strlen(rec_data[2].prototype) ;
                    rec_data[3].size=strlen(rec_data[3].prototype) ;
                    rec_data[4].size=strlen(rec_data[4].prototype) ;
                    rec_data[5].size=strlen(rec_data[5].prototype) ;
                    rec_data[6].size=strlen(rec_data[6].prototype) ;

        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status) break ;

                                 dgt_value++ ;

      } while(1) ;

                  dcl_sql_connect->SelectClose (Cursor) ;
                  dcl_sql_connect->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*           Получение истории изменения статусов сделки             */

   Dcl_decl *EMIR_dcl_GetHistory(Lang_DCL  *dcl_kernel,
                                 Dcl_decl  *source, 
                                 Dcl_decl **pars, 
                                      int   pars_cnt)

{
   SQL_cursor *Cursor ;
         char  address[128] ;
         char  rec_type[32] ;
         char  hist_version[128] ;
         char  hist_status[64] ;
         char  hist_remark[1024] ;
         char  hist_actor[64] ;
         char  text[1024] ;
          int  status ;

          Dcl_decl  rec_data[5] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)   0, rec_type,      32,   32},
                                  {_CHR_AREA, 0, 0, 0, "Version", (void *)  32, hist_version, 128,  128},
                                  {_CHR_AREA, 0, 0, 0, "Status",  (void *) 160, hist_status,   64,   64},
                                  {_CHR_AREA, 0, 0, 0, "Remark",  (void *) 224, hist_remark, 1024, 1024},
                                  {_CHR_AREA, 0, 0, 0, "Actor",   (void *)1248, hist_actor,    64,   64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_history", 1312, rec_data, 5} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

       if(pars_cnt    != 1   ||                                     /* Проверяем число параметров */
	  pars[0]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                                 }

                    memset(address, 0, sizeof(address)) ;           /* Извлекаем адрес контракта сделки */
        if(pars[0]->size>=sizeof(address))
                    memcpy(address, pars[0]->addr, sizeof(address)-1) ;
        else        memcpy(address, pars[0]->addr, pars[0]->size) ;

/*---------------------------------------------------- Запрос данных */

        Cursor=dcl_sql_connect->LockCursor("EMIR_dcl_GetHistory") ;
     if(Cursor==NULL) {
                        sprintf(dcl_err_details, "GetHistory - Cursor allocation error") ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;
                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                      }

                        sprintf(text, "select h.\"Version\", h.\"Status\", h.\"Remark\", m.\"Key\" "
                                      "from   %s h left join %s d on h.\"DealId\"=d.\"Id\" "
                                                  "left join %s m on h.\"ActorId\"=m.\"Account\" "
                                      "where  d.\"BlockChainId\"='%s' "
                                      "order by h.\"Version\"",
                                         __db_table_deals_history, __db_table_deals, __db_table_members, address) ;

        status=dcl_sql_connect->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(dcl_err_details, "GetHistory - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

   do {

        status=dcl_sql_connect->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(dcl_err_details, "GetHistory - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

            strcpy(rec_type,      "deal_history") ;
           strncpy(hist_version, (char *)Cursor->columns[0].value, sizeof(hist_version)-1) ;
           strncpy(hist_status,  (char *)Cursor->columns[1].value, sizeof(hist_status )-1) ;
           strncpy(hist_remark,  (char *)Cursor->columns[2].value, sizeof(hist_remark )-1) ;
           strncpy(hist_actor,   (char *)Cursor->columns[3].value, sizeof(hist_actor  )-1) ;

                    rec_data[0].size=strlen(rec_data[0].prototype) ;
                    rec_data[1].size=strlen(rec_data[1].prototype) ;
                    rec_data[2].size=strlen(rec_data[2].prototype) ;
                    rec_data[3].size=strlen(rec_data[3].prototype) ;
                    rec_data[4].size=strlen(rec_data[4].prototype) ;
        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status) break ;

                                 dgt_value++ ;

      } while(1) ;

                  dcl_sql_connect->SelectClose (Cursor) ;
                  dcl_sql_connect->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*              Получение списка участников сделки                   */

   Dcl_decl *EMIR_dcl_GetParty(Lang_DCL  *dcl_kernel,
                               Dcl_decl  *source, 
                               Dcl_decl **pars, 
                                    int   pars_cnt)

{
   SQL_cursor *Cursor ;
         char  address[128] ;
         char  rec_type[32] ;
         char  party_id[64] ;
         char  party_role[64] ;
         char  text[1024] ;
          int  status ;

          Dcl_decl  rec_data[3] ={
                                  {_CHR_AREA, 0, 0, 0, "$type$",  (void *)   0, rec_type,   32, 32},
                                  {_CHR_AREA, 0, 0, 0, "PartyId", (void *)  32, party_id,   64, 64},
                                  {_CHR_AREA, 0, 0, 0, "Role",    (void *)  96, party_role, 64, 64} 
                                 } ;
  Dcl_complex_type  rec_template={ "deal_party", 160, rec_data, 3} ;

 static   double  dgt_value ;          /* Буфер числового значения */
 static Dcl_decl  dgt_return={ _DGT_VAL, 0,0,0,"", &dgt_value, NULL, 1, 1} ;

/*---------------------------------------------------- Инициализация */

                              dgt_value=0 ;

/*-------------------------------------------- Извлечение параметров */

       if(pars_cnt    != 1   ||                                     /* Проверяем число параметров */
	  pars[0]->addr==NULL   ) {
                                    dcl_kernel->mError_code=_DCLE_PROTOTYPE ;
                                      return(&dgt_return) ; 
                                 }

                    memset(address, 0, sizeof(address)) ;           /* Извлекаем адрес контракта сделки */
        if(pars[0]->size>=sizeof(address))
                    memcpy(address, pars[0]->addr, sizeof(address)-1) ;
        else        memcpy(address, pars[0]->addr, pars[0]->size) ;

/*---------------------------------------------------- Запрос данных */

        Cursor=dcl_sql_connect->LockCursor("EMIR_dcl_GetParty") ;
     if(Cursor==NULL) {
                        sprintf(dcl_err_details, "GetParty - Cursor allocation error") ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;
                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                      }

                        sprintf(text, "select p.\"PartyId\", p.\"Role\" "
                                      "from   %s p left join %s d on p.\"DealId\"=d.\"Id\" "
                                      "where  d.\"BlockChainId\"='%s' ",
                                         __db_table_deals_parties, __db_table_deals, address) ;

        status=dcl_sql_connect->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(dcl_err_details, "GetParty - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

   do {

        status=dcl_sql_connect->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(dcl_err_details, "GetParty - Select error: %s", dcl_sql_connect->error_text) ;
                         strcpy(dcl_kernel->mError_details, dcl_err_details) ;

                            dcl_sql_connect->SelectClose (Cursor) ;
                            dcl_sql_connect->UnlockCursor(Cursor) ;

                                 dgt_value=-1 ;
                         return(&dgt_return) ;
                }

            strcpy(rec_type,   "deal_party") ;
           strncpy(party_id,   (char *)Cursor->columns[0].value, sizeof(party_id  )-1) ;
           strncpy(party_role, (char *)Cursor->columns[1].value, sizeof(party_role)-1) ;

                    rec_data[0].size=strlen(rec_data[0].prototype) ;
                    rec_data[1].size=strlen(rec_data[1].prototype) ;
                    rec_data[2].size=strlen(rec_data[2].prototype) ;
        status=dcl_kernel->iXobject_add(source, &rec_template) ;
     if(status) break ;

                                 dgt_value++ ;

      } while(1) ;

                  dcl_sql_connect->SelectClose (Cursor) ;
                  dcl_sql_connect->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(&dgt_return) ;
}


/*********************************************************************/
/*                                                                   */
/*                        Отладочная функция                         */

   int EMIR_dcl_debug(void)

{
          EMIR_log("Debug") ;

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                   Создание новых сделок                           */

   int  EMIRi_or_DeployDeals(SQL_link *db, char *error)

{
          SQL_cursor *Cursor ;
                char  deal_id[64] ;
                char  data[1024] ;
                char  value[128] ;
                char  text[17000] ;
                 int  status ;
                 int  n ;
                 int  i ;

/*---------------------------------------------------- Инициализация */

                    Cursor=NULL ;

/*--------------------------------------------------- Перебор сделок */

    for(n=0 ; n<or_deals_cnt ; n++) {

#define  D  or_deals[n]

              if(D.address[0]!=0)  continue ;                       /* Только для новых сделок */

/*------------------------------------------------ Выделение курсора */

        Cursor=db->LockCursor("DeployDeals") ;
     if(Cursor==NULL) {
                        sprintf(error, "DeployDeals - Cursor allocation error") ;
                          return(-1) ;
                      }
/*---------------------------------- Создание сделки в таблице DEALS */

                      sprintf(text, "Insert into %s (\"Kind\",\"DealsUUID\",\"Data\")"
                                    "Values('%s','%s','%s')",
                                       __db_table_deals, D.kind, D.uuid, D.data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "DeployDeals - Deal creation error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;

                               return(-1) ;
                }
/*---------------------------------- Получение идентификатора сделки */

                      sprintf(text, "select max(\"Id\") "
                                    "from   %s  "
                                    "where  \"DealsUUID\"='%s' ",
                                       __db_table_deals, D.uuid) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "DeployDeals - Deal's ID select error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;

                                   return(-1) ;
                }

        status=dcl_sql_connect->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(error, "DeployDeals - Deal's ID select error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;

                                   return(-1) ;
                }

            memset(deal_id, 0, sizeof(deal_id)) ;
           strncpy(deal_id, (char *)Cursor->columns[0].value, sizeof(deal_id)-1) ;

                  dcl_sql_connect->SelectClose(Cursor) ;

/*--------------------- Занесение участников в таблицу DEALS_PARTIES */

   for(i=0 ; i<or_parties_cnt ; i++) 
     if(or_parties[i].flag==n) {

                      sprintf(text, "Insert into %s (\"DealId\",\"PartyId\",\"Role\")"
                                    "Values(%s,'%s','%s')",
                                       __db_table_deals_parties, deal_id, or_parties[i].party_id, or_parties[i].role) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "DeployDeals - Deal's party insert error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                }
                               }
/*--------------- Занесение карты статусов в таблицу DEALS_STATUSMAP */

   for(i=0 ; i<or_map_cnt ; i++) 
     if(or_map[i].flag==n) {

                      sprintf(text, "Insert into %s (\"DealId\",\"Status\",\"StatusNext\",\"Role\")"
                                    "Values(%s,'%s','%s','%s')",
                                       __db_table_deals_statusmap, deal_id, or_map[i].status, or_map[i].status_next, or_map[i].role) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "DeployDeals - Deal's status map insert error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                }
                           }
/*---------------------------------------------- "Публикация" сделки */

             sprintf(data, "{\"Id\":\"%s\"", deal_id) ;             /* Формирование данных для поля Data */

      for(i=0 ; i<or_attr_cnt ; i++) 
        if(or_attr[i].flag==n) {
             sprintf(value, ",\"%s\":\"%s\"", or_attr[i].key, or_attr[i].value) ;
              strcat(data, value) ;
                               }

              strcat(data, "}") ;

                      sprintf(text, "Insert into %s (\"Action\",\"Data\",\"Executor\",\"Status\")"
                                    "Values('AddDeal','%s','%s','NEW')",
                                       __db_table_deals_actions, data, __member_executor) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "DeployDeals - AddDeal publication error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                }
/*--------------------------------------------------- Перебор сделок */

                                    }

/*-------------------------------------------- Освобождение ресурсов */

//                db->Commit      () ;
                  db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(0) ;

}


/*********************************************************************/
/*                                                                   */
/*                   Изменение состояния сделок                      */

   int  EMIRi_or_ChangeDeals(SQL_link *db, char *error, char *master_path)

{
          SQL_cursor *Cursor ;
                char  deal_id[64] ;
                char  data[1024] ;
                char  value[128] ;
                char  text[17000] ;
                char  path[FILENAME_MAX] ;
                char  command[1024] ;
                 int  status ;
                 int  n ;
                 int  i ;

/*---------------------------------------------------- Инициализация */

                    Cursor=NULL ;

/*--------------------------------------------------- Перебор сделок */

    for(n=0 ; n<or_deals_cnt ; n++) {

#define  D  or_deals[n]

              if(D.address[0]==0)  continue ;                       /* Только для существующих сделок */

/*------------------------------------------------ Выделение курсора */

        Cursor=db->LockCursor("ChangeDeals") ;
     if(Cursor==NULL) {
                        sprintf(error, "ChangeDeals - Cursor allocation error") ;
                          return(-1) ;
                      }
/*---------------------------------- Получение идентификатора сделки */

     if(D.address[0]=='N')
                      sprintf(text, "select max(\"Id\") "
                                    "from   %s  "
                                    "where  \"DealsUUID\"='%s' ",
                                      __db_table_deals, D.uuid) ;
     else
                      sprintf(text, "select max(\"Id\") "
                                    "from   %s  "
                                    "where  \"BlockChainId\"='%s' ",
                                      __db_table_deals, D.address) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "ChangeDeals - Deal's ID select error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;

                                   return(-1) ;
                }

        status=dcl_sql_connect->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        sprintf(error, "ChangeDeals - Deal's ID select error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->SelectClose (Cursor) ;
                            db->UnlockCursor(Cursor) ;

                                   return(-1) ;
                }

            memset(deal_id, 0, sizeof(deal_id)) ;
           strncpy(deal_id, (char *)Cursor->columns[0].value, sizeof(deal_id)-1) ;

                  dcl_sql_connect->SelectClose(Cursor) ;

/*------------------------ Изменение поля Data в таблице DEALS */

   if(D.data[0]!=0) {

                      sprintf(text, "update %s "
                                    "set    \"Data\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals, D.data, deal_id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->Rollback() ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                       return(-1) ;
                }

                    }
/*--------------------- Занесение файлов в таблицу DEALS_FILES */

   for(i=0 ; i<or_files_cnt ; i++) 
     if(or_files[i].flag==n) {
/*- - - - - - - - - - - - Сохранение файлов файловом хранилище */
       if(!strcmp(master_path, or_files[i].local_path)) {

            status=EMIRi_or_GetStoragePath(master_path, path, error) ;
         if(status) {
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                    }

#ifdef UNIX
                  sprintf(command, "cp \"%s\" \"%s\"", master_path, path) ;
            status=system(command) ;
         if(status) {
                        sprintf(error, "File copy to storage error (status=%d errno=%d)", status, errno) ;
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                    }
#else
                        sprintf(error, "File copy not implemented for Windows") ;
                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
#endif
              
             strcpy(or_files[i].local_path, path) ;

                                                        }
/*- - - - - - - - - - - - - - - -  Собственно занесение файлов */
                      sprintf(text, "Insert into %s (\"DealId\",\"Status\",\"Kind\",\"LocalPath\",\"Remark\",\"Recipients\",\"FileUUID\")"
                                    "Values(%s,'%s','%s','%s','%s','%s','%s')",
                                       __db_table_deals_files, deal_id, D.status,
                                                  or_files[i].kind,   or_files[i].local_path,
                                                  or_files[i].remark, or_files[i].recipients, or_files[i].file_uuid) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                        sprintf(error, "ChangeDeals - Deal's file insert error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                }
                             }
/*------------------------------------ "Публикация" изменения сделки */

             sprintf(data, "{\"Id\":\"%s\","                        /* Формирование данных для поля Data */
                           "\"Status\":\"%s\","
                           "\"Remark\":\"%s\"",
                               deal_id, D.status, D.remark) ;

        if(D.data[0]!=0)
              strcat(data, ",\"Data\":\"Y\"") ;

      for(i=0 ; i<or_attr_cnt ; i++) 
        if(or_attr[i].flag==n) {
             sprintf(value, ",\"%s\":\"%s\"", or_attr[i].key, or_attr[i].value) ;
              strcat(data, value) ;
                               }

              strcat(data, "}") ;

                      sprintf(text, "Insert into %s (\"Action\",\"Data\",\"Executor\",\"Status\")"
                                    "Values('SetStatus','%s','%s','NEW')",
                                       __db_table_deals_actions, data, __member_executor) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "ChangeDeals - SetStatus publication error: %s", dcl_sql_connect->error_text) ;

                            db->Rollback() ;
                            db->UnlockCursor(Cursor) ;
                                   return(-1) ;
                }
/*--------------------------------------------------- Перебор сделок */

                                    }

/*-------------------------------------------- Освобождение ресурсов */

   if(Cursor!=NULL) {
//                     db->Commit      () ;
                       db->UnlockCursor(Cursor) ;
                    }

/*-------------------------------------------------------------------*/

  return(0) ;

}


/*********************************************************************/
/*                                                                   */
/*                 Формирование пути сохранения файла                */

   int  EMIRi_or_GetStoragePath(char *ext_path, char *path, char *error)

{
  static unsigned long  cnt ;
                time_t  time_abs ;
             struct tm *hhmmss ;
                  char  folder[FILENAME_MAX] ;
                  char  name[FILENAME_MAX] ;
                   int  status ;
                  char *tmp ;

/*-------------------------------------------- Выделение имени файла */

                      cnt++ ;

#ifdef UNIX
              tmp=strrchr(ext_path, '/') ;
#else
              tmp=strrchr(ext_path, '\\') ;
#endif

           if(tmp!=NULL)  tmp++ ;
           else           tmp=ext_path ;  

             memset(name, 0, sizeof(name)) ;
            strncpy(name, tmp, sizeof(name)-1) ;

/*------------------------------------- Формирование папки хранилища */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

     sprintf(folder, "%02d-%02d-%02d",
                                 hhmmss->tm_mday,
                                 hhmmss->tm_mon+1,
                                 hhmmss->tm_year-100 ) ;

#ifdef UNIX
      sprintf(path, "%s/%s", __file_storage, folder) ;

   if(access(path, 0x00)) {

         status=mkdir(path, 0777) ;
      if(status) {
             sprintf(error, "Folder creation error %d: %s", errno, path) ;
            EMIR_log(error, __rpc_path) ;
                       return(-1) ;
                 }
                          }

#else
      sprintf(path, "%s\\%s", __file_storage, folder) ;

      sprintf(error, "File copy not implemented for Windows") ;
                       return(-1) ;
#endif

/*------------------------------------ Формирование пути в хранилище */

      for(tmp=name ; *tmp ; tmp++)
        if(*tmp==' ')  *tmp='_' ;

#ifdef UNIX
        sprintf(path, "%s/%s/%lx-%lx_%s", __file_storage, folder, time_abs, cnt, name) ;
#else
        sprintf(path, "%s\\%s\\%lx-%lx_%s", __file_storage, folder, time_abs, cnt, name) ;
#endif

/*-------------------------------------------------------------------*/

  return(0) ;
}
