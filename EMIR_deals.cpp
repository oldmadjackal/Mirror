/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                          Модуль "Сделки"                          */
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

#define    _PARTY_MAX    20
#define     _ATTR_MAX    50
#define    _FILES_MAX   500
#define      _MAP_MAX   100
#define  _HISTORY_MAX  1000

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

  void  EMIRi_dl_synch          (HWND hDlg, char *prefix, SQL_link *db) ;               /* Процедура фоновой синхронизации объектов модуля "Сделки" */
  void  EMIRi_dl_actions        (HWND hDlg, char *prefix, SQL_link *db) ;               /* Обработка очереди операций модуля "Сделки" */
   int  EMIR_dl_action_AddDeal  (Dl_action *action, SQL_link *db, char *error) ;        /* Операция ADD_DEAL */
   int  EMIR_dl_action_SetStatus(Dl_action *action, SQL_link *db, char *error) ;        /* Операция SET_STATUS */
   int  EMIR_dl_action_StartArb (Dl_action *action, SQL_link *db, char *error) ;        /* Операция START_ARBITRATION */
   int  EMIR_dl_action_AcceptArb(Dl_action *action, SQL_link *db, char *error) ;        /* Операция ACCEPT_ARBITRATION */
   int  EMIR_dl_action_RejectArb(Dl_action *action, SQL_link *db, char *error) ;        /* Операция REJECT_ARBITRATION */
   int  EMIR_dl_action_GetDeal  (Dl_action *action, SQL_link *db, char *error) ;        /* Операция GET_DEAL */
   int  EMIR_dl_action_GetArb   (Dl_action *action, SQL_link *db, char *error) ;        /* Операция GET_ARBITRATION */
   int  EMIRi_dl_GetDeal        (char *contract, Deal *deal,                            /* Запрос методов Get... смарт-контракта Deal... */
                                             DealAttr *attr,
                                                  int  attr_max,
                                                  int *attr_cnt,
                                            DealParty *parties,
                                                  int  parties_max,
                                                  int *parties_cnt,
                                              DealMap *map,
                                                  int  map_max,
                                                  int *map_cnt,
                                             DealFile *files,
                                                  int  files_max,
                                                  int *files_cnt,
                                          DealHistory *history,
                                                  int  history_max,
                                                  int *history_cnt, char *error) ;
   int  EMIRi_dl_GetArb         (char *contract,                                        /* Запрос методов Get... смарт-контракта Arbitration... */
                                          Arbitration *arbitration,
                                             DealAttr *attr,
                                                  int  attr_max,
                                                  int *attr_cnt,
                                            DealParty *parties,
                                                  int  parties_max,
                                                  int *parties_cnt,
                                              DealMap *map,
                                                  int  map_max,
                                                  int *map_cnt,
                                             DealFile *files,
                                                  int  files_max,
                                                  int *files_cnt,
                                          DealHistory *history,
                                                  int  history_max,
                                                  int *history_cnt, char *error) ;
   int  EMIRi_dl_CheckBox       (char *box, char *contract, char *error) ;              /* Запрос методов CheckContract смарт-контракта Box */
   int  EMIRi_dl_GetBox         (char *contract, Deal **list, char *error) ;            /* Запрос методa GetContracts смарт-контракта Box */
   int  EMIRi_dl_AccessBox      (char *box, char *contract, char *error) ;              /* Запрос методов CheckBank смарт-контракта Box */

   int  EMIRi_dl_GetDealId      (Dl_action *action) ;                                   /* Извлечение номера сделки из операции */


/********************************************************************/
/*                                                                  */
/*           Обработчик фонового потока модуля "Сделки"             */

   void  Deals_Process(SQL_link *DB)

{
   static time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;


#pragma warning(disable : 4244)

/*---------------------------------------------------- Инициализация */

      if(__net_locked)  return ;
      if( __db_locked)  return ;

      if(__dl_request_period<=0)  __dl_request_period= 10 ;
      if(__dl_view_frame    <=0)  __dl_view_frame    =100 ;

/*------------------------------------------------------- Общий цикл */

       do {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Тайминг */
               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%02d.%02d %02d:%02d:%02d ",
                                    hhmmss->tm_mday,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec  ) ;

            sprintf(text, "  Deals> %s (LVL.1) Pulse start", prefix) ;
           EMIR_log(text) ;
/*- - - - - - - - - - - - - - - - - - - - Обработка очереди операций */
             EMIRi_dl_actions(hDeals_Dialog, prefix, DB) ;
/*- - - - - - - - - - - - - - - - -  Процедура фоновой синхронизации */
          if(time_abs-time_0 > __dl_request_period) {               /* Если пауза не истекла... */

               EMIRi_dl_synch(hDeals_Dialog, prefix, DB) ;

                           time_0=time_abs ;
                                                    }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

            sprintf(text, "  Deals> %s - (LVL.1) Pulse stop", prefix) ;
           EMIR_log(text) ;

/*-------------------------------------------------------------------*/
                                    
  return ;

#pragma warning(default : 4244)
}


/********************************************************************/
/*                                                                  */
/*              Процедура фоновой синхронизации объектов            */
/*                     модуля "Сделки"                              */

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

/*--------------------------------------------------- Захват курсора */

        Cursor=db->LockCursor("EMIRi_dl_synch") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Cursor lock: %s", prefix, db->error_text) ;
                        LB_ADD_ROW(IDC_LOG, text) ;
                            db->error_text[0]=0 ;
                          __db_errors_cnt++ ;
                                return ;
                      }
/*------------------------------- Проверка наличия активных операций */

  if(!__events_force) {                                             /* Если не включен режим "форсирования" событий */

                      sprintf(text, "select count(*) "
                                    "from   %s "
                                    "where  \"Status\" not in ('DONE','HOLD') and (\"Error\"='' or \"Error\" is null)",
                                     __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Check active operations: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Check active operations: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;

     if(result[0]!='0') {                                           /* Если есть активные операции... */
                                 sprintf(text, "Active operations is detected...") ;
                              LB_ADD_ROW(IDC_LOG, text) ;
                            db->UnlockCursor(Cursor) ;
                                   return ;
                        }

                      }

/*--------------------------------- Запрос адреса СК "Почтовый ящик" */

                      sprintf(text, "select \"MemberBox\" "
                                    "from   %s", __db_table_system_pars) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get 'MemberBox': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get 'MemberBox': %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

            memset(box_addr, 0, sizeof(box_addr)) ;
           strncpy(box_addr, (char *)Cursor->columns[0].value, sizeof(box_addr)-1) ;

           sprintf(text, "%s - Box address: %s", prefix, box_addr) ;
        LB_ADD_ROW(IDC_LOG, text) ;

                 db->SelectClose(Cursor) ;

/*----------------------- Формирование списка обновлений по событиям */

   if(EMIR_active_section("EVENTS") ||
      EMIR_active_section("SCAN"  )   ) {
/*- - - - - - - - - - - - - - - - - Запрос событий "Почтового ящика" */
                      sprintf(text, "Select \"Id\", \"Topic\", \"Data\" "
                                    "From   %s "
                                    "Where  \"Address\"='%s' and \"Transaction\"<>'' "
                                    "Order by \"Id\""   ,
                                     __db_table_scan_events, box_addr) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

                box_list_cnt=0 ;
                  clear_flag=0 ;

   for(i=0 ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
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

     if(!memicmp(event_topic, "a226", 4)) {                         /* Добавление контракта */

            strcpy(box_list[i].kind,   "Deal") ;
           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag= 1 ;
                                          }
     else
     if(!memicmp(event_topic, "745d", 4)) {                         /* Очистка п/я или удаление контракта */

       if(!stricmp(event_data, _NULL_ADDR)) {                       /* Очистка п/я */ 
                                               clear_flag=1 ;
                                                 break ;
                                            }
       else                                 {                       /* Удаление контракта */

           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag=-1 ;
                                            }
                                          }

                    }

                 db->SelectClose (Cursor) ;

         box_list_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - -  Очистка "Почтового ящика" */
     if(clear_flag) {

EMIR_log("Box clear") ;

                        sprintf(text, "update %s set \"DataUpdate\"='D'",
                                        __db_table_deals) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Delete All Deals : %s", prefix, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                          db->UnlockCursor(Cursor) ;
                          db->Rollback() ;
                        __db_errors_cnt++ ;
                                 return ;
                  }

                        sprintf(text, "Delete from %s "
                                      "Where  \"Address\"='%s' "
                                      " and   \"Id\"<=%s"   ,
                                     __db_table_scan_events, box_addr, event_id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Delete All Deals : %s", prefix, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                          db->UnlockCursor(Cursor) ;
                          db->Rollback() ;
                        __db_errors_cnt++ ;
                                 return ;
                  }

                        db->Commit() ;
                        db->UnlockCursor(Cursor) ;

                             return ;
                    }
/*- - - - - - - - - - - - - - - - - - - Обработка событий контрактов */
                      sprintf(text, "Select e.\"Id\", \"Address\", \"Topic\" "
                                    "From   %s e, %s d "
                                    "Where  e.\"Address\"=d.\"BlockChainId\" and \"Transaction\"<>'' "
                                    "Order by \"Id\"" ,
                                     __db_table_scan_events, __db_table_deals) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Contracts events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
                }

   for(i=box_list_cnt ; ; ) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Box events : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
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

     if(!memicmp(event_topic, "0b27", 4)) {                         /* Изменение сделки */

EMIR_log("Deal event") ;
            strcpy(box_list[i].kind,   "Deal") ;
           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag= 1 ;
                            i++ ;
                                          }
     else
     if(!memicmp(event_topic, "f5b7", 4)) {                         /* Изменение арбитража */
EMIR_log("Arbitration event") ;

            strcpy(box_list[i].kind,   "Arbitration") ;
           strncpy(box_list[i].id,      event_id,   sizeof(box_list[i].id     )-1) ;
           strncpy(box_list[i].address, event_data, sizeof(box_list[i].address)-1) ;
                   box_list[i].flag= 1 ;
                            i++ ;
                                          }
     else                                 {
EMIR_log("Undefined event") ;
                                          }

                            }

                 db->SelectClose (Cursor) ;

         box_list_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*---- Формирование списка обновлений по состоянию "Почтового ящика" */

   else                              {
/*- - - - - - - - - - - - - - - Запрос содержимого "Почтового ящика" */
        box_list_cnt=EMIRi_dl_GetBox(box_addr, &box_list, error) ;
     if(box_list_cnt<0) {
                           snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - GetContracts : %s", prefix, error) ;
                         LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                        }

      for(i=0 ; i<box_list_cnt ; i++) {

            status=EMIRi_dl_GetDeal(box_list[i].address, &box_list[i], NULL, 0, NULL, 
                                                                       NULL, 0, NULL,
                                                                       NULL, 0, NULL,
                                                                       NULL, 0, NULL,
                                                                       NULL, 0, NULL, error) ;
         if(status<0) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Contract status : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                    continue ;
                      }

                         sprintf(text, "(LVL.1) Deal %s - %s : %s", box_list[i].address, box_list[i].status, box_list[i].version) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                                      }
/*- - - - - - - - - - - - - - - - - - - - - -  Запрос таблицы Сделок */
                      sprintf(text, "Select \"Id\", \"BlockChainId\", \"Version\" "
                                    "From   %s "
                                    "Where  \"BlockChainId\" is not null and \"DataUpdate\"<>'D' ",
                                     __db_table_deals) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Deals list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                              return ;
                }

   for(i=0 ; ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Get Deals list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - - - - - - - Сверка списков рестров */
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
/*- - - - - - - - - - - - - - - - - - - - - Отметка удаляемых сделок */
   for(i=0 ; i<tbl_list_cnt ; i++)
     if(tbl_list[i].flag) {

                        sprintf(text, "Deal %s - DELETE", tbl_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                        sprintf(text, "update %s set \"DataUpdate\"='D' where \"Id\"=%s",
                                        __db_table_deals, tbl_list[i].id) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Delete Deal (Id=%s) : %s", prefix, tbl_list[i].id, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                  }
                            }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                        }
/*------------------------------ Запрос добавления/обновления сделок */

   for(i=0 ; i<box_list_cnt ; i++)
     if(box_list[i].flag==1) {

                        sprintf(text, "%s %s - ADD/UPDATE", box_list[i].kind, box_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
/*- - - - - - - - - - - - - - - - - - Проверка дублирования операции */
                      sprintf(text, "select count(*) "
                                    "from   %s "
                                    "where  \"Object\"='%s'"
                                    " and   \"Status\" not in ('DONE','HOLD') and (\"Error\"='' or \"Error\" is null)",
                                     __db_table_deals_actions, box_list[i].address) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Check operations for %s: %s", prefix, box_list[i].address, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Check operations for %s: %s", prefix, box_list[i].address, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - Регистрация операции добавления/обновления */
       if(!strcmp(box_list[i].kind, "Deal"))
                        sprintf(text, "insert into %s (\"Action\",  \"Object\", \"Status\")"
                                      "         values('GetDeal', '%s',      'NEW')"  ,
                                        __db_table_deals_actions, box_list[i].address) ;
       else
       if(!strcmp(box_list[i].kind, "Arbitration"))
                        sprintf(text, "insert into %s (\"Action\",  \"Object\", \"Status\")"
                                      "         values('GetArbitration', '%s',      'NEW')"  ,
                                        __db_table_deals_actions, box_list[i].address) ;
       else             {
                             sprintf(text, "%s - ERROR - EMIRi_dl_synch - Unknown category of object: %s (%s)", prefix, box_list[i].kind, box_list[i].address) ;
                          LB_ADD_ROW(IDC_LOG, text) ;

                            db->UnlockCursor(Cursor) ;
                                    return ;
                        }

          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Registry Add/Update operation (Address=%s) : %s", prefix, box_list[i].address, db->error_text) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                          __db_errors_cnt++ ;
                  }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                             }
/*----------------------------------------- Отметка удалённых сделок */

   for(i=0 ; i<box_list_cnt ; i++)
     if(box_list[i].flag==-1) {

                        sprintf(text, "Deal %s - DELETE", box_list[i].address) ;
                     LB_ADD_ROW(IDC_LOG, text) ;

                        sprintf(text, "update %s set \"DataUpdate\"='D' where \"BlockChainId\"=%s",
                                        __db_table_deals, box_list[i].address) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Delete Deal (BlockChainId=%s) : %s", prefix, box_list[i].address, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                  }
                              }
/*------------------------------------ Удаление обработанных событий */

   if(EMIR_active_section("EVENTS") ||
      EMIR_active_section("SCAN"  )   ) {

      for(i=0 ; i<box_list_cnt ; i++) {

//                      sprintf(text, "Delete from %s "
                        sprintf(text, "Update %s set \"Transaction\"='' "
                                      "Where  \"Id\"=%s"   ,
                                     __db_table_scan_events, box_list[i].id) ;

          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_synch - Delete/mark Event (Id=%s) : %s", prefix, box_list[i].id, error) ;
                     LB_ADD_ROW(IDC_LOG, text) ;
                          db->UnlockCursor(Cursor) ;
                          db->Rollback() ;
                                 return ;
                  }
                                      }

                                        }
/*------------------------------------------------- Коммит изменений */

                     db->Commit() ;

           sprintf(text, "%s - Deals Uploaded", prefix) ;
        LB_ADD_ROW(IDC_LOG, text) ;

/*--------------------------------------------- Освобождение курсора */

                   db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/
}


/********************************************************************/
/*                                                                  */
/*    Обработка очереди операций модуля "Сделки"                    */

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

  static  time_t  last_purge ;
          time_t  block_time ;
       struct tm *time_ddmmyy ;
            char  ts[128] ;

#define   _PURGE_DELAY  3600                     // Периодичность очистки
#define   _PURGE_DEEP   3600*24*__purge_deep     // Глубина очистки

/*------------------------------------------ Очистка списка операций */

   for(i=0 ; i<_DL_ACTIONS_MAX ; i++)
     if(__dl_actions[i]!=NULL) {
                                   free(__dl_actions[i]) ;
                                        __dl_actions[i]=NULL ;
                               }
/*------------------------------------------ Анализ очереди операций */

        Cursor=db->LockCursor("EMIRi_dl_actions") ;
     if(Cursor==NULL) {
                          snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_actions - Cursor lock: %s", prefix, db->error_text) ;
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
                   snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_actions - Operations purge : %s", prefix, db->error_text) ;
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
                                       __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_action - Get actions list : %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return ;
              }

   for(i=0 ; i<_DL_ACTIONS_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA)   break ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_action - Get actions list: %s", prefix, db->error_text) ;
                      LB_ADD_ROW(IDC_LOG, text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
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
           strncpy(__dl_actions[i]->master_id,   (char *)Cursor->columns[8].value, sizeof(__dl_actions[i]->master_id  )-1) ;
           strncpy(__dl_actions[i]->error,       (char *)Cursor->columns[9].value, sizeof(__dl_actions[i]->error      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - -  Получение "уровня" ошибок */
                      sprintf(text, "select min(\"Id\") "
                                    "from   %s "
                                    "where  \"Error\"<>'' and \"Error\" is not null",
                                       __db_table_deals_actions) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIRi_dl_action - Get errors level : %s", prefix, db->error_text) ;
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

      for(i=0 ; i<_DL_ACTIONS_MAX ; i++) {

        if(__dl_actions[i]==NULL)  continue ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Операция ADD_DEAL */
        if(!stricmp(__dl_actions[i]->action, "ADDDEAL")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : ADD_DEAL : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : ADD_DEAL : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_AddDeal(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_AddDeal : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                         }
/*- - - - - - - - - - - - - - - - - - - - - - -  Операция SET_STATUS */
        if(!stricmp(__dl_actions[i]->action, "SETSTATUS" )) {

                for(j=0 ; j<i ; j++)                                /* Для одной и той же сделки операции исполняются последовательно */  
                  if((!stricmp(__dl_actions[j]->action, "ADDDEAL"   ) ||
                      !stricmp(__dl_actions[j]->action, "SETSTATUS" )   ) &&
                       EMIRi_dl_GetDealId(__dl_actions[j])==
                       EMIRi_dl_GetDealId(__dl_actions[i])                &&
                       stricmp(__dl_actions[j]->status, "DONE"      )        )  break ;

                  if(j<i)  continue ;

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : SET_STATUS : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : SET_STATUS : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_SetStatus(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_SetStatus : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                            }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Операция GET_DEAL */
        if(!stricmp(__dl_actions[i]->action, "GETDEAL")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_DEAL : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->object) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_DEAL : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->object) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_GetDeal(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_GetDeal : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                         }
/*- - - - - - - - - - - - - - - - - - - - - Операция GET_ARBITRATION */
        if(!stricmp(__dl_actions[i]->action, "GETARBITRATION")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : GET_ARBITRATION : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->object) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : GET_ARBITRATION : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->object) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_GetArb(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_GetArb : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                }
/*- - - - - - - - - - - - - - - - - - - - Операция START_ARBITRATION */
        if(!stricmp(__dl_actions[i]->action, "STARTARBITRATION")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : START_ARBITRATION : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : START_ARBITRATION : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_StartArb(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_StartArb : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                  }
/*- - - - - - - - - - - - - - - - - - -  Операция ACCEPT_ARBITRATION */
        if(!stricmp(__dl_actions[i]->action, "ACCEPTARBITRATION")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : ACCEPT_ARBITRATION : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : ACCEPT_ARBITRATION : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

                     status=EMIR_dl_action_AcceptArb(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_AcceptArb : %s", prefix, error) ;
                      LB_ADD_ROW(IDC_LOG, text) ;
                             }
                                                                   }
/*- - - - - - - - - - - - - - - - - - -  Операция REJECT_ARBITRATION */
        if(!stricmp(__dl_actions[i]->action, "REJECTARBITRATION")) {

                if(__dl_actions[i]->error[0]==0)
                         snprintf(text, sizeof(text)-1, "%s - %s : REJECT_ARBITRATION : %s - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;
                else     snprintf(text, sizeof(text)-1, "%s - %s : REJECT_ARBITRATION : %s/ERROR - %s", prefix, __dl_actions[i]->id, __dl_actions[i]->status, __dl_actions[i]->data) ;

                      LB_ADD_ROW(IDC_LOG, text) ;

                if(__dl_actions[i]->error[0]!=0)  continue ;

//                   status=EMIR_dl_action_RejectArb(__dl_actions[i], db, error) ;
                  if(status) {
                        snprintf(text, sizeof(text)-1, "%s - ERROR - EMIR_dl_action_RejectArb : %s", prefix, error) ;
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
/*                           Операция ADD_DEAL                       */

   int  EMIR_dl_action_AddDeal(Dl_action *action, SQL_link *db, char *error)
{
  SQL_cursor *Cursor ;
         int  status ;
 static char *code ;
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
        char  tmpl_version[128] ;
        char  kind[128] ;
        char  kind_a[128] ;
        char  kind_hex[128] ;
        char  uuid[128] ;
        char  uuid_hex[128] ;
        char  channel[128] ;
        char  channel_hex[128] ;
        char  role_hex[128] ;
        char  id_hex[128] ;
        char  link[256] ;
        char  link_hex[512] ;
        char  id_parent[128] ;
        char  id_parent_r[128] ;
        char  bc_parent[128] ;
        Deal  deal ;
 Arbitration  arbitration ;
    DealAttr  attr[_ATTR_MAX] ;
         int  attr_cnt ;
    DealAttr  attr_bc[_ATTR_MAX] ;
         int  attr_bc_cnt ;
   DealParty  party[_PARTY_MAX] ;
         int  party_cnt ;
   DealParty  party_bc[_PARTY_MAX] ;
         int  party_bc_cnt ;
     DealMap  map[_MAP_MAX] ;
         int  map_cnt ;
     DealMap  map_bc[_MAP_MAX] ;
         int  map_bc_cnt ;
        char  gas[128] ;
        char  txn[256] ;
        char  txn_a[128] ;
        char  block[128] ;
        char  block_max[128] ;
        char  contract[128] ;
        char  contract_a[128] ;
        char  reply[128] ;
        char  version[128] ;
        char  text[8192] ;
        char  tmp[2048] ;
        char *address ;
        char *attr_key ;
        char *attr_val ;
        char *attr_end ;
        char *end ;
         int  i ;
         int  j ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

     if(code==NULL)  code=(char *)calloc(1, _CODE_SIZE) ;

        status=EMIR_db_nodepars(db, error) ;
     if(status)  return(-1) ;

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
/*--------------------------------------- Разбор реквизитов операции */

                   status=0 ;
/*- - - - - - - - - - - - - - - - - - - -  Разбор базовых реквизитов */
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
/*- - - - - - - - - - - - - - - - - - - - -  Разбор атрибутов сделки */
           attr_cnt=0 ;

  if(r_id!=json)
   for(attr_end=end+1, attr_cnt=0 ; ; attr_cnt++) {

        attr_key=strchr(attr_end+1, '"') ;
     if(attr_key==NULL)  break ;

     if(attr_cnt>=_ATTR_MAX) {
                      strcpy(error, "Too many attributes in field Data") ;
                               status=-1 ;
                                  break ;
                             }

        attr_val=strstr(attr_key+1, "\":\"") ;
     if(attr_val==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }
        attr_end=strchr(attr_val+3, '"') ;
     if(attr_end==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }

         *attr_val=0 ;
         *attr_end=0 ;

         memset(&attr[attr_cnt], 0, sizeof(attr[attr_cnt])) ;
        strncpy(attr[attr_cnt].key,   attr_key+1, sizeof(attr[attr_cnt].key  )-1) ;
        strncpy(attr[attr_cnt].value, attr_val+3, sizeof(attr[attr_cnt].value)-1) ;

     if(strlen(attr[attr_cnt].key)>32) {
                      sprintf(error, "Attribute %s in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                       }

     if(strlen(attr[attr_cnt].value)>32) {
                      sprintf(error, "Attribute %s value in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                         }
                                                  }
/*----------------------------------- Формирование транзитного файла */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(status<0)  break ;

     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - Извлечение заголовочных данных */
                      sprintf(text, "Select r.\"Kind\", r.\"ArbitrationKind\", r.\"Data\", r.\"Version\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"='%s'",
                                    __db_table_deals, r_id) ;
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

            memset(kind,     0, sizeof(kind)) ;
           strncpy(kind,    (char *)Cursor->columns[0].value, sizeof(kind   )-1) ;
            memset(kind_a,   0, sizeof(kind_a)) ;
           strncpy(kind_a,  (char *)Cursor->columns[1].value, sizeof(kind_a )-1) ;
            memset(data,     0, sizeof(data)) ;
           strncpy(data,    (char *)Cursor->columns[2].value, sizeof(data   )-1) ;
            memset(version,  0, sizeof(version)) ;
           strncpy(version, (char *)Cursor->columns[3].value, sizeof(version)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - -  Проверка повторного создания сделки */
     if(version[0]!=0) {

                   sprintf(error, "Add operation already processed for this Deal") ;
                              status=-1 ;
                               break ;
                       }
/*- - - - - - - - - - - - - - - - -  Проверка наличия шаблона сделки */
     if(kind_a[0]!=0) {
                        sprintf(text, "%s Template", kind_a) ;
                         memset(tmpl, 0, sizeof(tmpl)) ;
          status=EMIR_db_syspar(db, text, tmpl, error) ;
       if(status)  break ;
                      }

                        sprintf(text, "%s Template", kind) ;
                         memset(tmpl, 0, sizeof(tmpl)) ;
          status=EMIR_db_syspar(db, text, tmpl, error) ;
       if(status)  break ;
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign ,    (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Проверка участников */
       if(party_cnt==0) {
                           sprintf(error, "Empty party list") ;
                            status=-1 ;
                             break ;
                        }

     for(i=0 ; i<party_cnt ; i++)                                   /* Проверка сушествования участников */
       if(party[i].sign[0]==0)  break ;

       if(i<party_cnt) {
                           sprintf(error, "Unknown party member %s", party[i].party_id) ;
                            status=-1 ;
                             break ;
                       }

     for(i=0 ; i<party_cnt ; i++)                                   /* Проверка наличия в списке участников данного узла */
       if(!strcmp(party[i].party_id, __member_key))  break ;

       if(i>=party_cnt) {
                           sprintf(error, "Node owner must be party member %s", __member_key) ;
                            status=-1 ;
                             break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - Формирование файла */
                   if(data[0]==0)  strcpy(data, " ") ;              /* Если данных нет - делаем заглушку */

#ifdef  UNIX
               snprintf(path, sizeof(path)-1, "%s/AddDeal.%s.KillAfter", __work_folder, action->id) ;

#else
               snprintf(path, sizeof(path)-1, "%s\\AddDeal.%s.KillAfter", __work_folder, action->id) ;
#endif

            file=fopen(path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, path) ;
                            status=-1 ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
/*- - - - - - - - - - - - - - - - Занесение операции в FILES_ACTIONS */
            strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

                      sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                    "values( 'PutEncryptedFile', '%s', '%s', 'NEW', 'Deal:%s')",
                                      __db_table_files_actions, path, sign_list, action->id) ;
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

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

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
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------- Формирование транзакции изменения */

   if(!stricmp(action->status, "FILE" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(status<0)  break ;

     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
                      sprintf(text, "select \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                      __db_table_files_actions, action->reply, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "1: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                                          __db_errors_cnt++ ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Check data file transfer: No record") ;
       else                      sprintf(error, "Check data file transfer: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(link,   0, sizeof(link)) ;
           strncpy(link,   (char *)Cursor->columns[1].value, sizeof(link)-1) ;

                 db->SelectClose(Cursor) ;

     if(stricmp(result, "DONE"))  break ;
/*- - - - - - - - - - - - - - - - - - - Проверка родительской сделки */
                      sprintf(text, "select r.\"Parent\", p.\"Parent\", p.\"BlockChainId\" "
                                    "from   %s r left join %s p on p.\"Id\"=r.\"Parent\" "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals,
                                    __db_table_deals,
                                      r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Parent deal check: %s", db->error_text) ;
                               db->error_text[0]=0 ;
                             __db_errors_cnt++ ;
                                     break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get parent: No record") ;
       else                      sprintf(error, "Get parent: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                           break ;
                }

            memset(id_parent,   0, sizeof(id_parent)) ;
           strncpy(id_parent,   (char *)Cursor->columns[0].value, sizeof(id_parent)-1) ;
            memset(id_parent_r, 0, sizeof(id_parent)) ;
           strncpy(id_parent_r, (char *)Cursor->columns[1].value, sizeof(id_parent_r)-1) ;
            memset(bc_parent,   0, sizeof(bc_parent)) ;
           strncpy(bc_parent,   (char *)Cursor->columns[2].value, sizeof(bc_parent)-1) ;

                 db->SelectClose(Cursor) ;


     if(id_parent[0]!=0) {

       if(id_parent_r[0]==0) {                                      /* Если сделка-родитель не найдена */
                      sprintf(error, "Unknown parent deal") ;
                                        status=-1 ;
                                           break ;
                             }

       if(!stricmp(id_parent, r_id)) {                              /* Если сделка ссылается на самого себя... */
                      sprintf(error, "Deal is the parent of itself") ;
                                        status=-1 ;
                                           break ;
                                     }

       if(bc_parent[0]==0) {                                        /* Если сделка-родитель еще не опубликован... */
                             EMIR_log("Parent deal not published yet") ;
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                           }
                         }
     else                {
                               strcpy(bc_parent, _NULL_ADDR) ;
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select r.\"Kind\", r.\"ArbitrationKind\", r.\"DealsUUID\", r.\"ChannelId\" "
                                    "from   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "3: %s", db->error_text) ;
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

            memset(kind,    0, sizeof(kind)) ;
           strncpy(kind,    (char *)Cursor->columns[0].value, sizeof(kind  )-1) ;
            memset(kind_a,  0, sizeof(kind_a)) ;
           strncpy(kind_a,  (char *)Cursor->columns[1].value, sizeof(kind_a)-1) ;
            memset(uuid,    0, sizeof(uuid)) ;
           strncpy(uuid,    (char *)Cursor->columns[2].value, sizeof(uuid)-1) ;
            memset(channel, 0, sizeof(channel)) ;
           strncpy(channel, (char *)Cursor->columns[3].value, sizeof(channel)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  Извлечение шаблона основного СК */
                      sprintf(text, "%s Template", kind) ;          /* Извлечение транзакции с шаблоном */ 
                       memset(tmpl, 0, sizeof(tmpl)) ;
        status=EMIR_db_syspar(db, text, tmpl, error) ;
     if(status)  break ;

                      sprintf(text, "%s Version(s)", kind) ;        /* Извлечение версии шаблона */
                       memset(tmpl_version, 0, sizeof(tmpl_version)) ;
        status=EMIR_db_syspar(db, text, tmpl_version, error) ;
     if(status) {
                   if(strstr(error, "No record")==NULL)  break ;    /* Если версия не указана - игнорируем */
                }

     if(!stricmp(tmpl_version, "default"))  tmpl_version[0]=0 ;     /* Если указана "версия по-умолчанию"... */
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Account\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_PARTY_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - -  Запрос карты изменения состояний сделки */
                      sprintf(text, "Select \"Status\", \"StatusNext\", \"Role\" "
                                    "From   %s "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_statusmap, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get status map 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_MAP_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get status map 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(map[i].status,      (char *)Cursor->columns[0].value, sizeof(map[i].status     )-1) ;
           strncpy(map[i].status_next, (char *)Cursor->columns[1].value, sizeof(map[i].status_next)-1) ;
           strncpy(map[i].role ,       (char *)Cursor->columns[2].value, sizeof(map[i].role       )-1) ;
                               }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_MAP_MAX) {                                               /* Если выборка слишком большая... */
                        sprintf(error, "Too large status map") ;
                            status=-1 ;
                              break ;
                    }  

         map_cnt=i ;
/*- - - - - - - - - - -  Подготовка транзакции создания основного СК */
              EMIR_txt2hex64(channel, channel_hex, strlen(channel)) ;
              EMIR_txt2hex64(uuid,       uuid_hex, strlen(uuid)) ;
              EMIR_txt2hex64(kind,       kind_hex, strlen(kind)) ;
              EMIR_txt2hex64(link,       link_hex, strlen(link)) ;

   if(stricmp(tmpl_version, "2021-05-01")>=0) {

             sprintf(text, "00000000000000000000000000000000000000000000000000000000000000c0"
                           "000000000000000000000000%s"
                           "0000000000000000000000000000000000000000000000000000000000000140"
                           "%064x"
                           "%064x"
                           "%064x",
                            bc_parent, 
                            0x0160+(int)strlen(link_hex)/2, 
                            0x0180+(int)strlen(link_hex)/2+32*2*attr_cnt,
                            0x01a0+(int)strlen(link_hex)/2+32*2*attr_cnt+32*3*party_cnt) ;

             sprintf(tmp, "%064x", 3) ;
              strcat(text, tmp) ;
              strcat(text, channel_hex) ;
              strcat(text, uuid_hex) ;
              strcat(text, kind_hex) ;

                                              }
   else                                       {

             sprintf(text, "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "00000000000000000000000000000000000000000000000000000000000000e0"
                           "%064x"
                           "%064x"
                           "%064x",
                            uuid_hex, kind_hex, bc_parent, 
                            0x0100+(int)strlen(link_hex)/2, 
                            0x0120+(int)strlen(link_hex)/2+32*2*attr_cnt,
                            0x0140+(int)strlen(link_hex)/2+32*2*attr_cnt+32*3*party_cnt) ;

                                              }

             sprintf(tmp, "%064x%s", (int)strlen(link), link_hex) ;
              strcat(text, tmp) ;

             sprintf(tmp, "%064x", attr_cnt*2) ;
              strcat(text, tmp) ;

      for(i=0 ; i<attr_cnt ; i++) {
              EMIR_txt2hex64(attr[i].key, tmp, strlen(attr[i].key)) ;
                      strcat(text, tmp) ;
              EMIR_txt2hex64(attr[i].value, tmp, strlen(attr[i].value)) ;
                      strcat(text, tmp) ;
                                   }

             sprintf(tmp, "%064x", party_cnt*3) ;
              strcat(text, tmp) ;

      for(i=0 ; i<party_cnt ; i++) {
              EMIR_txt2hex64(party[i].party_id,   id_hex, strlen(party[i].party_id)) ;
              EMIR_txt2hex64(party[i].role,     role_hex, strlen(party[i].role    )) ;

                 strcat(text, "000000000000000000000000") ;
                 strcat(text, party[i].account) ;
                 strcat(text, id_hex) ;
                 strcat(text, role_hex) ;
                                   }

             sprintf(tmp, "%064x", map_cnt*3) ;
              strcat(text, tmp) ;

      for(i=0 ; i<map_cnt ; i++) {
              EMIR_txt2hex64(map[i].status,      tmp, strlen(map[i].status     )) ;
                      strcat(text, tmp) ;
              EMIR_txt2hex64(map[i].status_next, tmp, strlen(map[i].status_next)) ;
                      strcat(text, tmp) ;
              EMIR_txt2hex64(map[i].role,        tmp, strlen(map[i].role       )) ;
                      strcat(text, tmp) ;
                                 }
/*- - - - - - - - - - - -  Отправка транзакции создания основного СК */
                            reply[0]=0 ;

      do {
              status=EMIR_node_getcode(tmpl,                        /* Извлечение кода контракта */
                                       code, _CODE_SIZE-1, error) ;
           if(status)  break ;

                                 strcat(code, text) ;               /* Добавляем параметры конструктора смарт-контракта */

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на создание контракта */
                                         NULL, code, gas, error) ;
           if(status)  break ;


              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_publcontract(account,                /* Отправка транзакции */ 
                                              code, gas, txn, error) ;
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

           if(status)  break ;
/*- - - - - - - - - - - - - - - - Извлечение шаблона арбитражного СК */
    if(kind_a[0]!=0) {

                      sprintf(text, "%s Template", kind_a) ;
                       memset(tmpl, 0, sizeof(tmpl)) ;
        status=EMIR_db_syspar(db, text, tmpl, error) ;
     if(status)  break ;

                     }
/*- - - - - - - - - - Подготовка транзакции создания арбитражного СК */
    if(kind_a[0]!=0) {

              EMIR_txt2hex64(kind_a, kind_hex, strlen(kind)) ;

             sprintf(text, "%s"
                           "0000000000000000000000000000000000000000000000000000000000000000"
                           "0000000000000000000000000000000000000000000000000000000000000060",
                            kind_hex) ;

             sprintf(tmp, "%064x", party_cnt*3) ;
              strcat(text, tmp) ;

      for(i=0 ; i<party_cnt ; i++) {
              EMIR_txt2hex64(party[i].party_id,   id_hex, strlen(party[i].party_id)) ;
              EMIR_txt2hex64(party[i].role,     role_hex, strlen(party[i].role    )) ;

                 strcat(text, "000000000000000000000000") ;
                 strcat(text, party[i].account) ;
                 strcat(text, id_hex) ;
                 strcat(text, role_hex) ;
                                   }

                     }
/*- - - - - - - - - - - Отправка транзакции создания арбитражного СК */
                            txn_a[0]=0 ;

    if(kind_a[0]!=0) {
                            reply[0]=0 ;

      do {
              status=EMIR_node_getcode(tmpl,                        /* Извлечение кода контракта */
                                       code, _CODE_SIZE-1, error) ;
           if(status)  break ;

                                 strcat(code, text) ;               /* Добавляем параметры конструктора смарт-контракта */

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на создание контракта */
                                         NULL, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

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

           if(status)  break ;

                     }
/*- - - - - - - - - - - - - - - - - Удаление записи из FILES_ACTIONS */
                      sprintf(text, "delete from %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                       __db_table_files_actions, action->reply, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "2: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                                          __db_errors_cnt++ ;
                         break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

              db->SelectClose(Cursor) ;

   if(!stricmp(result, "DONE")) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Reply\"='%s,%s' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, txn, txn_a, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

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
                                }

                                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*---------------------------- Привязка арбитражного смарт-контракта */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(status<0)  break ;

     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                  memset(txn,   0, sizeof(txn  )) ;
                  memset(txn_a, 0, sizeof(txn_a)) ;

                 strncpy(txn, action->reply, sizeof(txn)-1) ;
              end=strchr(txn, ',') ;
           if(end==NULL) {
                    sprintf(error, "Bad structure of column 'Reply'") ;
                             break ;
                         }

             *end= 0 ;

                 strncpy(txn_a, end+1, sizeof(txn_a)-1) ;

              status=EMIR_node_checktxn(txn, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;

             memmove(contract, contract+2, strlen(contract+2)+1) ;  /* Убираем префикс 0x */

        if(txn_a[0]!=0) {
 
              status=EMIR_node_checktxn(txn_a, block, contract_a, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;

             memmove(contract_a, contract_a+2,                      /* Убираем префикс 0x */
                                   strlen(contract_a+2)+1) ;
                        }
        else            {
                            contract_a[0]=0 ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Извлечение данных */
                      sprintf(text, "select r.\"Kind\", r.\"DealsUUID\" "
                                    "from   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals,
                                      r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "3: %s", db->error_text) ;
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

            memset(kind,  0, sizeof(kind)) ;
           strncpy(kind, (char *)Cursor->columns[0].value, sizeof(kind)-1) ;
            memset(uuid,  0, sizeof(uuid)) ;
           strncpy(uuid, (char *)Cursor->columns[1].value, sizeof(uuid)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Account\", m.\"Box\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - -  Запрос карты изменения состояний сделки */
                      sprintf(text, "Select \"Status\", \"StatusNext\", \"Role\" "
                                    "From   %s "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_statusmap, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get status map 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_MAP_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get status map 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(map[i].status,      (char *)Cursor->columns[0].value, sizeof(map[i].status     )-1) ;
           strncpy(map[i].status_next, (char *)Cursor->columns[1].value, sizeof(map[i].status_next)-1) ;
           strncpy(map[i].role ,       (char *)Cursor->columns[2].value, sizeof(map[i].role       )-1) ;
                               }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_MAP_MAX) {                                               /* Если выборка слишком большая... */
                        sprintf(error, "Too large status map") ;
                            status=-1 ;
                              break ;
                    }  

         map_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  Сверка данных */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(contract, &deal, attr_bc,  _ATTR_MAX,  &attr_bc_cnt,
                                                       party_bc, _PARTY_MAX, &party_bc_cnt,
                                                       map_bc,   _MAP_MAX,   &map_bc_cnt,
                                                       NULL,        0,        NULL,
                                                       NULL,        0,        NULL, error) ;
EMIR_log("Add.1") ;

           if(status<0)  break ;

           if(strcmp(deal.id,   uuid) ||
              strcmp(deal.kind, kind)   ) {
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

    for(i=0 ; i<attr_cnt ; i++) {

      for(j=0 ; j<attr_bc_cnt ; j++)
        if(!stricmp(attr[i].key,   attr_bc[j].key  ) &&
           !stricmp(attr[i].value, attr_bc[j].value)   )  break ;

        if(j>=attr_bc_cnt) {
                      strcpy(error, "Attributes check fail") ;
                              status=-1 ;
                                 break ;
                           }
                                }

    for(i=0 ; i<map_cnt ; i++) {

      for(j=0 ; j<map_bc_cnt ; j++)
        if(!stricmp(map[i].status,      map_bc[j].status     ) &&
           !stricmp(map[i].status_next, map_bc[j].status_next) &&
           !stricmp(map[i].role,        map_bc[j].role       )   )  break ;

        if(j>=map_bc_cnt) {
                      strcpy(error, "Status map check fail") ;
                              status=-1 ;
                                 break ;
                          }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - -  Обработка безарбитражных сделок */
     if(contract_a[0]==0) {
                             txn[0]=0 ;
                               break ;
                          }
/*- - - - - - - - - - - Отправка транзакции привязки арбитражного СК */
                            reply[0]=0 ;

      do {
             sprintf(text, "5d403f3c"                               /* Формируем блок данных транзакции */
                           "000000000000000000000000%s",
                            contract_a) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        contract, text, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_publcontract(account,                /* Отправка транзакции */ 
                                              code, gas, txn, error) ;
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

           if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT_A', \"Object\"='%s', \"Reply\"='%s,%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, contract_a, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*--------------------------------------------------- Раздача по п/я */

   if(!stricmp(action->status, "WAIT_A"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(status<0)  break ;

     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - Разбор транзитных данных */
                  memset(contract_a, 0, sizeof(contract_a)) ;
                  memset(txn,        0, sizeof(txn       )) ;

                 strncpy(contract_a, action->reply, sizeof(contract_a)-1) ;
              end=strchr(contract_a, ',') ;
           if(end==NULL) {
                    sprintf(error, "Bad structure of column 'Reply'") ;
                             break ;
                         }

             *end= 0 ;

                 strncpy(txn, end+1, sizeof(txn)-1) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
       if(contract_a[0]!=0) {

              status=EMIR_node_checktxn(txn, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;

                            }
/*- - - - - - - - -  Проверка связывания основного и арбитражного СК */
       if(contract_a[0]!=0) {

                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(action->object, &deal, NULL, 0, NULL,
                                                             NULL, 0, NULL,
                                                             NULL, 0, NULL,
                                                             NULL, 0, NULL,
                                                             NULL, 0, NULL, error) ;
           if(status<0)  break ;

                               memset(&arbitration, 0, sizeof(arbitration)) ;
              status=EMIRi_dl_GetArb(contract_a, &arbitration, NULL, 0, NULL,
                                                               NULL, 0, NULL,
                                                               NULL, 0, NULL,
                                                               NULL, 0, NULL,
                                                               NULL, 0, NULL, error) ;
           if(status<0)  break ;

           if(strcmp(deal.arbitration, contract_a)) {
                      strcpy(error, "Deal not assigned to proper arbitration smart-contract") ;
                              status=-1 ;
                                 break ;
                                                    }
           if(strcmp(arbitration.deal, action->object)) {
                      strcpy(error, "Arbitration smart-contract not refer to deal") ;
                              status=-1 ;
                                 break ;
                                                        }
                            }
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Account\", m.\"Box\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - Проверка доступности почтовых ящиков */
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
/*- - - - - - - - - - - - - -  Отправка транзакций на почтовые ящики */
                       strcpy(box_list, "") ;

                            reply[0]=0 ;

     for(i=0 ; i<party_cnt ; i++) {

             sprintf(text, "552de325"                               /* Формируем блок данных транзакции */
                           "000000000000000000000000%s",
                            action->object) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                        party[i].box, text, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                            party[i].box, text, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&     /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {  /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        i-- ;  continue ;
                                                                }
                             break ;
                      } 

                               strcat(box_list, txn) ;              /* Формируем список транзакций */
                               strcat(box_list, ",") ;

                                      reply[0]=0 ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='BOXES', \"Reply\"='a:%s,%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract_a, box_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           }
/*------------------------------------- Ожидание подтверждения с п/я */

   if(!stricmp(action->status, "BOXES")) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
            memset(contract_a, 0, sizeof(contract_a)) ;
            memset(block_max,  0, sizeof(block_max )) ;

      for(address=action->reply, end=address ;
                                 end!=NULL ; address=end+1, i++) {

              end=strchr(address, ',') ;
           if(end!=NULL)  *end=0 ;

           if(*address==0)  continue ;

           if( address[0]=='a' &&                                   /* Извлекаем адрес арбитражного СК */
               address[1]==':'   ) {
                strncpy(contract_a, address+2, sizeof(contract_a)-1) ;
                                      continue ;
                                   } 

              status=EMIR_node_checktxn(address, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }

           if(status < 0)   break ;

           if(stricmp(block, block_max)>0)  strcpy(block_max, block) ;

                                                                 }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", m.\"Box\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].box,      (char *)Cursor->columns[1].value, sizeof(party[i].box     )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - Проверка занесения данных в п/я напрямую */
   if(EMIR_active_section("EVENTS")==0 &&
      EMIR_active_section("SCAN"  )==0   ) {

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

                                           }
/*- - - - - - - - - - - - Проверка занесения данных в п/я по событию */
   else                                    {

         if(strtoul(block_max, &end, 0)>__block_db) {               /* Если сканер не дошел до нужного блока... */
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                                                    }

     for(i=0 ; i<party_cnt ; i++) {

           sprintf(text, "Select \"Address\" "
                         "From   %s "
                         "Where  \"Address\"='%s' "
                         " and   \"Topic\"='a226db3f664042183ee0281230bba26cbf7b5057e50aee7f25a175ff45ce4d7f' "
                         " and   \"Data\" like '%%%s%%'" ,
                          __db_table_scan_events, party[i].box, action->object) ;

         status=db->SelectOpen(Cursor, text, NULL, 0) ;
      if(status) {
                    strcpy(error, db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                             break ;
                 }

         status=db->SelectFetch(Cursor) ;
                db->SelectClose(Cursor) ;

      if(status==_SQL_NO_DATA) {
//                           sprintf(error, "Box %s failed", address) ;
//                                     break ;
                                 db->UnlockCursor(Cursor) ;
                                     return(0) ;
                               }

      if(status) {
                      sprintf(error, "Get events data: %s", db->error_text) ;
                                  db->error_text[0]=0 ;
                                __db_errors_cnt++ ;
                                      break ;
                 }

                                  }

      if(status)  break ; 

                                           }
/*- - - - - - - - - - - - - - - - - - - Занесение адреса СК в Сделку */
                    sprintf(text, "update %s "
                                  "set    \"BlockChainId\"='%s', \"ArbitrationBlockChainId\"='%s' "
                                  "where  \"Id\"=%s",
                                    __db_table_deals, action->object, contract_a, r_id) ;
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

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
/*                           Операция SET_STATUS                     */

   int  EMIR_dl_action_SetStatus(Dl_action *action, SQL_link *db, char *error)
{
         SQL_cursor *Cursor ;
                int  status ;
       static  char *code ;
               char  account[128] ;
               char *password ;
               char  json[3048] ;
               char *r_id ;
               char *r_status ;
               char *r_remark ;
               char *r_data ;
               char *r_files ;
               char  data[16200] ;
               char  path[FILENAME_MAX] ;
               FILE *file ;
               char  files_uuid[1024] ;
                int  files_uuid_cnt ;
               char  link[128] ;
               char  sign_list[512] ;
               Deal  deal ;
 static    DealAttr *attr ;
                int  attr_cnt ;
 static    DealAttr *attr_bc ;
                int  attr_bc_cnt ;
 static   DealParty *party ;
                int  party_cnt ;
 static    DealFile *files ;
                int  files_cnt ;
 static    DealFile *files_bc ;
                int  files_bc_cnt ;
 static DealHistory *history_bc ;
                int  history_bc_cnt ;
                int  done_flag ;
               char *oper ;
               char  status_hex[128] ;
               char  remark_hex[17000] ;
               char  link_hex[512] ;
               char  recipients_hex[1024] ;
               char  uuid_hex[128] ;
               char  kind_hex[128] ;
               char  ext_hex[128] ;
               char  contract[128] ;
               char  version[128] ;
               char  gas[128] ;
               char  txn[256] ;
               char  txn_list[4096] ;
               char  block[128] ;
      unsigned long  block_num ;
               char  result[128] ;
               char  reply[128] ;
               char  text[16000] ;
               char  tmp[2048] ;
               char *address ;
               char *attr_key ;
               char *attr_val ;
               char *attr_end ;
               char *end ;
                int  i ;
                int  j ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

     if(code==NULL) {
                          code   =(    char    *)calloc(1, _CODE_SIZE) ;
                          attr   =(DealAttr    *)calloc(   _ATTR_MAX, sizeof(*attr)) ;
                          attr_bc=(DealAttr    *)calloc(   _ATTR_MAX, sizeof(*attr_bc)) ;
                         party   =(DealParty   *)calloc(  _PARTY_MAX, sizeof(*party)) ;
                         files   =(DealFile    *)calloc(  _FILES_MAX, sizeof(*files)) ;
                         files_bc=(DealFile    *)calloc(  _FILES_MAX, sizeof(*files_bc)) ;
                       history_bc=(DealHistory *)calloc(_HISTORY_MAX, sizeof(*history_bc)) ;
                    }

        status=EMIR_db_nodepars(db, error) ;
     if(status)  return(-1) ;

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
/*--------------------------------------- Разбор реквизитов операции */

                 r_remark="" ;
           files_uuid_cnt= 0 ;
/*- - - - - - - - - - - - - - - - - - - -  Разбор базовых реквизитов */
  do {
                 memset(json, 0, sizeof(json)) ;
                strncpy(json, action->data, sizeof(json)-1) ;
       r_id    = strstr(json, "\"Id\":\"") ;
       r_status= strstr(json, "\"Status\":\"") ;
       r_remark= strstr(json, "\"Remark\":\"") ;
       r_data  = strstr(json, "\"Data\":\"") ;
       r_files = strstr(json, "\"Files\":\"") ;

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

                          end=strchr(r_data, '"') ;
                       if(end==NULL) {  r_id=NULL ;  break ;  }
                         *end=0 ;

                           strupr(r_data) ;
                    }

    if(r_files!=NULL) {
                         r_files=strstr(r_files,  "\":\"")+3 ;

                          end=strchr(r_files, '"') ;
                       if(end==NULL) {  r_id=NULL ;  break ;  }
                         *end=0 ;

                          strcpy(files_uuid, "'") ;
                          strcat(files_uuid, r_files) ;
                          strcat(files_uuid, "'") ;

                 EMIR_text_subst(files_uuid, ",", "';'", 0) ;
                 EMIR_text_subst(files_uuid, ";", ",", 0) ;
 
         for(files_uuid_cnt=1, i=0 ; files_uuid[i] ; i++)           /* Подсчитываем файлы */
           if(files_uuid[i]==',')  files_uuid_cnt++ ;

                     }

     } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - -  Разбор атрибутов сделки */
  for(attr_end=end+1, attr_cnt=0 ; ; attr_cnt++) {

        attr_key=strchr(attr_end+1, '"') ;
     if(attr_key==NULL)  break ;

     if(attr_cnt>=_ATTR_MAX) {
                      strcpy(error, "Too many attributes in field Data") ;
                               status=-1 ;
                                  break ;
                             }

        attr_val=strstr(attr_key+1, "\":\"") ;
     if(attr_val==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }
        attr_end=strchr(attr_val+3, '"') ;
     if(attr_end==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }

         *attr_val=0 ;
         *attr_end=0 ;


     if(!strcmp(attr_key, "Id"    ) ||
        !strcmp(attr_key, "Status") ||
        !strcmp(attr_key, "Remark") ||
        !strcmp(attr_key, "Data"  )   ) { attr_cnt-- ; continue ; }

         memset(&attr[attr_cnt], 0, sizeof(attr[attr_cnt])) ;
        strncpy(attr[attr_cnt].key,   attr_key+1, sizeof(attr[attr_cnt].key  )-1) ;
        strncpy(attr[attr_cnt].value, attr_val+3, sizeof(attr[attr_cnt].value)-1) ;

     if(strlen(attr[attr_cnt].key)>32) {
                      sprintf(error, "Attribute %s in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                       }

     if(strlen(attr[attr_cnt].value)>32) {
                      sprintf(error, "Attribute %s value in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                         }
                                                 }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "NEW" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                            return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - -  Проверка списка UUID файлов */
     if(files_uuid_cnt) {

                      sprintf(text, "Select count(*) " 
                                    "From   %s  "
                                    "Where  \"DealId\"=%s "
                                    " and   \"FileUUID\" in(%s) ",
                                     __db_table_deals_files, r_id, files_uuid ) ;

          status=db->SelectOpen(Cursor, text, NULL, 0) ;
       if(status) {
                    EMIR_log(db->error_text) ;
                     sprintf(error, "Chech files count 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                  }

           status=db->SelectFetch(Cursor) ;
//      if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
        if(status) {
                      sprintf(error, "Chech files count 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                   }

          files_cnt=strtoul((char *)Cursor->columns[0].value, &end, 10) ;

                 db->SelectClose (Cursor) ;

        if(files_cnt!=files_uuid_cnt) {
                        sprintf(error, "The list of files does not match the list of UUIDs: %d <> %d", files_cnt, files_uuid_cnt) ;
                            status=-1 ;
                              break ;
                                      }   
                        }
/*- - - - - - - - - - - -  Извлечение списка ещё не созданных файлов */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Sign\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;

                        }
/*- - - - - - - - - - - - - - - - - - - - - - -  Формирование файлов */
     for(i=0 ; i<files_cnt ; i++) {

       if(!stricmp(files[i].relation, "SIGN")) {

                          sprintf(text, "insert into %s (\"Action\", \"ObjectPath\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                        "values( 'SignFile', '%s', '%s.sign', '%s', 'NEW', 'Deal:%s')",
                                           __db_table_files_actions, files[i].p_local_path, files[i].p_local_path, files[i].sign, action->id) ;
            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Sign operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='SIGN', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "SIGN"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                               files_cnt=0 ;

     if(files_uuid_cnt) {
 
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"LocalPath\""
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'"
                                    " and   f2.\"LocalPath\"=a1.\"ObjectPath\""
                                    " and   a1.\"MasterId\"='Deal:%s' ",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, files_uuid, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "%s", db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                         break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].local_path,(char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on SIGN operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;

                       }
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
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
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                                }
/*- - - - - - - - - - - - - - - - -  Отбор файлов для передачи в DFS */
           files_cnt=0 ;

 if(files_uuid_cnt) {
	  
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"FromSC\" is null "
                                    " and   f1.\"DfsPath\" is null ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;

                    }
/*- - - - - - - - - - - - - - - - - -  Проверка существования файлов */
     for(i=0 ; i<files_cnt ; i++) {

          if(access(files[i].local_path, 0x04)) {

                        sprintf(error, "File %s is absent or access denied : %s", files[i].id, files[i].local_path) ;
                            status=-1 ;
                              break ;              
                                                }
                                  }
/*- - - - - - - - - - - - - - - - Извлечение сертификатов участников */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign,     (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - -  Передача файлов в DFS */   
                               strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }


     for(i=0 ; i<files_cnt ; i++) {

       if(strstr(files[i].kind, "Sign")!=NULL)  oper="TransferFile" ;
       else                                     oper="PutEncryptedFile" ;

       if(!stricmp(files[i].relation, "COPY")) {

                   sprintf(text, "select count(*) from %s where \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                          __db_table_files_actions, files[i].p_local_path, action->id) ;

             status=db->SelectOpen(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                       break ;
                     }

             status=db->SelectFetch(Cursor) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                       break ;
                     }

              memset(result, 0, sizeof(result)) ;
             strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
           
                 db->SelectClose(Cursor) ;

          if(!stricmp(result, "1"))  continue ;

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                 "values( '%s', '%s', '%s', 'NEW', 'Deal:%s')",
                                          __db_table_files_actions, oper, files[i].p_local_path, sign_list, action->id) ;

                                               }
       else                                    { 

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                 "values( '%s', '%s', '%s', 'NEW', 'Deal:%s')",
                                          __db_table_files_actions, oper, files[i].local_path, sign_list, action->id) ;

                                               }

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;        
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILES', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------- Передача файлов на СК сделки */

   if(!stricmp(action->status, "FILES"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - -  Контроль завершения операций с COPY-файлами */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f2.\"LocalPath\" "
                                    "     , a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"FromSC\" is null "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' and a1.\"LocalPath\"=f2.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')"
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, files_uuid, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].parent_id,  (char *)Cursor->columns[1].value, sizeof(files[i].parent_id )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].status,     (char *)Cursor->columns[3].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[4].value, sizeof(files[i].dfs_path  )-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;

                        }
/*- - - - - - - - - - - Обработка завершения операций с COPY-файлами */
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
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - Контроль пассивных операций с COPY-файлами */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
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
                           __db_errors_cnt++ ;
                                   break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[2].value, sizeof(files[i].dfs_path  )-1) ;

     if( stricmp(files[i].dfs_path, "" )) {                         /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                          }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;

                        }
/*- - - - - - - - - - -  Обработка пассивных операций с COPY-файлами */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"LocalPath\"='%s', \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, 
                                        files[i].local_path, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1, %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"FromSC\" is null "
                                    " and   f1.\"DfsPath\" is null "
                                    " and  (f1.\"Relation\"<>'COPY' or f1.\"Relation\" is null) "
                                    " and   a1.\"LocalPath\"=f1.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')"
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_files_actions,
                                       r_id, files_uuid, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,  (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;

                        }   
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f1.\"Kind\", f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Remark\", f1.\"Recipients\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"FromSC\" is null ",
                                     __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;

                        }
/*- - - - - - - - - - - - - - - - - - - -  Контроль трансфера файлов */
     for(i=0 ; i<files_cnt ; i++) {

        if(files[i].dfs_path[0]==0) {
                                         status=-1 ;
                      sprintf(error, "Check Transfer operation : File %s not transfered", files[i].id) ;
                     EMIR_log(error) ;
                                          break ;
                                    }

                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - Формирование хэшей и расширений файлов */
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
                                  __db_errors_cnt++ ;
                                          break ;
                   }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - Извлечение адреса смарт-контракта сделки */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - -  Формирование и отправка файловых транзакций */
             memset(txn_list, 0, sizeof(txn_list)) ;

     for(i=0 ; i<files_cnt ; i++) {

         EMIR_txt2hex64(files[i].file_uuid,  uuid_hex,       strlen(files[i].file_uuid)) ;
         EMIR_txt2hex64(files[i].kind,       kind_hex,       strlen(files[i].kind)) ;
         EMIR_txt2hex64(files[i].file_ext,   ext_hex,        strlen(files[i].file_ext)) ;
         EMIR_txt2hex64(files[i].dfs_path,   link_hex,       strlen(files[i].dfs_path)) ;
         EMIR_txt2hex64(files[i].remark,     remark_hex,     strlen(files[i].remark)) ;
         EMIR_txt2hex64(files[i].recipients, recipients_hex, strlen(files[i].recipients)) ;

                                         reply[0]=0 ;

      do {

             sprintf(code, "9b4a25f6"                               /* Формируем блок данных транзакции */
                           "%s"
                           "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "00000000000000000000000000000000000000000000000000000000000000e0"
                           "%064x"
                           "%064x",
                            uuid_hex,
                            kind_hex, ext_hex, files[i].hash,
                            0x100+(int)strlen(link_hex)/2,
                            0x120+(int)strlen(link_hex)/2+(int)strlen(remark_hex)/2 ) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].dfs_path)) ;
              strcat(code, tmp) ;
              strcat(code, link_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].remark)) ;
              strcat(code, tmp) ;
              strcat(code, remark_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].recipients)) ;
              strcat(code, tmp) ;
              strcat(code, recipients_hex) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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

           if(status)  break ;

                                      strcat(txn_list, txn) ;
                                      strcat(txn_list, ",") ;

                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*-------------------------------------------- Передача файла данных */

   if(!stricmp(action->status, "DATA" )) do {
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                       status=0 ;

      for(address=action->reply, end=address ;
                                 end!=NULL ; address=end+1, i++) {

              end=strchr(address, ',') ;
           if(end!=NULL)  *end=0 ;

           if(*address==0)  continue ;

              status=EMIR_node_checktxn(address, block, contract, error) ;
           if(status== 0)  return(0) ;

           if(status < 0)   break ;
                                                                 }

           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - Если файл данных не передаётся */
       if(r_data[0]=='N') {
                             strcpy(action->status, "STATUS") ;
                             strcpy(link, "") ;
                                 break ;
                          }
/*- - - - - - - - - - - - - - - - - - - - - - - Выделение курсора БД */
    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - Извлечение заголовочных данных */
                      sprintf(text, "Select r.\"Data\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"='%s'",
                                    __db_table_deals, r_id) ;
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

            memset(data,   0, sizeof(data)) ;
           strncpy(data,   (char *)Cursor->columns[0].value, sizeof(data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign ,    (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - Проверка наличия участников сделки */
       if(party_cnt==0) {
                           sprintf(error, "Empty party list") ;
                            status=-1 ;
                             break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - Формирование файла */
                   if(data[0]==0)  strcpy(data, " ") ;              /* Если данных нет - делаем заглушку */

#ifdef  UNIX
               snprintf(path, sizeof(path)-1, "%s/SetStatus.%s.KillAfter", __work_folder, action->id) ;
#else
               snprintf(path, sizeof(path)-1, "%s\\SetStatus.%s.KillAfter", __work_folder, action->id) ;
#endif

            file=fopen(path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, path) ;
                            status=-1 ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
/*- - - - - - - - - - - - - - - - Занесение операции в FILES_ACTIONS */
            strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

                      sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                    "values( 'PutEncryptedFile', '%s', '%s', 'NEW', 'Deal:%s')",
                                      __db_table_files_actions, path, sign_list, action->id) ;
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

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA-WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

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
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           } while(0) ;

/*----------------------------------- Ожидание передачи файла данных */

   if(!stricmp(action->status, "DATA-WAIT" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
                      sprintf(text, "select \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                      __db_table_files_actions, action->reply, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "1: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                                          __db_errors_cnt++ ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Check data file transfer: No record") ;
       else                      sprintf(error, "Check data file transfer: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(link,   0, sizeof(link)) ;
           strncpy(link,   (char *)Cursor->columns[1].value, sizeof(link)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Анализ результата */
       } while(0) ;

                 db->UnlockCursor(Cursor) ;

     if(!stricmp(result, "DONE"))  strcpy(action->status, "STATUS") ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*----------------------------------------- Изменение статуса сделки */

   if(!stricmp(action->status, "STATUS"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) "
                                    " and   f1.\"FromSC\" is null ",
                                     __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id       )-1) ;
           strncpy(files[i].file_uuid, (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;

                        }
/*- - - - - - - - - - - - - Извлечение адреса смарт-контракта сделки */
                      sprintf(text, "Select \"BlockChainId\", \"Version\" "
                                    "From   %s "
                                    "Where  \"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;
            memset(version,  0, sizeof(version)) ;
           strncpy(version,  (char *)Cursor->columns[1].value, sizeof(version)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - -  Формирование транзакции изменения */
           code=(char *)calloc(1, _CODE_SIZE) ;

         EMIR_txt2hex64(r_status, status_hex, strlen(r_status)) ;   /* Преобразование Key в HEX */
         EMIR_txt2hex64(r_remark, remark_hex, strlen(r_remark)) ;
         EMIR_txt2hex64(  link,   link_hex,   strlen(  link)) ;

                            reply[0]=0 ;

      do {

             sprintf(code, "cd9a26f6"                               /* Формируем блок данных транзакции */
                           "%s"
                           "00000000000000000000000000000000000000000000000000000000000000a0"
                           "%064x"
                           "%064x" 
                           "%064x", 
                            status_hex, 0x0c0+(int)strlen(remark_hex)/2,
                                        0x0e0+(int)strlen(remark_hex)/2+(int)strlen(link_hex)/2,
                                        0x100+(int)strlen(remark_hex)/2+(int)strlen(link_hex)/2+32*(files_cnt)) ;

             sprintf(tmp, "%064x", (int)strlen(r_remark)) ;
              strcat(code, tmp) ;
              strcat(code, remark_hex) ;

             sprintf(tmp, "%064x", (int)strlen(link)) ;
              strcat(code, tmp) ;
              strcat(code, link_hex) ;

             sprintf(tmp, "%064x", files_cnt) ;
              strcat(code, tmp) ;

        for(i=0 ; i<files_cnt ; i++) {
         EMIR_txt2hex64(files[i].file_uuid,  uuid_hex, strlen(files[i].file_uuid)) ;
                sprintf(text, "%s", uuid_hex) ;
                 strcat(code, text) ;
                                     }

             sprintf(tmp, "%064x", attr_cnt*2) ;
              strcat(code, tmp) ;

         for(i=0 ; i<attr_cnt ; i++) {
              EMIR_txt2hex64(attr[i].key, tmp, strlen(attr[i].key)) ;
                      strcat(code, tmp) ;
              EMIR_txt2hex64(attr[i].value, tmp, strlen(attr[i].value)) ;
                      strcat(code, tmp) ;
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Object\"='%s', \"Reply\"='%s:%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn, version, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - -  Разбор контекста операции */
             memset(txn, 0, sizeof(txn)) ;
            strncpy(txn, action->reply, sizeof(txn)-1) ;
         end=strchr(txn, ':') ;
      if(end==NULL) {
                      strcpy(error, "Invalid data structure in field Reply") ;
                          status=-1 ;
                             break ;
                    }  
        *end=0 ;

             memset(version, 0, sizeof(version)) ;
            strncpy(version, end+1, sizeof(version)-1) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
          status=EMIR_node_checktxn(txn, block, contract, error) ;
       if(status== 0) {
                          db->UnlockCursor(Cursor) ;
                                 return(0) ;
                      }
       if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Сверка статуса сделки */
                           memset(&deal, 0, sizeof(deal)) ;

     for(i=0 ; i<10 ; i++) {

          status=EMIRi_dl_GetDeal(action->object, &deal,
                                   attr_bc,    _ATTR_MAX, &attr_bc_cnt,
                                      NULL,        0,          NULL,
                                      NULL,        0,          NULL,
                                  files_bc,   _FILES_MAX, &files_bc_cnt,
                                history_bc, _HISTORY_MAX, &history_bc_cnt, error) ;

       if(status            )  break ;

       if(       deal.version[0]==0       ) {  Sleep( 1000) ;  break ;  }

       if(strcmp(deal.version, version)<=0) {  Sleep(10000) ;  break ;  }

                                break ;
                           }

       if(status)  break ;

       if(strcmp(deal.version, version)<=0) {

                      sprintf(error, "Transaction not affected contract: %s", deal.version) ;
                              status=-1 ;
                                 break ;
                                            }

       if(!strcmp(deal.status, r_status) &&
          !strcmp(deal.remark, r_remark)   ) {

         for(i=0 ; i<attr_cnt ; i++) {                             /* Атрибуты проверяем только если это "наш" статус */
          for(j=0 ; j<attr_bc_cnt ; j++)
            if(!stricmp(attr[i].key,   attr_bc[j].key  ) &&
               !stricmp(attr[i].value, attr_bc[j].value)   )  break ;

            if(j>=attr_bc_cnt) {
                     strcpy(error, "Attributes check fail") ;
                              status=-1 ;
                                 break ;
                               }
                                     }
                                             }
       else                                  {                     /* Проверяем по истории состояний... */

EMIR_log("SetStatus - history check") ;

          for(i=0 ; i<history_bc_cnt ; i++) {
            if(strcmp(history_bc[i].version, version)<=0)  continue ;

            if(!stricmp(history_bc[i].status, r_status) &&
               !stricmp(history_bc[i].remark, r_remark) &&
               !stricmp(history_bc[i].actor,   account)   )  break ;
                                            }  

            if(i>=history_bc_cnt) {
                      strcpy(error, "Values check fail") ;
                                  status=-1 ;
                                     break ;
                                  }
                                             }
/*- - - - - - - - - - - - - - - - - - - Извлечение списка документов */
                               files_cnt=0 ;

     if(files_uuid_cnt) {

                      sprintf(text, "Select f1.\"Id\", f1.\"FileUUID\", f1.\"Kind\", f1.\"LocalPath\", f1.\"Hash\", f1.\"DfsPath\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"FileUUID\" in (%s) ",
                                     __db_table_deals_files,
                                       r_id, files_uuid ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

            strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
            strncpy(files[i].file_uuid,  (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid )-1) ;
            strncpy(files[i].kind,       (char *)Cursor->columns[2].value, sizeof(files[i].kind      )-1) ;
            strncpy(files[i].local_path, (char *)Cursor->columns[3].value, sizeof(files[i].local_path)-1) ;
            strncpy(files[i].hash,       (char *)Cursor->columns[4].value, sizeof(files[i].hash      )-1) ;
            strncpy(files[i].dfs_path,   (char *)Cursor->columns[5].value, sizeof(files[i].dfs_path  )-1) ;

             memset(files[i].file_ext, 0, sizeof(files[i].file_ext)) ;
        end=strrchr(files[i].local_path, '.') ;
     if(end!=NULL)  strncpy(files[i].file_ext, end+1, sizeof(files[i].file_ext)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         files_cnt=i ;

                        }
/*- - - - - - - - - - - - - - - - - - - - - Сверка списка документов */
     for(i=0 ; i<files_cnt ; i++) {

  EMIR_log("Files Check") ;
   sprintf(text, "1: %s : %s : %s : %s : %s : %s", 
               r_status, files[i].file_uuid, files[i].kind, files[i].file_ext, files[i].hash, files[i].dfs_path) ;
  EMIR_log(text) ;

      for(j=0 ; j<files_bc_cnt ; j++) {

   sprintf(text, "2: %s : %s : %s : %s : %s : %s", 
               files_bc[j].status, files_bc[j].file_uuid, files_bc[j].kind, files_bc[j].file_ext, files_bc[j].hash, files_bc[j].dfs_path) ;
  EMIR_log(text) ;

        if(!stricmp(files_bc[j].status,    r_status          ) &&
           !stricmp(files_bc[j].file_uuid, files[i].file_uuid) &&
           !stricmp(files_bc[j].kind,      files[i].kind     ) &&
           !stricmp(files_bc[j].file_ext,  files[i].file_ext ) &&
           !stricmp(files_bc[j].hash,      files[i].hash     ) &&
           !stricmp(files_bc[j].dfs_path,  files[i].dfs_path )   )  break ;
                                      }

        if(j>=files_bc_cnt) {
                   sprintf(error, "Check fail for file %s", files[i].id) ;
                  EMIR_log(error) ;
                             status=-1 ;
                                break ;
                            }
                                  }
/*- - - - - - - - - - - - - - - Занесение номера блока с транзакцией */
          block_num=strtoul(block, &end, 0) ;

                    sprintf(text, "update %s "
                                  "set    \"TxnBlock\"='%lu' "
                                  "where  \"Id\"=%s",
                                       __db_table_deals, block_num, r_id) ;
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

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
/*                  Операция START_ARBITRATION                       */

   int  EMIR_dl_action_StartArb(Dl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
    static  char *code ;
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
     Arbitration  arbitration ;
        DealAttr  attr[_ATTR_MAX] ;
             int  attr_cnt ;
        DealAttr  attr_bc[_ATTR_MAX] ;
             int  attr_bc_cnt ;
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
            char  link_hex[512] ;
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
            char  text[16000] ;
            char  tmp[2048] ;
            char *address ;
            char *attr_key ;
            char *attr_val ;
            char *attr_end ;
            char *end ;
             int  i ;
             int  j ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

     if(code==NULL)  code=(char *)calloc(1, _CODE_SIZE) ;

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
/*--------------------------------------- Разбор реквизитов операции */

                 r_remark="" ;
/*- - - - - - - - - - - - - - - - - - - -  Разбор базовых реквизитов */
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

                          end=strchr(r_data, '"') ;
                       if(end==NULL) {  r_id=NULL ;  break ;  }
                         *end=0 ;

                           strupr(r_data) ;
                    }

     } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - -  Разбор атрибутов сделки */
  for(attr_end=end+1, attr_cnt=0 ; ; attr_cnt++) {

        attr_key=strchr(attr_end+1, '"') ;
     if(attr_key==NULL)  break ;

     if(attr_cnt>=_ATTR_MAX) {
                      strcpy(error, "Too many attributes in field Data") ;
                               status=-1 ;
                                  break ;
                             }

        attr_val=strstr(attr_key+1, "\":\"") ;
     if(attr_val==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }
        attr_end=strchr(attr_val+3, '"') ;
     if(attr_end==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                               status=-1 ;
                                  break ;
                        }

         *attr_val=0 ;
         *attr_end=0 ;


     if(!strcmp(attr_key, "Id"    ) ||
        !strcmp(attr_key, "Status") ||
        !strcmp(attr_key, "Remark") ||
        !strcmp(attr_key, "Data"  )   ) { attr_cnt-- ; continue ; }

         memset(&attr[attr_cnt], 0, sizeof(attr[attr_cnt])) ;
        strncpy(attr[attr_cnt].key,   attr_key+1, sizeof(attr[attr_cnt].key  )-1) ;
        strncpy(attr[attr_cnt].value, attr_val+3, sizeof(attr[attr_cnt].value)-1) ;

     if(strlen(attr[attr_cnt].key)>32) {
                      sprintf(error, "Attribute %s in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                       }

     if(strlen(attr[attr_cnt].value)>32) {
                      sprintf(error, "Attribute %s value in field Data too large - limited by 32 symbols", attr[attr_cnt].key) ;
                               status=-1 ;
                                  break ;
                                         }
                                                 }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "NEW" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - Проверка условий запуска арбитража */
                      sprintf(text, "Select d2.\"Locked\" "
                                    "From   %s d1 left join %s d2 on d2.\"Id\"=d1.\"Parent\" "
                                    "Where  d1.\"Id\"=%s ",
                                     __db_table_deals, __db_table_deals, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Check arbitration prerequisites 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                   sprintf(error, "Main deal is missed") ;
                                 break ;
                              }
     else
     if(status              ) {
                   sprintf(error, "Check arbitration prerequisites 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                                 break ;
                             }

            memset(result, 0, sizeof(result)) ;
           strncpy(result,(char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose (Cursor) ;

    if(*result!='0') {                                              /* Если арбитраж уже идёт... */
                        sprintf(error, "Arbitration already in progress") ;
                            status=-1 ;
                              break ;
                     }
/*- - - - - - - - - - - -  Извлечение списка ещё не созданных файлов */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Sign\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'",
                                     __db_table_deals_files, __db_table_deals_files, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Формирование файлов */
     for(i=0 ; i<files_cnt ; i++) {

       if(!stricmp(files[i].relation, "SIGN")) {

                          sprintf(text, "insert into %s (\"Action\", \"ObjectPath\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                        "values( 'SignFile', '%s', '%s.sign', '%s', 'NEW', 'Deal:%s')",
                                           __db_table_files_actions, files[i].p_local_path, files[i].p_local_path, files[i].sign, action->id) ;
            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Sign operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='SIGN', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                       db->UnlockCursor(Cursor) ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "SIGN"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"LocalPath\""
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'"
                                    " and   f2.\"LocalPath\"=a1.\"ObjectPath\""
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "%s", db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                         break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].local_path,(char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on SIGN operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
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
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                                }
/*- - - - - - - - - - - - - - - - -  Отбор файлов для передачи в DFS */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - -  Проверка существования файлов */
     for(i=0 ; i<files_cnt ; i++) {

          if(access(files[i].local_path, 0x04)) {

                        sprintf(error, "File %s is absent or access denied : %s", files[i].id, files[i].local_path) ;
                            status=-1 ;
                              break ;              
                                                }
                                  }
/*- - - - - - - - - - - - - - - - Извлечение сертификатов участников */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign,     (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - -  Передача файлов в DFS */   
                               strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

     for(i=0 ; i<files_cnt ; i++) {

       if(strstr(files[i].kind, "Sign")!=NULL)  oper="TransferFile" ;
       else                                     oper="PutEncryptedFile" ;

       if(!stricmp(files[i].relation, "COPY")) {

                   sprintf(text, "select count(*) from %s where \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                          __db_table_files_actions, files[i].p_local_path, action->id) ;

             status=db->SelectOpen(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                       break ;
                     }

             status=db->SelectFetch(Cursor) ;
          if(status) {
                        sprintf(error, "Check parent COPY - %s", db->error_text) ;
                       EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                       break ;
                     }

              memset(result, 0, sizeof(result)) ;
             strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
           
                 db->SelectClose(Cursor) ;

          if(!stricmp(result, "1"))  continue ;

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                 "values( '%s', '%s', '%s', 'NEW', 'Deal:%s')",
                                          __db_table_files_actions, oper, files[i].p_local_path, sign_list, action->id) ;

                                               }
       else                                    { 

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                 "values( '%s', '%s', '%s', 'NEW', 'Deal:%s')",
                                          __db_table_files_actions, oper, files[i].local_path, sign_list, action->id) ;

                                               }

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;        
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILES', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*---------------------------------- Передача файлов на СК арбитража */

   if(!stricmp(action->status, "FILES"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - -  Контроль завершения операций с COPY-файлами */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f2.\"LocalPath\" "
                                    "     , a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' and a1.\"LocalPath\"=f2.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')"
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].parent_id,  (char *)Cursor->columns[1].value, sizeof(files[i].parent_id )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].status,     (char *)Cursor->columns[3].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[4].value, sizeof(files[i].dfs_path  )-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - Обработка завершения операций с COPY-файлами */
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
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - Контроль пассивных операций с COPY-файлами */
                      sprintf(text, "Select f1.\"Id\", f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null "
                                    " and   f1.\"Relation\"='COPY' ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
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
                           __db_errors_cnt++ ;
                                   break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].dfs_path,   (char *)Cursor->columns[2].value, sizeof(files[i].dfs_path  )-1) ;

     if( stricmp(files[i].dfs_path, "" )) {                         /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                          }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - -  Обработка пассивных операций с COPY-файлами */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"LocalPath\"='%s', \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, 
                                        files[i].local_path, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1, %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null "
                                    " and  (f1.\"Relation\"<>'COPY' or f1.\"Relation\" is null) "
                                    " and   a1.\"LocalPath\"=f1.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')"
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_files_actions,
                                       r_id, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,  (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                      sprintf(text, "Select f1.\"Id\", f1.\"Kind\", f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Remark\", f1.\"Recipients\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s",
                                     __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - -  Контроль трансфера файлов */
     for(i=0 ; i<files_cnt ; i++) {

        if(files[i].dfs_path[0]==0) {
                                         status=-1 ;
                      sprintf(error, "Check Transfer operation : File %s not transfered", files[i].id) ;
                     EMIR_log(error) ;
                                          break ;
                                    }

                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - Формирование хэшей и расширений файлов */
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
                                  __db_errors_cnt++ ;
                                          break ;
                   }
                                  }

         if(status)  break ;
/*- - - - - - - - - - -  Извлечение адреса смарт-контракта арбитража */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - -  Формирование и отправка файловых транзакций */
             memset(txn_list, 0, sizeof(txn_list)) ;

     for(i=0 ; i<files_cnt ; i++) {

                 strcpy(uuid_hex, "0000000000000000000000000000000000000000000000000000000000000000") ;
                 strcat(uuid_hex, files[i].file_uuid) ;
                memmove(uuid_hex, uuid_hex+strlen(files[i].file_uuid), 64) ;
                        uuid_hex[64]=0 ;

         EMIR_txt2hex64(files[i].kind,       kind_hex,       strlen(files[i].kind)) ;
         EMIR_txt2hex64(files[i].file_ext,   ext_hex,        strlen(files[i].file_ext)) ;
         EMIR_txt2hex64(files[i].dfs_path,   link_hex,       strlen(files[i].dfs_path)) ;
         EMIR_txt2hex64(files[i].remark,     remark_hex,     strlen(files[i].remark)) ;
         EMIR_txt2hex64(files[i].recipients, recipients_hex, strlen(files[i].recipients)) ;

                            reply[0]=0 ;

      do {

             sprintf(code, "9b4a25f6"                               /* Формируем блок данных транзакции */
                           "%s"
                           "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "00000000000000000000000000000000000000000000000000000000000000e0"
                           "%064x"
                           "%064x",
                            uuid_hex,
                            kind_hex, ext_hex, files[i].hash,
                            0x100+(int)strlen(link_hex)/2,
                            0x120+(int)strlen(link_hex)/2+(int)strlen(remark_hex)/2 ) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].dfs_path)) ;
              strcat(code, tmp) ;
              strcat(code, link_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].remark)) ;
              strcat(code, tmp) ;
              strcat(code, remark_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].recipients)) ;
              strcat(code, tmp) ;
              strcat(code, recipients_hex) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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

           if(status)  break ;

                                      strcat(txn_list, txn) ;
                                      strcat(txn_list, ",") ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*-------------------------------------------- Передача файла данных */

   if(!stricmp(action->status, "DATA" )) do {
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                       status=0 ;

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
/*- - - - - - - - - - - - - - - - - - Если файл данных не передаётся */
       if(r_data[0]=='N') {
                             strcpy(action->status, "STATUS") ;
                             strcpy(link, "") ;
                                 break ;
                          }
/*- - - - - - - - - - - - - - - - - - - - - - - Выделение курсора БД */
    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - Извлечение заголовочных данных */
                      sprintf(text, "Select r.\"Data\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"='%s'",
                                    __db_table_deals, r_id) ;
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

            memset(data,   0, sizeof(data)) ;
           strncpy(data,   (char *)Cursor->columns[0].value, sizeof(data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - -  Запрос списка участников сделки */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role ,    (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign ,    (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_PARTY_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many parties") ;
                            status=-1 ;
                              break ;
                      }  

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - Проверка наличия участников сделки */
       if(party_cnt==0) {
                           sprintf(error, "Empty party list") ;
                            status=-1 ;
                             break ;
                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - Формирование файла */
                   if(data[0]==0)  strcpy(data, " ") ;              /* Если данных нет - делаем заглушку */

#ifdef  UNIX
//             snprintf(path, sizeof(path)-1, "%s/SetStatus_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(path, sizeof(path)-1, "%s/SetStatus.%s", __work_folder, action->id) ;
#else
//             snprintf(path, sizeof(path)-1, "%s\\SetStatus_%lx.%d", __work_folder, (long)time(NULL), __fl_file_idx++) ;
               snprintf(path, sizeof(path)-1, "%s\\SetStatus.%s", __work_folder, action->id) ;
#endif

            file=fopen(path, "wb") ;
         if(file==NULL) {
                           sprintf(error, "Data file creation error %d : %s", errno, path) ;
                            status=-1 ;
                             break ;
                        }

                  fwrite(data, 1, strlen(data), file) ;
                  fclose(file) ;
/*- - - - - - - - - - - - - - - - Занесение операции в FILES_ACTIONS */
            strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

                      sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                    "values( 'PutEncryptedFile', '%s', '%s', 'NEW', 'Deal:%s')",
                                      __db_table_files_actions, path, sign_list, action->id) ;
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

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='DATA-WAIT', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;

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
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                           } while(0) ;

/*----------------------------------- Ожидание передачи файла данных */

   if(!stricmp(action->status, "DATA-WAIT" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_AddDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - -  Проверка завершения FILE-операций */
                      sprintf(text, "select \"Status\", \"Reply\" "
                                    "from   %s "
                                    "where  \"LocalPath\"='%s' and \"MasterId\"='Deal:%s'",
                                       __db_table_files_actions, action->reply, action->id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "1: %s", db->error_text) ;
                                            db->error_text[0]=0 ;
                                          __db_errors_cnt++ ;
                         break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Check data file transfer: No record") ;
       else                      sprintf(error, "Check data file transfer: %s", db->error_text) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
            memset(link,   0, sizeof(link)) ;
           strncpy(link,   (char *)Cursor->columns[1].value, sizeof(link)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - -  Анализ результата */
       } while(0) ;

                 db->UnlockCursor(Cursor) ;

     if(!stricmp(result, "DONE"))  strcpy(action->status, "STATUS") ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*---------------------------------------- Запуск процесса арбитража */

   if(!stricmp(action->status, "STATUS"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                      sprintf(text, "Select f1.\"Id\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s",
                                     __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id       )-1) ;
           strncpy(files[i].file_uuid, (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - -  Извлечение адреса смарт-контракта арбитража */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - -  Формирование транзакции изменения */
           code=(char *)calloc(1, _CODE_SIZE) ;

         EMIR_txt2hex64(r_status, status_hex, strlen(r_status)) ;   /* Преобразование Key в HEX */
         EMIR_txt2hex64(r_remark, remark_hex, strlen(r_remark)) ;
         EMIR_txt2hex64(  link,   link_hex,   strlen(  link)) ;

                            reply[0]=0 ;

      do {

             sprintf(code, "812d557f"                               /* Формируем блок данных транзакции */
                           "%s"
                           "0000000000000000000000000000000000000000000000000000000000000080"
                           "%064x"
                           "%064x", 
                            status_hex, 0x0a0+(int)strlen(remark_hex)/2,
                                        0x0c0+(int)strlen(remark_hex)/2+(int)strlen(link_hex)/2) ;

             sprintf(tmp, "%064x", (int)strlen(r_remark)) ;
              strcat(code, tmp) ;
              strcat(code, remark_hex) ;

             sprintf(tmp, "%064x", (int)strlen(link)) ;
              strcat(code, tmp) ;
              strcat(code, link_hex) ;

             sprintf(tmp, "%064x", attr_cnt*2) ;
              strcat(code, tmp) ;

         for(i=0 ; i<attr_cnt ; i++) {
              EMIR_txt2hex64(attr[i].key, tmp, strlen(attr[i].key)) ;
                      strcat(code, tmp) ;
              EMIR_txt2hex64(attr[i].value, tmp, strlen(attr[i].value)) ;
                      strcat(code, tmp) ;
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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

           if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - -  Извлечение адреса СК сделки */
                      sprintf(text, "Select \"ParentBlockChainId\" "
                                    "From   %s  "
                                    "Where  \"Id\"=%s ",
                                     __db_table_deals, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get deal address 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                   sprintf(error, "Main deal is missed") ;
                                 break ;
                              }
     else
     if(status              ) {
                   sprintf(error, "Get deal address 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                                 break ;
                             }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract,(char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Сверка статуса сделки */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(contract, &deal,
                                            NULL, 0, NULL,
                                            NULL, 0, NULL,
                                            NULL, 0, NULL,
                                            NULL, 0, NULL,
                                            NULL, 0, NULL, error) ;
           if(status)  break ;

           if(deal.locked[0]!='1') {
                   sprintf(error, "Arbitration process mark not set on deal") ;
                                         status=-1 ;
                                            break ;
                                   }
/*- - - - - - - - - - - - - - - - - - - - - Сверка статуса арбитража */
                              memset(&arbitration, 0, sizeof(arbitration)) ;
              status=EMIRi_dl_GetArb(action->object, &arbitration,
                                            attr_bc,  _ATTR_MAX,  &attr_bc_cnt,
                                            NULL,       0,          NULL,
                                            NULL,       0,          NULL,
                                            files_bc, _FILES_MAX, &files_bc_cnt,
                                            NULL,       0,          NULL,  error) ;

           if(status)  break ;

           if(strcmp(arbitration.status,      "StatusArbitration") ||
              strcmp(arbitration.deal_status, r_status           ) ||
              strcmp(arbitration.deal_remark, r_remark           )   ) {
                      strcpy(error, "Values check fail") ;
                              status=-1 ;
                                 break ;
                                                                       }

       for(i=0 ; i<attr_cnt ; i++) {
         for(j=0 ; j<attr_bc_cnt ; j++)
           if(!stricmp(attr[i].key,   attr_bc[j].key  ) &&
              !stricmp(attr[i].value, attr_bc[j].value)   )  break ;

           if(j>=attr_bc_cnt) {
                      strcpy(error, "Attributes check fail") ;
                              status=-1 ;
                                 break ;
                              }
                                   }
/*- - - - - - - - - - - - - - - - - - - Извлечение списка документов */
                      sprintf(text, "Select f1.\"Id\", f1.\"Kind\", f1.\"LocalPath\", f1.\"Hash\", f1.\"DfsPath\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s ",
                                     __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - - - - - - Сверка списка документов */
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
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
/*                  Операция ACCEPT_ARBITRATION                      */

   int  EMIR_dl_action_AcceptArb(Dl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
    static  char *code ;
            char  account[128] ;
            char *password ;
            char  json[3048] ;
            char *r_id ;
            char *r_remark ;
            char  sign_list[512] ;
     Arbitration  arbitration ;
//      DealAttr  attr[_ATTR_MAX] ;
//           int  attr_cnt ;
//      DealAttr  attr_bc[_ATTR_MAX] ;
//           int  attr_bc_cnt ;
       DealParty  party[_PARTY_MAX] ;
             int  party_cnt ;
        DealFile  files[_FILES_MAX] ;
             int  files_cnt ;
        DealFile  files_bc[_FILES_MAX] ;
             int  files_bc_cnt ;
     DealHistory  history_bc[_FILES_MAX] ;
             int  history_bc_cnt ;
             int  done_flag ;
            char *oper ;
            char  remark_hex[17000] ;
            char  link_hex[512] ;
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
            char  text[16000] ;
            char  tmp[2048] ;
            char *address ;
            char *end ;
             int  i ;

#define  _CODE_SIZE    64000

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

     if(code==NULL)  code=(char *)calloc(1, _CODE_SIZE) ;

        status=EMIR_db_nodepars(db, error) ;
     if(status)  return(-1) ;

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
/*--------------------------------------- Разбор реквизитов операции */

                 r_remark="" ;
/*- - - - - - - - - - - - - - - - - - - -  Разбор базовых реквизитов */
  do {
                 memset(json, 0, sizeof(json)) ;
                strncpy(json, action->data, sizeof(json)-1) ;
       r_id    = strstr(json, "\"Id\":\"") ;
       r_remark= strstr(json, "\"Remark\":\"") ;
	
    if(r_id    ==NULL ||
       r_remark==NULL   ) {  r_id=NULL ;  break ;  }

       r_id    =strstr(r_id,      "\":\"")+3 ;
       r_remark=strstr(r_remark,  "\":\"")+3 ;
       end=strchr(r_id, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

       end=strchr(r_remark, '"') ;
    if(end==NULL) {  r_id=NULL ;  break ;  }
      *end=0 ;

    if(strlen(r_remark      )> 1000 ||
       strchr(r_remark, '\'')!=NULL   ) {  r_id=NULL ;  break ;  }

     } while(0) ;
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "NEW" )) {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - Проверка условий запуска арбитража */
                      sprintf(text, "Select d2.\"Locked\" "
                                    "From   %s d1 left join %s d2 on d2.\"Id\"=d1.\"Parent\" "
                                    "Where  d1.\"Id\"=%s ",
                                     __db_table_deals, __db_table_deals, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Check arbitration prerequisites 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                   sprintf(error, "Main deal is missed") ;
                                 break ;
                              }
     else
     if(status              ) {
                   sprintf(error, "Check arbitration prerequisites 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                                 break ;
                             }

            memset(result, 0, sizeof(result)) ;
           strncpy(result,(char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose (Cursor) ;

    if(*result=='0') {                                              /* Если арбитраж ещё не запущен... */
                        sprintf(error, "Arbitration not started") ;
                            status=-1 ;
                              break ;
                     }
/*- - - - - - - - - - - -  Извлечение списка ещё не созданных файлов */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\", f1.\"Sign\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'",
                                     __db_table_deals_files, __db_table_deals_files, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Формирование файлов */
     for(i=0 ; i<files_cnt ; i++) {

       if(!stricmp(files[i].relation, "SIGN")) {

                          sprintf(text, "insert into %s (\"Action\", \"ObjectPath\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                        "values( 'SignFile', '%s', '%s.sign', '%s', 'NEW', 'Deal:%s')",
                                           __db_table_files_actions, files[i].p_local_path, files[i].p_local_path, files[i].sign, action->id) ;
            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Sign operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
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
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='SIGN', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "SIGN"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"LocalPath\""
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\", %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"LocalPath\" is null "
                                    " and   f1.\"Relation\"<>'COPY'"
                                    " and   f2.\"LocalPath\"=a1.\"ObjectPath\""
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_deals_files, __db_table_files_actions,
                                       r_id, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "%s", db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                         break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].local_path,(char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on SIGN operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
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
                                 __db_errors_cnt++ ;
                               return(-1) ;
                }

                                }
/*- - - - - - - - - - - - - - - - -  Отбор файлов для передачи в DFS */
                      sprintf(text, "Select f1.\"Id\", f1.\"Parent\", f1.\"Relation\", f1.\"Kind\" "
                                    "     , f1.\"LocalPath\", f1.\"DfsPath\" "
                                    "     , f2.\"LocalPath\", f2.\"DfsPath\" "
                                    "From   %s f1 left join %s f2 on f2.\"Id\"=f1.\"Parent\" "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null ",
                                     __db_table_deals_files, __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - -  Проверка существования файлов */
     for(i=0 ; i<files_cnt ; i++) {

          if(access(files[i].local_path, 0x04)) {

                        sprintf(error, "File %s is absent or access denied : %s", files[i].id, files[i].local_path) ;
                            status=-1 ;
                              break ;              
                                                }
                                  }
/*- - - - - - - - - - - - - - - - Извлечение сертификатов участников */
                      sprintf(text, "Select p.\"PartyId\", p.\"Role\", m.\"Sign\" "
                                    "From   %s p left join %s m on m.\"Key\"=p.\"PartyId\""
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, __db_table_members, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].party_id, (char *)Cursor->columns[0].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[1].value, sizeof(party[i].role    )-1) ;
           strncpy(party[i].sign,     (char *)Cursor->columns[2].value, sizeof(party[i].sign    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - -  Передача файлов в DFS */   
                               strcpy(sign_list, "") ;

     for(i=0 ; i<party_cnt ; i++)
       if(party[i].sign[0]) {
            if(strlen(sign_list)>0)  strcat(sign_list, ","          ) ;
                                     strcat(sign_list, party[i].sign) ;
                            }

     for(i=0 ; i<files_cnt ; i++) {

         if(strstr(files[i].kind, "Sign")!=NULL)  oper="TransferFile" ;
         else                                     oper="PutEncryptedFile" ;

                   sprintf(text, "insert into %s (\"Action\", \"LocalPath\", \"Receivers\", \"Status\", \"MasterId\") "
                                 "values( '%s', '%s', '%s', 'NEW', 'Deal:%s')",
                                          __db_table_files_actions, oper, files[i].local_path, sign_list, action->id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Insert Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;        
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILES', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->reply, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*---------------------------------- Передача файлов на СК арбитража */

   if(!stricmp(action->status, "FILES"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - Контроль завершения операций с файлами */
                      sprintf(text, "Select f1.\"Id\", a1.\"Status\", a1.\"Reply\" "
                                    "From   %s f1, %s a1 "
                                    "Where  f1.\"DealId\"=%s "
                                    " and   f1.\"DfsPath\" is null "
                                    " and  (f1.\"Relation\"<>'COPY' or f1.\"Relation\" is null) "
                                    " and   a1.\"LocalPath\"=f1.\"LocalPath\" "
                                    " and   a1.\"Action\" in ('TransferFile', 'PutEncryptedFile')"
                                    " and   a1.\"MasterId\"='Deal:%s'",
                                     __db_table_deals_files, __db_table_files_actions,
                                       r_id, action->id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Check DFS-transfer list : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].status,    (char *)Cursor->columns[1].value, sizeof(files[i].status    )-1) ;
           strncpy(files[i].dfs_path,  (char *)Cursor->columns[2].value, sizeof(files[i].local_path)-1) ;

     if(!stricmp(files[i].status, "ERROR")) {                       /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation (DealFile.Id=%s)", files[i].id) ;
                                                  status=-1 ;
                                                     break ;
                                            }
     else
     if( stricmp(files[i].status, "DONE" )) {                       /* Если операция еще не выполнена... */
                                                 done_flag=0 ;
                                            }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }

                                files_cnt=i ;
/*- - - - - - - - - - - - -  Обработка завершения операций с файлами */
     for(i=0 ; i<files_cnt ; i++) {

                   sprintf(text, "update %s set \"DfsPath\"='%s' where \"Id\"=%s",
                                     __db_table_deals_files, files[i].dfs_path, files[i].id) ;

            status=db->SqlExecute(Cursor, text, NULL, 0) ;
         if(status) {
                      sprintf(error, "Registry result of Transfer operation : %s", db->error_text) ;
                     EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                                          break ;
                    }
                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                      sprintf(text, "Select \"Id\", \"Kind\", \"LocalPath\", \"DfsPath\", \"Remark\", \"Recipients\", \"FileUUID\" "
                                    "From   %s "
                                    "Where  \"DealId\"=%s and (\"DataUpdate\"='' or \"DataUpdate\" is null)",
                                     __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
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

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - -  Контроль трансфера файлов */
     for(i=0 ; i<files_cnt ; i++) {

        if(files[i].dfs_path[0]==0) {
                                         status=-1 ;
                      sprintf(error, "Check Transfer operation : File %s not transfered", files[i].id) ;
                     EMIR_log(error) ;
                                          break ;
                                    }

                                  }

         if(status)  break ;
/*- - - - - - - - - - - - - - Формирование хэшей и расширений файлов */
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
                                  __db_errors_cnt++ ;
                                          break ;
                   }
                                  }

         if(status)  break ;
/*- - - - - - - - - - -  Извлечение адреса смарт-контракта арбитража */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get deal contract - No record") ;
       else                      sprintf(error, "Get deal contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - -  Формирование и отправка файловых транзакций */
             memset(txn_list, 0, sizeof(txn_list)) ;

     for(i=0 ; i<files_cnt ; i++) {

                 strcpy(uuid_hex, "0000000000000000000000000000000000000000000000000000000000000000") ;
                 strcat(uuid_hex, files[i].file_uuid) ;
                memmove(uuid_hex, uuid_hex+strlen(files[i].file_uuid), 64) ;
                        uuid_hex[64]=0 ;

         EMIR_txt2hex64(files[i].kind,       kind_hex,       strlen(files[i].kind)) ;
         EMIR_txt2hex64(files[i].file_ext,   ext_hex,        strlen(files[i].file_ext)) ;
         EMIR_txt2hex64(files[i].dfs_path,   link_hex,       strlen(files[i].dfs_path)) ;
         EMIR_txt2hex64(files[i].remark,     remark_hex,     strlen(files[i].remark)) ;
         EMIR_txt2hex64(files[i].recipients, recipients_hex, strlen(files[i].recipients)) ;

                            reply[0]=0 ;

      do {

             sprintf(code, "9b4a25f6"                               /* Формируем блок данных транзакции */
                           "%s"
                           "%s"
                           "%s"
                           "000000000000000000000000%s"
                           "00000000000000000000000000000000000000000000000000000000000000e0"
                           "%064x"
                           "%064x",
                            uuid_hex,
                            kind_hex, ext_hex, files[i].hash,
                            0x100+(int)strlen(link_hex)/2,
                            0x120+(int)strlen(link_hex)/2+(int)strlen(remark_hex)/2 ) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].dfs_path)) ;
              strcat(code, tmp) ;
              strcat(code, link_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].remark)) ;
              strcat(code, tmp) ;
              strcat(code, remark_hex) ;

             sprintf(tmp, "%064x", (int)strlen(files[i].recipients)) ;
              strcat(code, tmp) ;
              strcat(code, recipients_hex) ;

              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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

           if(status)  break ;

                                      strcat(txn_list, txn) ;
                                      strcat(txn_list, ",") ;
                                  }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='STATUS', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn_list, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------------- Передача подтверждения */

   if(!stricmp(action->status, "STATUS" )) {
/*- - - - - - - - - - - - - - - - - - - - - - - Выделение курсора БД */
    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration or too large Remark (>1000) in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанций */
                       status=0 ;

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
/*- - - - - - - - - - - - - - -  Отбор файлов для привязки к статусу */
                      sprintf(text, "Select f1.\"Id\", f1.\"FileUUID\" "
                                    "From   %s f1 "
                                    "Where  f1.\"DealId\"=%s",
                                     __db_table_deals_files,
                                       r_id ) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }


   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id       )-1) ;
           strncpy(files[i].file_uuid, (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - -  Извлечение адреса смарт-контракта арбитража */
                      sprintf(text, "Select r.\"BlockChainId\" "
                                    "From   %s r "
                                    "Where  r.\"Id\"=%s",
                                    __db_table_deals, r_id) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get sertificates - %s",  db->error_text) ;
                   EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                             break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
       if(status==_SQL_NO_DATA)  sprintf(error, "Get arbitration contract - No record") ;
       else                      sprintf(error, "Get arbitration contract - %s", db->error_text) ;
                                EMIR_log(error) ;
                                    db->error_text[0]=0 ;
                                  __db_errors_cnt++ ;
                   break ;
                }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract, (char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - -  Формирование транзакции подтверждения */
         EMIR_txt2hex64(r_remark, remark_hex, strlen(r_remark)) ;   /* Преобразование Key в HEX */

                            reply[0]=0 ;

      do {

             sprintf(code, "c10243fc"                               /* Формируем блок данных транзакции */
                           "0000000000000000000000000000000000000000000000000000000000000020"
                           "%064x"
                           "%s", 
                            (int)strlen(r_remark), remark_hex) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
              status=EMIR_node_checkgas(account,                    /* Рассчёт газа на транзакцию */
                                         contract, code, gas, error) ;
           if(status)  break ;

EMIR_log("Gas for arbitrage") ;
EMIR_log(gas) ;

                strcpy(gas, "0x500000") ;

              status=EMIR_node_unlockaccount(account,               /* Разблокировка счета */
                                              password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(account,                /* Отправка транзакции */ 
                                             contract, code, gas, txn, error) ;
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

           if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

               db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='WAIT', \"Object\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, contract, txn, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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

                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         } 
/*------------------------------------------- Ожидание подтверждения */

   if(!stricmp(action->status, "WAIT"))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_SetStatus") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                                              return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - Контроль реквизитов операции */
     if(r_id==NULL) {
                      strcpy(error, "Invalid declaration in field Data") ;
                          status=-1 ;
                             break ;
                    }
/*- - - - - - - - - - - - - - - - - - - - - - -  Получение квитанции */
              status=EMIR_node_checktxn(action->reply, block, contract, error) ;
           if(status== 0) {
                              db->UnlockCursor(Cursor) ;
                                     return(0) ;
                          }
           if(status < 0)   break ;
/*- - - - - - - - - - - - - - - - - - Извлечение адреса СК арбитража */
                      sprintf(text, "Select \"ParentBlockChainId\" "
                                    "From   %s  "
                                    "Where  \"Id\"=%s ",
                                     __db_table_deals, r_id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get deal address 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                   sprintf(error, "Main deal is missed") ;
                                 break ;
                              }
     else
     if(status              ) {
                   sprintf(error, "Get main deal address 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                                 break ;
                             }

            memset(contract, 0, sizeof(contract)) ;
           strncpy(contract,(char *)Cursor->columns[0].value, sizeof(contract)-1) ;

                 db->SelectClose (Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - Сверка статуса арбитража */
                              memset(&arbitration, 0, sizeof(arbitration)) ;
              status=EMIRi_dl_GetArb(action->object, &arbitration,
                                            NULL,        0,          NULL,
                                            NULL,        0,          NULL,
                                            NULL,        0,          NULL,
                                            files_bc,   _FILES_MAX, &files_bc_cnt,
                                            history_bc, _FILES_MAX, &history_bc_cnt, error) ;
           if(status)  break ;

       for(i=0 ; i<history_bc_cnt ; i++)
         if(!stricmp(history_bc[i].status, "AcceptStatusArbitration") &&
            !stricmp(history_bc[i].actor,  __member_account         )   )  break ;
 
         if(i>=history_bc_cnt) {
                   sprintf(error, "Operation is missed on arbitration history") ;
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
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
/*                           Операция GET_DEAL                       */

   int  EMIR_dl_action_GetDeal(Dl_action *action, SQL_link *db, char *error)
{
          SQL_cursor *Cursor ;
                 int  status ;
                 int  new_deal ;
                Deal  deal ;
 static     DealAttr *attr ;
                 int  attr_cnt ;
 static     DealAttr *attr_bc ;
                 int  attr_bc_cnt ;
 static    DealParty *party ;
                 int  party_cnt ;
 static    DealParty *party_bc ;
                 int  party_bc_cnt ;
 static     DealFile *files ;
                 int  files_cnt ;
 static     DealFile *files_bc ;
                 int  files_bc_cnt ;
 static  DealHistory *history ;
                 int  history_cnt ;
 static  DealHistory *history_bc ;
                 int  history_bc_cnt ;
                char  folder[FILENAME_MAX] ;
                char  data[16400] ;
                char  data_path[FILENAME_MAX] ;
                FILE *file ;
                char *oper ;
                char  parent_id[128] ;
                char  result[128] ;
                char  text[16000] ;
                 int  done_flag ;
                 int  i ;
                 int  j ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

  if(attr==NULL) {
                       attr   =(DealAttr    *)calloc(   _ATTR_MAX, sizeof(*attr)) ;
                       attr_bc=(DealAttr    *)calloc(   _ATTR_MAX, sizeof(*attr_bc)) ;
                      party   =(DealParty   *)calloc(  _PARTY_MAX, sizeof(*party)) ;
                      party_bc=(DealParty   *)calloc(  _PARTY_MAX, sizeof(*party_bc)) ;
                      files   =(DealFile    *)calloc(  _FILES_MAX, sizeof(*files)) ;
                      files_bc=(DealFile    *)calloc(  _FILES_MAX, sizeof(*files_bc)) ;
                    history   =(DealHistory *)calloc(_HISTORY_MAX, sizeof(*history)) ;
                    history_bc=(DealHistory *)calloc(_HISTORY_MAX, sizeof(*history_bc)) ;
                 }
/*------------------------------------------ Получение данных Сделки */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - Запрос данных Сделки из БЧ */
                               memset(&deal, 0, sizeof(deal)) ;
              status=EMIRi_dl_GetDeal(action->object, &deal,
                                       attr_bc,       _ATTR_MAX,  &attr_bc_cnt,
                                       party_bc,     _PARTY_MAX, &party_bc_cnt,
                                       NULL,               0,     NULL,
                                       files_bc,     _FILES_MAX, &files_bc_cnt,
                                       history_bc, _HISTORY_MAX, &history_bc_cnt, error) ;
           if(status<0)  break ;
/*- - - - - - - - - - - - - - - - - - - Проверка наличия Сделки в БД */
                              new_deal=0 ;
                       memset(result, 0, sizeof(result)) ;

                      sprintf(text, "select \"DataUpdate\" from %s where  \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Check deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {
                                 status=0 ;  new_deal=1 ;
                              }
     else
     if(status              ) {

                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Check deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                              }
     else                     {
                                 new_deal=0 ;

           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
                              }

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - Если предыдущая операция GET_DEAL не завершена */
   if(result[0]=='W')  {
                          EMIR_log("GetDeal - The previous operation was not completed") ;

                              db->UnlockCursor(Cursor) ;
                                     return(0) ;

                       }
/*- - - - - - - - - - - - - - - - -  Определение родительской Сделки */
   if(new_deal) {

                strcpy(parent_id, "NULL") ;

      if(stricmp(deal.parent, _NULL_ADDR)) {

                         sprintf(text, "select \"Id\" from %s where  \"BlockChainId\"='%s'",
                                        __db_table_deals, deal.parent) ;
           status=db->SelectOpen(Cursor, text, NULL, 0) ;
        if(status) {
                     EMIR_log(text) ;
                     EMIR_log(db->error_text) ;
                      sprintf(error, "Check parent: %s", db->error_text) ;
                                     db->error_text[0]=0 ;
                                   __db_errors_cnt++ ;
                                         break ;
                   }

           status=db->SelectFetch(Cursor) ;
        if(status==_SQL_NO_DATA) {
                                 }
        else
        if(status) {
                     EMIR_log(text) ;
                     EMIR_log(db->error_text) ;
                      sprintf(error, "Check parent: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                   }
        else       {
                       memset(parent_id, 0, sizeof(result)) ;
                      strncpy(parent_id, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
                   }

                       db->SelectClose(Cursor) ;
                                           }

                }
/*- - - - - - - - - - - - - - - -  Создание/обновление записи Сделки */
 sprintf(text, "DealState> BlockChainId:%s Status:%s Version:%s", deal.address, deal.status, deal.version) ;
EMIR_log(text) ;

       if(new_deal)  sprintf(text, "insert into %s(\"DealsUUID\",\"Kind\",\"Status\",\"Remark\",\"Locked\","
                                                       " \"Version\",\"BlockChainId\",\"Parent\",\"ParentBlockChainId\",\"ArbitrationBlockChainId\","
                                                       " \"ChannelId\",\"DataUpdate\") "
                                   "values('%s','%s','%s','%s','%s','%s','%s',%s,'%s','%s','%s','WA')",
                                           __db_table_deals,
                                               deal.id,      deal.kind,    deal.status, deal.remark, deal.locked,
                                               deal.version, deal.address,   parent_id, deal.parent, deal.arbitration, deal.channel ) ;
       else          sprintf(text, "update %s set \"Status\"='%s',\"Remark\"='%s',\"Locked\"='%s',\"ArbitrationBlockChainId\"='%s',\"Version\"='%s',\"DataUpdate\"='WU' "
                                   "where  \"BlockChainId\"='%s'",
                                           __db_table_deals, deal.status, deal.remark, deal.locked, deal.arbitration, deal.version, deal.address) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Insert/Update deal record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - -  Запрос обновления арбитражного СК */
   if(       deal.arbitration[0]!=0        &&
      strcmp(deal.arbitration, _NULL_ADDR)   ) {

                        sprintf(text, "insert into %s (\"Action\",  \"Object\", \"Status\")"
                                      "         values('GetArbitration', '%s',      'NEW')"  ,
                                        __db_table_deals_actions, deal.arbitration) ;
          status=db->SqlExecute(Cursor, text, NULL, 0) ;
       if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Registry GetArbitration operation (Address=%s) : %s", deal.arbitration, db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                  }

                                               }
/*- - - - - - - - - - - - - - - -  Определение идентификатора Сделки */
                      sprintf(text, "select \"Id\" from %s where \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get deal Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get deal Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

            memset(action->data, 0, sizeof(action->data)) ;
           strncpy(action->data, (char *)Cursor->columns[0].value, sizeof(action->data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - Создание папки сделки в файловом хранилище */
#ifdef UNIX
                snprintf(folder, sizeof(folder)-1, "%s/%s", __file_storage, action->data) ;
#else
                snprintf(folder, sizeof(folder)-1, "%s\\%s", __file_storage, action->data) ;
#endif

         if(access(folder, 0x00)) {

#ifdef UNIX
              status= mkdir(folder, 0777) ;
#else
              status= mkdir(folder) ;
#endif

           if(status) {
                    sprintf(error, "Deal's folder creation error %d : %s", errno, folder) ;
                                       break ;
                      }
                     
                                  }
/*- - - - - - - - - - - - - - - - - - - - - - Запрос файла заголовка */
                            *data_path=0 ;

#ifdef  UNIX
                snprintf(data_path, sizeof(data_path)-1, "%s/%s/%s", __file_storage, action->data, deal.link) ;
#else
                snprintf(data_path, sizeof(data_path)-1, "%s\\%s\\%s", __file_storage, action->data, deal.link) ;
#endif

                              oper="GetEncryptedFile" ;

                      sprintf(text, "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\",\"MasterId\") "
                                         "values('%s','%s','%s','%s','NEW','Deal:%s')",
                                           __db_table_files_actions,
                                               oper, action->data, data_path, deal.link, action->id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        EMIR_log(text) ;
                        EMIR_log(db->error_text) ;
                         sprintf(error, "Insert/Update data file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
/*- - - - - - - - - - - - - Получение списка участников Сделки из БД */
                      sprintf(text, "Select p.\"Id\", p.\"PartyId\", p.\"Role\" "
                                    "From   %s p "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, action->data) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].id,       (char *)Cursor->columns[0].value, sizeof(party[i].id      )-1) ;
           strncpy(party[i].party_id, (char *)Cursor->columns[1].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[2].value, sizeof(party[i].role    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - Определение новых участников */
      for(i=0 ; i<party_bc_cnt ; i++)  party_bc[i].flag=1 ;

      for(i=0 ; i<party_bc_cnt ; i++)
      for(j=0 ; j<party_cnt    ; j++)
        if(!stricmp(party_bc[i].party_id, party[j].party_id) &&
           !stricmp(party_bc[i].role,     party[j].role    )   ) {
                party_bc[i].flag=0 ;   break ;
                                                                 }
/*- - - - - - - - - - - - - - - - - - - - Обработка новых участников */
      for(i=0 ; i<party_bc_cnt ; i++)
        if(party_bc[i].flag) {

                      sprintf(text, "insert into %s(\"DealId\",\"PartyId\",\"Role\",\"DataUpdate\") "
                                         "values(%s,'%s','%s','A')",
                                           __db_table_deals_parties,
                                               action->data, party_bc[i].party_id, party_bc[i].role) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Insert/Update party record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
                             }

    if(status)  break ;
/*- - - - - - - - - - - - - - - Получение списка файлов Сделки из БД */
                      sprintf(text, "Select \"Id\", \"FileUUID\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s and (\"DfsPath\"<>'' or \"DfsPath\"  is not null)",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                    sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,        (char *)Cursor->columns[0].value, sizeof(files[i].id       )-1) ;
           strncpy(files[i].file_uuid, (char *)Cursor->columns[1].value, sizeof(files[i].file_uuid)-1) ;
                                  }

                 db->SelectClose(Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - Определение новых файлов */
      for(i=0 ; i<files_bc_cnt ; i++)  files_bc[i].flag=1 ;

      for(i=0 ; i<files_bc_cnt ; i++)
      for(j=0 ; j<files_cnt    ; j++)
        if(!stricmp(files_bc[i].file_uuid, files[j].file_uuid)) {
                files_bc[i].flag=0 ;   break ;
                                                                }
/*- - - - - - - - - - - - - - - - - - - - - - Обработка новых файлов */
      for(i=0 ; i<files_bc_cnt ; i++)
        if(files_bc[i].flag) {

#ifdef UNIX
                snprintf(files_bc[i].local_path, sizeof(files_bc[i].local_path)-1, "%s/%s/%s.%s",
                          __file_storage, action->data, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#else
                snprintf(files_bc[i].local_path, sizeof(files_bc[i].local_path)-1, "%s\\%s\\%s.%s",
                          __file_storage, action->data, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#endif

                      sprintf(text, "insert into %s(\"DealId\",\"Version\",\"Status\",\"Kind\","
                                                  " \"Remark\",\"Recipients\",\"FileUUID\","
                                                  " \"LocalPath\",\"DfsPath\",\"Hash\",\"FromSC\",\"DataUpdate\") "
                                         "values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','Y','A')",
                                           __db_table_deals_files,
                                               action->data, files_bc[i].version,    files_bc[i].status,     files_bc[i].kind,
                                                             files_bc[i].remark,     files_bc[i].recipients, files_bc[i].file_uuid,
                                                             files_bc[i].local_path, files_bc[i].dfs_path,   files_bc[i].hash) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Insert/Update file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
      
          if(strstr(files_bc[i].kind, "Sign")!=NULL)  oper="GetFile" ;
          else                                        oper="GetEncryptedFile" ;

                      sprintf(text, "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\",\"MasterId\") "
                                         "values('%s','%s','%s','%s','NEW','Deal:%s')",
                                           __db_table_files_actions,
                                               oper, action->data, files_bc[i].local_path, files_bc[i].dfs_path, action->id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Insert/Update file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }

                             }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - -  Получение истории из БД */
                      sprintf(text, "Select \"Version\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s",
                                     __db_table_deals_history, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                  EMIR_log(text) ;
                  EMIR_log(db->error_text) ;
                   sprintf(error, "Get history 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_HISTORY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                  EMIR_log(text) ;
                  EMIR_log(db->error_text) ;
                   sprintf(error, "Get history 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(history[i].version, (char *)Cursor->columns[0].value, sizeof(history[i].version)-1) ;
                                   }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_HISTORY_MAX) {                                           /* Если выборка слишком большая... */
                          sprintf(error, "Too many history steps") ;
                            status=-1 ;
                              break ;
                        }

         history_cnt=i ;
/*- - - - - - - - - - - - - - - - -  Определение новых шагов истории */
      for(i=0 ; i<history_bc_cnt ; i++)  history_bc[i].flag=1 ;

      for(i=0 ; i<history_bc_cnt ; i++)
      for(j=0 ; j<history_cnt    ; j++)
        if(!stricmp(history_bc[i].version, history[j].version)) {
                history_bc[i].flag=0 ;   break ;
                                                                }
/*- - - - - - - - - - - - - - - - - -  Обработка новых шагов истории */
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
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Insert/Update history record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - -  Получение атрибутов из БД */
                      sprintf(text, "Select \"Id\", \"Key\", \"Value\" "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s",
                                     __db_table_deals_attributes, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                  EMIR_log(text) ;
                  EMIR_log(db->error_text) ;
                   sprintf(error, "Get attributes 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_ATTR_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                  EMIR_log(text) ;
                  EMIR_log(db->error_text) ;
                   sprintf(error, "Get attributes 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(attr[i].id,    (char *)Cursor->columns[0].value, sizeof(attr[i].id   )-1) ;
           strncpy(attr[i].key,   (char *)Cursor->columns[1].value, sizeof(attr[i].key  )-1) ;
           strncpy(attr[i].value, (char *)Cursor->columns[2].value, sizeof(attr[i].value)-1) ;
                                }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_ATTR_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many attributes") ;
                            status=-1 ;
                              break ;
                     }  

         attr_cnt=i ;
/*- - - - - - - - - - - - - Определение новых и изменённых атрибутов */
      for(i=0 ; i<attr_bc_cnt ; i++)  attr_bc[i].flag=1 ;

      for(i=0 ; i<attr_bc_cnt ; i++)
      for(j=0 ; j<attr_cnt    ; j++)
        if(!stricmp(attr_bc[i].key, attr[j].key)) {

              strcpy(attr_bc[i].id, attr[i].id) ;

         if(!stricmp(attr_bc[i].value, attr[j].value))  attr_bc[i].flag=0 ;
         else                                           attr_bc[i].flag=2 ;
                                     break ;
                                                  } 
/*- - - - - - - - - - - - - - Обработка новых и изменённых атрибутов */
      for(i=0 ; i<attr_bc_cnt ; i++)
        if(attr_bc[i].flag==1) {

                      sprintf(text, "insert into %s(\"DealId\",\"Key\",\"Value\", \"DataUpdate\") "
                                         "values('%s','%s','%s','A')",
                                           __db_table_deals_attributes,
                                               action->data, attr_bc[i].key, attr_bc[i].value) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Insert attribute record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
                               }
        else
        if(attr_bc[i].flag==2) {

                      sprintf(text, "update %s set \"Value\"='%s', \"DataUpdate\"='U' "
                                    "where  \"Id\"=%s",
                                           __db_table_deals_attributes,
                                               attr_bc[i].value, attr_bc[i].id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                       EMIR_log(db->error_text) ;
                        sprintf(error, "Update attribute record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                                       break ;
                     }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Data\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->data, data_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                   EMIR_log(db->error_text) ;
                     strcpy(error, db->error_text) ;
                                   db->error_text[0]=0 ;
                                   db->UnlockCursor(Cursor) ;
                                 __db_errors_cnt++ ;
                                 __db_locked=1 ;
                               return(-1) ;
                }

                    db->Commit() ;
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "FILE" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - Ожидание завершения операций с файлами */
                      sprintf(text, "Select \"Id\", \"DfsPath\", \"Status\" "
                                    "From   %s  "
                                    "Where  \"MasterId\"='Deal:%s'",
                                      __db_table_files_actions, action->id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[2].value, sizeof(result)-1) ;

     if(!stricmp(result, "ERROR")) {                                /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation") ;
                                        status=-1 ;
                                           break ;
                                   }
     else
     if( stricmp(result, "DONE" )) {                                /* Если операция еще не выполнена... */
                                        done_flag=0 ;
                                   }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }
/*- - - - - - - - - - - - - - - - - Обработка файла заголовка Сделки */
     if(action->reply[0]!=0) {

             file=fopen(action->reply, "rb") ;
          if(file==NULL) {
                    sprintf(error, "Title file open error %d: %s", errno, action->reply) ;
                                       break ;
                         }

                       memset(data, 0, sizeof(data)) ;
                        fread(data, 1, sizeof(data)-1, file) ;
                       fclose(file) ;

                     snprintf(text, sizeof(text)-1, 
                                    "update %s set \"Data\"='%s' "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals, data, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Title data insert: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

                                 unlink(action->reply) ;
                             }
/*- - - - - - - - - - - - - - - - -  Запрос перечня связанных файлов */
                      sprintf(text, "Select \"Id\",\"LocalPath\",\"Hash\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s and \"DataUpdate\"='A' ",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].hash,       (char *)Cursor->columns[2].value, sizeof(files[i].hash      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - Проверка корректности хэш-сумм */
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
/*- - - - - - - - - - - - - - - - - Разблокировка записи Сделки в БД */
                      sprintf(text, "update %s set \"DataUpdate\"=substring(\"DataUpdate\",2,1) "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Unlock deal record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                           Операция GET_ARBITRATION                */

   int  EMIR_dl_action_GetArb(Dl_action *action, SQL_link *db, char *error)
{
      SQL_cursor *Cursor ;
             int  status ;
     Arbitration  arbitration ;
        DealAttr  attr[_ATTR_MAX] ;
             int  attr_cnt ;
        DealAttr  attr_bc[_ATTR_MAX] ;
             int  attr_bc_cnt ;
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
            char  text[8192] ;
             int  done_flag ;
             int  i ;
             int  j ;

/*---------------------------------------------------- Инициализация */

                           *error=0 ;

/*------------------------------------------ Получение данных Сделки */

   if(!stricmp(action->status, "NEW" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetArb") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - Запрос данных Сделки из БЧ */
                               memset(&arbitration, 0, sizeof(arbitration)) ;
              status=EMIRi_dl_GetArb (action->object, &arbitration,
                                      attr_bc,    _ATTR_MAX,  &attr_bc_cnt,
                                      party_bc,   _PARTY_MAX, &party_bc_cnt,
                                      NULL,             0,     NULL,
                                      files_bc,   _FILES_MAX, &files_bc_cnt,
                                      history_bc, _FILES_MAX, &history_bc_cnt, error) ;
           if(status<0)  break ;
/*- - - - - - - - - - - - - - - - -  Проверка наличия Арбитража в БД */
                      sprintf(text, "select count(*) from %s where  \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check arbitration existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                    sprintf(error, "Check arbitration existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[0].value, sizeof(result)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - -  Определение основной Сделки */
                         sprintf(text, "select \"Id\" from %s where  \"BlockChainId\"='%s'",
                                        __db_table_deals, arbitration.deal) ;
           status=db->SelectOpen(Cursor, text, NULL, 0) ;
        if(status) {
                     sprintf(error, "Check main deal: %s", db->error_text) ;
                                     db->error_text[0]=0 ;
                                   __db_errors_cnt++ ;
                                         break ;
                   }

           status=db->SelectFetch(Cursor) ;
        if(status==_SQL_NO_DATA) {
                                 }
        else
        if(status) {
                     sprintf(error, "Check main deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                   }
        else       {
                       memset(parent_id, 0, sizeof(result)) ;
                      strncpy(parent_id, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
                   }

                         db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - Создание/обновление записи Арбитража */
       if(result[0]=='0')  sprintf(text, "insert into %s(\"Kind\",\"Status\",\"Remark\","
                                                       " \"Version\",\"BlockChainId\",\"Parent\",\"ParentBlockChainId\","
                                                       "\"DataUpdate\") "
                                         "values('%s','%s','%s','%s','%s',%s,'%s','WA')",
                                           __db_table_deals,
                                               arbitration.kind,    arbitration.status,  arbitration.remark,
                                               arbitration.version, arbitration.address,   parent_id, arbitration.deal ) ;
       else                sprintf(text, "update %s set \"Status\"='%s',\"Remark\"='%s',\"Version\"='%s',\"DataUpdate\"='WU' "
                                         "where  \"BlockChainId\"='%s'",
                                           __db_table_deals, arbitration.status,  arbitration.remark,
                                                             arbitration.version, arbitration.address) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                    sprintf(error, "Insert/Update arbitration record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }
/*- - - - - - - - - - Создание/обновление записи состояния Арбитража */
       if(result[0]=='0')  sprintf(text, "insert into %s(\"DealId\",\"Status\",\"Remark\","
                                                       "\"DataUpdate\") "
                                         "values(%s,'%s','%s','A')",
                                           __db_table_deals_arbitration,
                                               parent_id, arbitration.deal_status, arbitration.deal_remark ) ;
       else                sprintf(text, "update %s set \"Status\"='%s',\"Remark\"='%s',\"DataUpdate\"='U' "
                                         "where  \"DealId\"=%s",
                                           __db_table_deals_arbitration, 
                                                arbitration.deal_status, arbitration.deal_remark, parent_id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                   EMIR_log(text) ;
                    sprintf(error, "Insert/Update arbitration state record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - Определение идентификатора Арбитража */
                      sprintf(text, "select \"Id\" from %s where \"BlockChainId\"='%s'",
                                     __db_table_deals, action->object) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Get arbitration Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {
                    sprintf(error, "Get arbitration Id: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

            memset(action->data, 0, sizeof(action->data)) ;
           strncpy(action->data, (char *)Cursor->columns[0].value, sizeof(action->data)-1) ;

                 db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - Запрос файла заголовка */
                            *data_path=0 ;

     if(arbitration.link[0]!=0) {

#ifdef  UNIX
                snprintf(data_path, sizeof(data_path)-1, "%s/%s", __file_storage, arbitration.link) ;
#else
                snprintf(data_path, sizeof(data_path)-1, "%s\\%s", __file_storage, arbitration.link) ;
#endif

                              oper="GetEncryptedFile" ;

                     snprintf(text, sizeof(text)-1,
                                     "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\",\"MasterId\") "
                                     "values('%s','%s','%s','%s','NEW','Deal:%s')",
                                           __db_table_files_actions,
                                               oper, action->data, data_path, arbitration.link, action->id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Insert/Update data file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                     }

                                }
/*- - - - - - - - - - - - - Получение списка участников Сделки из БД */
                      sprintf(text, "Select p.\"Id\", p.\"PartyId\", p.\"Role\" "
                                    "From   %s p "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_parties, action->data) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get party list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_PARTY_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get party list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(party[i].id,       (char *)Cursor->columns[0].value, sizeof(party[i].id      )-1) ;
           strncpy(party[i].party_id, (char *)Cursor->columns[1].value, sizeof(party[i].party_id)-1) ;
           strncpy(party[i].role,     (char *)Cursor->columns[2].value, sizeof(party[i].role    )-1) ;
                                 }

                 db->SelectClose (Cursor) ;

    if(status)  break ;        

         party_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - Определение новых участников */
      for(i=0 ; i<party_bc_cnt ; i++)  party_bc[i].flag=1 ;

      for(i=0 ; i<party_bc_cnt ; i++)
      for(j=0 ; j<party_cnt    ; j++)
        if(!stricmp(party_bc[i].party_id, party[j].party_id) &&
           !stricmp(party_bc[i].role,     party[j].role    )   ) {
                party_bc[i].flag=0 ;   break ;
                                                                 }
/*- - - - - - - - - - - - - - - - - - - - Обработка новых участников */
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
                                 __db_errors_cnt++ ;
                                       break ;
                     }
                             }

    if(status)  break ;
/*- - - - - - - - - - - - -  Получение списка файлов Арбитража из БД */
                      sprintf(text, "Select \"Id\", \"DfsPath\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s ",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,       (char *)Cursor->columns[0].value, sizeof(files[i].id      )-1) ;
           strncpy(files[i].dfs_path, (char *)Cursor->columns[1].value, sizeof(files[i].dfs_path)-1) ;
                                  }

                 db->SelectClose(Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - - - - Определение новых файлов */
      for(i=0 ; i<files_bc_cnt ; i++)  files_bc[i].flag=1 ;

      for(i=0 ; i<files_bc_cnt ; i++)
      for(j=0 ; j<files_cnt    ; j++)
        if(!stricmp(files_bc[i].dfs_path, files[j].dfs_path)) {
                files_bc[i].flag=0 ;   break ;
                                                              }
/*- - - - - - - - - - - - - - - - - - - - - - Обработка новых файлов */
      for(i=0 ; i<files_bc_cnt ; i++)
        if(files_bc[i].flag) {

#ifdef UNIX
                snprintf(files_bc[i].local_path, sizeof(files_bc[i].local_path)-1, "%s/%s.%s",
                          __file_storage, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#else
                snprintf(files_bc[i].local_path, sizeof(files_bc[i].local_path)-1, "%s/%s.%s",
                          __file_storage, files_bc[i].dfs_path, files_bc[i].file_ext) ;
#endif

                     snprintf(text, sizeof(text)-1,
                                    "insert into %s(\"DealId\",\"Version\",\"Status\",\"Kind\","
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
                                 __db_errors_cnt++ ;
                                       break ;
                     }

          if(strstr(files_bc[i].kind, "Sign")!=NULL)  oper="GetFile" ;
          else                                        oper="GetEncryptedFile" ;

                      sprintf(text, "insert into %s(\"Action\",\"Receivers\",\"LocalPath\",\"DfsPath\",\"Status\",\"MasterId\") "
                                         "values('%s','%s','%s','%s','NEW','Deal:%s')",
                                           __db_table_files_actions,
                                               oper, action->data, files_bc[i].local_path, files_bc[i].dfs_path, action->id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                        sprintf(error, "Insert/Update file record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                     }

                             }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - -  Получение истории из БД */
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
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get history 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(history[i].version, (char *)Cursor->columns[0].value, sizeof(history[i].version)-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many history steps") ;
                            status=-1 ;
                              break ;
                      }  

         history_cnt=i ;
/*- - - - - - - - - - - - - - - - -  Определение новых шагов истории */
      for(i=0 ; i<history_bc_cnt ; i++)  history_bc[i].flag=1 ;

      for(i=0 ; i<history_bc_cnt ; i++)
      for(j=0 ; j<history_cnt    ; j++)
        if(!stricmp(history_bc[i].version, history[j].version)) {
                history_bc[i].flag=0 ;   break ;
                                                                }
/*- - - - - - - - - - - - - - - - - -  Обработка новых шагов истории */
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
                                 __db_errors_cnt++ ;
                                       break ;
                     }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - -  Получение атрибутов из БД */
                      sprintf(text, "Select \"Id\", \"Key\", \"Value\" "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s",
                                     __db_table_deals_attributes, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get attributes 1 : %s", db->error_text) ;
                  EMIR_log(text) ;
                  EMIR_log(error) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_ATTR_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get attributes 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(attr[i].id,    (char *)Cursor->columns[0].value, sizeof(attr[i].id   )-1) ;
           strncpy(attr[i].key,   (char *)Cursor->columns[1].value, sizeof(attr[i].key  )-1) ;
           strncpy(attr[i].value, (char *)Cursor->columns[2].value, sizeof(attr[i].value)-1) ;
                                }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_ATTR_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many attributes") ;
                            status=-1 ;
                              break ;
                     }  

         attr_cnt=i ;
/*- - - - - - - - - - - - - Определение новых и изменённых атрибутов */
      for(i=0 ; i<attr_bc_cnt ; i++)  attr_bc[i].flag=1 ;

      for(i=0 ; i<attr_bc_cnt ; i++)
      for(j=0 ; j<attr_cnt    ; j++)
        if(!stricmp(attr_bc[i].key, attr[j].key)) {

              strcpy(attr_bc[i].id, attr[i].id) ;

         if(!stricmp(attr_bc[i].value, attr[j].value))  attr_bc[i].flag=0 ;
         else                                           attr_bc[i].flag=2 ;
                                     break ;
                                                  } 
/*- - - - - - - - - - - - - - Обработка новых и изменённых атрибутов */
      for(i=0 ; i<attr_bc_cnt ; i++)
        if(attr_bc[i].flag==1) {

                      sprintf(text, "insert into %s(\"DealId\",\"Key\",\"Value\", \"DataUpdate\") "
                                         "values('%s','%s','%s','A')",
                                           __db_table_deals_attributes,
                                               action->data, attr_bc[i].key, attr_bc[i].value) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                        sprintf(error, "Insert attribute record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                     }
                               }
        else
        if(attr_bc[i].flag==2) {

                      sprintf(text, "update %s set \"Value\"='%s', \"DataUpdate\"='U' "
                                    "where  \"Id\"=%s",
                                           __db_table_deals_attributes,
                                               attr_bc[i].value, attr_bc[i].id) ;
             status=db->SqlExecute(Cursor, text, NULL, 0) ;
          if(status) {
                       EMIR_log(text) ;
                        sprintf(error, "Update attribute record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                     }
                               }

    if(status)  break ;
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(status==0)  sprintf(text, "update %s "
                                  "set    \"Status\"='FILE', \"Data\"='%s', \"Reply\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->data, data_path, action->id) ;
     else           sprintf(text, "update %s "
                                  "set    \"Error\"='%s'"
                                  "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*------------------------------------- Обработка прилагаемых файлов */

   if(!stricmp(action->status, "FILE" ))  {

    do {

        Cursor=db->LockCursor("EMIR_dl_action_GetDeal") ;
     if(Cursor==NULL) {
                           strcpy(error, db->error_text) ;
                                         db->error_text[0]=0 ;
                                       __db_errors_cnt++ ;
                               return(-1) ;
                      }
/*- - - - - - - - - - - - - - Ожидание завершения операций с файлами */
                      sprintf(text, "Select \"Id\", \"DfsPath\", \"Status\" "
                                    "From   %s  "
                                    "Where  \"MasterId\"='Deal:%s' ",
                                      __db_table_files_actions, action->id) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                   EMIR_log(error) ;
                             db->error_text[0]=0 ;
                           __db_errors_cnt++ ;
                                   break ;
                }

                                          done_flag=1 ;

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                    sprintf(error, "Check DFS-transfer operations : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

            memset(result, 0, sizeof(result)) ;
           strncpy(result, (char *)Cursor->columns[2].value, sizeof(result)-1) ;

     if(!stricmp(result, "ERROR")) {                                /* Если ошибка... */
                    sprintf(error, "Error on DFS-transfer operation") ;
                                        status=-1 ;
                                           break ;
                                   }
     else
     if( stricmp(result, "DONE" )) {                                /* Если операция еще не выполнена... */
                                        done_flag=0 ;
                                   }
                                 }

                         db->SelectClose(Cursor) ;

     if(status)  break ;                                            /* Если ошибка... */

     if(done_flag==0) {                                             /* Если есть незавершенные операции */
                         db->UnlockCursor(Cursor) ;
                               return(0) ;
                      }
/*- - - - - - - - - - - - - - - - - - -  Определение основной Сделки */
                         sprintf(text, "select \"Parent\" from %s where  \"Id\"=%s",
                                        __db_table_deals, action->data) ;
           status=db->SelectOpen(Cursor, text, NULL, 0) ;
        if(status) {
                     sprintf(error, "Check main deal: %s", db->error_text) ;
                                     db->error_text[0]=0 ;
                                   __db_errors_cnt++ ;
                                         break ;
                   }

           status=db->SelectFetch(Cursor) ;
        if(status==_SQL_NO_DATA) {
                                 }
        else
        if(status) {
                     sprintf(error, "Check main deal existance: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                   }
        else       {
                       memset(parent_id, 0, sizeof(result)) ;
                      strncpy(parent_id, (char *)Cursor->columns[0].value, sizeof(result)-1) ;
                   }

                         db->SelectClose(Cursor) ;
/*- - - - - - - - - - - - - - - - - Обработка файла заголовка Сделки */
     if(action->reply[0]!=0) {

             file=fopen(action->reply, "rb") ;
          if(file==NULL) {
                    sprintf(error, "Title file open error %d: %s", errno, action->reply) ;
                                       break ;
                         }

                       memset(data, 0, sizeof(data)) ;
                        fread(data, 1, sizeof(data)-1, file) ;
                       fclose(file) ;

                     snprintf(text, sizeof(text)-1,
                                    "update %s set \"Data\"='%s' "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals_arbitration, data, parent_id) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Title data insert: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }

                                 unlink(action->reply) ;
                             }
/*- - - - - - - - - - - - - - - - -  Запрос перечня связанных файлов */
                      sprintf(text, "Select \"Id\",\"LocalPath\",\"Hash\"  "
                                    "From   %s  "
                                    "Where  \"DealId\"=%s and \"DataUpdate\"='A' ",
                                     __db_table_deals_files, action->data) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                   sprintf(error, "Get files list 1 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

   for(i=0 ; i<_FILES_MAX ; i++) {

        status=db->SelectFetch(Cursor) ;
     if(status==_SQL_NO_DATA) {  status=0 ;  break ;  }
     if(status) {
                   sprintf(error, "Get files list 2 : %s", db->error_text) ;
                        db->error_text[0]=0 ;
                      __db_errors_cnt++ ;
                              break ;
                }

           strncpy(files[i].id,         (char *)Cursor->columns[0].value, sizeof(files[i].id        )-1) ;
           strncpy(files[i].local_path, (char *)Cursor->columns[1].value, sizeof(files[i].local_path)-1) ;
           strncpy(files[i].hash,       (char *)Cursor->columns[2].value, sizeof(files[i].hash      )-1) ;
                                      }

                 db->SelectClose (Cursor) ;

    if(status)  break ;

    if(i>=_FILES_MAX) {                                             /* Если выборка слишком большая... */
                        sprintf(error, "Too many files") ;
                            status=-1 ;
                              break ;
                      }  

         files_cnt=i ;
/*- - - - - - - - - - - - - - - - - - Проверка корректности хэш-сумм */
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
/*- - - - - - - - - - - - - - - - - Разблокировка записи Сделки в БД */
                      sprintf(text, "update %s set \"DataUpdate\"=substring(\"DataUpdate\",2,1) "
                                    "where  \"Id\"='%s'",
                                     __db_table_deals, action->data) ;

        status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                    sprintf(error, "Unlock deal record: %s", db->error_text) ;
                                   db->error_text[0]=0 ;
                                 __db_errors_cnt++ ;
                                       break ;
                }
/*- - - - - - - - - - - - - - - - - - - - - -  Занесение ответа в БД */
       } while(0) ;

            db->SelectClose(Cursor) ;

   if(status==0 || strstr(error, "TRANSPORT")==NULL) {

            EMIR_text_subst(error, "'", "", 0) ;

     if(error[0]!=0)  sprintf(text, "update %s "
                                    "set    \"Error\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, error, action->id) ;
     else
     if(__purge_completed && action->master_id[0]==0)
                      sprintf(text, "delete from %s "
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, action->id) ;
     else             sprintf(text, "update %s "
                                    "set    \"Status\"='DONE', \"Reply\"='%s'"
                                    "where  \"Id\"=%s",
                                       __db_table_deals_actions, "", action->id) ;

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
                                                     }

                    db->UnlockCursor(Cursor) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                         }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос методов Get... смарт-контракта Deal...          */

   int  EMIRi_dl_GetDeal(       char *contract,
                                Deal *deal, 
                            DealAttr *attr,
                                 int  attr_max,
                                 int *attr_cnt,
                           DealParty *parties,
                                 int  parties_max,
                                 int *parties_cnt,
                             DealMap *map, 
                                 int  map_max,
                                 int *map_cnt,
                            DealFile *files, 
                                 int  files_max,
                                 int *files_cnt,
                         DealHistory *history, 
                                 int  history_max,
                                 int *history_cnt, char *error)

{
          int  status ;
         char *result ;
         char *deal_lnk ;
         char *deal_rem ;
          int  size ;
         char  version[64] ;
         char  text[2048] ;
         char  value[2048] ;
         char *doc_lnk ;
         char *doc_rem ;
         char *doc_rcp ;
         char *end ;
          int  n  ;
          int  i  ;

  static char *buff ;

#define  _BUFF_MAX  64000

 static char *request_version="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x0d55e9f5\"},\"latest\"],\"id\":1}" ;
 static char *request_status ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xabd95b95\"},\"latest\"],\"id\":1}" ;
 static char *request_arbitr ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xbf26e80b\"},\"latest\"],\"id\":1}" ;
 static char *request_attr   ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x108b58c9\"},\"latest\"],\"id\":1}" ;
 static char *request_parties="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x3a9e9723\"},\"latest\"],\"id\":1}" ;
 static char *request_map    ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x2d7b2b27\"},\"latest\"],\"id\":1}" ;
 static char *request_docs   ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x1efecdf6\"},\"latest\"],\"id\":1}" ;
 static char *request_doc    ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x44c89bb8%s\"},\"latest\"],\"id\":1}" ;
 static char *request_history="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x920cc179%064x\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*------------------------------------- Отправка запроса SD_Identify */

                          sprintf(buff, request_version, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(text, result+0*64, 64) ;
        EMIR_hex2txt(text, text) ;                                  /* Преобразование в текст */

#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - - - - - Извлечение  версии */
EMIR_log("Version:") ;

     end=strrchr(text, '.') ;
  if(end==NULL)         version[0]=0 ;
  else           strcpy(version, end+1) ;

EMIR_log(version) ;

/*--------------------------------------- Отправка запроса GetStatus */

                          sprintf(buff, request_status, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

    if(stricmp(version, "2021-05-01")>=0) {
EMIR_log("Get.2021-05-1") ;
              memcpy(deal->channel,        result+0*64, 64) ;
                                              n=1 ;
                                          }
    else                                  {
                                              n=0 ;
                                          }

              memcpy(deal->id,             result+(n+0)*64, 64) ;
              memcpy(deal->kind,           result+(n+1)*64, 64) ;
              memcpy(deal->parent,         result+(n+2)*64, 64) ;
              memcpy(deal->version,        result+(n+3)*64, 64) ;
              memcpy(deal->status,         result+(n+4)*64, 64) ;
              memcpy(deal->locked,         result+(n+5)*64, 64) ;

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строчных' атрибутов */
              memcpy(value, result+(n+6)*64, 64) ;
             deal_lnk=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+(n+7)*64, 64) ;
             deal_rem=result+2*strtoul(value, &end, 16) ;

              memset(text, 0, sizeof(text)) ;                       /* Извлечение Link(string) */
              memcpy(text, deal_lnk, 64) ;
        size=strtoul(text, &end, 16) ;
              memset(deal->link, 0, sizeof(deal->link)) ;
              memcpy(deal->link, deal_lnk+64, size*2) ;

              memset(text, 0, sizeof(text)) ;                       /* Извлечение Remark(string) */
              memcpy(text, deal_rem, 64) ;
        size=strtoul(text, &end, 16) ;
              memset(deal->remark, 0, sizeof(deal->remark)) ;
              memcpy(deal->remark, deal_rem+64, size*2) ;

        EMIR_hex2txt(deal->channel, deal->channel) ;                /* Преобразование в текст */
        EMIR_hex2txt(deal->id,      deal->id) ;
        EMIR_hex2txt(deal->kind,    deal->kind) ;
        EMIR_hex2txt(deal->status,  deal->status) ;
        EMIR_hex2txt(deal->link,    deal->link) ;
        EMIR_hex2txt(deal->remark,  deal->remark) ;

              strcpy(deal->parent,  deal->parent+24) ;

                     deal->locked[0]=deal->locked[63] ;
                     deal->locked[1]=  0 ;

              strcpy(deal->address, contract) ;

#undef   _RESULT_PREFIX

/*---------------------------------- Отправка запроса GetArbitration */

                          sprintf(buff, request_arbitr, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(deal->arbitration, result+0*64, 64) ;

              strcpy(deal->arbitration, deal->arbitration+24) ;

#undef   _RESULT_PREFIX

/*----------------------------------- Отправка запроса GetAttributes */

  if(attr!=NULL) {

                          sprintf(buff, request_attr, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

                       memset(text, 0, sizeof(text)) ;  
                       memcpy(text, result+1*64, 64) ;
         *attr_cnt=strtoul(text, &end, 16)/2 ;
      if(*attr_cnt>attr_max) {
                                 sprintf(error, "Too many attributes") ;
                                    return(-1) ;
                             }

                 result+=2*64 ;

      for(i=0 ; i<*attr_cnt ; i++) {

              memset(&attr[i], 0, sizeof(attr[i])) ;

              memcpy(attr[i].key,   result+0*64, 64) ;
              memcpy(attr[i].value, result+1*64, 64) ;

        EMIR_hex2txt(attr[i].key,   attr[i].key  ) ;                /* Преобразование в текст */
        EMIR_hex2txt(attr[i].value, attr[i].value) ;

                                         result+=2*64 ;
                                   }

#undef   _RESULT_PREFIX

                 }
/*-------------------------------------- Отправка запроса GetParties */

  if(parties!=NULL) {

                          sprintf(buff, request_parties, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

        EMIR_hex2txt(parties[i].party_id, parties[i].party_id) ;    /* Преобразование в текст */
        EMIR_hex2txt(parties[i].role,     parties[i].role    ) ;

              strcpy(parties[i].account,  parties[i].account+24) ;

                                         result+=3*64 ;
                                      }

#undef   _RESULT_PREFIX

                    }
/*------------------------------------ Отправка запроса GetStatusMap */

  if(map!=NULL) {

                          sprintf(buff, request_map, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

                       memset(text, 0, sizeof(text)) ;  
                       memcpy(text, result+1*64, 64) ;
         *map_cnt=strtoul(text, &end, 16)/3 ;
      if(*map_cnt>map_max) {
                              sprintf(error, "Too large status map") ;
                                    return(-1) ;
                           }

                 result+=2*64 ;

      for(i=0 ; i<*map_cnt ; i++) {

              memset(&map[i], 0, sizeof(map[i])) ;

              memcpy(map[i].status,      result+0*64, 64) ;
              memcpy(map[i].status_next, result+1*64, 64) ;
              memcpy(map[i].role,        result+2*64, 64) ;

        EMIR_hex2txt(map[i].status,      map[i].status     ) ;      /* Преобразование в текст */
        EMIR_hex2txt(map[i].status_next, map[i].status_next) ;
        EMIR_hex2txt(map[i].role,        map[i].role       ) ;

                                         result+=3*64 ;
                                  }

#undef   _RESULT_PREFIX

                }
/*------------------------------------ Отправка запроса GetDocuments */

  if(files!=NULL) {

                          sprintf(buff, request_docs, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

        EMIR_hex2txt(files[i].status,   files[i].status  ) ;        /* Преобразование в текст */

                                         result+=3*64 ;
                                    }

#undef   _RESULT_PREFIX

                  }
/*------------------------------------ Отправка запросов GetDocument */

  if(files!=NULL) {

    for(i=0 ; i<*files_cnt ; i++) {

                          sprintf(buff, request_doc, contract, files[i].file_uuid) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(files[i].kind,     result+0*64, 64) ;          /* Извлечение 'простых' атрибутов */
              memcpy(files[i].file_ext, result+1*64, 64) ;
              memcpy(files[i].hash,     result+2*64, 64) ;
//            memcpy(files[i].actor,    result+3*64, 64) ;          /* Пропускаем */

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строчных' атрибутов */
              memcpy(value, result+4*64, 64) ;
             doc_lnk=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+5*64, 64) ;
             doc_rem=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+6*64, 64) ;
             doc_rcp=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, doc_lnk, 64) ;                      /* Извлекаем поле "Link" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].dfs_path, doc_lnk+64, size*2) ;

                  memcpy(value, doc_rem, 64) ;                      /* Извлекаем поле "Remark" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rem+64, size*2) ;

                  memcpy(value, doc_rcp, 64) ;                      /* Извлекаем поле "Recepients" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rcp+64, size*2) ;

        EMIR_hex2txt(files[i].file_uuid,  files[i].file_uuid ) ;    /* Преобразование HEX->TEXT */
        EMIR_hex2txt(files[i].kind,       files[i].kind      ) ;
        EMIR_hex2txt(files[i].file_ext,   files[i].file_ext  ) ;
        EMIR_hex2txt(files[i].dfs_path,   files[i].dfs_path  ) ;
        EMIR_hex2txt(files[i].remark,     files[i].remark    ) ;
        EMIR_hex2txt(files[i].recipients, files[i].recipients) ;

             memmove(files[i].hash,  files[i].hash +24, strlen(files[i].hash +24)+1) ;
#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                  }
                  }
/*----------------------------- Получение истории изменения статусов */

  if(history!=NULL) {
/*- - - - - - - - - - - - - - - - - - -  Определение глубины истории */
         *history_cnt=strtoul(deal->version, &end, 16)+1 ;
      if(*history_cnt>history_max) {
                                sprintf(error, "Too many history steps") ;
                                    return(-1) ;
                                   }
/*- - - - - - - - - - - - - - - - - - - - Циклический запрос истории */
     for(i=0 ; i<*history_cnt ; i++) {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Запрос */
                          sprintf(buff, request_history, contract, i) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memset(&history[i], 0, sizeof(history[i])) ;

              memcpy(history[i].version, result+0*64, 64) ;
              memcpy(history[i].status,  result+1*64, 64) ;
//            memcpy(history[i].remark,  result+2*64, 64) ;         /* Пропускаем ссылку на строку */
              memcpy(history[i].actor,   result+3*64, 64) ;

        EMIR_hex2txt(history[i].status, history[i].status) ;
              strcpy(history[i].actor , history[i].actor+24) ;

              memcpy(text, result+4*64, 64) ;
        size=strtoul(text, &end, 16) ;
     if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
     if(size<=0)  continue ;

              memset(text, 0, sizeof(text)) ;
              memcpy(text, result+5*64, size*2) ;
        EMIR_hex2txt(text, text) ;                                  /* Преобразование в текст */
             strncpy(history[i].remark, text, sizeof(history[i].remark)-1) ;

#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - Циклический запрос истории */
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                    }
/*-------------------------------------------------------------------*/

   return(0) ;
}



/*********************************************************************/
/*								     */
/*         Запрос методов Get... смарт-контракта Arbitration...      */

   int  EMIRi_dl_GetArb(       char *contract,
                        Arbitration *arb, 
                           DealAttr *attr,
                                int  attr_max,
                                int *attr_cnt,
                          DealParty *parties,
                                int  parties_max,
                                int *parties_cnt,
                            DealMap *map, 
                                int  map_max,
                                int *map_cnt,
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
         char *doc_lnk ;
         char *doc_rem ;
         char *doc_rcp ;
         char *end ;
          int  i  ;

  static char *buff ;

#define  _BUFF_MAX  64000

 static char *request_status ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xabd95b95\"},\"latest\"],\"id\":1}" ;
 static char *request_deal   ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0xd02d313d\"},\"latest\"],\"id\":1}" ;
 static char *request_docs   ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x1efecdf6\"},\"latest\"],\"id\":1}" ;
 static char *request_doc    ="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x44c89bb8%s\"},\"latest\"],\"id\":1}" ;
 static char *request_history="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x920cc179%064x\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*--------------------------------------- Отправка запроса GetStatus */

                          sprintf(buff, request_status, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(arb->kind,    result+0*64, 64) ;
              memcpy(arb->deal,    result+1*64, 64) ;
              memcpy(arb->version, result+2*64, 64) ;
              memcpy(arb->status,  result+3*64, 64) ;

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строк' и массивов */
              memcpy(value, result+4*64, 64) ;
             doc_rem=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+5*64, 64) ;
             doc_rcp=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, doc_rem, 64) ;                      /* Извлекаем поле "Remark" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(arb->remark, doc_rem+64, size*2) ;

  if(parties!=NULL) {                                               /* Извлекаем список участников */
                       memset(text, 0, sizeof(text)) ;
                       memcpy(text, doc_rcp, 64) ;
         *parties_cnt=strtoul(text, &end, 16)/3 ;
      if(*parties_cnt>parties_max) {
                                      sprintf(error, "Too many parties") ;
                                         return(-1) ;
                                   }

                 result=doc_rcp+1*64 ;

      for(i=0 ; i<*parties_cnt ; i++) {

              memset(&parties[i], 0, sizeof(parties[i])) ;

              memcpy(parties[i].account,  result+0*64, 64) ;
              memcpy(parties[i].party_id, result+1*64, 64) ;
              memcpy(parties[i].role,     result+2*64, 64) ;

        EMIR_hex2txt(parties[i].party_id, parties[i].party_id) ;    /* Преобразование в текст */
        EMIR_hex2txt(parties[i].role,     parties[i].role    ) ;

              strcpy(parties[i].account,  parties[i].account+24) ;

                                         result+=3*64 ;
                                      }
                    }

        EMIR_hex2txt(arb->kind,   arb->kind) ;                     /* Преобразование в текст */
        EMIR_hex2txt(arb->status, arb->status) ;
        EMIR_hex2txt(arb->remark, arb->remark) ;

             memmove(arb->deal, arb->deal+24, strlen(arb->deal+24)+1) ;

              strcpy(arb->address, contract) ;

#undef   _RESULT_PREFIX

/*----------------------------------- Отправка запроса GetMainStatus */

                          sprintf(buff, request_deal, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(arb->deal_status, result+0*64, 64) ;

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строк' и массивов */
              memcpy(value, result+1*64, 64) ;
             doc_rem=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+2*64, 64) ;
             doc_lnk=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+3*64, 64) ;
             doc_rcp=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, doc_rem, 64) ;                      /* Извлекаем поле "Remark" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(arb->deal_remark, doc_rem+64, size*2) ;

                  memcpy(value, doc_lnk, 64) ;                      /* Извлекаем поле "Link" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(arb->link, doc_lnk+64, size*2) ;

  if(attr!=NULL) {
                       memset(text, 0, sizeof(text)) ;  
                       memcpy(text, doc_rcp, 64) ;
         *attr_cnt=strtoul(text, &end, 16)/2 ;
      if(*attr_cnt>attr_max) {
                                 sprintf(error, "Too many attributes") ;
                                    return(-1) ;
                             }

                 result=doc_rcp+1*64 ;

      for(i=0 ; i<*attr_cnt ; i++) {

              memset(&attr[i], 0, sizeof(attr[i])) ;

              memcpy(attr[i].key,   result+0*64, 64) ;
              memcpy(attr[i].value, result+1*64, 64) ;

        EMIR_hex2txt(attr[i].key,   attr[i].key  ) ;                /* Преобразование в текст */
        EMIR_hex2txt(attr[i].value, attr[i].value) ;

                                         result+=2*64 ;
                                   }
                 }


        EMIR_hex2txt(arb->deal_status, arb->deal_status) ;          /* Преобразование в текст */
        EMIR_hex2txt(arb->deal_remark, arb->deal_remark) ;
        EMIR_hex2txt(arb->link,        arb->link       ) ;

#undef   _RESULT_PREFIX

/*------------------------------------ Отправка запроса GetDocuments */

  if(files!=NULL) {

                          sprintf(buff, request_docs, contract) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

        EMIR_hex2txt(files[i].status,   files[i].status  ) ;        /* Преобразование в текст */

                                         result+=3*64 ;
                                    }

#undef   _RESULT_PREFIX

                  }
/*------------------------------------ Отправка запросов GetDocument */

  if(files!=NULL) {

    for(i=0 ; i<*files_cnt ; i++) {

                          sprintf(buff, request_doc, contract, files[i].file_uuid) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memcpy(files[i].kind,     result+0*64, 64) ;          /* Извлечение 'простых' атрибутов */
              memcpy(files[i].file_ext, result+1*64, 64) ;
              memcpy(files[i].hash,     result+2*64, 64) ;
//            memcpy(files[i].actor,    result+3*64, 64) ;          /* Пропускаем */

              memset(value, 0, sizeof(value)) ;                     /* Позиционирование 'строк' и массивов */
              memcpy(value, result+4*64, 64) ;
             doc_lnk=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+5*64, 64) ;
             doc_rem=result+2*strtoul(value, &end, 16) ;
              memcpy(value, result+6*64, 64) ;
             doc_rcp=result+2*strtoul(value, &end, 16) ;

                  memcpy(value, doc_lnk, 64) ;                      /* Извлекаем поле "Link" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].dfs_path, doc_lnk+64, size*2) ;

                  memcpy(value, doc_rem, 64) ;                      /* Извлекаем поле "Remark" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rem+64, size*2) ;

                  memcpy(value, doc_rcp, 64) ;                      /* Извлекаем поле "Recepients" */
            size=strtoul(value, &end, 16) ;
         if(size>0)  memcpy(files[i].remark, doc_rcp+64, size*2) ;

        EMIR_hex2txt(files[i].file_uuid,  files[i].file_uuid ) ;    /* Преобразование HEX->TEXT */
        EMIR_hex2txt(files[i].kind,       files[i].kind      ) ;
        EMIR_hex2txt(files[i].file_ext,   files[i].file_ext  ) ;
        EMIR_hex2txt(files[i].dfs_path,   files[i].dfs_path  ) ;
        EMIR_hex2txt(files[i].remark,     files[i].remark    ) ;
        EMIR_hex2txt(files[i].recipients, files[i].recipients) ;

             memmove(files[i].hash,  files[i].hash +24, strlen(files[i].hash +24)+1) ;
#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                  }
                  }
/*----------------------------- Получение истории изменения статусов */

  if(history!=NULL) {
/*- - - - - - - - - - - - - - - - - - -  Определение глубины истории */
         *history_cnt=strtoul(arb->version, &end, 16)+1 ;
      if(*history_cnt>files_max) {
                                sprintf(error, "Too many history steps") ;
                                    return(-1) ;
                                 }
/*- - - - - - - - - - - - - - - - - - - - Циклический запрос истории */
     for(i=0 ; i<*history_cnt ; i++) {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Запрос */
                          sprintf(buff, request_history, contract, i) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_MAX-1) ;
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

              memset(&history[i], 0, sizeof(history[i])) ;

              memcpy(history[i].version, result+0*64, 64) ;
              memcpy(history[i].status,  result+1*64, 64) ;
//            memcpy(history[i].remark,  result+2*64, 64) ;         /* Пропускаем ссылку на строку */
              memcpy(history[i].actor,   result+3*64, 64) ;

        EMIR_hex2txt(history[i].status, history[i].status) ;
              strcpy(history[i].actor , history[i].actor+24) ;

              memcpy(text, result+4*64, 64) ;
        size=strtoul(text, &end, 16) ;
     if(size>=sizeof(text)/2)  size=sizeof(text)/2-1 ;
     if(size<=0)  continue ;

              memset(text, 0, sizeof(text)) ;
              memcpy(text, result+5*64, size*2) ;
        EMIR_hex2txt(text, text) ;                                  /* Преобразование в текст */
             strncpy(history[i].remark, text, sizeof(history[i].remark)-1) ;

#undef   _RESULT_PREFIX
/*- - - - - - - - - - - - - - - - - - - - Циклический запрос истории */
                                     }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                    }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*            Запрос методов CheckContract смарт-контракта Box       */

   int  EMIRi_dl_CheckBox(char *box, char *contract, char *error)

{
   int  status ;
  char *result ;
  char  buff[4096] ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x34d01d61000000000000000000000000%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*--------------------------------------- Отправка запроса GetStatus */

                          sprintf(buff, request, box, contract) ;
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
            EMIR_hex2txt(result, result) ;                          /* Преобразование в текст */

      if(result[0]=='Y')  return(1) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           Запрос методa GetContracts смарт-контракта Box          */

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

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*------------------------------------ Отправка запроса GetContracts */

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

/*------------------------------------------- Поджатие пустых слотов */

      for(i=0, j=0 ; i<list_cnt ; i++) {

        if(j!=i)  strcpy((*list)[j].address, (*list)[i].address) ;

        if(stricmp((*list)[j].address, _NULL_ADDR))  j++ ;

                                       }

                  list_cnt=j ;

/*-------------------------------------------------------------------*/

   return(list_cnt) ;
}


/*********************************************************************/
/*								     */
/*              Извлечение номера Сделки из операции                 */

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
/*            Запрос методов CheckBank смарт-контракта Box           */

   int  EMIRi_dl_AccessBox(char *box, char *contract, char *error)

{
   int  status ;
  char *result ;
  char  buff[4096] ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x376e6cc2000000000000000000000000%s\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*--------------------------------------- Отправка запроса CheckBank */

                          sprintf(buff, request, box, contract) ;
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
            EMIR_hex2txt(result, result) ;                          /* Преобразование в текст */

      if(result[0]=='Y')  return(1) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


