/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*********************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/ioctl.h>

#include "sql_postgre.h"

#define  __MAIN__

#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

/*--------------------------------------- Работа с командной строкой */

#define IS_KEY(text, key)        ( (text[0]=='-' || text[0]=='/') && \
                                        !stricmp(text+1, key)       )

#define IS_KEY_(text, key, len)  ( (text[0]=='-' || text[0]=='/') && \
                                        !memicmp(text+1, key, len)  )

/*------------------------------------ Процедуры обработки сообщений */

                int  EMIR_ExitSignal      (char *action) ;                /* Работа с сигналом остановки */
      BOOL CALLBACK  EMIR_EnumCallBack    (HWND hWnd, LPARAM lParam) ;

/*********************************************************************/
/*                                                                   */
/*	                      MAIN                     	             */
/*                                                                   */
/*   -mirror:... - задает иднтификатор экземпляра                    */
/*   -cfg:...    - путь к основному конфигурационному файлу          */
/*   -time:NNNN  - задаёт время работы в секундах                    */
/*   -version    - выдает версию сборки                              */

int main(int argc, char *argv[])

{
                int  help_flag ;
               char  mirror[128] ;
   SQL_Postgre_link  DB ;
             time_t  time_abs ;
             time_t  time_0 ;
          struct tm *hhmmss ;
                int  status ;
               char  prefix[128] ;
               char  text[2048] ;
               char *end ;
                int  i ;

/*---------------------------------------------------- Инициализация */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

                 getcwd(__cwd, sizeof(__cwd)-1) ;

/*------------------------------------------ Анализ командной строки */

                               help_flag=2 ;

                 memset(mirror,   0, sizeof(mirror)) ;

                      __exit_time=0 ;

   for(i=1 ; i<argc ; i++) {
/*- - - - - - - - - - - - - - - - - - - -  Отображение версии сборки */
#define     KEY   "version"

     if(IS_KEY_(argv[i], KEY, strlen(KEY))) {

          printf("ethereum_mirror %s\n", _VERSION) ;
                           exit(0) ;

                                            }
#undef      KEY 
/*- - - - - - - - - - - - - - - -  Задание идентификатора экземпляра */
#define     KEY   "mirror:"

     if(IS_KEY_(argv[i], KEY, strlen(KEY))) {

        strncpy(mirror, argv[i]+strlen(KEY)+1, sizeof(mirror)-1) ;

                          help_flag-- ;

                                            }
#undef      KEY 
/*- - - - - - - - - - - - - - - - - - - - - - - -  Файл конфигурации */
#define     KEY   "cfg:"
     else
     if(IS_KEY_(argv[i], KEY, strlen(KEY))) {

                                    end=strchr(argv[i], ':') ;
            strncpy(__context_path, end+1, sizeof(__context_path)-1) ;

                          help_flag-- ;
                                            }
#undef      KEY 
/*- - - - - - - - - - - - - - - - - - - - - - - -  Время перезапуска */
#define     KEY  "time:"
     else
     if(IS_KEY_(argv[i], KEY, strlen(KEY))) {

                                end=strchr(argv[i], ':') ;
                       __exit_time=strtoul(end+1, &end, 10) ;

                                            }
#undef   _KEY
/*- - - - - - - - - - - - - - - - - - - Обработка неизвестного ключа */
     else                                    {

                       printf("Unknown key: %s\n\n", argv[i]) ;

                                                 help_flag=1 ;
                                                   break ;
                                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                           }
/*------------------------------------------------- Выдача подсказки */
    
    if(help_flag) {

          printf("ethereum_mirror -mirror:<Id> -cfg:<config_path>\n"
                 "ethereum_mirror -version\n\n") ;
                           exit(1) ;
                  }
/*------------------------------- Считывание конфигурационных файлов */

         status=EMIR_save_context(__context_path, "READ") ;
      if(status)  exit(1) ;

         status=EMIR_db_context(__db_context_path) ;
      if(status)  exit(1) ;

         status=EMIR_mon_context(__mon_context_path) ;
      if(status)  exit(1) ;

/*------------------------------------------ Запуск в фоновом режиме */

//#define   _BACKGROUND

#ifdef _BACKGROUND

 do {
                  int  fd ;
                pid_t  pid ;
        struct rlimit  rl ;
/*- - - - - - - - - - - - - - - - -  Удаление управляющего терминала */
            fd=open("/dev/tty", 0, O_RDWR) ;
         if(fd>0) {
                      ioctl(fd, TIOCNOTTY, 0) ;
                      close(fd) ;
                  }
/*
         else     {
                     printf("\nTTY open error\n") ;
                          exit(3) ;
                  }
*/
/*- - - - - - - - - - - - - - - - - - - Порождение фонового процесса */
            pid=fork() ;
         if(pid<0) {
                     printf("\nDaemon process spawn fail\n") ;
                          exit(4) ;
                   }
         if(pid>0) {
                          exit(0) ;
                   }
/*- - - - - - - - - - - - - - - - - - - Закрытие стандартных потоков */
#define        _START_FD  0

                      getrlimit(RLIMIT_NOFILE, &rl) ;

            for(fd=_START_FD ; fd<rl.rlim_cur ; fd++)  close(fd) ;

#undef         _START_FD

    } while(0) ;

#endif

/*--------------------------------------------- Работа в ROOT-режиме */

/*----------------------------------------------- Выделение ресурсов */

/*------------------------------- Запуск отдельных потоков обработки */

/*------------------------------------------- Главный цикл обработки */

               sprintf(text, "--- STARTED Version %s", _VERSION) ;
              EMIR_log(text) ;

          time_0=time(NULL) ;

   do {

          if(__exit_flag)  break ;                                  /* Если общий стоп */ 

         if(__scan_rush==0)
          if(EMIR_active_section("SYSTEM"  ) ||
             EMIR_active_section("MEMBERS" ) ||
             EMIR_active_section("REESTERS") ||
             EMIR_active_section("DEALS"   ) ||
             EMIR_active_section("FILES"   )   )  Sleep(1000) ;

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

          if(__exit_time>0) 
           if(time_abs-time_0>__exit_time) {  __exit_flag=2 ;
                                                     break ;  }
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Соединение с БД */
     if(__db_name[0]!=0 && !DB.connected) {

         status=DB.Connect(__db_user, __db_password,
                           __db_name, "PostgreSQL", "WIN1251") ;
      if(status) {
                      sprintf(text, "%s - ERROR - DB connection: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }

         status=DB.SetAutoCommit(0) ;
      if(status) {
                      sprintf(text, "%s - ERROR - DB set autocommit: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
//                  DB.Disconnect() ;
                        break ;
                 }

         status=DB.AllocCursors(10) ;
      if(status) {
                      sprintf(text, "%s - ERROR - DB cursors allocation: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
//                  DB.Disconnect() ;
                        break ;
                 }

                    EMIR_db_config(&DB) ;                           /* Считывание настроек из БД */
                                          }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Процедуры модулей */
     if(__scan_rush==0) {
      if(EMIR_active_section("SYSTEM"  ))  SysConfig_Process(&DB) ;
                        }
     
      if(EMIR_active_section("SCAN"    ))       Scan_Process(&DB, "SCAN") ;
      if(EMIR_active_section("EVENTS"  ))       Scan_Process(&DB, "EVENTS") ;
      if(EMIR_active_section("SENTRY"  ))     Sentry_Process(&DB) ;

     if(__scan_rush==0) {
      if(EMIR_active_section("MEMBERS" ))    Members_Process(&DB) ;
      if(EMIR_active_section("DEALS"   ))      Deals_Process(&DB) ;
//    if(EMIR_active_section("REESTERS"))   Reesters_Process(&DB) ;
      if(EMIR_active_section("FILES"   ))      Files_Process(&DB) ;
//    if(EMIR_active_section("ORACLE"  ))     Oracle_Process(&DB) ;
                        }
/*- - - - - - - - - - - - - - - -  Проверка неосвобожденных курсоров */
    for(i=0 ; i<DB.cursors_cnt ; i++)
      if(DB.cursors[i]->user_label[0])  {
           sprintf(text, "Locked cursor %s", DB.cursors[i]->user_label) ;
          EMIR_log(text) ;
                 __exit_flag=1 ;
                                        }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

      } while(1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -Отсоединение с БД */
                DB.Disconnect() ;                                   /* Отсоединение с БД */

/*------------------------------------------------ Завершение работы */

/*-------------------------------------------- Освобождение ресурсов */

/*-------------------------------------------------------------------*/

//              EMIR_save_context(__context_path, "WRITE") ;

               sprintf(text, "--- STOPPED") ;
              EMIR_log(text) ;

  return(0) ;
}


