#pragma once
#include "Memory_cb.h"
#include "CMsgRecv.h"
using namespace std;
extern el::Logger* g_log;

bool CreateMemDb(const char* _dbFile, CSqlite_MemDb* _dbhdl, CMsgRecv* _msgrecv)
{
	sqlite3* pmemdb = _dbhdl->OpenDb(":memory:");
	if (pmemdb == nullptr)
		return false;
	string utf8file = ANSIToUTF8(_dbFile);
	sqlite3* pfdb = _dbhdl->OpenDb(utf8file.c_str());
	if (pfdb == nullptr)
		return false;
	_dbhdl->SetMainDbHdl(pmemdb);
	//设置文件同步
	if (!_dbhdl->AttachDb(utf8file.c_str(), "filedb"))
		return false;
	_dbhdl->SetSyncAttachDb("filedb");

	//默认存储表
	char create_sql[512] = "CREATE TABLE %s("
		"SOURCE_NAME TEXT,"
		"SOURCE_GUID CHAR(36) NOT NULL,"
		"SOURCE_IP   CHAR(15),"
		"TABLE_KEY   TEXT      NOT NULL,"
		"MSG_KEY     TEXT,"
		"MSG_VALUE   TEXT,"
		"TIME_CREATE DATETIME  NOT NULL,"
		"TIME_SEND   DATETIME  NOT NULL,"
		"TIME_RECV   DATETIME  NOT NULL"
		");"
		"CREATE INDEX %s_IK_GUID ON %s(SOURCE_GUID);"
		"CREATE INDEX %s_IK_MSGKEY ON %s(MSG_KEY);"
		"CREATE INDEX %s_IK_RECVTIME ON %s(TIME_RECV);";
	char sql[512] = "";
	sprintf_s(sql, create_sql, "data_general", "data_general", "data_general", "data_general", "data_general", "data_general", "data_general");
	_dbhdl->CreateTable(pmemdb, sql);
	_dbhdl->CreateTable(pfdb, sql);
	_dbhdl->SetSyncTable("data_general");

	//数据key与存储表的关系表
	strcpy_s(sql, "CREATE TABLE data_relation("
		"TABLE_NAME TEXT NOT NULL,"
		"TABLE_KEY TEXT NOT NULL,"
		"CONSTRAINT data_relation_PK PRIMARY KEY(TABLE_NAME, TABLE_KEY));");
	_dbhdl->CreateTable(pfdb, sql);

	//获取key和表的关系, 新建表, 消息接收线程添加关系

	st_Select sqlcmd;
	sqlcmd.Sql = "Select * from filedb.data_relation";
	_dbhdl->Select(&sqlcmd);
	while (sqlcmd.step())
	{
		if (strcmp(sqlcmd.toString("TABLE_NAME"), "NULL") != 0)
		{
			g_log->info("create table %v", sqlcmd.toString("TABLE_NAME"));
			sprintf_s(sql, create_sql, sqlcmd.toString("TABLE_NAME"), sqlcmd.toString("TABLE_NAME"), sqlcmd.toString("TABLE_NAME"), sqlcmd.toString("TABLE_NAME"),
				sqlcmd.toString("TABLE_NAME"), sqlcmd.toString("TABLE_NAME"), sqlcmd.toString("TABLE_NAME"));
			_dbhdl->CreateTable(pmemdb, sql);
			_dbhdl->CreateTable(pfdb, sql);
			_dbhdl->SetSyncTable(sqlcmd.toString("TABLE_NAME"), 1000);
		}
		_msgrecv->AddKeyToDb(sqlcmd.toString("TABLE_KEY"), sqlcmd.toString("TABLE_NAME"));
	}
	_dbhdl->CloseDbHdl(pfdb);

	return true;
}