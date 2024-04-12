/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                        Общие утилиты                              */
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

/*---------------------------------------------------- Прототипы п/п */

    int  EMIR_swarm_putfile     (char *, char *, char *) ;       /* Загрузка файла в файловое хранилище SWARM */
    int  EMIR_swarm_getfile     (char *, char *, char *) ;       /* Получение файла из файлового хранилища SWARM */
    int  EMIR_share_putfile     (char *, char *, char *) ;       /* Загрузка файла в файловое хранилище MS Share */
    int  EMIR_share_getfile     (char *, char *, char *) ;       /* Получение файла из файлового хранилища MS Share */
    int  EMIR_ipfs_putfile      (char *, char *, char *) ;       /* Загрузка файла в файловое хранилище IPFS */
    int  EMIR_ipfs_getfile      (char *, char *, char *) ;       /* Получение файла из файлового хранилища IPFS */
    int  EMIR_direct_putfile    (char *, char *, char *) ;       /* Загрузка файла в блокчейн */
    int  EMIR_direct_getfile    (char *, char *, char *) ;       /* Получение файла из блокчейна */

    int  EMIRi_Http_Receive_ctrl(char *, int) ;                  /* Функция управления приемом для HTTP */
    int  EMIRi_share_connect    (char *, char *) ;               /* Присоединение к файловому хранилищу MS Share */


/*********************************************************************/
/*								     */
/*	                   Обмен данными с узлом                     */

  int  EMIR_node_exchange(char *url_, char *text, int size)

{
          ABTP_tcp  Transport ;
              char  url[512] ;
              char *port ;
              char *end ;
               int  status ;
               int  cnt ;

      static  char *buff ;
      static   int  buff_size ;


#define _BUFF_MAX   512000

/*---------------------------------------------------- Инициализация */

   if(buff_size<size+8) {
                                 buff_size=size+8 ;
        if(buff_size<_BUFF_MAX)  buff_size=_BUFF_MAX ;

        if(buff!=NULL)  free(buff) ;

                             buff=(char *)calloc(1, buff_size) ;

                        }
/*-------------------------------------------- Раскладка URL сервера */

             memset(url,    0, sizeof(url)  ) ;
            strncpy(url, url_, sizeof(url)-1) ;
        port=strchr(url, ':')  ;
     if(port==NULL) {
                       sprintf(text, "Формат URL: <host>:<port>") ;
                          return(-1) ;
                    }

       *port=0 ;
        port++ ;

/*-------------------------------------------- Соединение с сервером */

                Transport.mServer_name=url ;
                Transport.mServer_port=strtoul(port, &end, 10) ;
 
    if(*end!=0) {
                   sprintf(text, "Формат URL - некорректный номер порта") ;
                      return(-1) ;
                }

       status=Transport.LinkToServer() ;
    if(status) {
                 sprintf(text, "Ошибка соединения с сервером %s по порту %s : %d", url, port, status) ;
                EMIR_log("Geth connection lost - wait 5 seconds") ;
                   Sleep(5000) ;
                     return(-1) ;
               } 
/*---------------------------------------- Формирование HTTP-запроса */

                    sprintf(buff, "POST / HTTP/1.0\r\n"
                                  "Host: %s\r\n"
//                                "Accept: application/json\r\n"
                                  "Content-Type: application/json; charset=UTF-8\r\n"
	                          "Content-Length: %d\r\n"
                                  "\r\n%s",
                                   url, (int)strlen(text), text) ;

                  EMIR_log(buff, __rpc_path) ;

/*------------------------------------------------- Отправка запроса */

         cnt=Transport.iSend(Transport.mSocket_cli,                /* Передаем запрос */ 
                               buff, strlen(buff), _WAIT_RECV, NULL) ;
      if(cnt==SOCKET_ERROR) {                                      /* Если ошибка... */
            sprintf(text, "Request send error - %d", WSAGetLastError()) ;
                                          return(-1) ;
                            }
/*----------------------------------------------------- Прием ответа */

                         memset(buff, 0, buff_size) ;              /* Очищаем приемный буфер */
                                     cnt=buff_size ;

             Transport.mReceiveCallback=EMIRi_Http_Receive_ctrl ;   /* Задаем управляющую функцию приема */
         cnt=Transport.iReceive(Transport.mSocket_cli,              /* Принимаем ответ */ 
                                    buff, cnt-1, _WAIT_RECV, NULL) ;
             Transport.mReceiveCallback=NULL ;
      if(cnt==SOCKET_ERROR) {                                       /* Если ошибка */
            sprintf(text, "Reply receive error - %d", WSAGetLastError()) ;
                                return(-1) ;
                            }

                    EMIR_log(buff, __rpc_path) ;

                     strncpy(text, buff, size-1) ;                  /* Выдача ответа на выход */

/*-------------------------------------------- Завершение соединения */

         status=Transport.ClientClose() ;
      if(status) {                                                  /* Если ошибка... */
                     sprintf(url, "Socket close error - %d", status) ;
                    EMIR_log(url) ;
                 }
/*-------------------------------------------------------------------*/

#undef  _BUFF_MAX

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                Функция управления приемом для HTTP                */

   int  EMIRi_Http_Receive_ctrl(char *data, int  data_size)

{
   char  work[8192] ;
   char *entry ;
   char *content ;
   char *end ;
    int  size ;
    int  cnt ;
    int  i ;

/*------------------------------------------ Выделение первой строки */

               memset(work, 0, sizeof(work)) ;                      /* Выделяем HTTP-заголовок */

        if(data_size<sizeof(work))  size=data_size ;
        else                        size=sizeof(work)-1 ;

              strncpy(work, data, size) ;

       content=strstr(work, "\r\n\r\n") ;                           /* Ищем вход контента */

           end=strchr(work, '\n') ;                                 /* Выделяем первую строку */
        if(end!=NULL)  *end=0 ;

/*---------------------------------------------------------- Если OK */

   if(strstr(work, " 200 OK")!=NULL) {

        if(end!=NULL)  *end='\n' ;                                  /* Восстанавливаем данные */
      
        if(content==NULL)  return(0) ;                              /* Если нет разделителя контента... */
 
          *content = 0 ;                                            /* Отсекаем заголовок */
           content+= 4 ;                                            /* Проходим на контент */

                 strupr(work) ;                                     /* Переводим заголовок в верхний регистр */
           entry=strstr(work, "CONTENT-LENGTH:") ;                  /* Ищем вход аттрибута длины данных */
/*- - - - - - - - - - - - - - - - Если тег CONTENT-LENGTH отсутсвует */
        if(entry==NULL) {                                           /* При отсутствии явного размера данных -        */
                                                                    /*  - определяем их завершение по json-контексту */
         if(memchr(data, '}', data_size)==NULL)  return(0) ;

          for(cnt=0, i=0 ; i<data_size ; i++)
            if(data[i]=='{')  cnt++ ;
            else 
            if(data[i]=='}')  cnt-- ;

                 if(cnt)  return(0) ;
                          return(1) ;          
                        }
/*- - - - - - - - - - - - - - - - - - - Если есть тег CONTENT-LENGTH */
                         entry+=strlen("CONTENT-LENGTH:") ;
            size=strtoul(entry, &end, 10) ;                         /* Извлекаем длину данных */

        if( (data_size-(content-work))>=size )  return(1) ;         /* Если все данные получены - выходим */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                     }
/*------------------------------------------------------ Если ошибка */

   else                              {

        if(content!=NULL)  return(1) ;                              /* Если есть разделитель контента... */

                                     }
/*-------------------------------------------------------------------*/

   return(0) ;

}


/*********************************************************************/
/*								     */
/*                  Перевод HEX в десятичное представление           */

  void EMIR_hex2dec(char *hex, char *dec)

#pragma warning(disable : 4018 4244)

{
  char  digits_10[512] ;
  char  digits_2[2048] ;
  char  power_10[512] ;
  char  value[2] ;
  char *end ;
   int  x_dgt ;
   int  shift ;
   int  tmp ;
   int  n ;
   int  i ;
   int  j ;


      memset(digits_10, 0, sizeof(digits_10)) ;
      memset(power_10,  0, sizeof(power_10)) ;
      memset(digits_2,  0, sizeof(digits_2)) ;
      memset(value,     0, sizeof(value)) ;

  for(n=0, i=strlen(hex)-1 ; i>=0 ; n+=4, i--) 
  {
     digits_2[n  ]='0' ;
     digits_2[n+1]='0' ;
     digits_2[n+2]='0' ;
     digits_2[n+3]='0' ;

    value[0]=hex[i] ;
       x_dgt=strtoul(value, &end, 16) ;        
    if(x_dgt>=8) {  digits_2[n+3]='1' ;  x_dgt-=8 ;  }
    if(x_dgt>=4) {  digits_2[n+2]='1' ;  x_dgt-=4 ;  }
    if(x_dgt>=2) {  digits_2[n+1]='1' ;  x_dgt-=2 ;  }
    if(x_dgt==1)    digits_2[n  ]='1' ;
  }   

      power_10[0]='1' ;

  for(n=0 ; n<strlen(digits_2) ; n++)
  {

     if(digits_2[n]=='1')
     {
       for(shift=0, i=0 ; i<strlen(power_10) ; i++)
       {
         if(digits_10[i]==0)  digits_10[i]='0' ;
                 
                                   tmp =(digits_10[i]-'0')+(power_10[i]-'0')+shift ;
                           digits_10[i]=tmp%10 ;
         if(digits_10[i]!=tmp)   shift = 1 ;
         else                    shift = 0 ;

                          digits_10[i]+='0' ;
       }

         if(shift==1)  digits_10[i]='1' ;
     }

       for(shift=0, i=0 ; i<strlen(power_10) ; i++)
       {
                                  tmp =(power_10[i]-'0')*2+shift ;
                           power_10[i]=tmp%10 ;
         if(power_10[i]!=tmp)   shift = 1 ;
         else                   shift = 0 ; 

                          power_10[i]+='0' ;
       }

         if(shift==1)  power_10[i]='1' ;
  }

    for(i=strlen(digits_10)-1, j=0 ; i>=0 ; i--, j++)  dec[j]=digits_10[i] ;
                                                       dec[j]= 0 ;
    
  return ;
}

#pragma warning(default  : 4018 4244)


/*********************************************************************/
/*								     */
/*                  Перевод строки в HEX-представление               */

  void EMIR_txt2hex(char *text, char *hex, int  size)

{
   char  tmp[32] ;
    int  i ;


    if(size>0)  memset(hex, 0, size+1) ;
    else        strcpy(hex, "") ;

   for( ; *text ; text++) {

       sprintf(tmp, "%02x", (unsigned int)(*(unsigned char *)(text))) ;
        strcat(hex, tmp) ;
                          }

    if(size>0) {
                  for(i=0 ; i<size ; i++)
                    if(hex[i]==0)  hex[i]='0' ;
               }
}


  void EMIR_bin2hex(char *bin, char *hex, int  size)

{
   char  tmp[32] ;
    int  i ;

   for(i=0 ; i<size ; bin++, i++) {

       sprintf(tmp, "%02x", (unsigned int)(*(unsigned char *)(bin))) ;

           hex[2*i  ]=tmp[0] ;
           hex[2*i+1]=tmp[1] ;
                                  }
}


  void EMIR_txt2hex64(char *text, char *hex, int  size)

{
    int  len ;
    int  i ;


            if(size   ==0)  len= 64           ;
       else if(size%32==0)  len= size      *2 ;
       else                 len=(size/32+1)*64 ;

                 memset(hex, 0, len+2) ;
           EMIR_bin2hex(text, hex, size) ;

   for(i=strlen(hex) ; i<len ; i++)  hex[  i]='0' ;
                                     hex[len]= 0 ;
}


  void EMIR_txt2hex128(char *text, char *hex, int  size)

{
  char text_1[64] ;
  char text_2[64] ;


                 memset(text_1, 0, sizeof(text_1)) ;
                 memset(text_2, 0, sizeof(text_2)) ;

                strncpy(text_1, text,    32) ;
   if(size>32)  strncpy(text_2, text+32, 32) ;

   EMIR_txt2hex64(text_1, hex,    strlen(text_1)) ;
   EMIR_txt2hex64(text_2, hex+64, strlen(text_2)) ;
}


/*********************************************************************/
/*								     */
/*                  Перевод HEX-представления в строку               */

  void EMIR_hex2txt(char *hex, char *text)

{
   char  tmp[32] ;
   char *end ;

#pragma warning(disable : 4244)

   for( ; *hex ; hex+=2, text++) {
                                    tmp[0]=hex[0] ;
                                    tmp[1]=hex[1] ;
                                    tmp[2]= 0 ;
                      *text=strtoul(tmp, &end, 16) ;
                                 }
   *text=0 ;

#pragma warning(default : 4244)
}


/********************************************************************/
/*								    */
/*                 Проверка состава символов на HEX-алфавит         */
/*								    */
/* Возвращает - номер плохого символа или -1                        */

   int  EMIR_check_hex(char *buff)

{
   int  i ;

     for(i=0 ; buff[i] ; i++) 
       if(!isxdigit(buff[i]))  return(i) ;

  return(-1) ;
}


/********************************************************************/
/*								    */
/*                        Расчет хэша файла                         */

   int  EMIR_file_hash(char *path, char *hash, char *error)

{
   char  command[1024] ;
   char  res_path[FILENAME_MAX] ;
   FILE *file ;
   char  buff[1024] ;
   char *end ;
    int  status ;

#ifdef UNIX
  static char *sys_util="sha1sum \"%s\" > %s" ;
#else
  static char *sys_util="CertUtil -hashfile \"%s\" SHA1 > %s" ;
#endif

/*------------------------------------- Проверка существования файла */

   if(access(path, 0x00)) {
                              sprintf(error, "Файл не найден") ;
                                      return(-1) ;
                          }
/*-------------------------------- Определение пути файла результата */

                                     res_path[0]=0 ;

   if(__work_folder[0]!=0) {
#ifdef UNIX
                             snprintf(res_path, sizeof(res_path)-1, "%s/", __work_folder) ;
#else
                             snprintf(res_path, sizeof(res_path)-1, "%s\\", __work_folder) ;
#endif
                           }

                               strcat(res_path, "sha.result") ;

/*------------------------------------- Исполнение системной утилиты */
            
            sprintf(command, sys_util, path, res_path) ;
      status=system(command) ;
   if(status) {
                 sprintf(error, "Hash calculation error (status=%d errno=%d)", status, errno) ;
                   return(-1) ;
              }
/*-------------------------------------------- Извлешение результата */

      file=fopen(res_path, "rt") ;
   if(file==NULL) {
                    sprintf(error, "Hash result file open error") ;
                      return(-1) ;
                  }

          memset(buff, 0, sizeof(buff)) ;
           fread(buff, sizeof(buff)-1, 1, file) ;
          fclose(file) ;
/*- - - - - - - - - - - - - - - - -  Извлечение результата для LINUX */
#ifdef UNIX

      end=strchr(buff, ' ') ;
   if(end==NULL) {
                    sprintf(error, "Hash result file processing error") ;
                      return(-1) ;
                 } 

     *end=0 ;

                  strcpy(hash, buff) ;
/*- - - - - - - - - - - - - - - -  Извлечение результата для Windows */
#else

      end=strchr(buff, '\n') ;
   if(end==NULL) {
                    sprintf(error, "Hash result file processing error") ;
                      return(-1) ;
                 } 

          strcpy(buff, end+1) ;

      end=strchr(buff, '\n') ;
   if(end==NULL) {
                    sprintf(error, "Hash result file processing error") ;
                      return(-1) ;
                 } 
     *end=0 ;

         EMIR_text_subst(buff, " ", "", 0) ;
                  strcpy(hash, buff) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#endif

                  unlink(res_path) ;

/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*                         Кодировка в UTF-8                         */
/*                         Кодировка из UTF-8                        */

#ifndef UNIX

  void  EMIR_toUTF8(char *data, char *work)

{
  int  cnt ;


      cnt=MultiByteToWideChar(CP_ACP,  0,         data, strlen(data), (LPWSTR)work, strlen(data)) ;
      cnt=WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)work,        cnt,           data, strlen(data)*2, NULL, NULL) ;

                data[cnt]=0 ;
}

  void  EMIR_fromUTF8(char *data, char *work)

{
   int  cnt ;


      cnt=MultiByteToWideChar(CP_UTF8, 0,         data, strlen(data), (LPWSTR)work, strlen(data)) ;
      cnt=WideCharToMultiByte(CP_ACP,  0, (LPWSTR)work,        cnt,           data, strlen(data), NULL, NULL) ;

                data[cnt]=0 ;
}

#endif


/*********************************************************************/
/*								     */
/*	                   Сравнение файлов                          */

  int  EMIR_compare_files(char *path_1, char *path_2)

{
  FILE *file_1 ;
  FILE *file_2 ;
   int  status ;
  char *buff_1 ;
  char *buff_2 ;
   int  size_1 ;
   int  size_2 ;

#define  _BUFF_MAX  64000

/*------------------------------------------------------- Подготовка */

          file_1=NULL ;
          file_2=NULL ;
          status=  1 ;

/*-------------------------------------------------- Открытие файлов */

    do {
            file_1=fopen(path_1, "rb") ;
            file_2=fopen(path_2, "rb") ;
         if(file_1==NULL ||
            file_2==NULL   )  break ;

            buff_1=(char *)calloc(1, _BUFF_MAX) ;
            buff_2=(char *)calloc(1, _BUFF_MAX) ;
         if(buff_1==NULL ||
            buff_2==NULL   )  break ;

/*------------------------------------------------- Сравнение файлов */

      do {
            size_1=fread(buff_1, 1, _BUFF_MAX, file_1) ;             
            size_2=fread(buff_2, 1, _BUFF_MAX, file_2) ;             

         if(size_1!=size_2)  break ;

         if(size_1!=0) 
          if(memcmp(buff_1, buff_2, size_1))  break ;

         if(size_1<_BUFF_MAX) { 
                                 status=0 ;
                                   break ;
                              }

         } while(1) ;      

/*-------------------------------------------------- Закрытие файлов */

       } while(0) ;

    if(file_1!=NULL)  fclose(file_1) ;
    if(file_2!=NULL)  fclose(file_2) ;

    if(buff_1!=NULL)  free(buff_1) ;
    if(buff_2!=NULL)  free(buff_2) ;

#undef  _BUFF_MAX

  return(status) ;     
}


/*********************************************************************/
/*								     */
/*	                  Запрос баланса счёта                       */

   int  EMIR_node_getbalance(char *account, char *block, char *balance, char *error)

{
   int  status ;
  char  buff[2048] ;  
  char *result ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBalance\",\"params\":[\"0x%s\",\"%s\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, account, block) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                           return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                 /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":\"0x") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

                     strcpy(balance, result) ;

/*-------------------------------------------------------------------*/

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*	               Запрос кода программы контракта               */

   int  EMIR_node_getcode(char *txn_id, char *source, int  source_max, char *error)

{
   int  status ;
  char *buff ;  
  char *result ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionByHash\",\"params\":[\"0x%s\"],\"id\":1}" ;

#define  _BUFF_SIZE  64000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

                     buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, txn_id) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                            free(buff) ;
                           return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"input\":\"") ;                  /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"input\":\"0x") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

          strncpy(source, result, source_max) ;

/*-------------------------------------------------------------------*/

       free(buff) ;

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*         Запрос стоимости исполнения программы контракта           */

   int  EMIR_node_checkgas(char *account, char *contract, char *source, char *gas, char *error)

{
   int  status ;
  char *buff ;
  char *result ;
  char *end ;

 static char *request1="{\"jsonrpc\":\"2.0\",\"method\":\"eth_estimateGas\",\""
                       "params\":[{ \"from\":\"0x%s\", \"data\":\"0x%s\" }],"
                       "\"id\":1}" ;

 static char *request2="{\"jsonrpc\":\"2.0\",\"method\":\"eth_estimateGas\",\""
                       "params\":[{ \"from\":\"0x%s\", \"to\":\"0x%s\", \"data\":\"0x%s\" }],"
                       "\"id\":1}" ;

#define  _BUFF_SIZE  64000

/*----------------------------------------- Подстановка без рассчета */

   if(__gas_value[0]!=0) {
                           sprintf(gas, "0x%s", __gas_value) ;
                            return(0) ;
                         } 
/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

              buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

     if(contract==NULL)  sprintf(buff, request1, account, source) ;
     else                sprintf(buff, request2, account, contract, source) ;

        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                    free(buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                 /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":\"") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

                     strcpy(gas, result) ;

/*-------------------------------------------------------------------*/

       free(buff) ;

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                      Создание контракта                           */

   int  EMIR_node_publcontract(char *account, char *source, char *gas, char *txn, char *error)

{
  char *buff ;
   int  status ;
  char *result ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
                      "\"params\":[{ \"from\":\"0x%s\", \"gas\":\"%s\", \"data\":\"0x%s\" }],"
                      "\"id\":1}" ;

#define  _BUFF_SIZE  64000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

                       buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, account, gas, source) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                    free(buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":\"0x") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

                     strcpy(txn, result) ;

/*-------------------------------------------------------------------*/

       free(buff) ;

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                       Исполнение метода контракта                 */

   int  EMIR_node_sendcontract(char *account, char *contract, char *source, char *gas, char *txn, char *error)

{
  char *buff ;
   int  status ;
  char *result ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
                      "\"params\":[{ \"from\":\"0x%s\", \"to\":\"0x%s\", \"gas\":\"%s\", \"data\":\"0x%s\" }],"
                      "\"id\":1}" ;

#define  _BUFF_SIZE  64000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

                       buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, account, contract, gas, source) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                    free(buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":\"0x") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

                     strcpy(txn, result) ;

/*-------------------------------------------------------------------*/

       free(buff) ;

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                      Проверка транзакции                          */

   int  EMIR_node_checktxn(char *txn, char *block, char *contract, char *error)

{
  char *buff ;
   int  status ;
  char *result ;
  char *value1 ;
  char *value2 ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionReceipt\",\""
                      "params\":[\"0x%s\"],\"id\":1}\"" ;

#define  _BUFF_SIZE  64000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

                       buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, txn) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                    free(buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "%-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":") ;

                    *block=0 ;
                 *contract=0 ;

          if(!memicmp(result, "null", 4)) {
                                            free(buff) ;
                                             return(0) ;
                                          }

          value1=strstr(result, "\"blockNumber\":\"") ;
          value2=strstr(result, "\"contractAddress\":\"") ;

       if(value1!=NULL) {
                               value1+=strlen("\"blockNumber\":\"") ;
                                  end =strchr(value1, '"') ;
                                 *end =0 ;

                                      memmove(block, value1, strlen(value1)+1) ;
                        }
       if(value2!=NULL) {
                               value2+=strlen("\"contractAddress\":\"") ;
                                  end =strchr(value2, '"') ;
                                 *end =0 ;

                                      memmove(contract, value2, strlen(value2)+1) ;
                        }
/*-------------------------------------------------------------------*/

       free(buff) ;

#undef  _BUFF_SIZE

   return(1) ;
}


/*********************************************************************/
/*								     */
/*                        Разблокировка аккаунта                     */
/*                                                                   */
/*   Если data="FORCE", то разблокировка аккаунта производится вне   */
/*    зависимости от истечения срока разблокировки                   */


   int  EMIR_node_unlockaccount(char *account, char *password, char *data, int  data_max, char *error)

{
           char *buff ;
            int  force_unlock ; 
            int  status ;
           char *result ;
           char  text[1024] ;
           char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"personal_unlockAccount\",\"params\":[\"0x%s\", \"%s\", %d],\"id\":1}" ;

#define  _UNLOCK_TIME    600
#define    _BUFF_SIZE  64000

  typedef struct {    char  account[128] ;
                    time_t  last_unlock ;   }  Account_unlock ;

  static Account_unlock *accounts ;
  static            int  accounts_cnt ; 
                    int  account_idx ; 

/*------------------------------------- Принудительная разблокировка */

                                 force_unlock=0 ;
    if(!stricmp(data, "FORCE"))  force_unlock=1 ;

/*------------------------------------- Проверка паузы разблокировки */

   for(account_idx=0 ; account_idx<accounts_cnt ; account_idx++)
     if(!stricmp(accounts[account_idx].account, account)) {

      if(force_unlock==0)
       if(accounts[account_idx].last_unlock+_UNLOCK_TIME-2>time(NULL))  return(0) ;

                                             break ;
                                                          }
     if(account_idx>=accounts_cnt) {

         accounts    =(Account_unlock *)realloc(accounts, (accounts_cnt+1)*sizeof(*accounts)) ;                
         accounts_cnt++ ;
          account_idx=accounts_cnt-1 ;

         strcpy(accounts[account_idx].account, account) ;
                                   }

                accounts[account_idx].last_unlock=0 ;

  if(force_unlock)  sprintf(text, "Forced unlock account %s", account) ;
  else              sprintf(text, "Unlock account %s", account) ;
                   EMIR_log(text) ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

                       buff=(char *)calloc(1, _BUFF_SIZE) ;

/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, request, account, password, _UNLOCK_TIME) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                    free(buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "TRANSPORT - %-128.128s", result) ;
                                  free(buff) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

          strncpy(data, result, data_max) ;

/*-------------------------------------------------------------------*/

       free(buff) ;

              accounts[account_idx].last_unlock=time(NULL) ;

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                Запрос номера последнего блока                     */

   int  EMIR_node_lastblock(char *block_num, char *error)

{
   int  status ;
  char  buff[2048] ;
  char *result ;
  char *end ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\""
                       "params\":[],"
                       "\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------------------- Отправка запроса */

                           strcpy(  buff, request) ;
        status=EMIR_node_exchange(__node_url, buff, sizeof(buff)-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "ERROR - %-128.128s", result) ;
                                 return(-1) ;
                           }

             result+=strlen("\"result\":\"") ;
                end =strchr(result, '"') ;
             if(end!=NULL)  *end=0 ;

                     strcpy(block_num, result) ;

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                Запрос заголовка данных блока по номеру            */

   int  EMIR_node_getblockh(char *block_num, struct Sn_block *header,
                                            struct Sn_record *records, 
                                                         int  records_max,
                                                         int  records_from,  char *error)

{
   int  status ;
  char  hash[128] ;
  char *result ;
  char *rec ;
  char *value ;
  char *end ;
   int  cnt ;
   int  offset ;

 static char *b_request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBlockByNumber\",\""
                         "params\":[\"0x%s\", false],"
                         "\"id\":1}" ;
 static char *t_request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionByHash\",\""
                         "params\":[\"0x%s\"],"
                         "\"id\":1}" ;

  static char *buff ;
  static char *record ;

#define  _BUFF_SIZE  512000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL) {
                        buff=(char *)calloc(1, _BUFF_SIZE) ;
                      record=(char *)calloc(1, _BUFF_SIZE) ;
                   }
/*------------------------------------------------- Отправка запроса */

                          sprintf(buff, b_request, block_num) ;
        status=EMIR_node_exchange(__node_url, buff, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "ERROR - %-128.128s", result) ;
                                 return(-1) ;
                           }
/*------------------------------------------ Разбор параметров блока */

#define  _KEY   "\"hash\":\"0x"

        value=strstr(result, _KEY) ;
     if(value==NULL)  return(-1) ;

                 strncpy(header->hash, value+strlen(_KEY), sizeof(header->hash)-1) ;
              end=strchr(header->hash, '"') ;
             *end= 0 ;
         
#undef   _KEY

#define  _KEY   "\"timestamp\":\"0x"

        value=strstr(result, _KEY) ;
     if(value==NULL)  return(-1) ;

                 strncpy(header->timestamp, value+strlen(_KEY), sizeof(header->timestamp)-1) ;
              end=strchr(header->timestamp, '"') ;
             *end= 0 ;
         
#undef   _KEY

     if(records==NULL)  return(0) ;                                 /* Если транзакции разбирать не надо */

/*------------------------------------------ Разбор блока транзакций */

        rec =strstr(result, "\"transactions\":[") ;
     if(rec==NULL)  return(-1) ;

        end=strchr(rec, ']') ;
     if(end==NULL) {
                       sprintf(error, "ERROR - Bad block structure") ;
                                 return(-1) ;
                   }
       *end= 0 ;

           cnt=0 ;
        offset=0 ;

   do {
/*- - - - - - - - - - - - - - - - - - - - -  Выделяем хэш транзакции */
           rec=strstr(rec, "\"0x") ;
        if(rec==NULL)  break ;
           rec+=strlen("\"0x") ;

           strncpy(hash, rec, 64) ;
                   hash[64]=0 ;
/*- - - - - - - - - - - - - - - - - - - - - Запрос данных транзакции */
                          sprintf(record, t_request, hash) ;
        status=EMIR_node_exchange(__node_url, record, _BUFF_SIZE-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                   return(-1) ;
                   }
/*- - - - - - - - - - - - - - - - - - - - - Разбор данных транзакции */
                memset(&records[cnt], 0, sizeof(*records)) ;

#define  _KEY   "\"hash\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(records[cnt].hash, value+strlen(_KEY), sizeof(records[cnt].hash)-1) ;
              end=strchr(records[cnt].hash, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"from\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(records[cnt].from, value+strlen(_KEY), sizeof(records[cnt].from)-1) ;
              end=strchr(records[cnt].from, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"to\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(records[cnt].to, value+strlen(_KEY), sizeof(records[cnt].to)-1) ;
              end=strchr(records[cnt].to, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"value\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(records[cnt].value, value+strlen(_KEY), sizeof(records[cnt].value)-1) ;
              end=strchr(records[cnt].value, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

     if(offset>=records_from) {
            cnt++ ;
         if(cnt==records_max)  break ;
                              }

       offset++ ;

      } while(1) ;

/*-------------------------------------------------------------------*/

   return(cnt) ;
}


/*********************************************************************/
/*								     */
/*                    Запрос событий блока по номеру                 */

   int  EMIR_node_getevents(char *block_num, struct Sn_event *events, 
                                                         int  events_max,
                                                         int  events_from,  char *error)

{
   int  status ;
  char *result ;
  char *rec ;
  char *value ;
  char *end ;
   int  cnt ;
   int  offset ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\""
                       "params\":[{\"fromBlock\":\"0x%s\", \"toBlock\":\"0x%s\"}],"
                       "\"id\":1}" ;

  static char *buff ;
  static  int  buff_size ;
  static char *record ;

#undef   _BUFF_SIZE  
#define  _BUFF_SIZE  512000

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла") ;
                                   return(-1) ;
                        }
/*----------------------------------------------- Выделение ресурсов */

    if(buff==NULL) {
                        buff_size=_BUFF_SIZE ;
                        buff     =(char *)calloc(1, buff_size) ;
                      record     =(char *)calloc(1, buff_size) ;
                   }
/*------------------------------------------------- Отправка запроса */

  do {
                          sprintf(buff, request, block_num, block_num) ;
        status=EMIR_node_exchange(__node_url, buff, buff_size-1) ;
     if(status!=0) {
                       sprintf(error, "TRANSPORT - %-128.128s", buff) ;
                                   return(-1) ;
                   }
/*------------------------------------------------ Разбор результата */

             result=strstr(buff, "\"result\":") ;                   /* Проверка наличия результата */
          if(result==NULL) {
                                      result=strchr(buff, '{') ;
                    if(result==NULL)  result=       buff ;

                       sprintf(error, "ERROR - %-128.128s", result) ;
                                 return(-1) ;
                           }

             end=strstr(result, "]}") ;
          if(end==NULL) {
//                                sprintf(record, "%d.snap", buff_size) ;
//                          EMIR_snapshot(buff, record) ;

                                   free(buff  ) ;
                                   free(record) ;

               if(buff_size>_BUFF_SIZE*64) {
                       sprintf(error, "ERROR - Invalid data structure") ;
                                   return(-1) ;
                                           }

                        buff_size*=2 ;
                        buff      =(char *)calloc(1, buff_size) ;
                      record      =(char *)calloc(1, buff_size) ;

                  EMIR_log("EMIR_node_getevents - Buffer expanded") ;

                            continue ;
                        }

           break ;

     } while(1) ;
/*- - - - - - - - - - - - - - - - - - - - - - - Разбор блока событий */
             rec=result ;
             cnt=    0 ;
          offset=    0 ;

   do {
            rec=strchr(rec+1, '{') ;
         if(rec==NULL)  break ;

                memset(record, 0, buff_size) ;
               strncpy(record, rec+1, buff_size-1) ;
            end=strchr(record, '}') ;
         if(end!=NULL)  *end= 0 ;

                memset(&events[cnt], 0, sizeof(*events)) ;

#define  _KEY   "\"address\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(events[cnt].address, value+strlen(_KEY), sizeof(events[cnt].address)-1) ;
              end=strchr(events[cnt].address, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"topics\":[\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(events[cnt].topic, value+strlen(_KEY), sizeof(events[cnt].topic)-1) ;
              end=strchr(events[cnt].topic, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"data\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(events[cnt].data, value+strlen(_KEY), sizeof(events[cnt].data)-1) ;
              end=strchr(events[cnt].data, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

#define  _KEY   "\"transactionHash\":\"0x"
            value=strstr(record, _KEY) ;
         if(value!=NULL) {
                 strncpy(events[cnt].transaction, value+strlen(_KEY), sizeof(events[cnt].transaction)-1) ;
              end=strchr(events[cnt].transaction, '"') ;
           if(end!=NULL)  *end= 0 ;
                         }
#undef   _KEY

     if(offset>=events_from) {
            cnt++ ;
         if(cnt==events_max)  break ;
                             }

       offset++ ;

      } while(1) ;


/*-------------------------------------------------------------------*/

   return(cnt) ;
}


/*********************************************************************/
/*								     */
/*                   Запрос версии смарт-контракта                   */

   int  EMIR_node_getversion(char *contract, char *version, char *error)

{
   int  status ;
  char  buff[4096] ;
  char  text[4096] ;
  char *result ;

 static char *request="{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"to\":\"0x%s\", \"data\":\"0x0d55e9f5\"},\"latest\"],\"id\":1}" ;

/*---------------------------------------- Проверка задания URL узла */

   if(__node_url[0]==0) {
                           strcpy(error, "Не задан URL узла") ;
                                   return(-1) ;
                        }
/*------------------------------------- Отправка запроса SD_Identify */

                          sprintf(buff, request, contract) ;
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

              memset(text, 0, sizeof(text)) ;                       /* Распаковка данных */
              memcpy(text, result+0*64, 64) ;
        EMIR_hex2txt(text, text) ;

              strcpy(version, text) ;

#undef   _RESULT_PREFIX

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                      Загрузка файла в DFS                         */
/*                      Получение файла из DFS                       */

   int  EMIR_dfs_putfile(char *path, char *link, char *error) 

{
        char  reply[128] ;
         int  status ;
         int  direct_flag ;
 struct stat  file_attr ;

/*------------------------ Определение отправки файла через блокчейн */

    if(__dfs_direct_max>0) {                                        /* Определяем размер файла */

              status=stat(path, &file_attr) ;
           if(status) {
                         sprintf(error, "File open error %d : %s", errno, path) ;
                                            return(-1) ;
                      }
                           }

                                           direct_flag=0 ;

    if(         __dfs_direct_max<=0  ) {
                                           direct_flag=0 ;
                                       }
    else
    if(!stricmp(__dfs_type, "DIRECT")) {
                                           direct_flag=1 ;

           if(file_attr.st_size>__dfs_direct_max) {
                         sprintf(error, "File too big for DIRECT transfer") ;
                                                        return(-1) ;
                                                  }

                                       }
    else                               {

           if(file_attr.st_size<=__dfs_direct_max)  direct_flag=1 ;

                                       }
/*--------------------------------------------------- Отправка файла */
   
    if( direct_flag                    ) {

         status=EMIR_direct_putfile(path, reply, error) ;
      if(status)  return(status) ;

          sprintf(link, "DIRECT:%s", reply) ;

                                         }
    else
    if(!stricmp(__dfs_type, "SWARM"   )) {

         status=EMIR_swarm_putfile(path, reply, error) ;
      if(status)  return(status) ;

          sprintf(link, "%s", reply) ;

                                         }
    else
    if(!stricmp(__dfs_type, "MS_SHARE")) {

         status=EMIR_share_putfile(path, reply, error) ;
      if(status)  return(status) ;

          sprintf(link, "MS_SHARE:%s", reply) ;

                                         }
    else
    if(!stricmp(__dfs_type, "IPFS"    )) {

         status=EMIR_ipfs_putfile(path, reply, error) ;
      if(status)  return(status) ;

          sprintf(link, "IPFS:%s", reply) ;

                                         }
    else                                 {

                     sprintf(error, "Unknown DFS type: %s", __dfs_type) ;
                                            return(-1) ;

                                         } 
/*-------------------------------------------------------------------*/

   return(0) ;
}


   int  EMIR_dfs_getfile(char *link, char *path, char *error)
{
   int  status ;


    if(!memicmp(link, "DIRECT:",   7)) {

          status=EMIR_direct_getfile(link+7, path, error) ;
       if(status!=0)  return(status) ;
                                       }
    else
    if(!memicmp(link, "MS_SHARE:", 9)) {

          status=EMIR_share_getfile(link+9, path, error) ;
       if(status!=0)  return(status) ;
                                       }
    else
    if(!memicmp(link, "IPFS:",     5)) {

          status=EMIR_ipfs_getfile(link+5, path, error) ;
       if(status!=0)  return(status) ;
                                       }
    else
    if(  strlen(link)==64            ) {

          status=EMIR_swarm_getfile(link, path, error) ;
       if(status!=0)  return(status) ;
                                       }
    else                               {

                     sprintf(error, "Unknown DFS type: %s", link) ;
                                  return(-1) ;

                                       }

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*           Загрузка файла в файловое хранилище SWARM               */

   int  EMIR_swarm_putfile(char *path, char *link, char *error)

{
           int  status ;
          char  result_path[FILENAME_MAX] ;
          char  command[1024] ;
          char  text[8000] ;
          FILE *file ;

  static  char *buff ;
#define        _BUFF_MAX   64000

  static  char *curl_put="%s --header \"Content-Type: application/octet-stream\" -X POST --data-binary \"@%s\" %s/bzz:/ > %s" ;

/*---------------------------------------------------- Инициализация */

     if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*---------------------------------------- Проверка задания URL узла */

   if(__swarm_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла SWARM") ;
                                   return(-1) ;
                         }
/*----------------------- Проверка существования и доступности файла */

   if(access(path, 0x04)) {
             sprintf(error, "ERROR - Файл не существует или не доступен: %s", path) ;
                                   return(-1) ;
                          }
/*----------------------------------------------- Подготовка запроса */

#ifdef  UNIX
        snprintf(result_path, sizeof(result_path)-1, "%s/swarm.result", __work_folder) ;     /* Определяем путь файла результата */  
#else
        snprintf(result_path, sizeof(result_path)-1, "%s\\swarm.result", __work_folder) ;
#endif

        sprintf(command, curl_put,__curl_path, path, __swarm_url, result_path) ;
       EMIR_log(command, __log_path) ;

/*----------------------------------------------- Исполнение запроса */

             unlink(result_path) ;                                  /* Удаляем файл результата */

#ifdef  UNIX
              errno=0 ;
#else
         _set_errno(0) ;
#endif

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(text, "Ошибка исполнения Curl (status=%d errno=%d) : %s", status, errno, command) ;
             EMIR_log(text, __rpc_path) ;
              sprintf(error, "TRANSPORT - %-128.128s", text) ;
                                return(-1) ;                           
                             }
/*- - - - - - - - - - - - - - - - - - - - - - - - - -  Анализ ответа */
            file=fopen(result_path, "rb") ;
         if(file==NULL) {
                            sprintf(text, "Ошибка открытия файла ответа %d : %s", errno, result_path) ;
                           EMIR_log(text, __rpc_path) ;
                            sprintf(error, "TRANSPORT - %-128.128s", text) ;
                              return(-1) ;                           
                        }

            memset(buff, 0, _BUFF_MAX) ;
             fread(buff, _BUFF_MAX-1, 1, file) ;
            fclose(file) ;
       EMIR_log(buff, __log_path) ;

            strcpy(link, buff) ;

       EMIR_log(link, __log_path) ;


/*-------------------------------------------------------------------*/

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*                                                                   */
/*           Получение файла из файлового хранилища SWARM            */
/*                                                                   */
/*   Return:  0  -  файл получен                                     */
/*           -1  -  ошибка получения файла                           */
/*            1  -  возможно, файл еще не прошел по сети             */

   int  EMIR_swarm_getfile(char *link, char *path, char *error)

{
        char  result_path[FILENAME_MAX] ;
        FILE *file ;
        char  command[2048] ;
        char  text[2048] ;
        char *mark ;
         int  timeout ;
         int  status ;
        char *work ;

 static char *curl_get="%s --max-time %d -X GET %s/bzz:/%s/ 1>\"%s\" 2>\"%s\" " ;

 static char *buff ;

#define  _BUFF_SIZE  64000

/*---------------------------------------------------- Инициализация */

   if(buff==NULL)  buff=(char *)calloc(1, _BUFF_SIZE) ;

/*---------------------------------------- Проверка задания URL узла */

   if(__swarm_url[0]==0) {
                           strcpy(error, "ERROR - Не задан URL узла SWARM") ;
                                   return(-1) ;
                         }
/*----------------------------- Формирование команды получения файла */

#ifdef  UNIX
        snprintf(result_path, sizeof(result_path)-1, "%s/swarm.result", __work_folder) ;    /* Определяем путь файла результата */  
#else
        snprintf(result_path, sizeof(result_path)-1, "%s\\swarm.result", __work_folder) ;
#endif

         unlink(result_path) ;                                      /* Удаляем файл результата */

                       timeout=2 ;

   do {                                                             /* Цикл исполнения запроса */ 

        sprintf(command, curl_get,                                  /* Формируем команду получения файла */
                            __curl_path, timeout, __swarm_url, link, path, result_path) ;
       EMIR_log(command, __log_path) ;

/*----------------------------------------------- Исполнение запроса */

#ifdef  UNIX
              errno=0 ;
#else
         _set_errno(0) ;
#endif

      status=system(command) ;
   if(status!=  0   ||
      errno ==ENOENT  ) {
                sprintf(error, "TRANSPORT - Ошибка исполнения curl (status=%d errno=%d) : %s", status, errno, command) ;
               EMIR_log(error, __rpc_path) ;
                            return(-1) ;                           
                        }
/*---------------------------------------------------- Анализ ответа */

            file=fopen(result_path, "rb") ;
         if(file==NULL) {
                            sprintf(error, "TRANSPORT - Ошибка открытия файла результата %d : %s", errno, result_path) ;
                           EMIR_log(error, __rpc_path) ;
                              return(-1) ;                           
                        }

              memset(buff, 0, _BUFF_SIZE) ;
               fread(buff, 1, _BUFF_SIZE-1, file) ;
              fclose(file) ;

         work=strstr(buff, "Error") ;
      if(work!=NULL) {

        if(       text!=work                              &&       /* Если файл есть, но превышено время докачки -  */
           strstr(text, "context deadline exceeded")!=NULL  ) {    /*  - увеличиваем время закачки в 2 раза         */

                                   timeout*=2 ;
                                    continue ;
                                                              }
        else                                                  {

                             sprintf(error, "TRANSPORT - ошибка закачки файла %200s", work) ;
                            EMIR_log(error, __rpc_path) ;
                                return(-1) ;
                                                              }
                     }

                          break ;

      } while(1) ;                                                  /* Цикл исполнения запроса */ 

//          unlink(result_path) ;                                   /*  Удаляем файл результата */

/*---------------------------------------- Проверка файла результата */

        file=fopen(path, "rb") ;
     if(file==NULL) {
                       sprintf(error, "ERROR - result file open error %d : %s", errno, path) ;
                                       return(-1) ;
                    }

            memset(buff, 0, _BUFF_SIZE) ;
             fread(buff, 1, _BUFF_SIZE-1, file) ;
            fclose(file) ;

             strupr(buff) ;
             strupr(link) ;
        mark=strstr(buff, link) ;                                   /* Если файл содержит свой собственный манифест - это ошибка */
     if(mark!=NULL)  return(1) ;

     if(strstr(buff, "Code: 404"         )!=NULL &&                 /* Если файл содержит в качестве результата сообщение об ошибке */
        strstr(buff, "Message: Not Found")!=NULL   )  return(1) ;

/*-------------------------------------------------------------------*/

#undef  _BUFF_SIZE

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           Загрузка файла в файловое хранилище MS Share            */

   int  EMIR_share_putfile(char *path, char *link, char *error)

{
                time_t  time_abs ;
             struct tm *hhmmss ;
                  char  path_dfs[FILENAME_MAX] ;
                  char  folder[FILENAME_MAX] ;
                  char  text[1024] ;
                   int  status ;

#ifdef  UNIX
#else
                  char  name[FILENAME_MAX] ;
   SECURITY_ATTRIBUTES  attr ;
                  GUID  guid ;
#endif

/*---------------------------------------- Проверка задания URL узла */

   if(__dfs_url[0]==0) {
                           strcpy(error, "TRANSPORT - Не задан URL узла DFS") ;
                         EMIR_log(error, __rpc_path) ;
                                   return(-1) ;
                       }
/*------------------------------------------------- Соединение с DFS */

       status=EMIRi_share_connect(__dfs_url, text) ;
    if(status) {
                          sprintf(error, "TRANSPORT - ошибка соединения с URL: %s", text) ;
                         EMIR_log(error, __rpc_path) ;
                                   return(-1) ;
               }
/*------------------------------------- Формирование папки хранилища */

               time_abs=     time( NULL) ;
                 hhmmss=localtime(&time_abs) ;

     sprintf(folder, "%02d-%02d-%02d.%02d",
                                 hhmmss->tm_mday,
                                 hhmmss->tm_mon+1,
                                 hhmmss->tm_year-100,
                                 hhmmss->tm_hour     ) ;

                snprintf(path_dfs, sizeof(path_dfs)-1, "%s\\%s", __dfs_url, folder) ;

#ifdef  UNIX
#else

   if(GetFileAttributes(path_dfs)==INVALID_FILE_ATTRIBUTES) {

/*
                sprintf(command, "mkdir %s > c:\\logs\\system.result 2>&1", path_dfs) ;
          status=system(command) ;
       if(status!=0 || errno!=0) {
             sprintf(error, "TRANSPORT - ошибка создания папки status=%d, errno=%d: %s", status, errno, path_dfs) ;
            EMIR_log(error, __rpc_path) ;
                       return(-1) ;
                                 }
*/

               memset(&attr, 0, sizeof(attr)) ;
                       attr.nLength=sizeof(attr) ;

         status=CreateDirectory(path_dfs, &attr) ;
      if(status==0) {
             sprintf(error, "TRANSPORT - ошибка создания папки %d: %s", GetLastError(), path_dfs) ;
            EMIR_log(error, __rpc_path) ;
                       return(-1) ;
                    }
                                                           }
#endif

/*--------------------------------------------------- Отправка файла */

#ifdef  UNIX
#else

             CoCreateGuid(&guid);

     sprintf(name, "%08lx%04hx%04hx%02x%02x%02x%02x%02x%02x%02x%02x",
                              guid.Data1, guid.Data2, guid.Data3,
                                        (unsigned int)guid.Data4[0],
                                        (unsigned int)guid.Data4[1],
                                        (unsigned int)guid.Data4[2],
                                        (unsigned int)guid.Data4[3],
                                        (unsigned int)guid.Data4[4],
                                        (unsigned int)guid.Data4[5],
                                        (unsigned int)guid.Data4[6],
                                        (unsigned int)guid.Data4[7] ) ;

              sprintf(path_dfs, "%s\\%s\\%s", __dfs_url, folder, name) ;
      status=CopyFile(path, path_dfs, false) ;
   if(status==0) {
             sprintf(error, "TRANSPORT - ошибка копирования файла %d: %s ", GetLastError(), path_dfs) ;
            EMIR_log(error, __rpc_path) ;
                      return(-1) ;
                 }

     sprintf(link, "%s\\%s", folder, name) ;

#endif

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           Получение файла из файлового хранилища MS Share         */
/*								     */
/*   Return:  0  -  файл получен                                     */
/*           -1  -  ошибка получения файла                           */
/*            1  -  возможно, файл еще не прошел по сети             */

   int  EMIR_share_getfile(char *link, char *path, char *error)

{
       char  path_dfs[FILENAME_MAX] ;
       char  text[1024] ;
        int  status ;

/*---------------------------------------- Проверка задания URL узла */

   if(__dfs_url[0]==0) {
                           strcpy(error, "TRANSPORT - Не задан URL узла DFS") ;
                         EMIR_log(error, __rpc_path) ;
                                   return(-1) ;
                       }
/*------------------------------------------------- Соединение с DFS */

       status=EMIRi_share_connect(__dfs_url, text) ;
    if(status) {
                          sprintf(error, "TRANSPORT - ошибка соединения с URL: %s", text) ;
                         EMIR_log(error, __rpc_path) ;
                                   return(-1) ;
               }
/*-------------------------------------------------- Получение файла */

              sprintf(path_dfs, "%s\\%s", __dfs_url, link) ;

#ifdef  UNIX
#else

      status=CopyFile(path_dfs, path, false) ;
   if(status==0) {
             sprintf(error, "TRANSPORT - ошибка копирования файла %d: %s ", GetLastError(), path_dfs) ;
            EMIR_log(error, __rpc_path) ;
                                   return(-1) ;
                 }    
#endif

/*--------------------------------------------------- Отправка файла */

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           Загрузка файла в файловое хранилище IPFS                */

   int  EMIR_ipfs_putfile(char *path, char *link, char *error)

{
       char  result_path[FILENAME_MAX] ;
       FILE *file ;
       char  command[2048] ;
        int  status ;
       char *work ;
       char *end ;

  static  char *buff ;

  static  char *ipfs_put="ipfs add \"%s\" 1>%s 2>&1" ;
  static  char *curl_put="%s -F \"file=@%s\" \"http://%s/api/v0/add\" 1>%s 2>&1" ;

#define _BUFF_MAX   64000

/*---------------------------------------------------- Инициализация */

     if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*------------------------------------ Формирование команды загрузки */

#ifdef  UNIX
        snprintf(result_path, sizeof(result_path)-1, "%s/ipfs.result", __work_folder) ;     /* Определяем путь файла результата */  
#else
        snprintf(result_path, sizeof(result_path)-1, "%s\\ipfs.result", __work_folder) ;
#endif

         unlink(result_path) ;                                      /* Удаляем файл результата */

   if(__ipfs_url[0]==0)                                             /* Формируем команду загрузки */
          sprintf(command, ipfs_put, path, result_path) ;           
   else   sprintf(command, curl_put, __curl_path, path, __ipfs_url, result_path) ;

         EMIR_log(command, __rpc_path) ;

/*----------------------------------------------- Исполнение запроса */

#ifdef  UNIX
              errno=0 ;
#else
         _set_errno(0) ;
#endif

      status=system(command) ;
   if((status!=0 && 
       status!=1   ) || errno!=0) {
                sprintf(error, "TRANSPORT - Ошибка исполнения ipfs (status=%d errno=%d) : %s", status, errno, command) ;
               EMIR_log(error, __rpc_path) ;
                            return(-1) ;                           
                                  }
/*---------------------------------------------------- Анализ ответа */
/*- - - - - - - - - - - - - - - - - - - -  Считываем файл результата */
            file=fopen(result_path, "rb") ;
         if(file==NULL) {
                            sprintf(error, "TRANSPORT - Ошибка открытия файла результата %d : %s", errno, result_path) ;
                           EMIR_log(error, __rpc_path) ;
                              return(-1) ;                           
                        }

                memset(buff, 0, _BUFF_MAX) ;
                 fread(buff, 1, _BUFF_MAX-1, file) ;
                fclose(file) ;
/*- - - - - - - - - - - - - - - - - - - При работе через IPFS-агента */
    if(__ipfs_url[0]==0) {
    
            work=strstr(buff, "Error") ;
         if(work!=NULL) {
                             sprintf(error, "TRANSPORT - ошибка закачки файла %200s", work) ;
                            EMIR_log(error, __rpc_path) ;
                                return(-1) ;
                        }

            work=strstr(buff, "added ") ;
         if(work!=NULL) {
                                 memset(link, 0, 128) ;
                                strncpy(link, work+strlen("added "), 127) ;
                            end =strchr(link, ' ') ;
             if(end!=NULL) *end = 0 ;
                        }
         else           {
                             sprintf(error, "TRANSPORT - некорректная структура файла результата") ;                   
                        }

         if(error[0]!=0) {
                             EMIR_log(error, __rpc_path) ;
                                 return(-1) ;
                         }    

                         } 
/*- - - - - - - - - - - - - - - - - - - - - - - При работе через API */
    else                 {

            work=strstr(buff, "\"Hash\":\"") ;
         if(work!=NULL) {
                                 memset(link, 0, 128) ;
                                strncpy(link, work+strlen("\"Hash\":\""), 127) ;
                            end =strchr(link, '"') ;
             if(end!=NULL) *end = 0 ;

                        }
         else           { 

              work=strstr(buff, "curl: ") ;
           if(work!=NULL) work+=strlen("curl: ") ;
           else           work =buff ;

              end=strchr(work, '\r') ;
           if(end!=NULL)  *end=0 ;
              end=strchr(work, '\n') ;
           if(end!=NULL)  *end=0 ;

                 sprintf(error, "TRANSPORT - ошибка закачки файла %200s", work) ;
                EMIR_log(error, __rpc_path) ;
                           return(-1) ;
                        }
                                           
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
            unlink(result_path) ;                                   /* Удаляем файл результата */

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*           Получение файла из файлового хранилища IPFS             */
/*								     */
/*   Return:  0  -  файл получен                                     */
/*           -1  -  ошибка получения файла                           */
/*            1  -  возможно, файл еще не прошел по сети             */

   int  EMIR_ipfs_getfile(char *link, char *path, char *error)

{
          char  result_path[FILENAME_MAX] ;
          FILE *file ;
          char  command[2048] ;
           int  timeout ;
           int  status ;
          char *prefix ;
          char *work ;

  static  char *buff ;

  static  char *ipfs_get="ipfs get --timeout=%ds --output=%s %s 1>%s 2>&1" ;
  static  char *curl_get="%s -X POST --max-time %d \"http://%s/api/v0/cat?arg=%s\" 1>\"%s\" 2>\"%s\"" ;

#define _BUFF_MAX   64000

/*---------------------------------------------------- Инициализация */

     if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*----------------------------- Формирование команды получения файла */

#ifdef  UNIX
        snprintf(result_path, sizeof(result_path)-1, "%s/ipfs.result", __work_folder) ;     /* Определяем путь файла результата */  
#else
        snprintf(result_path, sizeof(result_path)-1, "%s\\ipfs.result", __work_folder) ;
#endif

         unlink(result_path) ;                                      /* Удаляем файл результата */

                       timeout=2 ;

   do {                                                             /* Цикл исполнения запроса */

     if(__ipfs_url[0]==0)                                           /* Формируем команду получения файла */
            sprintf(command, ipfs_get, timeout, path, link, result_path) ;
     else   sprintf(command, curl_get, __curl_path, timeout, __ipfs_url, link, path, result_path) ;

             EMIR_log(command, __rpc_path) ;

/*----------------------------------------------- Исполнение запроса */

#ifdef  UNIX
              errno=0 ;
#else
         _set_errno(0) ;
#endif

      status=system(command) ;
   if((status!=  0 && 
       status!=  1 &&
       status!=256   ) || errno!=0) {
        
        if(errno)  prefix="" ;
        else       prefix="TRANSPORT - " ;

        if(__ipfs_url[0]==0)  sprintf(error, "%sОшибка исполнения ipfs (status=%d errno=%d) : %s", prefix, status, errno, command) ;
        else                  sprintf(error, "%sОшибка исполнения curl (status=%d errno=%d) : %s", prefix, status, errno, command) ;
                             EMIR_log(error, __rpc_path) ;
                                       return(-1) ;
                                    }
/*---------------------------------------------------- Анализ ответа */

            file=fopen(result_path, "rb") ;
         if(file==NULL) {
                            sprintf(error, "TRANSPORT - Ошибка открытия файла результата %d : %s", errno, result_path) ;
                           EMIR_log(error, __rpc_path) ;
                              return(-1) ;
                        }

              memset(buff, 0, _BUFF_MAX) ;
               fread(buff, 1, _BUFF_MAX-1, file) ;
              fclose(file) ;

         work=strstr(buff, "Error") ;
      if(work!=NULL) {

        if(       buff!=work                              &&       /* Если файл есть, но превышено время докачки -  */
           strstr(buff, "context deadline exceeded")!=NULL  ) {    /*  - увеличиваем время закачки в 2 раза         */

                                   timeout*=2 ;
                                    continue ;
                                                              }
        else                                                  {

                             sprintf(error, "TRANSPORT - ошибка закачки файла %200s", work) ;
                            EMIR_log(error, __rpc_path) ;
                                return(-1) ;
                                                              }
                     }

                          break ;

      } while(1) ;                                                  /* Цикл исполнения запроса */ 

//          unlink(result_path) ;                                   /*  Удаляем файл результата */

/*-------------------------------------------------------------------*/

   return(0) ;

}


/*********************************************************************/
/*								     */
/*                    Загрузка файла в блокчейн                      */

   int  EMIR_direct_putfile(char *path, char *link, char *error)

{
 typedef struct {
                   char  data[_DFS_DIRECT_FRAME] ;
                    int  size ;
                   char  txn[128] ;
                } FileFrame ;

  struct stat  file_attr ;
    FileFrame *frames ; 
          int  frames_cnt ;
         FILE *file ;
         char  gas[128] ;
         char  txn[256] ;
         char  block[128] ;
         char  contract[128] ;
         char  reply[128] ;
          int  status ;
          int  i ;

  static  char *buff ;

#define _BUFF_MAX   64000

/*---------------------------------------------------- Инициализация */

     if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

  if(__dfs_direct_box[0]==0) {
          sprintf(error, "Files transfer smart-contract address not defined - key DfsDirect") ;
            return(-1) ;
                             }
/*------------------------- Формирование цепочки кадров данных файла */

        status=stat(path, &file_attr) ;                             /* Определяем размер файла */
     if(status) {
                    sprintf(error, "File open error %d : %s", errno, path) ;
                                            return(-1) ;
                }

     if(file_attr.st_size==0) {                                     /* Если пустой файл */
                                  sprintf(link, "%064x", 0) ;
                                     return(0) ;
                              }

       frames_cnt=(file_attr.st_size-1)/_DFS_DIRECT_FRAME+1 ;       /* Определяем число кадров */

           frames=(FileFrame *)calloc(frames_cnt, sizeof(*frames)); /* Выделяем память */

        file=fopen(path, "rb") ;
     if(file==NULL) {
                         sprintf(error, "File open error %d : %s", errno, path) ;
                                           free(frames) ;
                                            return(-1) ;
                    }

   for(i=0 ; i<frames_cnt ; i++)                                    /* Считываем файл по кадрам */
      frames[i].size=fread(frames[i].data, 1, _DFS_DIRECT_FRAME, file) ;

          fclose(file) ;

/*--------------------------------------- Покадровая отправка данных */

                                memset(txn,  0, sizeof(txn)) ;
                                memset(txn, '0', 64) ;


   for(i=frames_cnt-1 ; i>=0 ; i--) {
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
                                         reply[0]=0 ;

      do {
/*- - - - - - - - - - - - - - - - - - - - -  Формирование транзакции */
             sprintf(buff, "99dd8cf1"                               /* Формируем блок данных транзакции */
                           "%s"
                           "0000000000000000000000000000000000000000000000000000000000000040"
                           "%064x",
                            txn, frames[i].size) ;

              EMIR_txt2hex64(frames[i].data, buff+strlen(buff), frames[i].size) ;
/*- - - - - - - - - - - - - - - - - - - - - - -  Отправка транзакции */
              status=EMIR_node_checkgas(__member_account,           /* Рассчёт газа на транзакцию */
                                        __dfs_direct_box, buff, gas, error) ;
           if(status)  break ;

              status=EMIR_node_unlockaccount(__member_account,      /* Разблокировка счета */
                                             __member_password, reply, sizeof(reply)-1, error) ;
           if(status)  break ;

              status=EMIR_node_sendcontract(__member_account,       /* Отправка транзакции */ 
                                            __dfs_direct_box, buff, gas, txn, error) ;
           if(status) {
               if(strstr(error, "password or unlock")!=NULL &&      /* Если первый раз встречается ошибка "authentication needed: password or unlock" - */
                         reply[0]==0                          ) {   /*  - производим явную разблокировку аккаунта                                       */
                                  strcpy(reply, "FORCE") ;
                                        continue ;
                                                                }
                             break ;
                      }

                            strcpy(frames[i].txn, txn) ;

                         break ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
         } while(1) ;

           if(status)  break ;
                                    }
/*------------------------------------------- Ожидание подтверждения */

  if(status==0) {

                     strcpy(link, frames[0].txn) ;


    for(i=0 ; i<frames_cnt ; i++) {

                         EMIR_log(frames[i].txn) ;
        status=EMIR_node_checktxn(frames[i].txn, block, contract, error) ;
     if(status <0) {  break ;  }  
     else
     if(status==0) {
                       Sleep(1000) ; 
                           i-- ;
                       continue ;
                   }

            status=0 ;  
                                  }

                }
/*-------------------------------------------------------------------*/

       free(frames) ;

  if(status)  return(-1) ;
              return( 0) ;
}


/*********************************************************************/
/*								     */
/*                    Получение файла из блокчейн                    */
/*								     */
/*   Return:  0  -  файл получен                                     */
/*           -1  -  ошибка получения файла                           */
/*            1  -  возможно, файл еще не прошел по сети             */

   int  EMIR_direct_getfile(char *link, char *path, char *error)

{
   FILE *file ;
   char  txn[128] ;
   char *data ;
    int  size ;
   char  value[128] ;
    int  status ;
   char *end   ;

  static  char *buff ;

#define _BUFF_MAX   64000

/*---------------------------------------------------- Инициализация */

     if(buff==NULL)  buff=(char *)calloc(1, _BUFF_MAX) ;

/*--------------------------------------------------- Открытие файла */

      file=fopen(path, "wb") ;
   if(file==NULL) {
                      sprintf(error, "File open error %d : %s", errno, path) ;
                         return(-1) ;
                  }
/*------------------------------------------------- Считывание файла */

              memset(txn, 0, sizeof(txn)) ;
             strncpy(txn, link, 64) ;

   while(strcmp(txn, "00000000000000000000000000000000"             /* Пока все фрагменты не собраны... */
                     "00000000000000000000000000000000")) {

                           memset(buff, 0, _BUFF_MAX) ;
         status=EMIR_node_getcode(txn, buff, _BUFF_MAX-1, error) ;  /* Извлечение данных транзакции */                                      
      if(status)  break ;

      if(strlen(buff)<64) return(1) ;                               /* Транзакция ещё не получена... */

              memset(value, 0, sizeof(value)) ;                     /* Размер данных */
              memcpy(value, buff+8+64*2, 64) ;
        size=strtoul(value, &end, 16) ;

                      data=buff+8+64*3 ;
         EMIR_hex2txt(data, data) ;                                 /* Преобразование данных из HEX */

          fwrite(data, 1, size, file) ;                             /* Запись данных в файл */

          memcpy(txn, buff+8, 64) ;
                                                           }
/*--------------------------------------------------- Закрытие файла */

                  fclose(file) ;

/*-------------------------------------------------------------------*/

  if(status)  return(-1) ;
              return( 0) ;
}


/*********************************************************************/
/*								     */
/*            Присоединение к файловому хранилищу MS Share           */

   int  EMIRi_share_connect(char *url, char *error)

{
  static char  url_prv[FILENAME_MAX] ;

#ifndef  UNIX
          int  status ;
#endif


    if(!strcmp(url, url_prv)) {
                                  return(0) ;
                              }
    else                      {
#ifdef  UNIX
#else
                                WNetCancelConnection(url_prv, true) ;
#endif
                              } 

#ifdef  UNIX
#else

       status=WNetAddConnection(url, __dfs_password, NULL) ;
    if(status!=NO_ERROR) {
                            sprintf(error, "ошибка соединения %d : %s", status, url) ;
                               return(-1) ;
                         }
#endif

         strcpy(url_prv, url) ;

   return(0) ;
}


/*********************************************************************/
/*								     */
/*	         Запрос конфигурационного параметра                  */

  int  EMIR_db_syspar(SQL_link *db, char *key, char *value, char *error)

{
     SQL_cursor *Cursor ;
           char  text[1024] ;
            int  status ;

/*--------------------------------------------------- Захват курсора */

        Cursor=db->LockCursor("EMIR_db_syspar") ;
     if(Cursor==NULL) {
                           sprintf(error, "Get <%s>: Cursor lock: %s", key, db->error_text) ;
                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                return(-1) ;
                      }
/*------------------------------------ Запрос по таблице SYSTEM_PARS */

   if(!strcmp(key, "MemberAccount" ) ||
      !strcmp(key, "MemberBox"     ) ||
      !strcmp(key, "MemberPassword") ||
      !strcmp(key, "MemberSign"    ) ||
      !strcmp(key, "Configuration" )   ) {

          sprintf(text, "select \"%s\" from   %s", key, __db_table_system_pars) ;
                                         }
/*--------------------------- Запрос по таблице SYSTEM_CONFIGURATION */

   else                                  {

          sprintf(text, "select \"Value\" from  %s where \"Key\"='%s'", __db_table_system_configuration, key) ;
                                         }
/*----------------------------------------------- Исполнение запроса */

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(error, "Get <%s>: %s", key, db->error_text) ;
                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                  return(-1) ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {

       if(status==_SQL_NO_DATA)  sprintf(error, "Get <%s>: No record", key) ;
       else                      sprintf(error, "Get <%s>: %s", key, db->error_text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;

       if(status!=_SQL_NO_DATA)  __db_errors_cnt++ ;

                                    return(-1) ;
                }

           strcpy(value, (char *)Cursor->columns[0].value) ;

                 db->SelectClose(Cursor) ;
                 db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*								     */
/*	         Запрос учетных реквизитов узла                      */

  int  EMIR_db_nodepars(SQL_link *db, char *error)

{
     SQL_cursor *Cursor ;
           char  text[1024] ;
            int  status ;

/*------------------------------------------------- Входной контроль */

    if(__member_account[0]!=0)  return(0) ;

/*--------------------------------------------------- Захват курсора */

        Cursor=db->LockCursor("EMIR_db_nodepars") ;
     if(Cursor==NULL) {
                           sprintf(error, "EMIR_db_nodepars : Cursor lock: %s", db->error_text) ;
                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                return(-1) ;
                      }
/*----------------------------------------------- Исполнение запроса */

          sprintf(text, "select \"MemberAccount\", \"MemberPassword\", \"MemberBox\", \"MemberSign\", \"MemberKey\" from %s", 
                        __db_table_system_pars) ;

        status=db->SelectOpen(Cursor, text, NULL, 0) ;
     if(status) {
                         sprintf(error, "EMIR_db_nodepars: %s", db->error_text) ;
                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                  return(-1) ;
                }

        status=db->SelectFetch(Cursor) ;
//   if(status==_SQL_NO_DATA)   break ;
     if(status) {

       if(status==_SQL_NO_DATA)  sprintf(error, "EMIR_db_nodepars : No record") ;
       else                      sprintf(error, "EMIR_db_nodepars : %s", db->error_text) ;

                            db->error_text[0]=0 ;
                            db->UnlockCursor(Cursor) ;
                          __db_errors_cnt++ ;
                                return(-1) ;
                }

           strcpy(__member_account,  (char *)Cursor->columns[0].value) ;
           strcpy(__member_password, (char *)Cursor->columns[1].value) ;
           strcpy(__member_box,      (char *)Cursor->columns[2].value) ;
           strcpy(__member_sign,     (char *)Cursor->columns[3].value) ;
           strcpy(__member_key,      (char *)Cursor->columns[4].value) ;
          sprintf(__member_executor, "%s:%s", __member_account, __member_password) ;

                      db->SelectClose(Cursor) ;
                      db->UnlockCursor(Cursor) ;

/*-------------------------------------------------------------------*/

  return(0) ;
}


/*********************************************************************/
/*								     */
/*	         Формирование случайной HEX-строки                   */

  void  EMIR_text_random(char *text, int  size)

{
     srand((unsigned int)time(NULL)) ;

   for( ; size>0 ; size--, text++)  sprintf(text, "%x", rand() & 0x000f) ;
}


/*********************************************************************/
/*								     */
/*	                Шифрование гаммой                            */

  void  EMIR_text_gamma(char *text, char *password)

{
   char  t_chr[8] ;
   char  p_chr[8] ;
    int  a ;
    int  b ;
   char *end ;
    int  i ;


        memset(t_chr, 0, sizeof(t_chr)) ;
        memset(p_chr, 0, sizeof(p_chr)) ;

   for(i=0 ; *text!=0 ; text++, i++) {

         if(password[i]==0)  i=0 ;

           t_chr[0]=     text[0] ;
           p_chr[0]= password[i] ;

                 a =strtoul(t_chr, &end, 16) ;
                 b =strtoul(p_chr, &end, 16) ;

                 a =a^b ;

           sprintf(t_chr, "%x", a & 0x000f) ;

                   text[0]=t_chr[0] ;

                                     }
}


/*********************************************************************/
/*                                                                   */
/*                     Формирование UUID                             */

  void  EMIR_uuid_generation(char *uuid)

{
#ifdef  UNIX
     char  command[1024] ;
     char  path[FILENAME_MAX] ;
     FILE *file ;
     char  data[1024] ;
#else
     GUID  guid ;
#endif


#ifdef  UNIX

         snprintf(path,    sizeof(path   )-1, "%s/uuid.dat", __work_folder) ;
         snprintf(command, sizeof(command)-1, "cat /proc/sys/kernel/random/uuid > %s", path) ;
           system(command) ;
    
           *uuid=0 ;

      file=fopen(path, "r") ;
   if(file==NULL)  return ;

           memset(data, 0, sizeof(data)) ;
            fread(data, 1, 64, file) ;
           fclose(file) ;
           unlink(path) ;

   EMIR_text_subst(data, "-", "", 0) ;

            memcpy(uuid, data, 32) ;
                   uuid[32]=0 ;

#else
       CoCreateGuid(&guid);

            sprintf(uuid, "%08lx%04hx%04hx%02x%02x%02x%02x%02x%02x%02x%02x",
                                          guid.Data1, guid.Data2, guid.Data3,
                                        (unsigned int)guid.Data4[0],
                                        (unsigned int)guid.Data4[1],
                                        (unsigned int)guid.Data4[2],
                                        (unsigned int)guid.Data4[3],
                                        (unsigned int)guid.Data4[4],
                                        (unsigned int)guid.Data4[5],
                                        (unsigned int)guid.Data4[6],
                                        (unsigned int)guid.Data4[7] ) ;
#endif
}
