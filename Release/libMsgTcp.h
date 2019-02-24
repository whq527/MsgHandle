#include "libMsgTcpIter.h"
#include <thread>
#include <mutex>
#include <deque>
#include "msgpack.hpp"

struct st_pack
{
	st_pack(const char* _source = nullptr)
	{
		if (_source != nullptr)
			source = _source;
		create_time = time(NULL);
	};
	std::string key = "";				//symbol.donevolume
	std::string value = "";				//10000

	std::string source = "";			//drv
	std::string source_guid = "";		//drv guid
	std::string source_ip = "";			//drv 本地ip
	std::time_t create_time = 0;		//生成包时间(3次时间同步后调整) 服务器时间-本地时间
	std::time_t send_time = 0;			//本地发送时间(3次时间同步后调整) 服务器时间-本地时间
	std::time_t recv_time = 0;			//服务器接收时间
	MSGPACK_DEFINE(key, value, source, source_guid, source_ip, create_time, send_time);
};

class ClibMsgTcp
	: public ClibMsg
{
public:
	ClibMsgTcp(void);
	virtual ~ClibMsgTcp();
	virtual bool Init(const char* _name, const char* _addr, int _port);
	virtual bool Start();
	virtual void GetMsg(const char* _key, const char* _value);
private:
	virtual bool ConnectSvr();
	void Stop();
	void Run();
	bool SendMsg(st_pack &_pack);
	bool RecvMsg(st_pack &_pack);
private:
	std::string m_name = "";
	std::string m_guid = "";
	std::string m_ip = "";
	double m_dTimeDiff = 0;
	std::deque<st_pack> m_msgs;

	bool m_Terminated = false;
	std::thread *m_th;
	std::mutex m_mtx; //互斥量

	std::string m_addr = "";
	int m_port = 0;
	int m_sock = 0;
	bool m_connected = false;
};
