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
	//model 1:д�������ӳ�
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
	std::mutex m_mtx; //������
	std::condition_variable m_cv;

	//0 ��ʱд�����ݿ�, 1 ����������ʱд�����ݿ�
	int m_model = 0;

	//��������
	int m_sock = 0;
	bool m_connected = false;

	//���ݿ�
	std::deque<st_pack> m_msgs;
	CSqlite_MemDb* m_dbhdl = nullptr;
	std::map<std::string, std::string> m_key_db;

	//����
	int m_do_count = 0;//������Ŀ��
};
