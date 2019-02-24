#pragma once
#include "common.h"
#include <thread>
#include <mutex>
#include <deque>
#include "Memory_cb.h"

class CMsgRecv
{
public:
	CMsgRecv();
	~CMsgRecv();
	//model 1:写库增加延迟
	bool Init(int _port, CSqlite_MemDb* _dbhdl, bool _debug, int _model = 0);
	bool Start();
	void RecvRun();
	void WtDbRun();
	void Terminate(void);
	void AddKeyToDb(const char* _key, const char* _table);
	void RecvState(int &_count);
private:
	//_wait=NN_DONTWAIT
	bool SendMsg(st_pack &_pack, int _wait = 0, double _timesync = 0);
	bool RecvMsg(st_pack &_pack, int _wait = 0);
private:
	bool m_debug = false;
	bool m_Terminated = false;
	std::thread *m_th_recv = nullptr;
	std::thread *m_th_wtdb = nullptr;
	std::mutex m_mtx; //互斥量
	std::condition_variable m_cv;

	//0 即时写入数据库, 1 缓存数据延时写入数据库
	int m_model = 0;

	//网络连接
	int m_sock = 0;
	bool m_connected = false;

	//数据库
	std::deque<st_pack> m_msgs;
	CSqlite_MemDb* m_dbhdl = nullptr;
	std::map<std::string, std::string> m_key_db;

	//性能
	int m_do_count = 0;//处理条目数
};
