#include "CMsgRecv.h"
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include "easylogging++.h"
#include "CEncodeMsg.h"

using namespace std;
extern el::Logger* g_log;

#define FEEDBACK "W+-REP"

CMsgRecv::CMsgRecv()
{
}

CMsgRecv::~CMsgRecv()
{
	if (!m_Terminated)
	{
		Terminate();
	}

	if (m_th_recv != nullptr)
	{
		m_th_recv->join();
		m_th_recv = nullptr;
	}

	if (m_th_wtdb != nullptr)
	{
		m_th_wtdb->join();
		m_th_wtdb = nullptr;
	}

	if (m_connected && m_sock >= 0)
	{
		nn_shutdown(m_sock, 0);
		if (nn_close(m_sock) < 0)
			printf("nn_close: %s\n", nn_strerror(errno));
		m_connected = false;
	}
}

bool CMsgRecv::Init(int _port, CSqlite_MemDb * _dbhdl, bool _debug, int _model/* = 0*/)
{
	m_debug = _debug;
	m_model = _model;

	m_sock = nn_socket(AF_SP, NN_REP);
	if (m_sock < 0)
	{
		g_log->error("nn_socket failed: %v", nn_strerror(errno));
		return false;
	}
	int timeout = -1;
	char buf[512] = "";
	sprintf_s(buf, "tcp://0.0.0.0:%d", _port);
	if (nn_bind(m_sock, buf) < 0)
	{
		g_log->error("Open failed: %v", nn_strerror(errno));
		return false;
	}

	//120��û���յ�, �ط����һ��
	int linger = 120;
	nn_setsockopt(m_sock, NN_REP, NN_REQ_RESEND_IVL, &linger, sizeof(linger));

	g_log->info("Open %v success: %v", _port, nn_strerror(errno));
	m_connected = true;

	m_dbhdl = _dbhdl;
	return true;
}

bool CMsgRecv::Start()
{
	m_th_recv = new thread(std::bind(&CMsgRecv::RecvRun, this));
	m_th_wtdb = new thread(std::bind(&CMsgRecv::WtDbRun, this));
	return false;
}

void CMsgRecv::RecvRun()
{
	while (!m_Terminated)
	{
		if (!m_connected)
		{
			Sleep(1);
			continue;
		}

		//���տͻ���������Ϣ,
		st_pack one_pack;
		if (RecvMsg(one_pack))
		{
			if (one_pack.key == FEEDBACK)
			{
				//��������������������Ϣ, �ͻ��˽��е�Ӧ��
			}
			//�ͻ���������������Ϣ
			else
			{
				//ʱ��ת��
				char create_time[26];
				char send_time[26];
				char recv_time[26];
				struct tm ttm;
				localtime_s(&ttm, &one_pack.create_time);
				strftime(create_time, sizeof(create_time), "%Y-%m-%d %H:%M:%S", &ttm);
				one_pack.str_create_time = create_time;
				localtime_s(&ttm, &one_pack.send_time);
				strftime(send_time, sizeof(send_time), "%Y-%m-%d %H:%M:%S", &ttm);
				one_pack.str_send_time = send_time;
				localtime_s(&ttm, &one_pack.recv_time);
				strftime(recv_time, sizeof(recv_time), "%Y-%m-%d %H:%M:%S", &ttm);
				one_pack.str_recv_time = recv_time;

				if (m_debug || one_pack.key == "timecheck")
				{
					LOG(INFO) << one_pack.source << " "
						<< one_pack.source_guid << " "
						<< one_pack.source_ip << " "
						<< one_pack.index << " "
						<< one_pack.key << " ["
						<< one_pack.value << "] "
						<< create_time << " "
						<< send_time << " "
						<< recv_time;
				}

				//Ӧ��ͻ���
				st_pack rep;
				rep.source = one_pack.source;
				rep.source_guid = one_pack.source_guid;
				rep.source_ip = one_pack.source_ip;
				rep.index = one_pack.index;
				rep.key = FEEDBACK;
				if (one_pack.key == "timecheck")
				{
					rep.value = "timecheck";
				}
				else
				{
					rep.value = "check";
				}
				SendMsg(rep, NN_DONTWAIT);
			}
			unique_lock<mutex> ulk(m_mtx);
			m_msgs.push_back(one_pack);//��������, ׼��д��
			m_do_count++;
			m_cv.notify_all();
		}
	}
}

void CMsgRecv::WtDbRun()
{
	ULONGLONG tk_delay = GetTickCount64();
	while (!m_Terminated)
	{
		ULONGLONG tk_db_use = 0;//db��ʱ

		//ģʽѡ��, ��ʱ������д��
		if (m_model == 1 && (GetTickCount64() - tk_delay) < 10000)
		{
			std::chrono::milliseconds(1);
			continue;
		}
		else if (m_model == 1 && (GetTickCount64() - tk_delay) >= 10000)
		{
			tk_delay = GetTickCount64();
		}

		//ȡ��ʱ����
		int MAX_NUM = 100000;
		vector<st_pack> tmplist;
		{
			unique_lock<mutex> ulk(m_mtx);
			if (m_msgs.size() > 0)
			{
				while (m_msgs.begin() != m_msgs.end())
				{
					tmplist.push_back(m_msgs.front());
					m_msgs.pop_front();
					if (--MAX_NUM <= 0)
					{
						break;
					}
				}
			}
		}

		if (tmplist.size() > 0)
		{
			tk_db_use = GetTickCount64();
			for (auto var : tmplist)
			{
				ostringstream  ss_sql;
				ss_sql << "("
					<< "'" << var.source << "',"
					<< "'" << var.source_guid << "',"
					<< "'" << var.source_ip << "',"
					<< "'" << var.key << "',"
					<< "'" << var.value << "',"
					<< "'" << var.str_create_time.c_str() << "',"
					<< "'" << var.str_send_time.c_str() << "',"
					<< "'" << var.str_recv_time.c_str() << "')";
				string value = ANSIToUTF8(ss_sql.str());

				auto iter = m_key_db.find(var.key);
				if (iter == m_key_db.end())
				{
					m_dbhdl->BatchInsert("data_general", value.c_str(),
						"(SOURCE_NAME,SOURCE_GUID,SOURCE_IP,"
						"MSG_KEY,MSG_VALUE,"
						"TIME_CREATE,TIME_SEND,TIME_RECV)");
					if (m_debug)
					{
						LOG(INFO) << "->data_general:" << value;
					}
				}
				else
				{
					if (iter->second != "NULL")
					{
						m_dbhdl->BatchInsert(iter->second.c_str(), value.c_str(),
							"(SOURCE_NAME,SOURCE_GUID,SOURCE_IP,"
							"MSG_KEY,MSG_VALUE,"
							"TIME_CREATE,TIME_SEND,TIME_RECV)");
						if (m_debug)
						{
							LOG(INFO) << "->" << iter->second << ":" << value;
						}
					}
					//else =null ����
				}
			}
		}
		else
		{
			unique_lock<mutex> ulk(m_mtx);
			m_cv.wait_for(ulk, std::chrono::milliseconds(100));
		}
	}
}

void CMsgRecv::Terminate(void)
{
	nn_term();
	m_Terminated = true;
}

void CMsgRecv::AddKeyToDb(const char* _key, const char* _table)
{
	m_key_db[_key] = _table;
}

void CMsgRecv::RecvState(int &_count)
{
	unique_lock<mutex> ulk(m_mtx);
	_count = m_do_count;
	m_do_count = 0;
}

//_wait=NN_DONTWAIT
bool CMsgRecv::SendMsg(st_pack & _pack, int _wait, double _timesync)
{
	_pack.send_time = time(NULL);
	if (_timesync > 0)
	{
		_pack.create_time += static_cast<time_t>(_timesync);
		_pack.send_time += static_cast<time_t>(_timesync);
	}

	//���л�
	msgpack::sbuffer sbuf;
	msgpack::pack(sbuf, _pack);//���

	//����ѹ��
	CEncodeMsg one_msg;
	if (!one_msg.Encode(sbuf.data(), sbuf.size()))
	{
		printf("����һ����,����ʧ�� %s\n ", one_msg.GetErr());
		return false;
	}

	if (!one_msg.Compress())
	{
		printf("ѹ��ʧ�� %s\n", one_msg.GetErr());
		return false;
	}

	int bytes = nn_send(m_sock, (char*)one_msg.data(), one_msg.length(), _wait);
	if (bytes < 0)
	{
		printf("nn_send failed: %s\n", nn_strerror(errno));
	}
	else
	{
		//printf("nn_send size %d\n", bytes);
	}
	return true;
}

//_wait=NN_DONTWAIT
bool CMsgRecv::RecvMsg(st_pack & _pack, int _wait)
{
	char *buf = NULL;
	int bytes = nn_recv(m_sock, &buf, NN_MSG, _wait);
	if (bytes <= 0)
		return false;

	//������ѹ
	CEncodeMsg one_msg;
	memcpy_s(&one_msg, bytes, buf, bytes);

	if (one_msg.Decode())
	{
		if (one_msg.CompressType() != CEncodeMsg::NOCOMPRESS)
		{
			if (!one_msg.UnCompress())
			{
				printf("��ѹ��ʧ��: %s", one_msg.GetErr());
				return false;
			}
		}
	}
	else
	{
		printf("��Ϣ������ʧ��: %s", one_msg.GetErr());
		return false;
	}

	//�����л�
	msgpack::sbuffer sbuf;
	sbuf.write(one_msg.body(), one_msg.body_length());
	msgpack::object_handle unpack = msgpack::unpack(sbuf.data(), sbuf.size());
	msgpack::object obj = unpack.get();

	obj.convert(_pack);
	if (_pack.key.empty())
		return false;

	_pack.recv_time = time(NULL);

	nn_freemsg(buf);
	return true;
}