/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                        Криптография                               */
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


#include "Ethereum_Mirror.h"
#include "Ethereum_Mirror_db.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4267)

/*---------------------------------------------------- Прототипы п/п */

    int  EMIRi_gpg_cert   (char *cert_path, char *signer, char *error) ;
    int  EMIRi_gpg_inpack (char *file_path, char *pack_path, char *receivers, char *error) ;
    int  EMIRi_gpg_unpack (char *pack_path, char *file_path, char *receiver, char *error) ;
    int  EMIRi_gpg_sign   (char *file_path, char *sign_path, char *signer, char *error) ;
    int  EMIRi_gpg_check  (char *file_path, char *sign_path, char *signer, char *error) ;

    int  EMIRi_cp_cert    (char *cert_path, char *signer, char *error) ;
    int  EMIRi_cp_inpack  (char *file_path, char *pack_path, char *receivers, char *error) ;
    int  EMIRi_cp_unpack  (char *pack_path, char *file_path, char *receiver, char *error) ;


/*********************************************************************/
/*								     */
/*                         Регистрация сертификата                   */

    int  EMIR_crypto_cert(char *cert_path, char *signer, char *error)
{
          char  command[1024] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

#ifdef  UNIX
   const  char *command_template="%s \"%s\" \"%s\" \"%s\"" ;
#else
   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\"" ;
#endif

/*------------------------------------------------- Входной контроль */

    if(__crypto_cert[0]==0)  return(0) ;


    if(signer[0]==0) {
            sprintf(error, "Trust signer is missed") ;
                                 return(-1) ;
                     }
/*-------------------------------------------- Встроенная реализация */

   if(__crypto_cert[0]==':') {

       if(!memcmp(__crypto_cert, ":gpg:", 5))  status=EMIRi_gpg_cert(cert_path, signer, error) ;
       else   
       if(!memcmp(__crypto_cert, ":cp:",  4))  status=EMIRi_cp_cert (cert_path, signer, error) ;
       else                                   {

            sprintf(error, "Unknown cryptography type") ;
                                 return(-1) ;
                                              }

                                 return(status) ;
                             }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_cert, 
                                              cert_path, signer, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта регистрации сертификата (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Регистрация сертификата - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка регистрации сертификата : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                         Формирование шифро-пакета                 */

    int  EMIR_crypto_inpack(char *file_path, char *pack_path, char *receivers, char *error)
{
          char  command[1024] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

#ifdef  UNIX
   const  char *command_template="%s \"%s\" \"%s\" \"%s\" \"%s\"" ;
#else
   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;
#endif

/*------------------------------------------------- Входной контроль */

    if(__crypto_pack[0]==0) {
            sprintf(error, "Не задан скрипт формирования шифро-пакета (ключ CryptoPack конфигурации)") ;
                                 return(-1) ;
                            }

    if(receivers[0]==0) {
            sprintf(error, "Empty receivers list") ;
                                 return(-1) ;
                        }
/*-------------------------------------------- Встроенная реализация */

   if(__crypto_pack[0]==':') {

       if(!memcmp(__crypto_pack, ":gpg:",5))  status=EMIRi_gpg_inpack(file_path, pack_path, receivers, error) ;
       else
       if(!memcmp(__crypto_pack, ":cp:", 4))  status=EMIRi_cp_inpack (file_path, pack_path, receivers, error) ;
       else                                   {

            sprintf(error, "Unknown cryptography type") ;
                                 return(-1) ;
                                              }

                                 return(status) ;
                             }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_pack, 
                                              file_path,
                                              pack_path, receivers, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта формирования крипто-пакета (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Формирование крипто-пакета - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка формирования крипто-пакета : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                      Распаковка шифро-пакета                      */

    int  EMIR_crypto_unpack(char *pack_path, char *file_path, char *receiver, char *error)
{
          char  command[1024] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

#ifdef  UNIX
   const  char *command_template="%s \"%s\" \"%s\" \"%s\" \"%s\"" ;
#else
   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;
#endif

/*------------------------------------------------- Входной контроль */

    if(__crypto_unpack[0]==0) {
            sprintf(error, "Не задан скрипт распаковки шифро-пакета (ключ CryptoUnPack конфигурации)") ;
                                 return(-1) ;
                              }
/*-------------------------------------------- Встроенная реализация */

   if(__crypto_unpack[0]==':') {

       if(!memcmp(__crypto_unpack, ":gpg:", 5))  status=EMIRi_gpg_unpack(pack_path, file_path, receiver, error) ;
       else
       if(!memcmp(__crypto_unpack, ":cp:",  4))  status=EMIRi_cp_unpack (pack_path, file_path, receiver, error) ;
       else                                     {

            sprintf(error, "Unknown cryptography type") ;
                                 return(-1) ;
                                                }

                                 return(status) ;
                               }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_unpack, 
                                              pack_path,
                                              file_path, receiver, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта распаковки крипто-пакета (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Распаковки крипто-пакета - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка распаковки крипто-пакета : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                         Формирование подписи файла                */

    int  EMIR_crypto_sign(char *file_path, char *sign_path, char *signer, char *error)
{
          char  command[1024] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;

/*------------------------------------------------- Входной контроль */

    if(__crypto_sign[0]==0) {
            sprintf(error, "Не задан скрипт формирования подписи (ключ CryptoSign конфигурации)") ;
                                 return(-1) ;
                            }

    if(signer[0]==0) {
            sprintf(error, "Не задан сертификат подписи") ;
                                 return(-1) ;
                     }
/*-------------------------------------------- Встроенная реализация */

   if(__crypto_sign[0]==':') {

       if(!memcmp(__crypto_sign, ":gpg:", 5))  status=EMIRi_gpg_sign(file_path, sign_path, signer, error) ;
       else                                   {

            sprintf(error, "Unknown cryptography type") ;
                                 return(-1) ;
                                              }

                                 return(status) ;
                             }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_sign, 
                                              file_path,
                                              sign_path, signer, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта подписи (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Формирование подписи - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка формирования подписи : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                        Проверка подписи файла                     */

    int  EMIR_crypto_check(char *file_path, char *sign_path, char *signer, char *error)
{
          char  command[1024] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;

/*------------------------------------------------- Входной контроль */

    if(__crypto_check[0]==0) {
            sprintf(error, "Не задан скрипт проверки подписи (ключ CryptoCheck конфигурации)") ;
                                 return(-1) ;
                            }

    if(signer[0]==0) {
            sprintf(error, "Не задан сертификат проверки подписи") ;
                                 return(-1) ;
                     }
/*-------------------------------------------- Встроенная реализация */

   if(__crypto_check[0]==':') {

       if(!memcmp(__crypto_check, ":gpg:", 5))  status=EMIRi_gpg_check(file_path, sign_path, signer, error) ;
       else                                   {

            sprintf(error, "Unknown cryptography type") ;
                                 return(-1) ;
                                              }

                                 return(status) ;
                             }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_check, 
                                              file_path,
                                              sign_path, signer, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта проверки подписи (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Проверка подписи - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка проверки подписи : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                      Шифрование файла на пароле                   */

    int  EMIR_gamma_inpack(char *file_path, char *pack_path, char *password, char *error)
{
          char  command[1024] ;
          FILE *file ;
          FILE *file_g ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

#define  _BUFF_SIZE  32000 

          char *buff ;
           int  cnt ;  
           int  i ;  
           int  j ;  

#ifdef  UNIX
   const  char *command_template="%s \"%s\" \"%s\" \"%s\" \"%s\"" ;
#else
   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;
#endif

/*------------------------------------------------- Входной контроль */

    if(password[0]==0) {
            sprintf(error, "Не задан пароль шифрования") ;
                                 return(-1) ;
                        }
/*-------------------------------------------- Встроенное шифрование */

    if(__gamma_pack[0]==0) {
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Открытие файлов */
           file  =fopen(file_path, "rb") ;
        if(file  ==NULL) {
            sprintf(error, "Ошибка открытия файла %d: %s ", errno, file_path) ;
                                 return(-1) ;    
                         }

           file_g=fopen(pack_path, "wb") ;
        if(file_g==NULL) {
            sprintf(error, "Ошибка открытия файла %d: %s ", errno, pack_path) ;
                                 return(-1) ;    
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - Шифрование */
               buff=(char *)calloc(1, _BUFF_SIZE) ;
 
       do {
               cnt=fread(buff, 1, _BUFF_SIZE, file) ;
            if(cnt==0)  break ;

           for(i=0, j=0 ; i<cnt ; i++, j++) {

                if(password[j]==0)  j=0 ;

                     buff[i]^=password[j] ; 
                                            }

                  fwrite(buff, 1, cnt, file_g) ;

          } while(1) ;

                 free(buff) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Закрытие файлов */
               fclose(file  ) ;
               fclose(file_g) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                 return(0) ;
                           }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __gamma_pack, 
                                              file_path,
                                              pack_path, password, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта шифрования (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Шифрование - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка шифрования : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                   Дешифровка файла по паролю                      */

    int  EMIR_gamma_unpack(char *pack_path, char *file_path, char *password, char *error)
{
          char  command[1024] ;
          FILE *file ;
          FILE *file_g ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;

#define  _BUFF_SIZE  32000 

          char *buff ;
           int  cnt ;  
           int  i ;  
           int  j ;  

#ifdef  UNIX
   const  char *command_template="%s \"%s\" \"%s\" \"%s\" \"%s\"" ;
#else
   const  char *command_template="wscript /b \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"" ;
#endif

/*------------------------------------------------- Входной контроль */

    if(password[0]==0) {
            sprintf(error, "Не задан пароль шифрования") ;
                                 return(-1) ;
                        }
/*-------------------------------------------- Встроенное шифрование */

    if(__gamma_unpack[0]==0) {

/*- - - - - - - - - - - - - - - - - - - - - - - - -  Открытие файлов */
           file_g=fopen(pack_path, "rb") ;
        if(file_g==NULL) {
            sprintf(error, "Ошибка открытия файла %d: %s ", errno, pack_path) ;
                                 return(-1) ;    
                         }

           file  =fopen(file_path, "wb") ;
        if(file  ==NULL) {
            sprintf(error, "Ошибка открытия файла %d: %s ", errno, file_path) ;
                                 return(-1) ;    
                         }
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - Шифрование */
               buff=(char *)calloc(1, _BUFF_SIZE) ;
 
       do {
               cnt=fread(buff, 1, _BUFF_SIZE, file_g) ;
            if(cnt==0)  break ;

           for(i=0, j=0 ; i<cnt ; i++, j++) {

                if(password[j]==0)  j=0 ;

                     buff[i]^=password[j] ; 
                                            }

                  fwrite(buff, 1, cnt, file) ;

          } while(1) ;

                 free(buff) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - -  Закрытие файлов */
               fclose(file_g) ;
               fclose(file  ) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
                                   return(0) ;
                             }
/*-------------------------- Формирование команды исполнения скрипта */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/result.dat", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\result.dat", __work_folder) ;
#endif

       sprintf(command, command_template, __crypto_unpack, 
                                              pack_path,
                                              file_path, password, res_path) ;

/*----------------------------------------------- Исполнение скрипта */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
              sprintf(error, "Ошибка исполнения скрипта распаковки крипто-пакета (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "Распаковки крипто-пакета - Ошибка открытия рабочего файла result.dat : %d", errno) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(memicmp(data, "SUCCESS", strlen("SUCCESS"))) {
                            sprintf(error, "Ошибка распаковки крипто-пакета : %s", data) ;
                              return(-1) ;                           
                                                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                    GPG - Регистрация сертификата                  */

    int  EMIRi_gpg_cert(char *cert_path, char *signer, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  password[FILENAME_MAX] ;
          char  cert_name[128] ;
          char  command[2048] ;
          FILE *file ;
          char  data[1024] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;
          char *value ;
          char *end ;

// gpg2 --import DEV-1.gpg
// gpg2 --sign-key --batch --yes -u DEV-T --passphrase 1234567890 DEV-1

#ifdef  UNIX
   const  char *import_template="\"%s\" --import --batch --yes \"%s\" 1>\"%s\" 2>&1" ;
   const  char *  sign_template="\"%s\" --sign-key --batch --yes -u %s %s 1>\"%s\" 2>&1" ;
#else
   const  char *import_template="cmd /c \"\"%s\" --import --batch --yes \"%s\" 1>\"%s\" 2>&1\"" ;
   const  char *  sign_template="cmd /c \"\"%s\" --sign-key --batch --yes -u %s --passphrase %s %s 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_cert, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_cert - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

#ifdef  UNIX

       value=strstr(__crypto_cert, "password=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_cert - Parameter 'password' is missed") ;
                                 return(-1) ;
                 }

            memset(password, 0, sizeof(password)) ;
           strncpy(password, value+strlen("password="), sizeof(password)-1) ;
        end=strchr(password, ';') ;
     if(end!=NULL)  *end=0 ;

#endif

/*----------------------------------------------------- Импорт ключа */
/*- - - - - - - - - - - - - - - - -  Формирование команды исполнения */
#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

       sprintf(command, import_template, exe_path, cert_path, res_path) ;
/*- - - - - - - - - - - - - - - - - - - - - - - - Исполнение команды */
          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_cert - Import command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*- - - - - - - - - - - - - - - - - - - - Извлечение и анализ ответа */
            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_cert - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]==0) {
                        sprintf(error, "EMIRi_gpg_cert - Empty result file: %s", res_path) ;
                              return(-1) ;
                     }

      if(strstr(data, " failed:")!=NULL) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                  sprintf(error, "EMIRi_gpg_cert - Import error: %s", data) ;
                       return(-1) ;                           
                                         }

#define  KEY  "Total number processed: 1"

         value=strstr(data, KEY) ;
      if(value==NULL) {
                         sprintf(error, "EMIRi_gpg_cert - No successfull import mark detected") ;
                            return(-1) ;                           
                      }

#undef  KEY

/*---------------------------------------------------- Подпись ключа */
/*- - - - - - - - - - - - - - - - - - Выделение идентификатора ключа */
     for(value=cert_path+strlen(cert_path) ;
         value>cert_path                   ; value--) 
       if(*value=='/' || *value=='\\')  break ;

       if(*value=='/' || *value=='\\')  value++ ;

              memset(cert_name, 0, sizeof(cert_name)) ;
             strncpy(cert_name, value, sizeof(cert_name)-1) ;

          end=strchr(cert_name, '.') ;
       if(end!=NULL)  *end=0 ;
/*- - - - - - - - - - - - - - - - -  Формирование команды исполнения */
#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

#ifdef  UNIX
       sprintf(command, sign_template, exe_path, signer, cert_name, res_path) ;
#else
       sprintf(command, sign_template, exe_path, signer, password, cert_name, res_path) ;
#endif
/*- - - - - - - - - - - - - - - - - - - - - - - - Исполнение команды */
          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_cert - Cert signing command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*- - - - - - - - - - - - - - - - - - - - Извлечение и анализ ответа */
            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_cert - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]==0) {
                        sprintf(error, "EMIRi_gpg_cert - Empty result file: %s", res_path) ;
                              return(-1) ;
                     }

      if(strstr(data, " failed:"  )!=NULL ||
         strstr(data, "not found:")!=NULL   ) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                  sprintf(error, "EMIRi_gpg_cert - Cert signing error: %s", data) ;
                       return(-1) ;                           
                                              }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                 GPG - Формирование шифро-пакета                   */

    int  EMIRi_gpg_inpack(char *file_path, char *pack_path, char *receivers, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[512] ;
          char  res_path[FILENAME_MAX] ;
          char  receivers_list[1024] ;
          char  r_list[2048] ;
          char *receiver ;
           int  status ;
          char *value ;
          char *end ;
           int  i ;

// gpg2 -e --batch --yes -r DEV-1 -r DEV-2 --output test.inc test

#ifdef  UNIX
   const  char *command_template="\"%s\" -e --batch --yes %s --output \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
#else
   const  char *command_template="cmd /c \"\"%s\" -e --batch --yes %s --output \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_pack, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_inpack - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

/*---------------------------------------- Разбор списка получателей */

    if(strlen(receivers)>=sizeof(receivers_list)) {
                    sprintf(error, "EMIRi_gpg_inpack - Receivers list too big") ;
                                 return(-1) ;
                                                  }

          strcpy(receivers_list, receivers) ;
          memset(r_list, 0, sizeof(r_list)) ;

     for(receiver=receivers_list, i=0 ; ; receiver=end+1, i++) {    /* Разбиваем строку на слова и формируем список получателей */

           end=strchr(receiver, ',') ;
        if(end==NULL)  break ;
          *end=0 ;

        if(receiver[0]!=0) {
                               strcat(r_list, " -r ") ;
                               strcat(r_list, receiver) ;
                           } 
                                                               } 
/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

       sprintf(command, command_template, exe_path, r_list,
                                          pack_path, file_path, res_path) ;

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_inpack - Encrypting command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_inpack - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]!=0) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                        sprintf(error, "EMIRi_gpg_inpack - Encrypting error: %s", data) ;
                              return(-1) ;
                     }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                 GPG - Распаковка шифро-пакета                     */

    int  EMIRi_gpg_unpack(char *pack_path, char *file_path, char *receiver, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  password[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[512] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;
          char *value ;
          char *end ;

// gpg2 -d --batch --yes --passphrase 1234567890 --output test.dec test.inc

#ifdef  UNIX
   const  char *command_template="\"%s\" -d --batch --yes --passphrase %s --output \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
#else
   const  char *command_template="cmd /c \"\"%s\" -d --batch --yes --output \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_unpack, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_unpack - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

#ifdef  UNIX

       value=strstr(__crypto_unpack, "password=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_unpack - Parameter 'password' is missed") ;
                                 return(-1) ;
                 }

            memset(password, 0, sizeof(password)) ;
           strncpy(password, value+strlen("password="), sizeof(password)-1) ;
        end=strchr(password, ';') ;
     if(end!=NULL)  *end=0 ;

#endif

/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

#ifdef  UNIX
       sprintf(command, command_template, exe_path, password,
                                          file_path, pack_path, res_path) ;
#else
       sprintf(command, command_template, exe_path, 
                                          file_path, pack_path, res_path) ;
#endif

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_unpack - Decrypting command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_unpack - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]==0) {
                        sprintf(error, "EMIRi_gpg_unpack - Empty result file: %s", res_path) ;
                              return(-1) ;
                     }

      if(strstr(data, " failed:")!=NULL) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                  sprintf(error, "EMIRi_gpg_unpack - Decrypting error: %s", data) ;
                       return(-1) ;                           
                                         }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                    GPG - Формирование подписи                     */

    int  EMIRi_gpg_sign(char *file_path, char *sign_path, char *signer, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  password[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[512] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;
          char *value ;
          char *end ;

// gpg2 -b --batch --yes -u DEV-ETH-3 --passphrase 1234567890 --armor --output test.sig test.dat

#ifdef  UNIX
   const  char *command_template="\"%s\" -b --batch --yes -u %s --passphrase %s --armor --output \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
#else
   const  char *command_template="cmd /c \"\"%s\" -b --batch --yes -u %s --armor --output \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_sign, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_sign - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

#ifdef  UNIX

       value=strstr(__crypto_sign, "password=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_sign - Parameter 'password' is missed") ;
                                 return(-1) ;
                 }

            memset(password, 0, sizeof(password)) ;
           strncpy(password, value+strlen("password="), sizeof(password)-1) ;
        end=strchr(password, ';') ;
     if(end!=NULL)  *end=0 ;

#endif

/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

#ifdef  UNIX
       sprintf(command, command_template, exe_path, signer, password,
                                          sign_path, file_path, res_path) ;
#else
       sprintf(command, command_template, exe_path, signer,
                                          sign_path, file_path, res_path) ;
#endif

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_sign - Signing command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_sign - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;                           
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]!=0) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                        sprintf(error, "EMIRi_gpg_sign - Signing error: %s", data) ;
                              return(-1) ;
                     }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                       GPG - проверка подписи                      */

    int  EMIRi_gpg_check(char *file_path, char *sign_path, char *signer, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[512] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;
          char *value ;
          char *end ;

// gpg2 --verify --batch --yes test.sig test

#ifdef  UNIX
   const  char *command_template="\"%s\" --verify --batch --yes  \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
#else
   const  char *command_template="cmd /c \"\"%s\" --verify --batch --yes \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_check, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_gpg_check - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/gpg.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\gpg.res", __work_folder) ;
#endif

#ifdef  UNIX
       sprintf(command, command_template, exe_path, 
                                          sign_path, file_path, res_path) ;
#else
       sprintf(command, command_template, exe_path, 
                                          sign_path, file_path, res_path) ;
#endif

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_gpg_check - Verifing command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;
                             }
/*--------------------------------------- Извлечение и анализ ответа */

            file=fopen(res_path, "rt") ;
         if(file==NULL) {
                            sprintf(error, "EMIRi_gpg_check - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;
                        }

            memset(data, 0, sizeof(data)) ;
             fread(data, sizeof(data)-1, 1, file) ;
            fclose(file) ;

      if(data[0]==0) {
                        sprintf(error, "EMIRi_gpg_check - Empty result file: %s", res_path) ;
                              return(-1) ;
                     }

      if(strstr(data, " failed:")!=NULL) {
              
          do {
                               end=strchr(data, '\r') ;
                if(end==NULL)  end=strchr(data, '\n') ;
                if(end!=NULL) *end= 0 ;

             } while(end!=NULL) ;
 
                  sprintf(error, "EMIRi_gpg_check - Verifing error: %s", data) ;
                       return(-1) ;                           
                                         }

#define  KEY  "Good signature from \""

         value=strstr(data, KEY) ;
      if(value==NULL) {
                         sprintf(error, "EMIRi_gpg_check - No success verifing mark detected") ;
                            return(-1) ;                           
                      }

                    value+=strlen(KEY) ;
         end=strchr(value, ' ') ;
      if(end==NULL) {
                        sprintf(error, "EMIRi_gpg_check - Invalid verifing record") ;
                            return(-1) ;                           
                    }
        *end=0 ; 

      if(stricmp(value, signer)) {
                        sprintf(error, "EMIRi_gpg_check - Signed by different key") ;
                                     return(-1) ;                           
                                 }

#undef  KEY

/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*             Crypto PRO - Регистрация сертификата                  */

    int  EMIRi_cp_cert(char *cert_path, char *signer, char *error)
{
   return(0) ;
}


/*********************************************************************/
/*								     */
/*                Crypto PRO - Формирование шифро-пакета             */

    int  EMIRi_cp_inpack(char *file_path, char *pack_path, char *receivers, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[2048] ;
          char  res_path[FILENAME_MAX] ;
          char  receivers_list[1024] ;
           int  receivers_cnt ;
          char  r_list[2048] ;
          char *receiver ;
           int  status ;
          char  text[1024] ;
          char *value ;
          char *end ;
           int  i ;

// cryptcp.x64 -encr -f C:\Certs\TEST_DIXY_ENCODING.cer -f C:\Certs\TEST_GPBF_ENCODING.cer -nochain C:\Temp\AddReester.data C:\Temp\AddReester.enc

#ifdef  UNIX
   const  char * command_template="\"%s\" -encr %s -nochain \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
   const  char *receiver_template=" -f \"%s/%s.cer\"" ;
#else
   const  char * command_template="cmd /c \"\"%s\" -encr %s -nochain \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
   const  char *receiver_template=" -f \"%s\\%s.cer\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_pack, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_cp_inpack - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

/*---------------------------------------- Разбор списка получателей */

    if(strlen(receivers)>=sizeof(receivers_list)) {
                    sprintf(error, "EMIRi_cp_inpack - Receivers list too big") ;
                                 return(-1) ;
                                                  }


          strcpy(receivers_list, receivers) ;
          strcat(receivers_list, ",") ;
          memset(r_list, 0, sizeof(r_list)) ;

     for(receiver=receivers_list, i=0 ; ; receiver=end+1, i++) {    /* Разбиваем строку на слова и формируем список получателей */

           end=strchr(receiver, ',') ;
        if(end==NULL)  break ;
          *end=0 ;

        if(receiver[0]!=0) {
                              sprintf(text, receiver_template, __cert_storage, receiver) ;
                               strcat(r_list, text) ;
                           } 
                                                               } 

                      receivers_cnt=i ;

 
/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/cp.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\cp.res", __work_folder) ;
#endif

       sprintf(command, command_template, exe_path, r_list,
                                          file_path, pack_path, res_path) ;

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_cp_inpack - Encrypting command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;                           
                             }
/*--------------------------------------- Извлечение и анализ ответа */

      file=fopen(res_path, "rt") ;
   if(file==NULL) {
                            sprintf(error, "EMIRi_cp_inpack - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;                           
                  }

          memset(data, 0, sizeof(data)) ;
           fread(data, sizeof(data)-1, 1, file) ;
          fclose(file) ;

   if(strstr(data, "[ErrorCode: 0x00000000]")==NULL) {
                        sprintf(error, "EMIRi_cp_inpack - Success mark is absent in encrypting result file: %s", res_path) ;
                                     return(-1) ;
                                                     }

  if(receivers_cnt>1) {

     sprintf(text, ": %d", receivers_cnt) ;
   if(strstr(data, text)==NULL) {
                        sprintf(error, "EMIRi_cp_inpack - Insufficient number of certificates used, details in file: %s", res_path) ;
                                     return(-1) ;
                                }
                      }
/*-------------------------------------------------------------------*/

   return(0) ;
}


/*********************************************************************/
/*								     */
/*                 Crypto PRO - Распаковка шифро-пакета              */

    int  EMIRi_cp_unpack(char *pack_path, char *file_path, char *receiver, char *error)
{
          char  exe_path[FILENAME_MAX] ;
          char  password[FILENAME_MAX] ;
          char  command[2048] ;
          FILE *file ;
          char  data[2048] ;
          char  res_path[FILENAME_MAX] ;
           int  status ;
          char *value ;
          char *end ;

// cryptcp.x64 -decr -dn TEST_DIXY_ENCODING -nochain C:\Temp\AddReester.enc C:\Temp\AddReester.data

#ifdef  UNIX
   const  char *command_template="\"%s\" -decr -dn %s -nochain -pin %s \"%s\" \"%s\" 1>\"%s\" 2>&1" ;
#else
   const  char *command_template="cmd /c \"\"%s\" -decr -dn %s -nochain \"%s\" \"%s\" 1>\"%s\" 2>&1\"" ;
#endif

/*------------------------------------------------ Разбор параметров */

       value=strstr(__crypto_unpack, "exe_path=") ;
    if(value==0) {
            sprintf(error, "EMIRi_cp_unpack - Parameter 'exe_path' is missed") ;
                                 return(-1) ;
                 }

            memset(exe_path, 0, sizeof(exe_path)) ;
           strncpy(exe_path, value+strlen("exe_path="), sizeof(exe_path)-1) ;
        end=strchr(exe_path, ';') ;
     if(end!=NULL)  *end=0 ;

#ifdef  UNIX

       value=strstr(__crypto_unpack, "password=") ;
    if(value==0) {
            sprintf(error, "EMIRi_cp_unpack - Parameter 'password' is missed") ;
                                 return(-1) ;
                 }

            memset(password, 0, sizeof(password)) ;
           strncpy(password, value+strlen("password="), sizeof(password)-1) ;
        end=strchr(password, ';') ;
     if(end!=NULL)  *end=0 ;

#endif

/*---------------------------------- Формирование команды исполнения */

#ifdef  UNIX
       snprintf(res_path, sizeof(res_path)-1, "%s/cp.res", __work_folder) ;
#else
       snprintf(res_path, sizeof(res_path)-1, "%s\\cp.res", __work_folder) ;
#endif

#ifdef  UNIX
       sprintf(command, command_template, exe_path, 
                                receiver, password, pack_path, file_path, res_path) ;
#else
       sprintf(command, command_template, exe_path, 
                                receiver, pack_path, file_path, res_path) ;
#endif

/*----------------------------------------------- Исполнение команды */

          unlink(res_path) ;                                        /* Удаление рабочих файлов */

                errno=0 ;

      status=system(command) ;
   if(status!=0 || errno!=0) {
             EMIR_log(command) ; 
              sprintf(error, "EMIRi_cp_unpack - Decrypting command execute fail (status=%d errno=%d) : %s", status, errno, command) ;
                                return(-1) ;
                             }
/*--------------------------------------- Извлечение и анализ ответа */

      file=fopen(res_path, "rt") ;
   if(file==NULL) {
                            sprintf(error, "EMIRi_cp_unpack - Result file open error %d : %s", errno, res_path) ;
                              return(-1) ;
                  }

          memset(data, 0, sizeof(data)) ;
           fread(data, sizeof(data)-1, 1, file) ;
          fclose(file) ;

EMIR_log(data) ;

   if(strstr(data, "[ErrorCode: 0x00000000]")==NULL) {
                    
                        sprintf(error, "EMIRi_cp_unpack - Success mark is absent in decrypting result file: %s", res_path) ;
                                     return(-1) ;
                                                     }
/*-------------------------------------------------------------------*/

   return(0) ;
}


