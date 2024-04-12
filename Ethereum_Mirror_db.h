/*********************************************************************/
/*                                                                   */
/*      Синхронизация объектов БД с Ethereum по RPC-протоколу        */
/*                                                                   */
/*                     Назначение объектов БД                        */
/*                                                                   */
/*********************************************************************/

#ifdef  __MAIN__
#define  _EXTERNAL
#else
#define  _EXTERNAL  extern
#endif

/*-------------------------------------------------- Общие настройки */

  _EXTERNAL            char  __db_char_sep[128] ; 

#define    _DB_CHAR_SEPARATOR   "'"

/*---------------------------------- Модуль "Системная конфигурация" */

  _EXTERNAL            char  __db_table_system_configuration[128] ;
  _EXTERNAL            char  __db_table_system_pars         [128] ;
  _EXTERNAL            char  __db_table_system_alert        [128] ;
  _EXTERNAL            char  __db_table_system_actions      [128] ;

  _EXTERNAL            char  __db_table_members_actions     [128] ;
  _EXTERNAL            char  __db_table_members             [128] ;
  _EXTERNAL            char  __db_table_members_files       [128] ;
  _EXTERNAL            char  __db_table_clients             [128] ;
  _EXTERNAL            char  __db_table_clients_files       [128] ;
  _EXTERNAL            char  __db_table_client2factor       [128] ;

  _EXTERNAL            char  __db_table_deals_actions       [128] ;
  _EXTERNAL            char  __db_table_deals               [128] ;
  _EXTERNAL            char  __db_table_deals_files         [128] ;
  _EXTERNAL            char  __db_table_deals_parties       [128] ;
  _EXTERNAL            char  __db_table_deals_history       [128] ;
  _EXTERNAL            char  __db_table_deals_attributes    [128] ;
  _EXTERNAL            char  __db_table_deals_statusmap     [128] ;
  _EXTERNAL            char  __db_table_deals_arbitration   [128] ;

  _EXTERNAL            char  __db_table_files_actions       [128] ;

  _EXTERNAL            char  __db_table_scan_state          [128] ;
  _EXTERNAL            char  __db_table_scan_transactions   [128] ;
  _EXTERNAL            char  __db_table_scan_accounts       [128] ;
  _EXTERNAL            char  __db_table_scan_events         [128] ;

  _EXTERNAL            char  __db_table_sentry_pars         [128] ;
  _EXTERNAL            char  __db_table_sentry_state        [128] ;
  _EXTERNAL            char  __db_table_sentry_nodes        [128] ;

  _EXTERNAL            char  __db_table_oracle_processor    [128] ;

  _EXTERNAL            char  __db_table_ext_control         [128] ;

#define    _TABLE_SYSTEM_CONFIGURATION   "public.\"SYSTEM_CONFIGURATION\""
#define    _TABLE_SYSTEM_PARS            "public.\"SYSTEM_PARS\""
#define    _TABLE_SYSTEM_ALERT           "public.\"SYSTEM_ALERT\""
#define    _TABLE_SYSTEM_ACTIONS         "public.\"SYSTEM_ACTIONS\""

#define    _TABLE_MEMBERS_ACTIONS        "public.\"MEMBERS_ACTIONS\""
#define    _TABLE_MEMBERS                "public.\"MEMBERS\""
#define    _TABLE_MEMBERS_FILES          "public.\"MEMBERS_FILES\""

#define    _TABLE_DEALS_ACTIONS          "public.\"DEALS_ACTIONS\""
#define    _TABLE_DEALS                  "public.\"DEALS\""
#define    _TABLE_DEALS_FILES            "public.\"DEALS_FILES\""
#define    _TABLE_DEALS_PARTIES          "public.\"DEALS_PARTIES\""
#define    _TABLE_DEALS_HISTORY          "public.\"DEALS_HISTORY\""
#define    _TABLE_DEALS_ATTRIBUTES       "public.\"DEALS_ATTRIBUTES\""
#define    _TABLE_DEALS_STATUSMAP        "public.\"DEALS_STATUSMAP\""
#define    _TABLE_DEALS_ARBITRATION      "public.\"DEALS_ARBITRATION\""

#define    _TABLE_FILES_ACTIONS          "public.\"FILES_ACTIONS\""

#define    _TABLE_SCAN_STATE             "public.\"SCAN_STATE\""
#define    _TABLE_SCAN_TRANSACTIONS      "public.\"SCAN_TRANSACTIONS\""
#define    _TABLE_SCAN_ACCOUNTS          "public.\"SCAN_ACCOUNTS\""
#define    _TABLE_SCAN_EVENTS            "public.\"SCAN_EVENTS\""

#define    _TABLE_SENTRY_PARS            "public.\"SENTRY_PARS\""
#define    _TABLE_SENTRY_STATE           "public.\"SENTRY_STATE\""
#define    _TABLE_SENTRY_NODES           "public.\"SENTRY_NODES\""

#define    _TABLE_ORACLE_PROCESSOR       "public.\"ORACLE_PROCESSOR\""

#define    _TABLE_EXT_CONTROL            "public.\"EXTERNAL_CONTROL\""
