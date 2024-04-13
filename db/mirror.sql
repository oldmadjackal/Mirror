--
-- PostgreSQL database dump
--

-- Dumped from database version 10.12
-- Dumped by pg_dump version 10.12

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: DATABASE mirror3; Type: COMMENT; Schema: -; Owner: ftp_owner
--

-- COMMENT ON DATABASE mirror3 IS 'RSK';


--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- Name: BCHAIN_ACCOUNTS_Id_seq; Type: SEQUENCE; Schema: public; Owner: ftp_owner
--

CREATE SEQUENCE public."BCHAIN_ACCOUNTS_Id_seq"
    START WITH 4
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."BCHAIN_ACCOUNTS_Id_seq" OWNER TO rchain;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: BCHAIN_ACCOUNTS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."BCHAIN_ACCOUNTS" (
    "Id" integer DEFAULT nextval('public."BCHAIN_ACCOUNTS_Id_seq"'::regclass) NOT NULL,
    "Account" character varying(64),
    "Balance" character varying(64),
    "TimeStamp" character varying(64)
);


ALTER TABLE public."BCHAIN_ACCOUNTS" OWNER TO rchain;

--
-- Name: TABLE "BCHAIN_ACCOUNTS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."BCHAIN_ACCOUNTS" IS 'Таблица адресов блокчейна';


--
-- Name: BCHAIN_EVENTS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."BCHAIN_EVENTS" (
    "Id" integer NOT NULL,
    "Address" character varying(64),
    "Topic" character varying(64),
    "Transaction" character varying(64),
    "TimeStamp" character varying(64),
    "Data" character varying(4000),
    "BlockNum" character varying(128),
    "BlockHash" character varying(128)
);


ALTER TABLE public."BCHAIN_EVENTS" OWNER TO rchain;

--
-- Name: TABLE "BCHAIN_EVENTS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."BCHAIN_EVENTS" IS 'Лог событий блокчейна';


--
-- Name: BCHAIN_EVENTS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."BCHAIN_EVENTS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."BCHAIN_EVENTS_Id_seq" OWNER TO rchain;

--
-- Name: BCHAIN_EVENTS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."BCHAIN_EVENTS_Id_seq" OWNED BY public."BCHAIN_EVENTS"."Id";


--
-- Name: BCHAIN_STATE; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."BCHAIN_STATE" (
    "BlockLast" character varying(32),
    "BlockDb" character varying(32),
    "Id" integer,
    "BlockDbHash" character varying(128)
);


ALTER TABLE public."BCHAIN_STATE" OWNER TO rchain;

--
-- Name: TABLE "BCHAIN_STATE"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."BCHAIN_STATE" IS 'Таблица состояния синхронизации блокчейна';


--
-- Name: BCHAIN_TRANSACTIONS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."BCHAIN_TRANSACTIONS_Id_seq"
    START WITH 4
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."BCHAIN_TRANSACTIONS_Id_seq" OWNER TO rchain;

--
-- Name: BCHAIN_TRANSACTIONS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."BCHAIN_TRANSACTIONS" (
    "Id" integer DEFAULT nextval('public."BCHAIN_TRANSACTIONS_Id_seq"'::regclass) NOT NULL,
    "Hash" character varying(64),
    "From" character varying(64),
    "To" character varying(64),
    "Value" character varying(64),
    "TimeStamp" character varying(64)
);


ALTER TABLE public."BCHAIN_TRANSACTIONS" OWNER TO rchain;

--
-- Name: TABLE "BCHAIN_TRANSACTIONS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."BCHAIN_TRANSACTIONS" IS 'Таблица транзакций блокчейна';


--
-- Name: CLIENTS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."CLIENTS" (
    "Id" integer NOT NULL,
    "Key" character varying(32),
    "Lock" character varying(32),
    "BlockChainId" character varying(100),
    "Account" character varying(100),
    "Box" character varying(100),
    "Sign" character varying(100),
    "R_Name" character varying(256),
    "R_Address" character varying(256),
    "Version" character varying(100),
    "Data" character varying(16000),
    "DataFile" character varying(100),
    "DataUpdate" character(1),
    "ActionId" integer,
    "Password" character varying(64),
    "CertFile" character varying(512)
);


ALTER TABLE public."CLIENTS" OWNER TO rchain;

--
-- Name: TABLE "CLIENTS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."CLIENTS" IS 'Таблица клиентов';


--
-- Name: CLIENTS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."CLIENTS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."CLIENTS_Id_seq" OWNER TO rchain;

--
-- Name: CLIENTS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."CLIENTS_Id_seq" OWNED BY public."CLIENTS"."Id";


--
-- Name: DEALS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS" (
    "Id" integer NOT NULL,
    "Kind" character varying(32),
    "Status" character varying(32),
    "BlockChainId" character varying(100),
    "Parent" integer,
    "ParentBlockChainId" character varying(100),
    "R_Address" character varying(256),
    "Version" character varying(100),
    "Data" character varying(16000),
    "DataFile" character varying(100),
    "DataUpdate" character(2),
    "ActionId" integer,
    "Remark" character varying(1000),
    "DealsUUID" character varying(64),
    "Locked" character(2),
    "ArbitrationKind" character varying(32),
    "ArbitrationBlockChainId" character varying(100),
    "OracleNextTime" bigint,
    "OracleData" character varying(256),
    "OracleVersion" character varying(100),
    "TxnBlock" character varying(32),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS" OWNER TO rchain;

--
-- Name: TABLE "DEALS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS" IS 'Таблица сделок';


--
-- Name: DEALS_ACTIONS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_ACTIONS" (
    "Id" integer NOT NULL,
    "Action" character varying(100),
    "Object" character varying(100),
    "ObjectType" character varying(100),
    "Executor" character varying(100),
    "Data" character varying(1000),
    "Status" character varying(10),
    "Reply" character varying(2000),
    "Error" character varying(1000),
    "MasterId" character varying(128),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_ACTIONS" OWNER TO rchain;

--
-- Name: TABLE "DEALS_ACTIONS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_ACTIONS" IS 'Таблица операций над сделками';


--
-- Name: DEALS_ACTIONS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_ACTIONS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_ACTIONS_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_ACTIONS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_ACTIONS_Id_seq" OWNED BY public."DEALS_ACTIONS"."Id";


--
-- Name: DEALS_ARBITRATION; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_ARBITRATION" (
    "Id" integer NOT NULL,
    "DealId" integer,
    "Status" character varying(32),
    "Remark" character varying(1000),
    "Data" character varying(16000),
    "DataUpdate" character varying(2),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_ARBITRATION" OWNER TO rchain;

--
-- Name: TABLE "DEALS_ARBITRATION"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_ARBITRATION" IS 'Состояние арбитража сделок';


--
-- Name: DEALS_ARBITRATION_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_ARBITRATION_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_ARBITRATION_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_ARBITRATION_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_ARBITRATION_Id_seq" OWNED BY public."DEALS_ARBITRATION"."Id";


--
-- Name: DEALS_ATTRIBUTES_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_ATTRIBUTES_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_ATTRIBUTES_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_ATTRIBUTES; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_ATTRIBUTES" (
    "Id" integer DEFAULT nextval('public."DEALS_ATTRIBUTES_Id_seq"'::regclass) NOT NULL,
    "DealId" integer,
    "Key" character varying(32),
    "Value" character varying(32),
    "DataUpdate" character varying(2),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_ATTRIBUTES" OWNER TO rchain;

--
-- Name: TABLE "DEALS_ATTRIBUTES"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_ATTRIBUTES" IS 'Таблица аттрибутов сделок';


--
-- Name: DEALS_FILES; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_FILES" (
    "Id" integer NOT NULL,
    "DealId" integer,
    "Parent" integer,
    "Relation" character varying(32),
    "Kind" character varying(32),
    "Status" character varying(32),
    "LocalPath" character varying(256),
    "DfsPath" character varying(100),
    "Hash" character varying(100),
    "Sign" character varying(100),
    "Error" character varying(256),
    "DataUpdate" character(1),
    "Version" character varying(100),
    "Remark" character varying(16000),
    "Recipients" character varying(16000),
    "FileUUID" character varying(64),
    "InsertTimestamp" timestamp with time zone DEFAULT now(),
    "FromSC" character varying(16)
);


ALTER TABLE public."DEALS_FILES" OWNER TO rchain;

--
-- Name: TABLE "DEALS_FILES"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_FILES" IS 'Таблица файлов, прилагаемых к сделкам';


--
-- Name: DEALS_FILES_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_FILES_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_FILES_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_FILES_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_FILES_Id_seq" OWNED BY public."DEALS_FILES"."Id";


--
-- Name: DEALS_HISTORY; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_HISTORY" (
    "Id" integer NOT NULL,
    "DealId" integer,
    "Version" character varying(100),
    "Status" character varying(32),
    "Remark" character varying(1000),
    "ActorId" character varying(100),
    "DataUpdate" character(1),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_HISTORY" OWNER TO rchain;

--
-- Name: TABLE "DEALS_HISTORY"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_HISTORY" IS 'Таблица истории статусов сделок';


--
-- Name: DEALS_HISTORY_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_HISTORY_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_HISTORY_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_HISTORY_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_HISTORY_Id_seq" OWNED BY public."DEALS_HISTORY"."Id";


--
-- Name: DEALS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_Id_seq" OWNED BY public."DEALS"."Id";


--
-- Name: DEALS_PARTIES; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_PARTIES" (
    "Id" integer NOT NULL,
    "DealId" integer,
    "PartyId" character varying(32),
    "Role" character varying(32),
    "DataUpdate" character(1),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_PARTIES" OWNER TO rchain;

--
-- Name: TABLE "DEALS_PARTIES"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_PARTIES" IS 'Таблица участников сделок';


--
-- Name: DEALS_PARTIES_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_PARTIES_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_PARTIES_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_PARTIES_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_PARTIES_Id_seq" OWNED BY public."DEALS_PARTIES"."Id";


--
-- Name: DEALS_STATUSMAP; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."DEALS_STATUSMAP" (
    "Id" integer NOT NULL,
    "DealId" integer,
    "Status" character varying(32),
    "StatusNext" character varying(32),
    "Role" character varying(32),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."DEALS_STATUSMAP" OWNER TO rchain;

--
-- Name: TABLE "DEALS_STATUSMAP"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."DEALS_STATUSMAP" IS 'Карты статусов сделок';


--
-- Name: DEALS_STATUSMAP_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."DEALS_STATUSMAP_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."DEALS_STATUSMAP_Id_seq" OWNER TO rchain;

--
-- Name: DEALS_STATUSMAP_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."DEALS_STATUSMAP_Id_seq" OWNED BY public."DEALS_STATUSMAP"."Id";


--
-- Name: FILES_ACTIONS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."FILES_ACTIONS" (
    "Id" integer NOT NULL,
    "Action" character varying(100),
    "LocalPath" character varying(256),
    "DfsPath" character varying(256),
    "Executor" character varying(100),
    "Receivers" character varying(1000),
    "Status" character varying(10),
    "Reply" character varying(1000),
    "Error" character varying(1000),
    "ObjectPath" character varying(256),
    "MasterId" character varying(128),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."FILES_ACTIONS" OWNER TO rchain;

--
-- Name: TABLE "FILES_ACTIONS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."FILES_ACTIONS" IS 'Таблица операций с файлами';


--
-- Name: FILES_ACTIONS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."FILES_ACTIONS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."FILES_ACTIONS_Id_seq" OWNER TO rchain;

--
-- Name: FILES_ACTIONS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."FILES_ACTIONS_Id_seq" OWNED BY public."FILES_ACTIONS"."Id";


--
-- Name: MEMBERS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."MEMBERS" (
    "Id" integer NOT NULL,
    "Key" character varying(100),
    "Lock" character varying(32),
    "Name" character varying(32),
    "Role" character varying(32),
    "Account" character varying(100),
    "Box" character varying(100),
    "Sign" character varying(100),
    "R_Address" character varying(256),
    "Version" character varying(100),
    "Data" character varying(16000),
    "DataFile" character varying(100),
    "DataUpdate" character(1),
    "ActionId" integer,
    "CertFile" character varying(512),
    "BlockChainId" character varying(128),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."MEMBERS" OWNER TO rchain;

--
-- Name: TABLE "MEMBERS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."MEMBERS" IS 'Таблица узлов-участников';


--
-- Name: MEMBERS_ACTIONS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."MEMBERS_ACTIONS" (
    "Id" integer NOT NULL,
    "Action" character varying(100),
    "Object" character varying(100),
    "ObjectType" character varying(100),
    "Executor" character varying(100),
    "Data" character varying(1000),
    "Status" character varying(10),
    "Reply" character varying(1000),
    "Error" character varying(1000),
    "MasterId" character varying(128),
    "InsertTimestamp" timestamp with time zone DEFAULT now()
);


ALTER TABLE public."MEMBERS_ACTIONS" OWNER TO rchain;

--
-- Name: TABLE "MEMBERS_ACTIONS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."MEMBERS_ACTIONS" IS 'Таблица операций над таблицей узлов-участников';


--
-- Name: MEMBERS_ACTIONS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."MEMBERS_ACTIONS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."MEMBERS_ACTIONS_Id_seq" OWNER TO rchain;

--
-- Name: MEMBERS_ACTIONS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."MEMBERS_ACTIONS_Id_seq" OWNED BY public."MEMBERS_ACTIONS"."Id";


--
-- Name: MEMBERS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."MEMBERS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."MEMBERS_Id_seq" OWNER TO rchain;

--
-- Name: MEMBERS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."MEMBERS_Id_seq" OWNED BY public."MEMBERS"."Id";


--
-- Name: ORACLE_PROCESSOR_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."ORACLE_PROCESSOR_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."ORACLE_PROCESSOR_Id_seq" OWNER TO rchain;

--
-- Name: ORACLE_PROCESSOR; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."ORACLE_PROCESSOR" (
    "Id" integer DEFAULT nextval('public."ORACLE_PROCESSOR_Id_seq"'::regclass) NOT NULL,
    "IFaceKind" character varying(32),
    "IFacePars" character varying(1000),
    "Processor" character varying(2000),
    "Period" integer,
    "OracleVersion" character varying(100),
    "IFaceKey1" character varying(32),
    "IFaceKey2" character varying(32),
    "NextTime" bigint
);


ALTER TABLE public."ORACLE_PROCESSOR" OWNER TO rchain;

--
-- Name: TABLE "ORACLE_PROCESSOR"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."ORACLE_PROCESSOR" IS 'Описание процессоров обработки состояний';


--
-- Name: SENTRY_NODES; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SENTRY_NODES" (
    "NodeId" character varying(32),
    "Address" character varying(64),
    "NodeTimestamp" character varying(32),
    "BlockLast" character varying(64),
    "DfsCheck" character varying(64),
    "Balance" character varying(64),
    "Errors" character varying(1000),
    "Pending" character varying(1000)
);


ALTER TABLE public."SENTRY_NODES" OWNER TO rchain;

--
-- Name: TABLE "SENTRY_NODES"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SENTRY_NODES" IS 'Состояние узлов платформы';


--
-- Name: SENTRY_PARS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SENTRY_PARS" (
    "NodeId" character varying(100),
    "BlockCheckTime" character varying(32),
    "DfsCheckTime" character varying(32),
    "BalanceLimit" character varying(32),
    "AliveTime" character varying(32),
    "CheckNet" character varying(32)
);


ALTER TABLE public."SENTRY_PARS" OWNER TO rchain;

--
-- Name: TABLE "SENTRY_PARS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SENTRY_PARS" IS 'Параметры системы централизованного мониторинга';


--
-- Name: SENTRY_STATE; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SENTRY_STATE" (
    "BlockLast" character varying(32),
    "BlockLastTs" character varying(32),
    "DfsCheck" character varying(64),
    "DfsCheckTs" character varying(32),
    "DfsMark" character varying(64)
);


ALTER TABLE public."SENTRY_STATE" OWNER TO rchain;

--
-- Name: TABLE "SENTRY_STATE"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SENTRY_STATE" IS 'Состояние мониторинга текущего узла';


--
-- Name: SYSTEM_ACTIONS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SYSTEM_ACTIONS" (
    "Id" integer NOT NULL,
    "Action" character varying(100),
    "Object" character varying(100),
    "ObjectType" character varying(100),
    "Executor" character varying(100),
    "Data" character varying(1000),
    "Status" character varying(10),
    "Reply" character varying(1000),
    "Error" character varying(1000),
    "AgentId" character varying(100)
);


ALTER TABLE public."SYSTEM_ACTIONS" OWNER TO rchain;

--
-- Name: TABLE "SYSTEM_ACTIONS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SYSTEM_ACTIONS" IS 'Таблица операций над блокчейном';


--
-- Name: SYSTEM_ACTIONS_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."SYSTEM_ACTIONS_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."SYSTEM_ACTIONS_Id_seq" OWNER TO rchain;

--
-- Name: SYSTEM_ACTIONS_Id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."SYSTEM_ACTIONS_Id_seq" OWNED BY public."SYSTEM_ACTIONS"."Id";


--
-- Name: SYSTEM_ADAPTER_Id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."SYSTEM_ADAPTER_Id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."SYSTEM_ADAPTER_Id_seq" OWNER TO rchain;

--
-- Name: SYSTEM_ADAPTER; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SYSTEM_ADAPTER" (
    "Id" integer DEFAULT nextval('public."SYSTEM_ADAPTER_Id_seq"'::regclass) NOT NULL,
    "Module" character varying(64),
    "AdapterId" character varying(64),
    "Conflict" character varying(2)
);


ALTER TABLE public."SYSTEM_ADAPTER" OWNER TO rchain;

--
-- Name: SYSTEM_ALERT; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SYSTEM_ALERT" (
    "Module" character varying(100),
    "Flag" character(1),
    "Info" character varying(1000),
    "Id" integer NOT NULL
);


ALTER TABLE public."SYSTEM_ALERT" OWNER TO rchain;

--
-- Name: TABLE "SYSTEM_ALERT"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SYSTEM_ALERT" IS 'Modules notification';


--
-- Name: SYSTEM_CONFIGURATION; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SYSTEM_CONFIGURATION" (
    "Key" character varying(100),
    "Value" character varying(1000),
    "ValueNew" character varying(100),
    "ActionId" integer,
    "Id" integer NOT NULL
);


ALTER TABLE public."SYSTEM_CONFIGURATION" OWNER TO rchain;

--
-- Name: TABLE "SYSTEM_CONFIGURATION"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SYSTEM_CONFIGURATION" IS 'List of configuration parameters';


--
-- Name: SYSTEM_PARS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."SYSTEM_PARS" (
    "AddressMain" character varying(100),
    "ConfigurationTemplate" character varying(100),
    "Configuration" character varying(100),
    "SystemState" character varying(100),
    "MemberAccount" character varying(100),
    "MemberPassword" character varying(100),
    "MemberBox" character varying(100),
    "Id" integer NOT NULL,
    "MemberSign" character varying(100),
    "MemberKey" character varying(32)
);


ALTER TABLE public."SYSTEM_PARS" OWNER TO rchain;

--
-- Name: TABLE "SYSTEM_PARS"; Type: COMMENT; Schema: public; Owner: rchain
--

COMMENT ON TABLE public."SYSTEM_PARS" IS 'List of system parameters';


--
-- Name: USERS; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public."USERS" (
    "ID" integer NOT NULL,
    "FIO" character varying(255),
    "certId" character varying(16000),
    "publicKey" character varying(16000),
    "memberId" integer
);


ALTER TABLE public."USERS" OWNER TO rchain;

--
-- Name: USERS_ID_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public."USERS_ID_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."USERS_ID_seq" OWNER TO rchain;

--
-- Name: USERS_ID_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public."USERS_ID_seq" OWNED BY public."USERS"."ID";


--
-- Name: databasechangelog; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public.databasechangelog (
    id character varying(255) NOT NULL,
    author character varying(255) NOT NULL,
    filename character varying(255) NOT NULL,
    dateexecuted timestamp without time zone NOT NULL,
    orderexecuted integer NOT NULL,
    exectype character varying(10) NOT NULL,
    md5sum character varying(35),
    description character varying(255),
    comments character varying(255),
    tag character varying(255),
    liquibase character varying(20),
    contexts character varying(255),
    labels character varying(255),
    deployment_id character varying(10)
);


ALTER TABLE public.databasechangelog OWNER TO rchain;

--
-- Name: databasechangeloglock; Type: TABLE; Schema: public; Owner: rchain
--

CREATE TABLE public.databasechangeloglock (
    id integer NOT NULL,
    locked boolean NOT NULL,
    lockgranted timestamp without time zone,
    lockedby character varying(255)
);


ALTER TABLE public.databasechangeloglock OWNER TO rchain;

--
-- Name: deals_id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public.deals_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.deals_id_seq OWNER TO rchain;

--
-- Name: deals_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public.deals_id_seq OWNED BY public."DEALS"."Id";


--
-- Name: system_alert_id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public.system_alert_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.system_alert_id_seq OWNER TO rchain;

--
-- Name: system_alert_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public.system_alert_id_seq OWNED BY public."SYSTEM_ALERT"."Id";


--
-- Name: system_configuration_id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public.system_configuration_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.system_configuration_id_seq OWNER TO rchain;

--
-- Name: system_configuration_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public.system_configuration_id_seq OWNED BY public."SYSTEM_CONFIGURATION"."Id";


--
-- Name: system_pars_id_seq; Type: SEQUENCE; Schema: public; Owner: rchain
--

CREATE SEQUENCE public.system_pars_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.system_pars_id_seq OWNER TO rchain;

--
-- Name: system_pars_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: rchain
--

ALTER SEQUENCE public.system_pars_id_seq OWNED BY public."SYSTEM_PARS"."Id";


--
-- Name: BCHAIN_EVENTS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."BCHAIN_EVENTS" ALTER COLUMN "Id" SET DEFAULT nextval('public."BCHAIN_EVENTS_Id_seq"'::regclass);


--
-- Name: CLIENTS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."CLIENTS" ALTER COLUMN "Id" SET DEFAULT nextval('public."CLIENTS_Id_seq"'::regclass);


--
-- Name: DEALS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_Id_seq"'::regclass);


--
-- Name: DEALS_ACTIONS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_ACTIONS" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_ACTIONS_Id_seq"'::regclass);


--
-- Name: DEALS_ARBITRATION Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_ARBITRATION" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_ARBITRATION_Id_seq"'::regclass);


--
-- Name: DEALS_FILES Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_FILES" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_FILES_Id_seq"'::regclass);


--
-- Name: DEALS_HISTORY Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_HISTORY" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_HISTORY_Id_seq"'::regclass);


--
-- Name: DEALS_PARTIES Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_PARTIES" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_PARTIES_Id_seq"'::regclass);


--
-- Name: DEALS_STATUSMAP Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_STATUSMAP" ALTER COLUMN "Id" SET DEFAULT nextval('public."DEALS_STATUSMAP_Id_seq"'::regclass);


--
-- Name: FILES_ACTIONS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."FILES_ACTIONS" ALTER COLUMN "Id" SET DEFAULT nextval('public."FILES_ACTIONS_Id_seq"'::regclass);


--
-- Name: MEMBERS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."MEMBERS" ALTER COLUMN "Id" SET DEFAULT nextval('public."MEMBERS_Id_seq"'::regclass);


--
-- Name: MEMBERS_ACTIONS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."MEMBERS_ACTIONS" ALTER COLUMN "Id" SET DEFAULT nextval('public."MEMBERS_ACTIONS_Id_seq"'::regclass);


--
-- Name: SYSTEM_ACTIONS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_ACTIONS" ALTER COLUMN "Id" SET DEFAULT nextval('public."SYSTEM_ACTIONS_Id_seq"'::regclass);


--
-- Name: SYSTEM_ALERT Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_ALERT" ALTER COLUMN "Id" SET DEFAULT nextval('public.system_alert_id_seq'::regclass);


--
-- Name: SYSTEM_CONFIGURATION Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_CONFIGURATION" ALTER COLUMN "Id" SET DEFAULT nextval('public.system_configuration_id_seq'::regclass);


--
-- Name: SYSTEM_PARS Id; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_PARS" ALTER COLUMN "Id" SET DEFAULT nextval('public.system_pars_id_seq'::regclass);


--
-- Name: USERS ID; Type: DEFAULT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."USERS" ALTER COLUMN "ID" SET DEFAULT nextval('public."USERS_ID_seq"'::regclass);


--
-- Name: BCHAIN_ACCOUNTS bchain_accounts_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."BCHAIN_ACCOUNTS"
    ADD CONSTRAINT bchain_accounts_id_pk PRIMARY KEY ("Id");


--
-- Name: BCHAIN_TRANSACTIONS bchain_transactions_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."BCHAIN_TRANSACTIONS"
    ADD CONSTRAINT bchain_transactions_id_pk PRIMARY KEY ("Id");


--
-- Name: CLIENTS clients_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."CLIENTS"
    ADD CONSTRAINT clients_id_pk PRIMARY KEY ("Id");


--
-- Name: databasechangeloglock databasechangeloglock_pkey; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public.databasechangeloglock
    ADD CONSTRAINT databasechangeloglock_pkey PRIMARY KEY (id);


--
-- Name: DEALS_ACTIONS deals_actions_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_ACTIONS"
    ADD CONSTRAINT deals_actions_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS_ATTRIBUTES deals_attributes_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_ATTRIBUTES"
    ADD CONSTRAINT deals_attributes_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS_FILES deals_files_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_FILES"
    ADD CONSTRAINT deals_files_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS_HISTORY deals_history_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_HISTORY"
    ADD CONSTRAINT deals_history_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS deals_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS"
    ADD CONSTRAINT deals_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS_PARTIES deals_parties_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_PARTIES"
    ADD CONSTRAINT deals_parties_id_pk PRIMARY KEY ("Id");


--
-- Name: DEALS_STATUSMAP deals_statusmap_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."DEALS_STATUSMAP"
    ADD CONSTRAINT deals_statusmap_id_pk PRIMARY KEY ("Id");


--
-- Name: FILES_ACTIONS files_actions_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."FILES_ACTIONS"
    ADD CONSTRAINT files_actions_id_pk PRIMARY KEY ("Id");


--
-- Name: MEMBERS_ACTIONS members_actions_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."MEMBERS_ACTIONS"
    ADD CONSTRAINT members_actions_id_pk PRIMARY KEY ("Id");


--
-- Name: MEMBERS members_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."MEMBERS"
    ADD CONSTRAINT members_id_pk PRIMARY KEY ("Id");


--
-- Name: SYSTEM_ACTIONS system_actions_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_ACTIONS"
    ADD CONSTRAINT system_actions_id_pk PRIMARY KEY ("Id");


--
-- Name: SYSTEM_ALERT system_alert_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_ALERT"
    ADD CONSTRAINT system_alert_id_pk PRIMARY KEY ("Id");


--
-- Name: SYSTEM_CONFIGURATION system_configuration_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_CONFIGURATION"
    ADD CONSTRAINT system_configuration_id_pk PRIMARY KEY ("Id");


--
-- Name: SYSTEM_PARS system_pars_id_pk; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."SYSTEM_PARS"
    ADD CONSTRAINT system_pars_id_pk PRIMARY KEY ("Id");


--
-- Name: USERS users_pkey; Type: CONSTRAINT; Schema: public; Owner: rchain
--

ALTER TABLE ONLY public."USERS"
    ADD CONSTRAINT users_pkey PRIMARY KEY ("ID");


--
-- Name: members_key_idx; Type: INDEX; Schema: public; Owner: rchain
--

CREATE UNIQUE INDEX members_key_idx ON public."MEMBERS" USING btree ("Key");


--
-- Name: system_alert_id_uindex; Type: INDEX; Schema: public; Owner: rchain
--

CREATE UNIQUE INDEX system_alert_id_uindex ON public."SYSTEM_ALERT" USING btree ("Id");


--
-- Name: system_configuration_id_uindex; Type: INDEX; Schema: public; Owner: rchain
--

CREATE UNIQUE INDEX system_configuration_id_uindex ON public."SYSTEM_CONFIGURATION" USING btree ("Id");


--
-- Name: system_configuration_key_uindex; Type: INDEX; Schema: public; Owner: rchain
--

CREATE UNIQUE INDEX system_configuration_key_uindex ON public."SYSTEM_CONFIGURATION" USING btree ("Key");


--
-- Name: users_id_uindex; Type: INDEX; Schema: public; Owner: rchain
--

CREATE UNIQUE INDEX users_id_uindex ON public."USERS" USING btree ("ID");


--
-- PostgreSQL database dump complete
--

