/*********************************************************************/
/*                                                                   */
/*      ������������� �������� �� � Ethereum �� RPC-���������        */
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

/*---------------------------------------------- ��������� ��������� */

#define   _APPLICATION     "Ethereum_Mirror"
#define   _PROGRAM_TITLE   "Ethereum_Mirror"

#undef    _VERSION
#define   _VERSION   "09.04.2024"

/*--------------------------------------------------- ������� ������ */

  _EXTERNAL   int  __exit_flag ;             /* ���� ���������� ������ */
  _EXTERNAL  long  __exit_time ;             /* ����� ������ */

/*---------------------------------- ���� ���������������� ��������� */

#define    _USER_SECTION_ENABLE    1
#define    _USER_RELOAD            2

/*-------------------------------------------------- ���������� ���� */

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

  _EXTERNAL            UINT  TaskBar_Msg ;        /* ��������� ����������� ����� � TaskBar */
  _EXTERNAL  NOTIFYICONDATA  TbIcon ;             /* �������� TaskBar-������ */

  _EXTERNAL       HINSTANCE  hInst ;
  _EXTERNAL        WNDCLASS  FrameWindow ;
  _EXTERNAL            HWND  hFrameWindow ;

  _EXTERNAL            HWND  __dialog ;

  _EXTERNAL             int  __window_closed ;    /* ���� ������� ���� � ���� */

  _EXTERNAL             int  __console_process ;  /* ���� ������ � ���������� ������ */

#define                    _NO_ICON              0
#define               _WARNING_ICON              1
#define                 _ERROR_ICON              2
#define            _G_COMPLETE_ICON              3
#define            _B_COMPLETE_ICON              4
#define            _S_COMPLETE_ICON              5
#define              _EXECUTED_ICON              6
#define                _MANUAL_ICON              7

#endif

/*------------------------------------------------- ����� ���������� */

  _EXTERNAL            char  __cwd[FILENAME_MAX] ;               /* ������� ������ */

  _EXTERNAL            char  __signal_path[FILENAME_MAX] ;       /* ���� ����������� ����� */
  _EXTERNAL            char  __context_path[FILENAME_MAX] ;      /* ���� ����� ���������� ��������� */
  _EXTERNAL            char  __db_context_path[FILENAME_MAX] ;   /* ���� ����� ��������� �� */
  _EXTERNAL            char  __log_path[FILENAME_MAX] ;          /* ����� ���� ����� ������ ���� */
  _EXTERNAL            char  __rpc_path[FILENAME_MAX] ;          /* ����� ���� ����� ���� ������ �� RPC */

  _EXTERNAL             int  __log_level ;                       /* ������� ������ ���� */
  _EXTERNAL             int  __log_rotation ;                    /* ����� ������� ����� */

  _EXTERNAL             int  __critical_stop ;                   /* ���� ��������� ��� ����������� ��� ����������� ������� */

  _EXTERNAL            char  __mon_context_path[FILENAME_MAX] ;  /* ���� ����� �������� ����������� */
  _EXTERNAL            char  __monitoring_path[FILENAME_MAX] ;   /* ���� ����� ����������� */
  _EXTERNAL            char  __monitoring_rules[FILENAME_MAX] ;  /* ����������� ������� ����������� */
  _EXTERNAL            char  __monitoring_format[FILENAME_MAX] ; /* ������ ����� ����������� */

  _EXTERNAL            char  __file_storage[FILENAME_MAX] ;      /* ����� ��������� ��������� */
  _EXTERNAL            char  __cert_storage[FILENAME_MAX] ;      /* ����� �������� ������ ������������ ���������� ����� ��������� */
  _EXTERNAL            char  __members_storage[FILENAME_MAX] ;   /* ����� �������� ������ ���������� */
  _EXTERNAL            char  __work_folder[FILENAME_MAX] ;       /* ������� ����� */
  _EXTERNAL            char  __reports_folder[FILENAME_MAX] ;    /* ����� �������� ������� */
  _EXTERNAL            char  __dcl_storage[FILENAME_MAX] ;       /* ����� �������� DCL-�������� */

  _EXTERNAL            char  __node_url[FILENAME_MAX] ;          /* URL ���� Ethereum */
  _EXTERNAL            char  __swarm_url[512] ;                  /* URL ���� Swarm */
  _EXTERNAL            char  __ipfs_url[512] ;                   /* URL ���� IPFS */
  _EXTERNAL            char  __dfs_url[512] ;                    /* URL �������� DFS */
  _EXTERNAL            char  __curl_path[FILENAME_MAX] ;         /* ���� � ������� CURL */

  _EXTERNAL            char  __member_account[128] ;             /* ����� ����� ���� */
  _EXTERNAL            char  __member_password[128] ;            /* ������ ����� ���� */
  _EXTERNAL            char  __member_executor[256] ;            /* �����:������ ����� ���� */
  _EXTERNAL            char  __member_box[128] ;                 /* ����� �/� ���� */
  _EXTERNAL            char  __member_sign[128] ;                /* ��� ����������� ���������� ����� ���� */
  _EXTERNAL            char  __member_key[128] ;                 /* ������������� ���� */

  _EXTERNAL            char  __gas_value[128] ;                  /* ��������� �������� ���� ��� ����������, ���� 0 - ������������� ����������� */

  _EXTERNAL             int  __purge_completed ;                 /* �������� �������� ��� �������� ���������� */
  _EXTERNAL             int  __purge_deep ;                      /* ������� �������� ��������, ���� */

  _EXTERNAL            char  __net_type[128] ;                   /* ��� ����: Ethereum, Quorum */

#define   _NULL_ADDR  "0000000000000000000000000000000000000000"

/*----------------------------------------------------- ������������ */

  typedef struct {
                    char  type[32] ;        /* ��� ������-������� */
                    char  cert[1024] ;      /* ������ ����������� ����������� ��������� ����� */
                    char  pack[1024] ;      /* ������ ������������ ������-������ �� ����������� */
                    char  unpack[1024] ;    /* ������ ���������� ������-������ */
                    char  sign[1024] ;      /* ������ ������������ ������� */
                    char  check[1024] ;     /* ������ �������� ������� */
                 } Crypto ;
 
#define  _CRYPTO_MAX  10

  _EXTERNAL          Crypto __crypto[_CRYPTO_MAX] ;              /* ������������ ����� ������������ */

  _EXTERNAL            char  __crypto_cert[FILENAME_MAX] ;       /* ������ ��������� ����������� ������� */
  _EXTERNAL            char  __crypto_sign[FILENAME_MAX] ;       /* ������ ������������ ������� */
  _EXTERNAL            char  __crypto_check[FILENAME_MAX] ;      /* ������ �������� ������� */
  _EXTERNAL            char  __crypto_pack[FILENAME_MAX] ;       /* ������ ������������ �����-������ */
  _EXTERNAL            char  __crypto_unpack[FILENAME_MAX] ;     /* ������ �������� �����-������ */
  _EXTERNAL            char  __gamma_pack[FILENAME_MAX] ;        /* ������ ���������� �� ������ */
  _EXTERNAL            char  __gamma_unpack[FILENAME_MAX] ;      /* ������ ������������ �� ������ */

/*-------------------------------------------------- �������� ������ */

  _EXTERNAL            char  __active[1024] ;    /* �������� �������� ������ */

  typedef struct {
                    HWND  hWnd ;           /* ���������� ���� */
                    char  title[128] ;     /* ��������� */           
                 } Section ;

  _EXTERNAL      Section  __sections[10] ;       /* ������ ������ */
  _EXTERNAL          int  __sections_cnt ;

  _EXTERNAL          int  __sec_work ;           /* ������ ������� ������ */

  _EXTERNAL       time_t  __sec_change_time ;    /* ����� ���������� ������������ ������ */

/*------------------------------------------------------ ���� ������ */

  _EXTERNAL      char  __db_name[512] ;
  _EXTERNAL      char  __db_user[512] ;
  _EXTERNAL      char  __db_password[512] ;

  _EXTERNAL       int  __db_errors_cnt ;

/*---------------------------------------- ������� ����������� ����� */

  _EXTERNAL   DWORD  hMainQueue_PID ;

  _EXTERNAL  HANDLE  hMain_Thread ;
  _EXTERNAL   DWORD  hMain_PID ;
  _EXTERNAL    HWND  hMain_Dialog ;

  _EXTERNAL  HANDLE  hParent_Process ;

/*---------------------------------- ������ "��������� ������������" */

  _EXTERNAL     int  __sc_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __sc_view_frame ;       /* ����� ������������ ����� ���� */

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

  _EXTERNAL              char  __scan_exclude[1000] ;  /* ������ ������� ����������� �� ����������� � �� */
  _EXTERNAL               int  __scan_rush ;           /* ������� ������ ������ */

/*----------------------------------------- ������ "������ ��������" */

  _EXTERNAL     int  __sn_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __sn_view_frame ;       /* ����� ������������ ����� ���� */

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

  _EXTERNAL               int  __events_deep ;   /* ������� �������� �������, ���� */
  _EXTERNAL               int  __events_force ;  /* ������� ������������ �������� �� �������� ���������� �� ��������� ������� �������� */

/*------------------------------ ������ "���������������� ���������" */

  _EXTERNAL     int  __net_locked ;          /* ������� ������� ������� � ���� */
  _EXTERNAL     int  __db_locked ;           /* ������� ������� ������� � ������ � ����� ������ */

  _EXTERNAL     int  __st_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __st_view_frame ;       /* ����� ������������ ����� ���� */

  _EXTERNAL  HANDLE  hSentry_Thread ;
  _EXTERNAL   DWORD  hSentry_PID ;
  _EXTERNAL    HWND  hSentry_Dialog ;

  _EXTERNAL    char  __mon_state_path[FILENAME_MAX] ;  /* ���� ����� ��������� */
  _EXTERNAL    char  __mon_node_id[128] ;              /* ������������� ���� */
  _EXTERNAL     int  __mon_bh_pulse ;                  /* ������������� �������� �������� */
  _EXTERNAL     int  __mon_dfs_pulse ;                 /* ������������� �������� DFS */
  _EXTERNAL     int  __mon_alive_pulse ;               /* ������������� �������� ALIVE-��������� */
  _EXTERNAL    char  __mon_basic_account[128] ;        /* ������� ���� ���� */
  _EXTERNAL    char  __mon_basic_password[128] ;       /* ������ �������� ����� ���� */
  _EXTERNAL    char  __mon_balance_limit[128] ;        /* ����� ������� �������� ����� */
  _EXTERNAL    char  __mon_alive_address[128] ;        /* ����� �����-��������� ����������������� ����������� */
  _EXTERNAL     int  __mon_blocks_delay ;              /* ���������� �������� � �������� ����� ������ */
  _EXTERNAL     int  __mon_gen_period ;                /* ������������� ��������� ����� ������ ��������, ������ */

/*----------------------------------------------- ������ "���������" */

  _EXTERNAL     int  __mb_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __mb_view_frame ;       /* ����� ������������ ����� ���� */

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

/*--------------------------------------------------- ������ "�����" */

  _EXTERNAL     int  __fl_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __fl_view_frame ;       /* ����� ������������ ����� ���� */

  _EXTERNAL     int  __files_delivery ;      /* ���� ����-���������������� ������ */

  _EXTERNAL     int  __fl_file_idx ;         /* ������� ����� ����� */

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

  _EXTERNAL    char  __dfs_type[128] ;       /* ��� ������������� ������� */
  _EXTERNAL    char  __dfs_password[128] ;   /* ������ ������� � ������������� ������� */
  _EXTERNAL     int  __swarm_pause ;         /* ����� �������� ������ ����� Swarm */
  _EXTERNAL     int  __dfs_direct_only ;     /* ������� �������� ������ ������ ����� �������� */
  _EXTERNAL     int  __dfs_direct_max ;      /* ������������ ������ ����� ��� �������� ����� �������� */
  _EXTERNAL    char  __dfs_direct_box[128] ; /* ����� �����-��������� ��� �������� ������ ����� �������� */

#define _DFS_DIRECT_FRAME   14000


#define  _REESTERS_CHECK_KIND  "ReestersCheck"
#define  _REESTERS_CHECK_UUID  "90001"

/*------------------------------------------------- ������ "������" */

  _EXTERNAL     int  __dl_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __dl_view_frame ;       /* ����� ������������ ����� ���� */

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

/*-------------------------------------------------- ������ "Oracle" */

  _EXTERNAL     int  __or_request_period ;   /* ����� �������� */
  _EXTERNAL     int  __or_view_frame ;       /* ����� ������������ ����� ���� */

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

/*----------------------------------------------- ������� ���������� */

  _EXTERNAL     int  __external_control ;                /* ���� ���������� �������� ���������� */

/*-------------------------------------------------------- ��������� */

/* Ethereum_Mirror.cpp */
            int  EMIR_active_section(char *) ;                          /* �������� �������� ������ ��������� */
            int  EMIR_system        (void) ;                            /* ��������� ��������� ��������� */
           void  EMIR_message       (char *) ;                          /* ������� ������ ��������� */
           void  EMIR_message       (char *, int) ;
            int  EMIR_log           (char *, char *) ;
            int  EMIR_log           (char *) ;
            int  EMIR_snapshot      (char *, char *) ;                  /* ����� ������ ������ � ���� */
            int  EMIR_save_context  (char *, char *) ;                  /* ������ � ������ ���������� ��������� */
            int  EMIR_db_context    (char *) ;                          /* ������ � ������ ��������� �� */
            int  EMIR_mon_context   (char *) ;                          /* ������ � ������ �������� ����������� */
            int  EMIR_db_config     (SQL_link *db) ;                    /* ���������� �������� �� �� */
           char *EMIR_loadfile      (char *, char *) ;                  /* �������� ����� � ������ */
           void  EMIR_Change_Section(char *, int) ;                     /* ����� ������ */
           void  EMIR_text_trim     (char *) ;                          /* ������� ��������� � �������� ���������� �������� */
            int  EMIR_create_path   (char *) ;                          /* ������������ ���� � ������� */
            int  EMIR_text_subst    (char *, char *, char *, int) ;     /* ����������� ����� ������ */
           void  EMIR_view_html     (char *) ;                          /* �������� ������ � �������� */
   DWORD WINAPI  Main_Thread        (LPVOID) ;

/* EMIR_common.cpp */
            int  EMIR_node_exchange     (char *, char *, int) ;          /* ����� � ����� ETHEREUM */
           void  EMIR_hex2dec           (char *, char *) ;               /* ������� HEX � ���������� ������������� */
           void  EMIR_txt2hex           (char *, char *, int) ;          /* ������� ������ � HEX-������������� */
           void  EMIR_txt2hex64         (char *, char *, int) ;
           void  EMIR_txt2hex128        (char *, char *, int) ;
           void  EMIR_bin2hex           (char *, char *, int) ;
           void  EMIR_hex2txt           (char *, char *) ;               /* ������� HEX-������������� � ������ */
            int  EMIR_check_hex         (char *) ;                       /* �������� ������� �������� �� HEX-������� */
            int  EMIR_file_hash         (char *, char *, char *) ;       /* ������ ���� ����� */
           void  EMIR_toUTF8            (char *, char *) ;               /* ��������� � UTF-8 */
           void  EMIR_fromUTF8          (char *, char *) ;               /* ��������� �� UTF-8 */
            int  EMIR_compare_files     (char *, char *) ;               /* ��������� ������ */
            int  EMIR_node_getbalance   (char *, char *,                 /* ������ ������� ����� */
                                                 char *, char *) ;
            int  EMIR_node_getcode      (char *, char *, int, char *) ;  /* ������ ���� ��������� ��������� */
            int  EMIR_node_checkgas     (char *, char *,                 /* ������ ��������� ���������� ��������� ��������� */
                                         char *, char *, char *) ;
            int  EMIR_node_publcontract (char *, char *,                 /* �������� ��������� */
                                                 char *, char *, char *) ;
            int  EMIR_node_sendcontract (char *, char *,                 /* ���������� ������ ��������� */
                                                 char *, char *, char *, char *) ;
            int  EMIR_node_checktxn     (char *, char *,                 /* �������� ���������� */
                                                 char *, char * ) ;
            int  EMIR_node_unlockaccount(char *, char *,
                                                 char *, int, char *) ;
            int  EMIR_node_lastblock    (char *, char *) ;               /* ������ ������ ���������� ����� */
            int  EMIR_node_getblockh    (char *, struct Sn_block *,      /* ������ ��������� ������ ����� �� ������ */
                                                struct Sn_record *, int, int, char *) ;
            int  EMIR_node_getevents    (char *,                         /* ������ ������� ����� �� ������ */
                                                struct Sn_event *, int, int, char *) ;
            int  EMIR_node_getversion   (char *, char *, char *) ;       /* ������ ������ �����-��������� */

            int  EMIR_dfs_putfile       (char *, char *, char *) ;       /* �������� ����� � DFS */
            int  EMIR_dfs_getfile       (char *, char *, char *) ;       /* ��������� ����� �� DFS */

            int  EMIR_db_syspar         (SQL_link *,                     /* ������ ����������������� ��������� */
                                             char *, char *, char *) ;
            int  EMIR_db_nodepars       (SQL_link *, char *) ;           /* ������ ������� ���������� ���� */
           void  EMIR_text_random       (char *, int) ;                  /* ������������ ��������� HEX-������ */
           void  EMIR_text_gamma        (char *, char *) ;               /* ���������� ������ */
           void  EMIR_uuid_generation   (char *) ;                       /* ������������ UUID */

/* EMIR_crypto.cpp */
            int  EMIR_crypto_cert       (char *, char *, char *) ;       /* ����������� ����������� ���������� */
            int  EMIR_crypto_inpack     (char *, char *,                 /* ������������ �����-������ */
                                                 char *, char *) ; 
            int  EMIR_crypto_unpack     (char *, char *,                 /* ���������� �����-������ */
                                                 char *, char *) ; 
            int  EMIR_crypto_sign       (char *, char *,                 /* ������������ ������� ����� */
                                                 char *, char *) ; 
            int  EMIR_gamma_inpack      (char *, char *,                 /* ���������� �� ������ */
                                                 char *, char *) ; 
            int  EMIR_gamma_unpack      (char *, char *,                 /* ������������ �� ������ */
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
