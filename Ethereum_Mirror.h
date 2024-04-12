/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*********************************************************************/

#ifdef  __MAIN__
#define  _EXTERNAL
#else
#define  _EXTERNAL  extern
#endif

#ifdef  UNIX
#include "abtp_msg.h"
#include "abtp_exh.h"
#include "abtp_tcp.h"
#include "http_tcp.h"
#include "sql_common.h"
#else
#include "..\..\grabtp\abtp_msg.h"
#include "..\..\grabtp\abtp_exh.h"
#include "..\..\grabtp\abtp_tcp.h"
#include "..\..\grabtp\http_tcp.h"
#include "..\..\grsql\sql_common.h"
#include "..\..\grsql\sql_odbc.h"
#endif

#if defined UNIX || defined _CONSOLE
#undef   LB_ADD_ROW
#define  LB_ADD_ROW(dummy, t)    EMIR_log(t) ;
#endif

#ifdef  UNIX
#define  Sleep(t)  sleep(t/1000)
#define  strupr(s)               { char *p ; for(p=s ; *p!=0 ; p++) *p=toupper(*p) ; }


#define  SOCKET_ERROR        -1
#define  WSAGetLastError()  errno
#define  memicmp            memcmp
#endif

/*---------------------------------------------- Параметры генерации */

#define   _APPLICATION     "Ethereum_Mirror"
#define   _PROGRAM_TITLE   "Ethereum_Mirror"

#undef    _VERSION
#define   _VERSION   "09.04.2024"

/*--------------------------------------------------- Система команд */

  _EXTERNAL   int  __exit_flag ;             /* Флаг завершения работы */
  _EXTERNAL  long  __exit_time ;             /* Время работы */

/*---------------------------------- Коды пользовательских сообщений */

#define    _USER_SECTION_ENABLE    1
#define    _USER_RELOAD            2

/*-------------------------------------------------- Диалоговые окна */

#ifdef UNIX
#define      HWND            void *
#define     DWORD   unsigned long
#define    HANDLE            void *
#define    LPVOID            void *
#define    WINAPI
#define   INT_PTR             int *
#define  CALLBACK
#define      UINT    unsigned int
#define    WPARAM   unsigned long
#define    LPARAM   unsigned long
#define     BOOL     unsigned int
#else

  _EXTERNAL            UINT  TaskBar_Msg ;        /* Сообщение активизации иконы в TaskBar */
  _EXTERNAL  NOTIFYICONDATA  TbIcon ;             /* Описание TaskBar-иконки */

  _EXTERNAL       HINSTANCE  hInst ;
  _EXTERNAL        WNDCLASS  FrameWindow ;
  _EXTERNAL            HWND  hFrameWindow ;

  _EXTERNAL            HWND  __dialog ;

  _EXTERNAL             int  __window_closed ;    /* Флаг скрытия окна в трей */

  _EXTERNAL             int  __console_process ;  /* Флаг работы в консольном режиме */

#define                    _NO_ICON              0
#define               _WARNING_ICON              1
#define                 _ERROR_ICON              2
#define            _G_COMPLETE_ICON              3
#define            _B_COMPLETE_ICON              4
#define            _S_COMPLETE_ICON              5
#define              _EXECUTED_ICON              6
#define                _MANUAL_ICON              7

#endif

/*------------------------------------------------- Общие переменные */

  _EXTERNAL            char  __cwd[FILENAME_MAX] ;               /* Рабочий раздел */

  _EXTERNAL            char  __signal_path[FILENAME_MAX] ;       /* Путь сигнального файла */
  _EXTERNAL            char  __context_path[FILENAME_MAX] ;      /* Путь файла сохранения контекста */
  _EXTERNAL            char  __db_context_path[FILENAME_MAX] ;   /* Путь файла контекста БД */
  _EXTERNAL            char  __log_path[FILENAME_MAX] ;          /* Маска пути файла общего лога */
  _EXTERNAL            char  __rpc_path[FILENAME_MAX] ;          /* Маска пути файла лога обмена по RPC */

  _EXTERNAL             int  __log_level ;                       /* Уровень общего лога */
  _EXTERNAL             int  __log_rotation ;                    /* Режим ротации логов */

  _EXTERNAL             int  __critical_stop ;                   /* Флаг остановки без перезапуска при критических ошибках */

  _EXTERNAL            char  __mon_context_path[FILENAME_MAX] ;  /* Путь файла настроек мониторинга */
  _EXTERNAL            char  __monitoring_path[FILENAME_MAX] ;   /* Путь файла мониторинга */
  _EXTERNAL            char  __monitoring_rules[FILENAME_MAX] ;  /* Реализуемые правила мониторинга */
  _EXTERNAL            char  __monitoring_format[FILENAME_MAX] ; /* Формат файла мониторинга */

  _EXTERNAL            char  __file_storage[FILENAME_MAX] ;      /* Папка файлового хранилища */
  _EXTERNAL            char  __cert_storage[FILENAME_MAX] ;      /* Папка хранения файлов сертификатов публичного ключа участника */
  _EXTERNAL            char  __members_storage[FILENAME_MAX] ;   /* Папка хранения данных участников */
  _EXTERNAL            char  __work_folder[FILENAME_MAX] ;       /* Рабочая папка */
  _EXTERNAL            char  __reports_folder[FILENAME_MAX] ;    /* Папка хранения отчетов */
  _EXTERNAL            char  __dcl_storage[FILENAME_MAX] ;       /* Папка хранения DCL-процедур */

  _EXTERNAL            char  __node_url[FILENAME_MAX] ;          /* URL узла Ethereum */
  _EXTERNAL            char  __swarm_url[512] ;                  /* URL узла Swarm */
  _EXTERNAL            char  __ipfs_url[512] ;                   /* URL узла IPFS */
  _EXTERNAL            char  __dfs_url[512] ;                    /* URL облачной DFS */
  _EXTERNAL            char  __curl_path[FILENAME_MAX] ;         /* Путь к утилите CURL */

  _EXTERNAL            char  __member_account[128] ;             /* Адрес счета узла */
  _EXTERNAL            char  __member_password[128] ;            /* Пароль счета узла */
  _EXTERNAL            char  __member_executor[256] ;            /* Адрес:пароль счета узла */
  _EXTERNAL            char  __member_box[128] ;                 /* Адрес п/я узла */
  _EXTERNAL            char  __member_sign[128] ;                /* Имя сертификата приватного ключа узла */
  _EXTERNAL            char  __member_key[128] ;                 /* Идентификатор узла */

  _EXTERNAL            char  __gas_value[128] ;                  /* Табличное значение газа для транзакции, если 0 - расчитывается динамически */

  _EXTERNAL             int  __purge_completed ;                 /* Удаление операций при успешном завершении */
  _EXTERNAL             int  __purge_deep ;                      /* Глубина хранения операций, дней */

  _EXTERNAL            char  __net_type[128] ;                   /* Тип сети: Ethereum, Quorum */

#define   _NULL_ADDR  "0000000000000000000000000000000000000000"

/*----------------------------------------------------- Криптография */

  typedef struct {
                    char  type[32] ;        /* Тип крипто-системы */
                    char  cert[1024] ;      /* Скрипт регистрации сертификата открытого ключа */
                    char  pack[1024] ;      /* Скрипт формирования крипто-пакета на получателей */
                    char  unpack[1024] ;    /* Скрипт распаковки крипто-пакета */
                    char  sign[1024] ;      /* Скрипт формирования подписи */
                    char  check[1024] ;     /* Скрипт проверки подписи */
                 } Crypto ;
 
#define  _CRYPTO_MAX  10

  _EXTERNAL          Crypto __crypto[_CRYPTO_MAX] ;              /* Используемые схемы криптографии */

  _EXTERNAL            char  __crypto_cert[FILENAME_MAX] ;       /* Скрипт установки сертификата подписи */
  _EXTERNAL            char  __crypto_sign[FILENAME_MAX] ;       /* Скрипт формирования подписи */
  _EXTERNAL            char  __crypto_check[FILENAME_MAX] ;      /* Скрипт проверки подписи */
  _EXTERNAL            char  __crypto_pack[FILENAME_MAX] ;       /* Скрипт формирования шифро-пакета */
  _EXTERNAL            char  __crypto_unpack[FILENAME_MAX] ;     /* Скрипт вскрытия шифро-пакета */
  _EXTERNAL            char  __gamma_pack[FILENAME_MAX] ;        /* Скрипт шифрования на пароле */
  _EXTERNAL            char  __gamma_unpack[FILENAME_MAX] ;      /* Скрипт дешифрования по паролю */

/*-------------------------------------------------- Описание секций */

  _EXTERNAL            char  __active[1024] ;    /* Перечень активных секций */

  typedef struct {
                    HWND  hWnd ;           /* Дескриптор окна */
                    char  title[128] ;     /* Заголовок */           
                 } Section ;

  _EXTERNAL      Section  __sections[10] ;       /* Список секций */
  _EXTERNAL          int  __sections_cnt ;

  _EXTERNAL          int  __sec_work ;           /* Индекс рабочий секции */

  _EXTERNAL       time_t  __sec_change_time ;    /* Время последнего переключения секций */

/*------------------------------------------------------ База данных */

  _EXTERNAL      char  __db_name[512] ;
  _EXTERNAL      char  __db_user[512] ;
  _EXTERNAL      char  __db_password[512] ;

  _EXTERNAL       int  __db_errors_cnt ;

/*---------------------------------------- Главный управляющий поток */

  _EXTERNAL   DWORD  hMainQueue_PID ;

  _EXTERNAL  HANDLE  hMain_Thread ;
  _EXTERNAL   DWORD  hMain_PID ;
  _EXTERNAL    HWND  hMain_Dialog ;

  _EXTERNAL  HANDLE  hParent_Process ;

/*---------------------------------- Модуль "Системная конфигурация" */

  _EXTERNAL     int  __sc_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __sc_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hSysConfig_Thread ;
  _EXTERNAL   DWORD  hSysConfig_PID ;
  _EXTERNAL    HWND  hSysConfig_Dialog ;

   typedef struct {
                       char  key[128] ;
                       char  value[128] ;
                  } SysPar ;

    struct Sc_action {
                       char  id[32] ;
                       char  action[128] ;
                       char  object[128] ;
                       char  object_type[128] ;
                       char  executor[128] ;
                       char  data[1024] ;
                       char  status[16] ;
                       char  reply[1024] ;
                       char  master_id[2048] ;
                       char  error[2048] ;
                     } ;

#define  _SC_ACTIONS_MAX  10000

  _EXTERNAL  struct Sc_action *__sc_actions[_SC_ACTIONS_MAX] ;

  _EXTERNAL              char  __scan_exclude[1000] ;  /* Список адресов исключаемых из регистрации в БД */
  _EXTERNAL               int  __scan_rush ;           /* Признак режима догона */

/*----------------------------------------- Модуль "Сканер блокчейн" */

  _EXTERNAL     int  __sn_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __sn_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hScan_Thread ;
  _EXTERNAL   DWORD  hScan_PID ;
  _EXTERNAL    HWND  hScan_Dialog ;

  _EXTERNAL    long  __block_last ;
  _EXTERNAL    long  __block_db ;
  _EXTERNAL    char  __block_db_hash[128] ;

    struct Sn_block {
                       char  number[128] ;
                       char  hash[128] ;
                       char  timestamp[128] ;
                    } ;

    struct Sn_record {
                       char  hash[128] ;
                       char  from[128] ;
                       char  to[128] ;
                       char  value[128] ;
                     } ;

    struct Sn_event {
                       char  id[128] ;
                       char  address[128] ;
                       char  topic[128] ;
                       char  transaction[128] ;
                       char  data[2000] ;
                        int  flag ;
                    } ;

#define  _SN_RECORDS_MAX  1000

  _EXTERNAL  struct Sn_record *__sn_records ;
  _EXTERNAL  struct  Sn_event *__sn_events ;

  _EXTERNAL               int  __events_deep ;   /* Глубина хранения событий, дней */
  _EXTERNAL               int  __events_force ;  /* Признак формированию оперяций по событиям независимо от состоянию очереди операций */

/*------------------------------ Модуль "Централизованный монторинг" */

  _EXTERNAL     int  __net_locked ;          /* Признак наличия проблем в сети */
  _EXTERNAL     int  __db_locked ;           /* Признак наличия проблем в работе с базой данных */

  _EXTERNAL     int  __st_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __st_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hSentry_Thread ;
  _EXTERNAL   DWORD  hSentry_PID ;
  _EXTERNAL    HWND  hSentry_Dialog ;

  _EXTERNAL    char  __mon_state_path[FILENAME_MAX] ;  /* Путь файла состояния */
  _EXTERNAL    char  __mon_node_id[128] ;              /* Идентификатор узла */
  _EXTERNAL     int  __mon_bh_pulse ;                  /* Периодичность проверки блокчейн */
  _EXTERNAL     int  __mon_dfs_pulse ;                 /* Периодичность проверки DFS */
  _EXTERNAL     int  __mon_alive_pulse ;               /* Периодичность передачи ALIVE-сообщения */
  _EXTERNAL    char  __mon_basic_account[128] ;        /* Базовый счет узла */
  _EXTERNAL    char  __mon_basic_password[128] ;       /* Пароль базового счета узла */
  _EXTERNAL    char  __mon_balance_limit[128] ;        /* Лимит баланса базового счета */
  _EXTERNAL    char  __mon_alive_address[128] ;        /* Адрес смарт-контракта централизованного мониторинга */
  _EXTERNAL     int  __mon_blocks_delay ;              /* Допустимая задержка в принятии новых блоков */
  _EXTERNAL     int  __mon_gen_period ;                /* Периодичность генерации новых блоков блокчейн, секунд */

/*----------------------------------------------- Модуль "Участники" */

  _EXTERNAL     int  __mb_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __mb_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hMembers_Thread ;
  _EXTERNAL   DWORD  hMembers_PID ;
  _EXTERNAL    HWND  hMembers_Dialog ;

   typedef struct {
                       char  id[128] ;
                       char  key[128] ;
                       char  name[128] ;
                       char  lock[128] ;
                       char  role[128] ;
                       char  bc_id[128] ;
                       char  account[128] ;
                       char  box[128] ;
                       char  sign[128] ;
                       char  cert_type[128] ;
                       char  cert_link[512] ;
                       char  data_link[512] ;
                       char  version[128] ;
                        int  flag ;
                  } Member ;

   typedef struct {
                       char  id[128] ;
                       char  kind[128] ;
                       char  file_ext[128] ;
                       char  local_path[FILENAME_MAX] ;
                       char  dfs_path[256] ;
                       char  hash[128] ;
                       char  uid[128] ;
                       char  actor_id[128] ;
                        int  flag ;
                  } MemberFile ;

#define  _MB_FILES_MAX  50

    struct Mb_action {
                       char  id[32] ;
                       char  action[128] ;
                       char  object[128] ;
                       char  object_type[128] ;
                       char  executor[128] ;
                       char  data[1024] ;
                       char  status[16] ;
                       char  reply[1024] ;
                       char  master_id[2048] ;
                       char  error[2048] ;
                     } ;

#define  _MB_ACTIONS_MAX  1000

  _EXTERNAL  struct Mb_action *__mb_actions[_MB_ACTIONS_MAX] ;

/*--------------------------------------------------- Модуль "Файлы" */

  _EXTERNAL     int  __fl_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __fl_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL     int  __files_delivery ;      /* Флаг узла-распространителя файлов */

  _EXTERNAL     int  __fl_file_idx ;         /* Рабочий номер файла */

  _EXTERNAL  HANDLE  hFiles_Thread ;
  _EXTERNAL   DWORD  hFiles_PID ;
  _EXTERNAL    HWND  hFiles_Dialog ;

    struct Fl_action {
                       char  id[32] ;
                       char  action[128] ;
                       char  object_path[FILENAME_MAX] ;
                       char  local_path[FILENAME_MAX] ;
                       char  dfs_path[128] ;
                       char  executor[128] ;
                       char  receivers[1024] ;
                       char  status[16] ;
                       char  reply[1024] ;
                       char  master_id[2048] ;
                       char  error[2048] ;
                     } ;

#define  _FL_ACTIONS_MAX  1000

  _EXTERNAL  struct Fl_action *__fl_actions[_FL_ACTIONS_MAX] ;

  _EXTERNAL    char  __dfs_type[128] ;       /* Тип файлообменной системы */
  _EXTERNAL    char  __dfs_password[128] ;   /* Пароль доступа к файлообменной системе */
  _EXTERNAL     int  __swarm_pause ;         /* Пауза отправки файлов через Swarm */
  _EXTERNAL     int  __dfs_direct_only ;     /* Признак отправки файлов только через блокчейн */
  _EXTERNAL     int  __dfs_direct_max ;      /* Максимальный размер файла при отправке через блокчейн */
  _EXTERNAL    char  __dfs_direct_box[128] ; /* Адрес смарт-контракта для отправки файлов через блокчейн */

#define _DFS_DIRECT_FRAME   14000


#define  _REESTERS_CHECK_KIND  "ReestersCheck"
#define  _REESTERS_CHECK_UUID  "90001"

/*------------------------------------------------- Модуль "Сделки" */

  _EXTERNAL     int  __dl_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __dl_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hDeals_Thread ;
  _EXTERNAL   DWORD  hDeals_PID ;
  _EXTERNAL    HWND  hDeals_Dialog ;

   typedef struct {
                       char  id[128] ;
                       char  address[128] ;
                       char  channel[128] ;
                       char  kind[128] ;
                       char  uuid[128] ;
                       char  parent[128] ;
                       char  arbitration[128] ;
                       char  version[128] ;
                       char  status[128] ;
                       char  link[256] ;
                       char  data[16008] ;
                       char  remark[2048] ;
                       char  locked[128] ;
                        int  flag ;
                  } Deal ;

   typedef struct {
                       char  id[128] ;
                       char  address[128] ;
                       char  kind[128] ;
                       char  deal[128] ;
                       char  version[128] ;
                       char  status[128] ;
                       char  link[256] ;
                       char  remark[2048] ;
                       char  deal_status[128] ;
                       char  deal_remark[2048] ;
                        int  flag ;
                  } Arbitration ;

   typedef struct {
                       char  id[128] ;
                       char  key[128] ;
                       char  value[128] ;
                        int  flag ;
                  } DealAttr ;

   typedef struct {
                       char  id[128] ;
                       char  party_id[128] ;
                       char  role[128] ;
                       char  sign[128] ;
                       char  account[128] ;
                       char  box[128] ;
                        int  flag ;
                  } DealParty ; 

   typedef struct {
                       char  id[128] ;
                       char  status[128] ;
                       char  status_next[128] ;
                       char  role[128] ;
                        int  flag ;
                  } DealMap ; 

   typedef struct {
                       char  id[128] ;
                       char  parent_id[128] ;
                       char  relation[128] ;
                       char  kind[128] ;
                       char  file_ext[128] ;
                       char  remark[8192] ;
                       char  recipients[512] ;
                       char  file_uuid[128] ;
                       char  local_path[FILENAME_MAX] ;
                       char  dfs_path[128] ;
                       char  p_local_path[FILENAME_MAX] ;
                       char  p_dfs_path[128] ;
                       char  hash[128] ;
                       char  sign[128] ;
                       char  version[128] ;
                       char  status[128] ;
                        int  flag ;
                  } DealFile ;

   typedef struct {
                       char  id[128] ;
                       char  version[128] ;
                       char  status[128] ;
                       char  remark[1024] ;
                       char  actor[128] ;
                        int  flag ;
                  } DealHistory ;

    struct Dl_action {
                       char  id[32] ;
                       char  action[128] ;
                       char  object[128] ;
                       char  object_type[128] ;
                       char  executor[128] ;
                       char  data[1024] ;
                       char  status[16] ;
                       char  reply[2048] ;
                       char  master_id[2048] ;
                       char  error[2048] ;
                     } ;

#define  _DL_ACTIONS_MAX  1000

  _EXTERNAL  struct Dl_action *__dl_actions[_DL_ACTIONS_MAX] ;

/*-------------------------------------------------- Модуль "Oracle" */

  _EXTERNAL     int  __or_request_period ;   /* Пауза запросов */
  _EXTERNAL     int  __or_view_frame ;       /* Число отображаемых строк лога */

  _EXTERNAL  HANDLE  hOracle_Thread ;
  _EXTERNAL   DWORD  hOracle_PID ;
  _EXTERNAL    HWND  hOracle_Dialog ;

    struct Or_deal {
                       char  id[32] ;
                       char  address[128] ;
                       char  kind[64] ;
                       char  status[64] ;
                       char  version[128] ;
                       char  data[2048] ;
                       char  processor[2048] ;
                       char  period[128] ;
                       char  pre_version[128] ;
                       char  txn_block[64] ;
                   } ;

    struct Or_extfile {
                        char  path[FILENAME_MAX] ;
                        char  errors_folder[FILENAME_MAX] ;
                        char  processor[2048] ;
                        char  period[128] ;
                      } ;

#define  _OR_ACTIONS_MAX  1000

  _EXTERNAL  struct    Or_deal *__or_deals[_OR_ACTIONS_MAX] ;
  _EXTERNAL  struct Or_extfile *__or_extfiles[_OR_ACTIONS_MAX] ;

/*----------------------------------------------- Внешнее управление */

  _EXTERNAL     int  __external_control ;                /* Флаг разрешения внешнего управления */

/*-------------------------------------------------------- Прототипы */

/* Ethereum_Mirror.cpp */
            int  EMIR_active_section(char *) ;                          /* Проверка активных секций обработки */
            int  EMIR_system        (void) ;                            /* Обработка системных сообщений */
           void  EMIR_message       (char *) ;                          /* Система выдачи сообщений */
           void  EMIR_message       (char *, int) ;
            int  EMIR_log           (char *, char *) ;
            int  EMIR_log           (char *) ;
            int  EMIR_snapshot      (char *, char *) ;                  /* Сброс снимка данных в файл */
            int  EMIR_save_context  (char *, char *) ;                  /* Работа с файлом сохранения контекста */
            int  EMIR_db_context    (char *) ;                          /* Работа с файлом контекста БД */
            int  EMIR_mon_context   (char *) ;                          /* Работа с файлом настроек мониторинга */
            int  EMIR_db_config     (SQL_link *db) ;                    /* Считывание настроек из БД */
           char *EMIR_loadfile      (char *, char *) ;                  /* Загрузка файла в память */
           void  EMIR_Change_Section(char *, int) ;                     /* Выбор секции */
           void  EMIR_text_trim     (char *) ;                          /* Отсечка начальных и конечных пробельных символов */
            int  EMIR_create_path   (char *) ;                          /* Формирование пути к разделу */
            int  EMIR_text_subst    (char *, char *, char *, int) ;     /* Подстановка полей данных */
           void  EMIR_view_html     (char *) ;                          /* Просмотр ссылки в броузере */
   DWORD WINAPI  Main_Thread        (LPVOID) ;

/* EMIR_common.cpp */
            int  EMIR_node_exchange     (char *, char *, int) ;          /* Обмен с узлом ETHEREUM */
           void  EMIR_hex2dec           (char *, char *) ;               /* Перевод HEX в десятичное представление */
           void  EMIR_txt2hex           (char *, char *, int) ;          /* Перевод строки в HEX-представление */
           void  EMIR_txt2hex64         (char *, char *, int) ;
           void  EMIR_txt2hex128        (char *, char *, int) ;
           void  EMIR_bin2hex           (char *, char *, int) ;
           void  EMIR_hex2txt           (char *, char *) ;               /* Перевод HEX-представления в строку */
            int  EMIR_check_hex         (char *) ;                       /* Проверка состава символов на HEX-алфавит */
            int  EMIR_file_hash         (char *, char *, char *) ;       /* Расчет хэша файла */
           void  EMIR_toUTF8            (char *, char *) ;               /* Кодировка в UTF-8 */
           void  EMIR_fromUTF8          (char *, char *) ;               /* Кодировка из UTF-8 */
            int  EMIR_compare_files     (char *, char *) ;               /* Сравнение файлов */
            int  EMIR_node_getbalance   (char *, char *,                 /* Запрос баланса счёта */
                                                 char *, char *) ;
            int  EMIR_node_getcode      (char *, char *, int, char *) ;  /* Запрос кода программы контракта */
            int  EMIR_node_checkgas     (char *, char *,                 /* Запрос стоимости исполнения программы контракта */
                                         char *, char *, char *) ;
            int  EMIR_node_publcontract (char *, char *,                 /* Создание контракта */
                                                 char *, char *, char *) ;
            int  EMIR_node_sendcontract (char *, char *,                 /* Исполнение метода контракта */
                                                 char *, char *, char *, char *) ;
            int  EMIR_node_checktxn     (char *, char *,                 /* Проверка транзакции */
                                                 char *, char * ) ;
            int  EMIR_node_unlockaccount(char *, char *,
                                                 char *, int, char *) ;
            int  EMIR_node_lastblock    (char *, char *) ;               /* Запрос номера последнего блока */
            int  EMIR_node_getblockh    (char *, struct Sn_block *,      /* Запрос заголовка данных блока по номеру */
                                                struct Sn_record *, int, int, char *) ;
            int  EMIR_node_getevents    (char *,                         /* Запрос событий блока по номеру */
                                                struct Sn_event *, int, int, char *) ;
            int  EMIR_node_getversion   (char *, char *, char *) ;       /* Запрос версии смарт-контракта */

            int  EMIR_dfs_putfile       (char *, char *, char *) ;       /* Загрузка файла в DFS */
            int  EMIR_dfs_getfile       (char *, char *, char *) ;       /* Получение файла из DFS */

            int  EMIR_db_syspar         (SQL_link *,                     /* Запрос конфигурационного параметра */
                                             char *, char *, char *) ;
            int  EMIR_db_nodepars       (SQL_link *, char *) ;           /* Запрос учетных реквизитов узла */
           void  EMIR_text_random       (char *, int) ;                  /* Формирование случайной HEX-строки */
           void  EMIR_text_gamma        (char *, char *) ;               /* Шифрование гаммой */
           void  EMIR_uuid_generation   (char *) ;                       /* Формирование UUID */

/* EMIR_crypto.cpp */
            int  EMIR_crypto_cert       (char *, char *, char *) ;       /* Регистрация сертификата шифрования */
            int  EMIR_crypto_inpack     (char *, char *,                 /* Формирование шифро-пакета */
                                                 char *, char *) ; 
            int  EMIR_crypto_unpack     (char *, char *,                 /* Распаковка шифро-пакета */
                                                 char *, char *) ; 
            int  EMIR_crypto_sign       (char *, char *,                 /* Формирование подписи файла */
                                                 char *, char *) ; 
            int  EMIR_gamma_inpack      (char *, char *,                 /* Шифрование на пароле */
                                                 char *, char *) ; 
            int  EMIR_gamma_unpack      (char *, char *,                 /* Дешифрование на пароле */
                                                 char *, char *) ; 

/* EMIR_sysconfig.cpp */
  INT_PTR CALLBACK  EMIR_sysconfig_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  SysConfig_Thread     (LPVOID) ;
              void  SysConfig_Process    (SQL_link *) ;

/* EMIR_members.cpp */
  INT_PTR CALLBACK  EMIR_members_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Members_Thread     (LPVOID) ;
              void  Members_Process    (SQL_link *) ;

/* EMIR_deals.cpp */
  INT_PTR CALLBACK  EMIR_deals_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Deals_Thread     (LPVOID) ;
              void  Deals_Process    (SQL_link *) ;

/* EMIR_files.cpp */
  INT_PTR CALLBACK  EMIR_files_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Files_Thread     (LPVOID) ;
              void  Files_Process    (SQL_link *) ;

/* EMIR_scan.cpp */
  INT_PTR CALLBACK  EMIR_scan_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Scan_Thread     (LPVOID) ;
              void  Scan_Process    (SQL_link *, char *) ;

/* EMIR_sentry.cpp */
  INT_PTR CALLBACK  EMIR_sentry_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Sentry_Thread     (LPVOID) ;
              void  Sentry_Process    (SQL_link *) ;

/* EMIR_oracle.cpp */
  INT_PTR CALLBACK  EMIR_oracle_dialog(HWND, UINT, WPARAM, LPARAM) ;
      DWORD WINAPI  Oracle_Thread     (LPVOID) ;
              void  Oracle_Process    (SQL_link *) ;
