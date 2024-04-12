
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
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
#include <shlobj.h>
#include <shlwapi.h>
#include <tlhelp32.h>
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

#if defined(UNIX) || defined(_CONSOLE)
#else
#define  __MAIN__
#endif

#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

#pragma warning(disable : 4267)
#pragma warning(disable : 4996)
#pragma warning(disable : 6053)
#pragma warning(disable : 6054)
#pragma warning(disable : 6387)

/*----------------------------------------- Общесистемные переменные */

  typedef struct {
                   int  elem ;
                   int  x ;
                   int  y ;
                   int  xs ;
                   int  ys ;
                 } Elem_pos_list ;

/*------------------------------------ Обработчики элементов диалога */

#ifndef UNIX

  union WndProc_par {
                        long            par ;
                     LRESULT (CALLBACK *call)(HWND, UINT, WPARAM, LPARAM) ; 
                    } ;

  static union WndProc_par  Tmp_WndProc ;

#endif

/*------------------------------------ Процедуры обработки сообщений */

#ifndef UNIX
                int  EMIR_ExitSignal      (char *action) ;                /* Работа с сигналом остановки */
      BOOL CALLBACK  EMIR_EnumCallBack    (HWND hWnd, LPARAM lParam) ;
   LRESULT CALLBACK  EMIR_window_processor(HWND, UINT, WPARAM, LPARAM) ;
#endif

               void  Service_Log          (char *, char *) ;              /* Функция логирования */

/*********************************************************************/
/*                                                                   */
/*	                      MAIN                     	             */	

#if defined(UNIX) || defined(_CONSOLE)
#else

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
            int  root_flag ;
           WORD  version ;
        WSADATA  winsock_data ;        /* Данные системы WINSOCK */
          HICON  hIcon ;
            MSG  SysMessage ;
         time_t  time_abs ;
      struct tm *hhmmss ;
            int  status ;
           char *key ;
           char *end ;
           char  id[128] ;
           char  text[2048] ;
            int  i ;

/*---------------------------------------------------- Инициализация */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

                 getcwd(__cwd, sizeof(__cwd)-1) ;

/*------------------------------------------ Разбор командной строки */

#define  _KEY  "-time:"

       key=strstr(lpCmdLine, _KEY) ;
    if(key!=NULL) {
                       __exit_time=strtoul(key+strlen(_KEY), &end, 10) ;
                  }
#undef   _KEY

#define  _KEY  "-id:"

                      strcpy(id, "Ethereum-Mirror") ;

       key=strstr(lpCmdLine, _KEY) ;
    if(key!=NULL) {
                              memset(id, 0, sizeof(id)) ;
                            strncpy(id, key+strlen(_KEY), sizeof(id)) ;
                        end= strchr(id, ' ') ;
                     if(end!=NULL)  *end=0 ;
                  }
#undef   _KEY

#define  _KEY   "-cfg:"

       key=strstr(lpCmdLine, _KEY) ;
    if(key!=NULL) {
                            strncpy(__context_path, key+strlen(_KEY), sizeof(__context_path)) ;
                        end= strchr(__context_path, ' ') ;
                     if(end!=NULL)  *end=0 ;
                  }
#undef   _KEY 

#define  _KEY  "-spawned"

       key=strstr(lpCmdLine, _KEY) ;
    if(key!=NULL)  root_flag=0 ;
    else           root_flag=1 ;

#undef   _KEY

/*------------------------------- Считывание конфигурационных файлов */

         status=EMIR_save_context(__context_path, "READ") ;
      if(status)  exit(-1) ;

         status=EMIR_db_context(__db_context_path) ;
      if(status)  exit(-1) ;

/*--------------------------------------------- Работа в ROOT-режиме */

   if(__exit_time && root_flag) {

                    char  command[1024] ;
             STARTUPINFO  StartupInfo ;
     PROCESS_INFORMATION  ProcessInformation ;

                     memset(command, 0, sizeof(command)) ;
                    strncpy(command, GetCommandLine(), sizeof(command)-20) ;

                    sprintf(text,    " -spawned") ;
                     strcat(command, text) ;

           do {
                 if(EMIR_ExitSignal("CHECK"))  break ;              /* При обнаружении сигнального файла - завершаем работу */

                    status=EnumWindows(EMIR_EnumCallBack, (LPARAM)id) ;
                 if(status==TRUE) {
                               memset(&StartupInfo, 0, sizeof(StartupInfo)) ;
                                       StartupInfo.cb=sizeof(StartupInfo) ;
                        CreateProcess(NULL, command,
                                      NULL, NULL, false, 0, NULL, NULL, 
                                        &StartupInfo, &ProcessInformation) ;     
                                  }

                    Sleep(5000) ;

              } while(1) ;

                                      exit(0) ;
                               }
/*----------------------------------------------- Выделение ресурсов */

                        version=MAKEWORD(1, 1) ;
      status=WSAStartup(version, &winsock_data) ;                   /* Иниц. Win-Sockets */
   if(status) {
                EMIR_message("Win-socket DLL loading error: %d\n", WSAGetLastError()) ;
                    exit(-1) ;
              }
/*------------------------------- Регистрация класса первичного окна */

                            hInst=hInstance ;

	FrameWindow.lpszClassName="EMIR_Frame" ;
	FrameWindow.hInstance    = hInstance ;
	FrameWindow.lpfnWndProc  = EMIR_window_processor ;
	FrameWindow.hCursor      = LoadCursor(NULL, IDC_ARROW) ;
	FrameWindow.hIcon        =  NULL ;
	FrameWindow.lpszMenuName =  NULL ;
	FrameWindow.hbrBackground=(HBRUSH__ *)GetStockObject(LTGRAY_BRUSH) ;
	FrameWindow.style        =    0 ;
	FrameWindow.hIcon        =  NULL ;

    if(!RegisterClass(&FrameWindow)) {
              sprintf(text, "EMIR_Frame register error %d", GetLastError()) ;
         EMIR_message(text) ;
	                                return(-1) ;
				    }
/*----------------------------------------- Создание первичного окна */

                      sprintf(text, "%s: Царице Ethereum и всея блокчейна Марине I посвящается", id) ;

    hFrameWindow=CreateWindow("EMIR_Frame", 
                               text, 
//                             WS_OVERLAPPEDWINDOW,
                               WS_OVERLAPPED  |
                               WS_CAPTION     |
                               WS_THICKFRAME  |
                               WS_SYSMENU     |
                               WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX   ,
			       CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT,
				        NULL, NULL, 
				   hInstance, NULL ) ;

/*---------------------------------- Создание диалоговых окон секций */

   do {
/*- - - - - - - - - - - - - - - - - - - - Формирование списка секций */
                                                             i=0 ;      
    if(EMIR_active_section("DEALS"   )) {  strcpy(__sections[i].title, "Сделки"    ) ;
                                                             i++ ;                     }
    if(EMIR_active_section("SYSTEM"  )) {  strcpy(__sections[i].title, "Система"   ) ;
                                                             i++ ;                     }
    if(EMIR_active_section("MEMBERS" )) {  strcpy(__sections[i].title, "Участники" ) ;
                                                             i++ ;                     }
    if(EMIR_active_section("FILES"   )) {  strcpy(__sections[i].title, "Файлы"     ) ;
                                                             i++ ;                     }
    if(EMIR_active_section("SCAN"    )) {  strcpy(__sections[i].title, "Блокчейн"  ) ;
                                                             i++ ;                     }
    if(EMIR_active_section("SENTRY"  )) {  strcpy(__sections[i].title, "Мониторинг") ;
                                                             i++ ;                     }
    if(EMIR_active_section("ORACLE"  )) {  strcpy(__sections[i].title, "Оракул"    ) ;
                                                             i++ ;                     }


                                              __sections_cnt=i ;
                                                             i=0 ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Модуль "Сделки" */
   if(EMIR_active_section("DEALS"   )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_DEALS",
	                                    hFrameWindow, EMIR_deals_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "DEALS dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                     hDeals_Dialog=__sections[i].hWnd ;
                                              i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - -  Модуль "Системная конфигурация" */
   if(EMIR_active_section("SYSTEM"  )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_SYSCONFIG",
	                                    hFrameWindow, EMIR_sysconfig_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "SYSCONFIG dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                hSysConfig_Dialog=__sections[i].hWnd ;
                                             i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - - - - Модуль "Участники" */
   if(EMIR_active_section("MEMBERS" )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_MEMBERS",
	                                    hFrameWindow, EMIR_members_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "MEMBERS dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                  hMembers_Dialog=__sections[i].hWnd ;
                                             i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - Модуль "Файлы" */
   if(EMIR_active_section("FILES"   )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_FILES",
	                                    hFrameWindow, EMIR_files_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "FILES dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                  hFiles_Dialog=__sections[i].hWnd ;
                                           i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - Модуль "Сканер блокчейн" */
   if(EMIR_active_section("SCAN"    )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_SCAN",
	                                    hFrameWindow, EMIR_scan_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "SCAN dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                   hScan_Dialog=__sections[i].hWnd ;
                                           i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - - -  Модуль "Мониторинг" */
   if(EMIR_active_section("SENTRY"  )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_SENTRY",
	                                    hFrameWindow, EMIR_sentry_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "SENTRY dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                 hSentry_Dialog=__sections[i].hWnd ;
                                           i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Модуль "Оракул" */
   if(EMIR_active_section("ORACLE"  )) {

           __sections[i].hWnd=CreateDialog(hInst, "IDD_ORACLE",
	                                    hFrameWindow, EMIR_oracle_dialog) ;
        if(__sections[i].hWnd==NULL) {
                   sprintf(text, "ORACLE dialog load error %d", GetLastError()) ;
              EMIR_message(text) ;
	                               return(-1) ;
                                     }

                 hOracle_Dialog=__sections[i].hWnd ;
                                           i++ ;    
                                       }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
      } while(0) ;

/*------------------------------------------ Пропись иконы в TaskBar */

       TaskBar_Msg=RegisterWindowMessage("ERPC_Activate") ;         /* Регистр.сообщение активизации */

//       hIcon=LoadIcon(hInstance, "IDI_TASKBAR_ICON") ;            /* Грузим иконку */
         hIcon=LoadIcon(hInstance, "IDI_FACTORIN") ;

                     TbIcon.cbSize          = sizeof(TbIcon) ;
                     TbIcon.hWnd            = hFrameWindow ;
                     TbIcon.uID             =    1 ;
                     TbIcon.uFlags          = NIF_TIP    |
                                              NIF_ICON   |
                                              NIF_MESSAGE ;
                     TbIcon.uCallbackMessage= TaskBar_Msg ;
                     TbIcon.hIcon           = hIcon ;
              strcpy(TbIcon.szTip, _PROGRAM_TITLE)  ;

    Shell_NotifyIcon(NIM_ADD, &TbIcon) ;

         SendMessage(hFrameWindow, WM_SETICON, ICON_BIG, (LPARAM) hIcon) ;

/*-------------------------------------------------- Активируем окна */

                  ShowWindow(hFrameWindow, SW_SHOW) ;
		UpdateWindow(hFrameWindow) ;

             for(i=0 ; i<__sections_cnt ; i++)
  		  ShowWindow(__sections[i].hWnd, SW_HIDE) ;

  		  ShowWindow(__sections[0].hWnd, SW_SHOW) ;

                  SendMessage(__sections[0].hWnd, WM_USER,
                                (WPARAM)_USER_SECTION_ENABLE, NULL) ;

/*------------------------------- Запуск отдельных потоков обработки */
 
             hMainQueue_PID=GetCurrentThreadId() ;

               hMain_Thread=CreateThread(NULL, 0, Main_Thread,     
                                         NULL, 0, &hMain_PID) ;

/*
          hSysConfig_Thread=CreateThread(NULL, 0, SysConfig_Thread,     
                                         NULL, 0, &hSysConfig_PID) ;

            hMembers_Thread=CreateThread(NULL, 0, Members_Thread,     
                                         NULL, 0, &hMembers_PID) ;

              hFiles_Thread=CreateThread(NULL, 0, Files_Thread,     
                                         NULL, 0, &hFiles_PID) ;

               hScan_Thread=CreateThread(NULL, 0, Scan_Thread,     
                                         NULL, 0, &hScan_PID) ;

             hSentry_Thread=CreateThread(NULL, 0, Sentry_Thread,
                                         NULL, 0, &hSentry_PID) ;

             hOracle_Thread=CreateThread(NULL, 0, Oracle_Thread,
                                         NULL, 0, &hOracle_PID) ;
*/
/*------------------------------------------ Главный диалоговый цикл */

   while(1) {

        if(GetMessage(&SysMessage, NULL, 0, 0)==0) {

            if(__exit_flag!=2)  EMIR_ExitSignal("SEND") ;

                                     break ;
                                                   }

                TranslateMessage(&SysMessage) ;
                 DispatchMessage(&SysMessage) ;
            } 

/*------------------------------------------------ Завершение работы */

//        DestroyWindow(FrameWindow_h) ;
        UnregisterClass("ERPC_Frame", GetModuleHandle(NULL)) ;

       Shell_NotifyIcon(NIM_DELETE, &TbIcon) ;                      /* Удаление TaskBar-иконки */

/*-------------------------------------------- Освобождение ресурсов */

                        WSACleanup() ;                              /* Освобождение Win-Sockets */

/*-------------------------------------------------------------------*/

                EMIR_save_context(__context_path, "WRITE") ;

  return(0) ;
}


/*********************************************************************/
/*								     */
/*              Функция обратной связи перебора окон		     */

 BOOL CALLBACK  EMIR_EnumCallBack(HWND hWnd, LPARAM lParam)

{
  char title[256] ;


                memset(title, 0, sizeof(title)) ;
         GetWindowText(hWnd, title, sizeof(title)-1) ;

     if(!memcmp(title, (char *)lParam, strlen((char *)lParam)))  return(FALSE) ;
   
   return(TRUE) ;
}


/*********************************************************************/
/*								     */
/*              Работа с сигналом остановки       		     */

  int  EMIR_ExitSignal(char *action)

{
   int  status ;
  FILE *file  ;
  char  text[1024] ;

/*--------------------------------------- Проверка сигнала остановки */

    if(!stricmp(action, "CHECK")) {

         if(access(__signal_path, 0x00)) return(0) ;

                    Sleep(2000) ;

            status=unlink(__signal_path) ;
         if(status) {
                       sprintf(text, "Signal file remove error %d : %s", errno, __signal_path) ;
                  EMIR_message(text) ;
                    }                                           
                                     return(1) ;
                                  }
/*--------------------------------------- Отправка сигнала остановки */

    if(!stricmp(action, "SEND" )) {

              file=fopen(__signal_path, "w") ;
           if(file==NULL) {
                       sprintf(text, "Signal file create error %d : %s", errno, __signal_path) ;
                  EMIR_message(text) ;
                          }
           else           {
                               fclose(file) ;
                          }

                                  }
/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*								     */
/*		Обработчик сообщений "рамочного" окна		     */

 LRESULT CALLBACK  EMIR_window_processor(  HWND  hWnd,     UINT  Msg,
  		                         WPARAM  wParam, LPARAM  lParam)
{
            HWND  hChild ;
            RECT  Rect ;
             int  i ;

/*----------------------------- Сообщение активизации TaskBar-иконки */

    if(Msg==TaskBar_Msg) {

        if(lParam!=WM_LBUTTONDBLCLK)  return(0) ;

       SetWindowPos(               hFrameWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) ;
       SetWindowPos(__sections[__sec_work].hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) ;

         ShowWindow(               hFrameWindow, SW_RESTORE) ;
         ShowWindow(__sections[__sec_work].hWnd, SW_RESTORE) ;

       SetWindowPos(               hFrameWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) ;
       SetWindowPos(__sections[__sec_work].hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) ;

                                              __window_closed=0 ;

                                    return(0) ;
                          }
/*---------------------------------------------- Системные сообщения */

  switch(Msg) {

/*---------------------------------------------------- Создание окна */

    case WM_CREATE: {
                       break ;
                    }
/*------------------------------------ Отработка внутренних сообений */

    case WM_USER:  {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
			  return(FALSE) ;
  			     break ;
  		   }
/*------------------------------------------------ Команды управления */

    case WM_COMMAND:  break ;

/*----------------------------------------------- Изменение размеров */

    case WM_SIZE:   {

//      if(hWnd==hExportWindow ||
//         hWnd==hFileDialog     ) {
//                  return( DefWindowProc(hWnd, Msg, wParam, lParam) ) ;
//                                 }

                    hChild=GetWindow(hWnd, GW_CHILD) ;

        if(wParam==SIZE_RESTORED  ||
           wParam==SIZE_MAXIMIZED   )
         if(IsWindow(hChild)) {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - -  Окна секций */
            for(i=0 ; i<__sections_cnt ; i++) 
              if(hChild==__sections[i].hWnd)  hChild=__sections[0].hWnd ;
           
            if(hChild==__sections[0].hWnd) {

              for(i=0 ; i<__sections_cnt ; i++) {
                     GetWindowRect(__sections[i].hWnd, &Rect) ;
                        MoveWindow(__sections[i].hWnd,  0, 0,
                                              LOWORD(lParam),
                                              HIWORD(lParam), true) ;
                                                } 

                                                break ;
                                           } 
/*- - - - - - - - - - - - - - - - - - - - - - - - - - -  Прочие окна */
                     GetWindowRect(hChild, &Rect) ;
                        MoveWindow(hChild,  0, 0,
                                              LOWORD(lParam),
                                              HIWORD(lParam), true) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                              } 

  			     break ;
  		    }
/*---------------------------------------------------- Закрытие окна */

    case WM_CLOSE: {
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - Главное окно */
      if(hWnd==hFrameWindow) {

                                PostQuitMessage(0) ;  
              			    break ;
                            }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
		return( DefWindowProc(hWnd, Msg, wParam, lParam) ) ;
			    break ;
		     }
/*------------------------------------------------- Уничтожение окна */

    case WM_DESTROY: {

               if(hWnd==hFrameWindow)  break ;

//   			PostQuitMessage(0) ;  
			     break ;
		     }
/*----------------------------------------------------------- Прочие */

    default :        {
		return( DefWindowProc(hWnd, Msg, wParam, lParam) ) ;
			    break ;
		     }
	      }
/*-------------------------------------------------------------------*/

    return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                 Обработка системных сообщений                     */

  int  EMIR_system(void)

{
      MSG  SysMessage ;

/*------------------------------------- Обработка команды завершения */

  if(__exit_flag) {
                        return(1) ;
                  }
/*------------------------------------ Обработка системных сообщений */

      while( PeekMessage(&SysMessage, NULL, 0, 0, PM_NOREMOVE) ) {

              if(SysMessage.message==WM_QUIT) {
                                                __exit_flag=1 ;
                                                       break ;
                                              }

             PeekMessage(&SysMessage, NULL, 0, 0, PM_REMOVE) ;
	TranslateMessage(&SysMessage) ;
	 DispatchMessage(&SysMessage) ;
						                 }
/*-------------------------------------------------------------------*/

  return(0) ;
}

#endif // _CONSOLE || UNIX


/*********************************************************************/
/*                                                                   */
/*                 Проверка активных секций обработки                */

  int  EMIR_active_section(char *section)

{
   char  name[128] ;

         memset(  name, 0, sizeof(name)) ;
        strncpy(  name, section, sizeof(name)-1) ;
         strupr(  name) ;
         strupr(__active) ;

   if(       __active[0]==0       )  return(1) ;
   if(strstr(__active, name)!=NULL)  return(1) ;


  return(0) ;
}


/********************************************************************/
/*                                                                  */
/*                       Система выдачи сообщений                   */

  void  EMIR_message(char *text)
{
#ifdef  UNIX
               printf("%s\n\n", text) ;
#else
         EMIR_message(text, MB_ICONSTOP) ;
#endif
}

  void  EMIR_message(char *text, int  icon)
{
       EMIR_log(text) ;

#ifdef  UNIX
               printf("%s\n\n", text) ;
#else
#ifdef _CONSOLE

    if(icon==MB_ICONINFORMATION)  Service_Log(text, "INFO") ;
    if(icon==MB_ICONWARNING    )  Service_Log(text, "WARNING") ;
    else                          Service_Log(text, "ERROR") ;

#else
     MessageBox(NULL, text, _PROGRAM_TITLE, MB_OK | MB_TASKMODAL | MB_TOPMOST | icon) ;  
#endif
#endif
}


/*********************************************************************/
/*								     */
/*	                         Ведение лога                        */

  int  EMIR_log(char *text)
{
   return(EMIR_log(text, __log_path)) ;
}

  int  EMIR_log(char *text, char *path_mask)

{
        int  log_level ;
     time_t  time_abs ;
  struct tm *hhmmss ;
       char  path[FILENAME_MAX] ;
       FILE *file ;
       char  prefix[512] ;
       char  error[8000] ;

#ifndef  UNIX
        int  status ;
#endif
 
/*------------------------------------------------- Входной контроль */
  
    if(path_mask[0]==0)  return(0) ;

/*-------------------------------------------------- Контроль уровня */

    if(text!=NULL) {
                                       log_level=0 ;
      if(strstr(text, "LVL.1")!=NULL)  log_level=1 ;
      if(strstr(text, "LVL.2")!=NULL)  log_level=2 ;
      if(strstr(text, "LVL.3")!=NULL)  log_level=3 ;

      if(log_level>__log_level)  return(0) ;

                   }           

/*------------------------------------------ Работа с датой/временем */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

          sprintf(prefix, "%4d-%02d-%02dT%02d:%02d:%02d (e:%d)",
                                    hhmmss->tm_year+1900,
                                    hhmmss->tm_mon+1,
                                    hhmmss->tm_mday,
                                    hhmmss->tm_hour,
                                    hhmmss->tm_min,    
                                    hhmmss->tm_sec,
                                   __db_errors_cnt) ;

/*---------------------------------------------- Открытие файла лога */

    if(__log_rotation)
          sprintf(path, "%s.%02d.%02d.log", path_mask, hhmmss->tm_mon+1, hhmmss->tm_mday) ;
    else  sprintf(path, "%s.log",           path_mask) ;

       file=fopen(path, "at") ;
    if(file==NULL) {
                          sprintf(error, "Ошибка открытия файла лога %d : %s", errno, path) ;
#ifndef  UNIX
#ifdef _CONSOLE
                      Service_Log(error, "ERROR") ;
#else
                       MessageBox(NULL, error, _PROGRAM_TITLE, MB_OK | MB_TASKMODAL | MB_TOPMOST | MB_ICONSTOP) ;
#endif 
#endif 
                            return(-1) ;
                   }
/*-------------------------------------------- Проверка краха памяти */
   
#ifndef UNIX

        status=_heapchk() ;
     if(status!=_HEAPOK    && 
        status!=_HEAPEMPTY   ) {

          sprintf(prefix, "Heap crash!!!") ;
           fwrite(prefix, 1, strlen(prefix), file) ;

                               } 

#endif

/*------------------------------------------------------ Запись лога */

    if(text!=NULL) {

           fwrite(prefix, 1, strlen(prefix), file) ;
           fwrite(text,   1, strlen(text),   file) ;
           fwrite("\n",   1, strlen("\n"),   file) ;

                  }
/*---------------------------------------------- Закрытие файла лога */

           fclose(file) ;

/*-------------------------------------------------------------------*/
                       
   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                      Сброс снимка данных в файл                   */

  int  EMIR_snapshot(char *text, char *file_name)

{
       char  path[FILENAME_MAX] ;
       FILE *file ;
       char  error[8000] ;

#ifndef  UNIX
        int  status ;
#endif

/*------------------------------------------- Присвоение имени файла */

  if(file_name==NULL) {
		         snprintf(path, sizeof(path)-1, "%s/%lx", __work_folder, time(NULL)) ;
                            Sleep(1000) ;
                      }
  else                {
                         snprintf(path, sizeof(path)-1, "%s/%s", __work_folder, file_name) ;
                      }
 
/*--------------------------------------------------- Открытие файла */

       file=fopen(path, "wb") ;
    if(file==NULL) {
                        sprintf(error, "Ошибка открытия файла снимка %d : %s", errno, path) ;
                       EMIR_log(error) ;
                           return(-1) ;
                   }
/*---------------------------------------------------- Запись данных */

           fwrite(text,   1, strlen(text),   file) ;

/*--------------------------------------------------- Закрытие файла */

           fclose(file) ;

/*-------------------------------------------------------------------*/
                       
   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*               Работа с файлом сохранения контекста                */

   int  EMIR_save_context(char *path, char *oper)

{
       FILE *file ;
       char  text[1024] ;
       char  message[1024] ;
//     char  dfs_type[32] ;
       char *code ;
       char *value ;
       char *end ;
        int  row ;
        int  i ;

 static  struct {
                  char *key ;
                  void *value ;     /* ВНИМАНИЕ!!! Все адреса, включаемые сюда должны иметь буфер не менее FILENAME_MAX */
                  char  type ;
                   int  size ;
                }  pars[]={
                           { "Sections=",          __active,            'C', sizeof(__active)            },
                           { "ProcLog=",           __log_path,          'C', sizeof(__log_path)          },
                           { "RpcLog=",            __rpc_path,          'C', sizeof(__rpc_path)          },
                           { "LogLevel=",         &__log_level,         'I', sizeof(__log_level)         },
                           { "LogRotation=",      &__log_rotation,      'I', sizeof(__log_rotation)      },
                           { "CriticalStop=",     &__critical_stop,     'I', sizeof(__critical_stop)     },
                           { "SignalFile=",        __signal_path,       'C', sizeof(__signal_path)       },
                           { "MonConfigFile=",     __mon_context_path,  'C', sizeof(__mon_context_path)  },
                           { "MonitoringFile=",    __monitoring_path,   'C', sizeof(__monitoring_path)   },
                           { "MonitoringRules=",   __monitoring_rules,  'C', sizeof(__monitoring_rules)  },
                           { "MonitoringFormat=",  __monitoring_format, 'C', sizeof(__monitoring_format) },
                           { "GenerationPeriod=", &__mon_gen_period,    'I', sizeof(__mon_gen_period)    },
                           { "NewBlocksDelay=",   &__mon_blocks_delay,  'I', sizeof(__mon_blocks_delay)  },
                           { "ScanExclude=",       __scan_exclude,      'C', sizeof(__scan_exclude)      },
                           { "DB_context=",        __db_context_path,   'C', sizeof(__db_context_path)   },
                           { "DB_name=",           __db_name,           'C', sizeof(__db_name)           },
                           { "DB_user=",           __db_user,           'C', sizeof(__db_user)           },
                           { "DB_password=",       __db_password,       'C', sizeof(__db_password)       },
                           { "GasValue=",          __gas_value,         'C', sizeof(__gas_value)         },
                           { "NetType=",           __net_type,          'C', sizeof(__net_type)          },
                           { "NodeURL=",           __node_url,          'C', sizeof(__node_url)          },
                           { "SwarmURL=",          __swarm_url,         'C', sizeof(__swarm_url)         },
                           { "IpfsURL=",           __ipfs_url,          'C', sizeof(__ipfs_url)          },
                           { "SwarmPause=",       &__swarm_pause,       'I', sizeof(__swarm_pause)       },
                           { "DfsPassword=",      &__dfs_password,      'C', sizeof(__dfs_password)      },
                           { "CurlPath=",          __curl_path,         'C', sizeof(__curl_path)         },
//                         { "CryptoCert=",        __crypto_cert,       'C', sizeof(__crypto_cert)       },
//                         { "CryptoSign=",        __crypto_sign,       'C', sizeof(__crypto_sign)       },
//                         { "CryptoCheck=",       __crypto_check,      'C', sizeof(__crypto_check)      },
//                         { "CryptoPack=",        __crypto_pack,       'C', sizeof(__crypto_pack)       },
//                         { "CryptoUnpack=",      __crypto_unpack,     'C', sizeof(__crypto_unpack)     },
                           { "GammaPack=",         __gamma_pack,        'C', sizeof(__gamma_pack)        },
                           { "GammaUnpack=",       __gamma_unpack,      'C', sizeof(__gamma_unpack)      },
                           { "MembersStorage=",    __members_storage,   'C', sizeof(__members_storage)   },
                           { "CertStorage=",       __cert_storage,      'C', sizeof(__cert_storage)      },
                           { "FileStorage=",       __file_storage,      'C', sizeof(__file_storage)      },
                           { "WorkFolder=",        __work_folder,       'C', sizeof(__work_folder)       },
                           { "ReportsFolder=",     __reports_folder,    'C', sizeof(__reports_folder)    },
                           { "DclStorage=",        __dcl_storage,       'C', sizeof(__dcl_storage)       },
                           { "PurgeCompleted=",   &__purge_completed,   'I', sizeof(__purge_completed)   },
                           { "PurgeDeep=",        &__purge_deep,        'I', sizeof(__purge_deep)        },
                           { "EventsDeep=",       &__events_deep,       'I', sizeof(__events_deep)       },
                           { "EventsForce=",      &__events_force,      'I', sizeof(__events_force)      },
                           { "ExternalControl=",  &__external_control,  'I', sizeof(__external_control)  },
                           { "FilesDelivery=",    &__files_delivery,    'I', sizeof(__files_delivery)    },
                           { "SysConfig_pulse=",  &__sc_request_period, 'I', sizeof(__sc_request_period) },
                           { "SysConfig_view=",   &__sc_view_frame,     'I', sizeof(__sc_view_frame)     },
                           { "Members_pulse=",    &__mb_request_period, 'I', sizeof(__mb_request_period) },
                           { "Members_view=",     &__mb_view_frame,     'I', sizeof(__mb_view_frame)     },
                           { "Deals_pulse=",      &__dl_request_period, 'I', sizeof(__dl_request_period) },
                           { "Deals_view=",       &__dl_view_frame,     'I', sizeof(__dl_view_frame)     },
                           { "Files_pulse=",      &__fl_request_period, 'I', sizeof(__fl_request_period) },
                           { "Files_view=",       &__fl_view_frame,     'I', sizeof(__fl_view_frame)     },

                           {  NULL }
                          } ;

/*==================================================== Чтение данных */

   if(!stricmp(oper, "READ" )) {

/*------------------------------------------------------- Подготовка */

//                           dfs_type[0] =0 ;
                           __log_rotation=1 ;

/*--------------------------------------------------- Открытие файла */

#ifndef  _CONSOLE

     if(access(path, 0x00)) {
                              sprintf(message, "Context file %s is missed - ignore it", path) ;
                         EMIR_message(message) ;
                               return(0) ;
                            }
#endif

        file=fopen(path, "rb") ;
     if(file==NULL) {
                        sprintf(message, "Context file open error %d :%s", errno, path) ;
                   EMIR_message(message) ;
                          return(-1) ;
                    }
/*------------------------------------------------- Считывание файла */

                    row=0 ;
     while(1) {                                                     /* CIRCLE.1 - Построчно читаем файл */
                    row++ ;
/*- - - - - - - - - - - - - - - - - - -  Считывание очередной строки */
                      memset(text, 0, sizeof(text)) ;
                   end=fgets(text, sizeof(text)-1, file) ;          /* Считываем строку */
                if(end==NULL)  break ;

         if(text[0]==';')  continue ;                               /* Проходим комментарий */

            end=strchr(text, '\n') ;                                /* Удаляем символ конца строки */
         if(end!=NULL)  *end=0 ;
            end=strchr(text, '\r') ;
         if(end!=NULL)  *end=0 ;

             EMIR_text_trim(text) ;                                 /* Поджимаем крайние пробелы */
         if(text[0]== 0 )  continue ;                               /* Игнорируем пустые строки */
/*- - - - - - - - - - - - - - - - - - - - - - Настройки криптографии */
         if(!memcmp(text, "CryptoCert=",   strlen("CryptoCert="  )) ||
            !memcmp(text, "CryptoSign=",   strlen("CryptoSign="  )) ||
            !memcmp(text, "CryptoCheck=",  strlen("CryptoCheck=" )) ||
            !memcmp(text, "CryptoPack=",   strlen("CryptoPack="  )) ||
            !memcmp(text, "CryptoUnpack=", strlen("CryptoUnpack="))   )
         {

              value=strchr(text, '=')+1 ;

                end=strchr(text, '$')  ;
             if(end==NULL) { code="DEFAULT" ; }
             else          { code=value ;  value=end+1 ;  *end=0 ; }

           for(i=0 ; i<_CRYPTO_MAX ; i++) {
             if(        __crypto[i].type[0]==0 )  break ;
             if(stricmp(__crypto[i].type, code))  break ;
                                          }

             if(i>=_CRYPTO_MAX) {
                                   sprintf(message, "Context file %s - too many crypto schemes defined in row %d", path, row) ;
                              EMIR_message(message) ;
                                      fclose(file) ;
                                       return(-1) ;
                                }

            if(!strcmp(code, "DEFAULT")) {
             if(strstr(text, "Cert="  )!=NULL)  strncpy(__crypto_cert,   value, sizeof(__crypto_cert  )-1) ;
             if(strstr(text, "Sign="  )!=NULL)  strncpy(__crypto_sign,   value, sizeof(__crypto_sign  )-1) ;
             if(strstr(text, "Check=" )!=NULL)  strncpy(__crypto_check,  value, sizeof(__crypto_check )-1) ;
             if(strstr(text, "Pack="  )!=NULL)  strncpy(__crypto_pack,   value, sizeof(__crypto_pack  )-1) ;
             if(strstr(text, "Unpack=")!=NULL)  strncpy(__crypto_unpack, value, sizeof(__crypto_unpack)-1) ;
                                         }

             if(strstr(text, "Cert="  )!=NULL)  strncpy(__crypto[i].cert,   value, sizeof(__crypto[i].cert  )-1) ;
             if(strstr(text, "Sign="  )!=NULL)  strncpy(__crypto[i].sign,   value, sizeof(__crypto[i].sign  )-1) ;
             if(strstr(text, "Check=" )!=NULL)  strncpy(__crypto[i].check,  value, sizeof(__crypto[i].check )-1) ;
             if(strstr(text, "Pack="  )!=NULL)  strncpy(__crypto[i].pack,   value, sizeof(__crypto[i].pack  )-1) ;
             if(strstr(text, "Unpack=")!=NULL)  strncpy(__crypto[i].unpack, value, sizeof(__crypto[i].unpack)-1) ;

                                 continue ;
         }
/*- - - - - - - - - - - - - - - - - - - -  Разборка остальных ключей */
       for(i=0 ; pars[i].key!=NULL ; i++)                           /* Идентиф.ключ */
         if(!memcmp(text, pars[i].key,
                   strlen(pars[i].key ))) {

             if(pars[i].type=='I')  *(int *)pars[i].value=atoi(text+strlen(pars[i].key)) ;
             else           strncpy((char *)pars[i].value, text+strlen(pars[i].key), pars[i].size-1) ;

                                                break ;
                                          }

         if(pars[i].key!=NULL)  continue ;                          /* Если строка идентифицирована - */
                                                                    /*     переходим к следующей      */
               sprintf(message, "Context file %s - Unknown key in row %d", path, row) ;
          EMIR_message(message) ;
                fclose(file) ;
                 return(-1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
              }                                                     /* CONTINUE.1 */
/*--------------------------------------------------- Закрытие файла */

                   fclose(file) ;

/*---------------------------------------- Криптография по-умолчанию */

       for(i=0 ; i<_CRYPTO_MAX ; i++)
         if(!stricmp(__crypto[i].type, "DEFAULT")) {

               strcpy(__crypto_cert,   __crypto[i].cert  ) ;
               strcpy(__crypto_sign,   __crypto[i].sign  ) ;
               strcpy(__crypto_check,  __crypto[i].check ) ;
               strcpy(__crypto_pack,   __crypto[i].pack  ) ;
               strcpy(__crypto_unpack, __crypto[i].unpack) ;

                                  break ;
                                                   }

/*----------------------------------- Преобразование особых значений */

           strupr(__net_type) ;
        if(strcmp(__net_type, ""        ) &&
           strcmp(__net_type, "QUORUM"  ) &&
           strcmp(__net_type, "ETHEREUM")   ) {
                     sprintf(message, "Unknown value for key NetType - must be ETHEREUM, QUORUM or empty value") ;
                EMIR_message(message) ;
                          return(-1) ;
                                              }
/*--------------------------------------------- Назначение умолчаний */
 
        if(__mon_gen_period  <=0)    __mon_gen_period= 5 ;
        if(__mon_blocks_delay<=0)  __mon_blocks_delay=20 ;

/*-------------------------------------------------------------------*/
                               }
/*==================================================== Запись данных */

   if(!stricmp(oper, "WRITE")) {

/*--------------------------------------------------- Открытие файла */

        file=fopen(path, "wb") ;
     if(file==NULL) {
                        sprintf(message, "Context file open error %d :%s", errno, path) ;
                   EMIR_message(message) ;
                          return(-1) ;
                    }
/*----------------------------------- Преобразование особых значений */

/*--------------------------------------------- Запись прочих ключей */

       for(i=0 ; pars[i].key!=NULL ; i++) {

             fwrite(pars[i].key, 1, strlen(pars[i].key), file) ;

         if(pars[i].type=='C') {
             fwrite(pars[i].value,  1, strlen((char *)pars[i].value), file) ;
                               }

             fwrite("\r\n",  1, strlen("\r\n"), file) ;

                                          }
/*--------------------------------------------------- Закрытие файла */

                   fclose(file) ;

/*------------------- Обратное преобразование многострочных значений */

/*-------------------------------------------------------------------*/
                               }
/*===================================================================*/

  return(0) ;
}


/*********************************************************************/
/*								     */
/*               Работа с файлом назначения таблиц БД                */

   int  EMIR_db_context(char *path)

{
       FILE *file ;
       char  text[1024] ;
       char  message[1024] ;
       char *key ;
       char *value ;
       char *end ;
        int  row ;
        int  i ;

 static  struct {
                  char *key ;
                  void *value ;     /* ВНИМАНИЕ!!! Все адреса, включаемые сюда должны иметь буфер не менее FILENAME_MAX */
                  char  type ;
                   int  size ;
                }  pars[]={
                           { "db_char_separator",          __db_char_sep                  , 'C', sizeof(__db_char_sep                  ) },
                           { "table_system_configuration", __db_table_system_configuration, 'C', sizeof(__db_table_system_configuration) },
                           { "table_system_pars",          __db_table_system_pars         , 'C', sizeof(__db_table_system_pars         ) },
                           { "table_system_alert",         __db_table_system_alert        , 'C', sizeof(__db_table_system_alert        ) },
                           { "table_system_actions",       __db_table_system_actions      , 'C', sizeof(__db_table_system_actions      ) },
                           { "table_members",              __db_table_members             , 'C', sizeof(__db_table_members             ) },
                           { "table_members_files",        __db_table_members_files       , 'C', sizeof(__db_table_members_files       ) },
                           { "table_members_actions",      __db_table_members_actions     , 'C', sizeof(__db_table_members_actions     ) },
                           { "table_deals",                __db_table_deals               , 'C', sizeof(__db_table_deals               ) },
                           { "table_deals_files",          __db_table_deals_files         , 'C', sizeof(__db_table_deals_files         ) },
                           { "table_deals_parties",        __db_table_deals_parties       , 'C', sizeof(__db_table_deals_parties       ) },
                           { "table_deals_attributes",     __db_table_deals_attributes    , 'C', sizeof(__db_table_deals_attributes    ) },
                           { "table_deals_statusmap",      __db_table_deals_statusmap     , 'C', sizeof(__db_table_deals_statusmap     ) },
                           { "table_deals_history",        __db_table_deals_history       , 'C', sizeof(__db_table_deals_history       ) },
                           { "table_deals_arbitration",    __db_table_deals_arbitration   , 'C', sizeof(__db_table_deals_arbitration   ) },
                           { "table_deals_actions",        __db_table_deals_actions       , 'C', sizeof(__db_table_deals_actions       ) },
                           { "table_files_actions",        __db_table_files_actions       , 'C', sizeof(__db_table_files_actions       ) },
                           { "table_scan_state",           __db_table_scan_state          , 'C', sizeof(__db_table_scan_state          ) },
                           { "table_scan_transactions",    __db_table_scan_transactions   , 'C', sizeof(__db_table_scan_transactions   ) },
                           { "table_scan_accounts",        __db_table_scan_accounts       , 'C', sizeof(__db_table_scan_accounts       ) },
                           { "table_scan_events",          __db_table_scan_events         , 'C', sizeof(__db_table_scan_events         ) },
                           { "table_sentry_pars",          __db_table_sentry_pars         , 'C', sizeof(__db_table_sentry_pars         ) },
                           { "table_sentry_state",         __db_table_sentry_state        , 'C', sizeof(__db_table_sentry_state        ) },
                           { "table_sentry_nodes",         __db_table_sentry_nodes        , 'C', sizeof(__db_table_sentry_nodes        ) },
                           { "table_oracle_processor",     __db_table_oracle_processor    , 'C', sizeof(__db_table_oracle_processor    ) },
                           { "table_ext_control",          __db_table_ext_control         , 'C', sizeof(__db_table_ext_control         ) },
                           {  NULL }
                          } ;

/*---------------------------------------------- Установка умолчаний */

     strcpy(__db_char_sep                  , _DB_CHAR_SEPARATOR) ;

     strcpy(__db_table_system_configuration, _TABLE_SYSTEM_CONFIGURATION) ;
     strcpy(__db_table_system_pars         , _TABLE_SYSTEM_PARS) ;
     strcpy(__db_table_system_alert        , _TABLE_SYSTEM_ALERT) ;
     strcpy(__db_table_system_actions      , _TABLE_SYSTEM_ACTIONS) ;
     strcpy(__db_table_members             , _TABLE_MEMBERS) ;
     strcpy(__db_table_members_files       , _TABLE_MEMBERS_FILES) ;
     strcpy(__db_table_members_actions     , _TABLE_MEMBERS_ACTIONS) ;
     strcpy(__db_table_deals               , _TABLE_DEALS) ;
     strcpy(__db_table_deals_files         , _TABLE_DEALS_FILES) ;
     strcpy(__db_table_deals_parties       , _TABLE_DEALS_PARTIES) ;
     strcpy(__db_table_deals_history       , _TABLE_DEALS_HISTORY) ;
     strcpy(__db_table_deals_attributes    , _TABLE_DEALS_ATTRIBUTES) ;
     strcpy(__db_table_deals_statusmap     , _TABLE_DEALS_STATUSMAP) ;
     strcpy(__db_table_deals_arbitration   , _TABLE_DEALS_ARBITRATION) ;
     strcpy(__db_table_deals_actions       , _TABLE_DEALS_ACTIONS) ;
     strcpy(__db_table_files_actions       , _TABLE_FILES_ACTIONS) ;
     strcpy(__db_table_scan_state          , _TABLE_SCAN_STATE) ;
     strcpy(__db_table_scan_transactions   , _TABLE_SCAN_TRANSACTIONS) ;
     strcpy(__db_table_scan_accounts       , _TABLE_SCAN_ACCOUNTS) ;
     strcpy(__db_table_scan_events         , _TABLE_SCAN_EVENTS) ;
     strcpy(__db_table_sentry_pars         , _TABLE_SENTRY_PARS) ;
     strcpy(__db_table_sentry_state        , _TABLE_SENTRY_STATE) ;
     strcpy(__db_table_sentry_nodes        , _TABLE_SENTRY_NODES) ;
     strcpy(__db_table_oracle_processor    , _TABLE_ORACLE_PROCESSOR) ;
     strcpy(__db_table_ext_control         , _TABLE_EXT_CONTROL) ;

/*------------------------------------------------- Входной контроль */

    if(path[0]==0) {
                                return(0) ;
                   }
/*--------------------------------------------------- Открытие файла */

     if(access(path, 0x00)) {
                              sprintf(message, "DB-context file %s is missed - ignore it", path) ;
#ifdef UNIX
                         EMIR_message(message) ;
#else
                         EMIR_message(message, MB_ICONWARNING) ;
#endif
                               return(0) ;
                            }

        file=fopen(path, "rb") ;
     if(file==NULL) {
                        sprintf(message, "DB-context file open error %d :%s", errno, path) ;
                   EMIR_message(message) ;
                          return(-1) ;
                    }
/*------------------------------------------------- Считывание файла */

                    row=0 ;
     while(1) {                                                     /* CIRCLE.1 - Построчно читаем файл */
                    row++ ;
/*- - - - - - - - - - - - - - - - - - -  Считывание очередной строки */
                      memset(text, 0, sizeof(text)) ;
                   end=fgets(text, sizeof(text)-1, file) ;          /* Считываем строку */
                if(end==NULL)  break ;

         if(text[0]==';')  continue ;                               /* Проходим комментарий */

            end=strchr(text, '\n') ;                                /* Удаляем символ конца строки */
         if(end!=NULL)  *end=0 ;
            end=strchr(text, '\r') ;
         if(end!=NULL)  *end=0 ;

             EMIR_text_trim(text) ;                                 /* Поджимаем крайние пробелы */
         if(text[0]== 0 )  continue ;                               /* Игнорируем пустые строки */
/*- - - - - - - - - - - - - - - - - - - - Выделение ключа и значения */
            key  =       text ;
            value=strchr(text, '=') ;
         if(value==NULL) {
                           sprintf(message, "DB-context file %s - Invalid record structure in row %d", path, row) ;
                      EMIR_message(message) ;
                            return(-1) ;
                         }
           *value=0 ; 
            value++ ; 

            EMIR_text_trim(key) ;
            EMIR_text_trim(value) ;
/*- - - - - - - - - - - - - - - - - - - -  Разборка остальных ключей */
       for(i=0 ; pars[i].key!=NULL ; i++)                           /* Идентиф.ключ */
         if(!stricmp(key, pars[i].key)) {

             if(pars[i].type=='I')  *(int *)pars[i].value=atoi(value) ;
             else           strncpy((char *)pars[i].value, value, pars[i].size-1) ;

                                                break ;
                                        }

         if(pars[i].key!=NULL)  continue ;                          /* Если строка идентифицирована - */
                                                                    /*     переходим к следующей      */
               sprintf(message, "DB-context file %s - Unknown key in row %d", path, row) ;
          EMIR_message(message) ;
                 return(-1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
              }                                                     /* CONTINUE.1 */
/*--------------------------------------------------- Закрытие файла */

                   fclose(file) ;

/*----------------------------------- Преобразование особых значений */
 
/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*               Работа с файлом настроек мониторинга                */

   int  EMIR_mon_context(char *path)

{
       FILE *file ;
       char  text[1024] ;
       char  message[1024] ;
       char *end ;
        int  row ;
        int  i ;

 static  struct {
                  char *key ;
                  void *value ;     /* ВНИМАНИЕ!!! Все адреса, включаемые сюда должны иметь буфер не менее FILENAME_MAX */
                  char  type ;
                   int  size ;
                }  pars[]={
                           { "StateFile=",         __mon_state_path,     'C', sizeof(__mon_state_path)     },
                           { "NodeId=",            __mon_node_id,        'C', sizeof(__mon_node_id)        },
                           { "BlockChainPulse=",  &__mon_bh_pulse,       'I', sizeof(__mon_bh_pulse)       },
                           { "DfsPulse=",         &__mon_dfs_pulse,      'I', sizeof(__mon_dfs_pulse)      },
                           { "AlivePulse=",       &__mon_alive_pulse,    'I', sizeof(__mon_alive_pulse)    },
                           { "BasicAccount=",      __mon_basic_account,  'C', sizeof(__mon_basic_account)  },
                           { "BasicPassword=",     __mon_basic_password, 'C', sizeof(__mon_basic_password) },
                           { "BalanceLimit=",      __mon_balance_limit,  'C', sizeof(__mon_balance_limit)  },
                           { "АliveАddress=",      __mon_alive_address,  'C', sizeof(__mon_alive_address)  },
                           {  NULL }
                          } ;

/*------------------------------------------------- Входной контроль */

    if(path[0]==0) {
                                return(0) ;
                   }
/*--------------------------------------------------- Открытие файла */

#ifndef  _CONSOLE

     if(access(path, 0x00)) {
                              sprintf(message, "Monitoring configuration file %s is missed", path) ;
                         EMIR_message(message) ;
                               return(-1) ;
                            }
#endif

        file=fopen(path, "rb") ;
     if(file==NULL) {
                        sprintf(message, "Monitoring configuration file open error %d :%s", errno, path) ;
                   EMIR_message(message) ;
                          return(-1) ;
                    }
/*------------------------------------------------- Считывание файла */

                    row=0 ;
     while(1) {                                                     /* CIRCLE.1 - Построчно читаем файл */
                    row++ ;
/*- - - - - - - - - - - - - - - - - - -  Считывание очередной строки */
                      memset(text, 0, sizeof(text)) ;
                   end=fgets(text, sizeof(text)-1, file) ;          /* Считываем строку */
                if(end==NULL)  break ;

         if(text[0]==';')  continue ;                               /* Проходим комментарий */

            end=strchr(text, '\n') ;                                /* Удаляем символ конца строки */
         if(end!=NULL)  *end=0 ;
            end=strchr(text, '\r') ;
         if(end!=NULL)  *end=0 ;

             EMIR_text_trim(text) ;                                 /* Поджимаем крайние пробелы */
         if(text[0]== 0 )  continue ;                               /* Игнорируем пустые строки */
/*- - - - - - - - - - - - - - - - - - - -  Разборка остальных ключей */
       for(i=0 ; pars[i].key!=NULL ; i++)                           /* Идентиф.ключ */
         if(!memcmp(text, pars[i].key,
                   strlen(pars[i].key ))) {

             if(pars[i].type=='I')  *(int *)pars[i].value=atoi(text+strlen(pars[i].key)) ;
             else           strncpy((char *)pars[i].value, text+strlen(pars[i].key), pars[i].size-1) ;

                                                break ;
                                          }

         if(pars[i].key!=NULL)  continue ;                          /* Если строка идентифицирована - */
                                                                    /*     переходим к следующей      */
               sprintf(message, "Monitoring configuration file %s - Unknown key in row %d", path, row) ;
          EMIR_message(message) ;
                 return(-1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
              }                                                     /* CONTINUE.1 */
/*--------------------------------------------------- Закрытие файла */

                   fclose(file) ;

/*----------------------------------- Преобразование особых значений */
 
/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                        Выбор секции                               */

  void  EMIR_Change_Section(char *name, int  indicate_flag)

{
#if defined(UNIX) || defined(_CONSOLE)
#else

  char  text[1024] ;
   int  idx ;

/*--------------------------------------- Определение индекса секции */

     for(idx=0 ; idx<__sections_cnt ; idx++)
        if(!strcmp(name, __sections[idx].title)) break ;

        if(idx>=__sections_cnt) {

          if(indicate_flag) {
                  sprintf(text, "Секция <%s> не включена в конфигурацию", name) ;
             EMIR_message(text) ;
                            }
               
                                        return ;
                                }
/*--------------------------------------- Переключение секции секции */

         if(idx!=__sec_work) {
                                 __sec_change_time=time(NULL) ;

                    ShowWindow(__sections[  idx     ].hWnd, SW_RESTORE) ;
                    ShowWindow(__sections[__sec_work].hWnd, SW_HIDE) ;
                                          __sec_work=idx ;
                   SendMessage(__sections[  idx     ].hWnd, WM_USER,
                                (WPARAM)_USER_SECTION_ENABLE, NULL) ;
                             }

               TabCtrl_SetCurSel(GetDlgItem(__sections[__sec_work].hWnd, 
                                                   IDC_SECTIONS_SWITCH), __sec_work) ;

/*-------------------------------------------------------------------*/     

#endif
}


/*********************************************************************/
/*                                                                   */
/*          Отсечка начальных и конечных пробельных символов         */

  void  EMIR_text_trim(char *text)

{
  char *beg ;
  char *end ;


    for(end=text+strlen(text)-1 ; 
        end>=text && (*end==' ' || *end=='\t'
                                || *end=='\n'
                                || *end=='\r') ; end--)  *end=0 ;

    for(beg=text ;    *beg==' ' || *beg=='\t'
                                || *beg=='\n'
                                || *beg=='\r'  ; beg++)  ;
  

        strcpy(text, beg) ;
}


/*********************************************************************/
/*                                                                   */
/*                 Формирование пути к разделу                       */
/*                                                                   */
/*  Возвращает: 0 или код ошибки                                     */

  int  EMIR_create_path(char *folder)

{
  char  path[FILENAME_MAX] ;
  char *cut ;
   int  status ;

/*------------------------------------------------------- Подготовка */

               strncpy(path, folder, sizeof(path)-1) ;

/*-------------------------------------------------- Замена символов */

    for(cut=path ; *cut ; cut++)  if(*cut=='/')  *cut='\\' ;

       if(path[1]==':') {                                           /* Замена символов драйвера */
         if(path[0]=='a' || path[0]=='А')  path[0]='A' ;
         if(path[0]=='в' || path[0]=='В')  path[0]='B' ;
         if(path[0]=='с' || path[0]=='С')  path[0]='C' ;
                        }
/*-------------------------------------- Проверка существования пути */

        status=access(path, 0x00) ;
     if(status==0)  return(0) ;

/*------------------------------------------------ Формирование пути */

          if(path[0]=='\\' &&                                       /* Для сетевых ресурсов */
             path[1]=='\\'   )  cut=strchr(path+2, '\\')+1 ;
          else                  cut=path ;

        for( ; ; cut=cut+1) {
                        
              cut=strchr(cut, '\\') ;
           if(cut!=NULL) *cut=0 ;

                       status=access(path, 0x00) ;
#ifdef UNIX
           if(status)  status= mkdir(path, 0777) ;
#else
           if(status)  status= mkdir(path) ;
#endif

           if(status) {
                           EMIR_log(path) ;
                             return(errno) ;
                      }

           if(cut!=NULL) *cut='\\' ;
           if(cut==NULL)  break ;
                            }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/********************************************************************/
/*								    */
/*                 Подстановка полей данных                         */

   int  EMIR_text_subst(char *buff, char *name, char *value, int  cnt)

{
   char *entry ;
    int  i ;


             entry=buff ;

     for(i=0 ; i<10000 ; i++) {

             entry=strstr(entry, name) ;
          if(entry==NULL)  break ;
                         
               memmove(entry+strlen(value), entry+strlen(name), 
                                       strlen(entry+strlen(name))+1) ;
                memcpy(entry, value, strlen(value)) ;

             cnt-- ;
          if(cnt==0)  break ;
                           } ;

  return(i) ;
}


/*********************************************************************/
/*                                                                   */
/*                       Просмотр ссылки в броузере                  */

   void  EMIR_view_html(char *link)

{
#ifndef  UNIX

    SHELLEXECUTEINFO  ShInfo ;


                        ShInfo.cbSize      =sizeof(SHELLEXECUTEINFO) ;
//                      ShInfo.fMask       = NULL ;
                        ShInfo.fMask       = SEE_MASK_CLASSNAME ;
                        ShInfo.hwnd        = NULL ;
                        ShInfo.lpVerb      = NULL ;
                        ShInfo.lpFile      = link ;
                        ShInfo.lpParameters= NULL ;
                        ShInfo.lpDirectory = NULL ;
                        ShInfo.nShow       =SW_SHOW ;
                        ShInfo.hInstApp    = NULL ;
                        ShInfo.lpClass     = ".html" ;
        ShellExecuteEx(&ShInfo);

#endif
}


/********************************************************************/
/*                                                                  */
/*               THREAD - Главный фоновый поток                     */

#ifndef  UNIX

  DWORD WINAPI  Main_Thread(LPVOID Pars)

{
   SQL_ODBC_link  DB ;
            HWND  hDlg ;
          time_t  time_0 ;
          time_t  time_abs ;
       struct tm *hhmmss ;
            char  prefix[512] ;
            char  text[1024] ;
             int  status ;

/*---------------------------------------------------- Инициализация */

              hDlg=hSysConfig_Dialog ;

            time_0=time(NULL) ;

/*------------------------------------------------------- Общий цикл */

   do {

          if(__exit_flag)  break ;                                  /* Если общий стоп */ 
 
          if(EMIR_active_section("SYSTEM"  ) ||
             EMIR_active_section("MEMBERS" ) ||
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
     if(!DB.connected) {

         status=DB.Connect(__db_user, __db_password,
                           __db_name, "ODBC", NULL  ) ; 
      if(status) {
                      sprintf(text, "%s - ERROR - DB connection: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }

                DB.SetAutoCommit(0) ;

         status=DB.AllocCursors(10) ; 
      if(status) {
                      sprintf(text, "%s - ERROR - DB cursors allocation: %s", prefix, DB.error_text) ;
                   LB_ADD_ROW(IDC_LOG, text) ;
                        break ;
                 }

                                           EMIR_db_config(&DB) ;    /* Считывание настроек из БД */
                       }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Процедуры модулей */
   if(EMIR_active_section("SYSTEM"  ))  SysConfig_Process(&DB) ;
   if(EMIR_active_section("MEMBERS" ))    Members_Process(&DB) ;
   if(EMIR_active_section("DEALS"   ))      Deals_Process(&DB) ;
   if(EMIR_active_section("FILES"   ))      Files_Process(&DB) ;
   if(EMIR_active_section("SCAN"    ))       Scan_Process(&DB, "SCAN") ;
   if(EMIR_active_section("EVENTS"  ))       Scan_Process(&DB, "EVENTS") ;
   if(EMIR_active_section("SENTRY"  ))     Sentry_Process(&DB) ;

#ifdef  UNIX
   if(EMIR_active_section("ORACLE"  ))     Oracle_Process(&DB) ;
#endif
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
          } while(0) ;

      } while(1) ;

/*------------------------------------------------ Отсоединение с БД */

            PostThreadMessage(hMainQueue_PID, WM_QUIT, NULL, NULL) ;

                DB.Disconnect() ;                                   /* Отсоединение с БД */

/*-------------------------------------------------------------------*/
                                    
  return(0) ;
}

#endif


/********************************************************************/
/*                                                                  */
/*                    Считывание настроек из БД                     */

  int  EMIR_db_config(SQL_link *db)

{
  SQL_cursor *Cursor ;
        char error[2048] ;
        char text[8000] ;
         int  status ;

/*----------------------------------- Инициализация статусных таблиц */

        Cursor=db->LockCursor("EMIR_db_config") ;
     if(Cursor==NULL) {
                           sprintf(text, "ERROR - EMIR_db_config - Cursor lock: %s", db->error_text) ;
                          EMIR_log(text) ;
                          __db_errors_cnt++ ;
                             return(-1) ;
                      }
/*- - - - - - - - - - - - - - - - - - - - - - - -  Таблица ScanState */
   do {
                      sprintf(text, "select * from  %s",
                                     __db_table_scan_state) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                       sprintf(text, "ScanState select error: %s", db->error_text) ;
                      EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                           return(-1) ;
                }

        status=db->SelectFetch(Cursor) ;
               db->SelectClose(Cursor) ;

     if(status==  0         )  break ;
     if(status!=_SQL_NO_DATA) {
                                     sprintf(text, "ScanState select error: %s", db->error_text) ;
                                    EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                    return(-1) ;
                              }

                     sprintf(text, "insert into %s(\"BlockLast\",\"BlockDb\") values(0, 0)",
                                     __db_table_scan_state) ;
       status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                       sprintf(text, "ScanState insert error: %s", db->error_text) ;
                      EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                         return(-1) ;
                }

                     db->Commit() ;

      } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Таблица SentryState */
   do {
                      sprintf(text, "select * from  %s",
                                     __db_table_sentry_state) ;
        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(text, "SentryState select error: %s", db->error_text) ;
                        EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                             return(-1) ;
                }

        status=db->SelectFetch(Cursor) ;
               db->SelectClose(Cursor) ;

     if(status==  0         )  break ;
     if(status!=_SQL_NO_DATA) {
                                     sprintf(text, "SentryState select error: %s", db->error_text) ;
                                    EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                         return(-1) ;
                              }

                     sprintf(text, "insert into %s(\"BlockLast\") values('')",
                                     __db_table_sentry_state) ;
       status=db->SqlExecute(Cursor, text, NULL, 0) ;
     if(status) {
                       sprintf(text, "SentryState insert error: %s", db->error_text) ;
                      EMIR_log(text) ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                         return(-1) ;
                }

                     db->Commit() ;

      } while(0) ;
/*- - - - - - - - - - - - - - - - - - - - - -  Освобождение ресурсов */
               db->UnlockCursor(Cursor) ;

/*---------------------------------------------------- Параметры DFS */
/*- - - - - - - - - - - - - - - - -  Запрос типа файлового хранилища */
        status=EMIR_db_syspar(db, "DfsType(s)", __dfs_type, error) ;
     if(status) {
       if(strstr(error, "No record")!=NULL) {
                            strcpy(__dfs_type, "SWARM") ;
                                            }
       else                                 { 
                           sprintf(text, "EMIR_db_context - DFS type: %s", error) ;
                          EMIR_log(text) ;
                               return(-1) ;
                                            }
                }

     if(stricmp(__dfs_type, "DIRECT"  ) &&
        stricmp(__dfs_type, "SWARM"   ) &&
        stricmp(__dfs_type, "IPFS"    ) &&
        stricmp(__dfs_type, "MS_SHARE")   ) {
                           sprintf(text, "EMIR_db_context - DFS type unknown value: %s", __dfs_type) ;
                          EMIR_log(text) ;
                                 __dfs_type[0]=0 ;
                                            }
/*- - - - - - - - - - - - - - - -  Запрос адреса файлового хранилища */
        status=EMIR_db_syspar(db, "DfsUrl(s)", __dfs_url, error) ;
     if(status) {
       if(strstr(error, "No record")!=NULL) {
                            strcpy(__dfs_url, "") ;
                                            }
       else                                 { 
                           sprintf(text, "EMIR_db_context - DFS Url: %s", error) ;
                          EMIR_log(text) ;
                               return(-1) ;
                                            }

                }
/*- - - - - - - - - - - - - - - - - - - - - Критический размер файла */
        status=EMIR_db_syspar(db, "DfsDirectMax(s)", text, error) ;
     if(status) {
       if(strstr(error, "No record")!=NULL) {
                            strcpy(text, "0") ;
                                            }
       else                                 { 
                           sprintf(text, "EMIR_db_context - Dfs Direct Max: %s", error) ;
                          EMIR_log(text) ;
                               return(-1) ;
                                            }

                }

                    __dfs_direct_max=atoi(text) ;
/*- - - - - - - - - - - - - - - - - - - - - Критический размер файла */
        status=EMIR_db_syspar(db, "DfsDirectBox(s)", __dfs_direct_box, error) ;
     if(status) {
       if(strstr(error, "No record")!=NULL) {
                            strcpy(__dfs_direct_box, "") ;
                                            }
       else                                 { 
                           sprintf(text, "EMIR_db_context - Dfs Direct Box: %s", error) ;
                          EMIR_log(text) ;
                               return(-1) ;
                                            }

                }
/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                   Загрузка файла в память                         */

typedef  struct {
                  char  path[FILENAME_MAX] ;
                  char *data ;
                } FileData ;

   char *EMIR_loadfile(char *path, char *error)
{
  static FileData **files ;
  static      int   files_cnt ;
      struct stat   file_stat ;
             FILE  *file ;
             long   size ;
              int   status ;
              int   i ;

/*-------------------------------- Проверка среди загрущенных файлов */

   for(i=0 ; i<files_cnt ; i++) 
     if(!strcmp(path, files[i]->path))  return(files[i]->data) ;

/*-------------------------------------------- Загрузка нового файла */

       status=stat(path, &file_stat) ;
    if(status) {
                      sprintf(error, "Get file size error %d : %s", errno, path) ;
                       return(NULL) ;
               }

            size=file_stat.st_size ;

          files                 =(FileData **)realloc(files, (files_cnt+1)*sizeof(*files)) ;
          files[files_cnt]      =(FileData * ) calloc(1, sizeof(**files)) ;
   strcpy(files[files_cnt]->path, path) ;
          files[files_cnt]->data=(char *)calloc(1, size+2) ;

       file=fopen(path, "rb") ;
    if(file==NULL) {
                      sprintf(error, "File open error %d : %s", errno, path) ;
                       return(NULL) ;
                   }

            fread(files[files_cnt]->data, 1, size, file) ;
           fclose(file) ;

               files_cnt++ ;  

/*-------------------------------------------------------------------*/

  return(files[files_cnt-1]->data) ;
}

