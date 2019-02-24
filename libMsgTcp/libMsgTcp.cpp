// libMsgTcp.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "libMsgTcp.h"
#include "malloc.h"
#include <stdio.h>
#include <iostream>
#include <combaseapi.h>
#include <iphlpapi.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include "CEncodeMsg.h"

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
using namespace std;

#define FEEDBACK "W+-REP"

string Create_GUID()
{
	char buf[64] = { 0 };
	GUID guid;
	if (S_OK == ::CoCreateGuid(&guid))
	{
		sprintf_s(buf, "%08X-%04X-%04x"//-%02X%02X-%02X%02X%02X%02X%02X%02X"
			, guid.Data1
			, guid.Data2
			, guid.Data3
			//, guid.Data4[0], guid.Data4[1]
			//, guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5]
			//, guid.Data4[6], guid.Data4[7]
		);
	}
	return buf;
}

bool GetIPandMac(string &_ip, string &_mac)
{
	//PIP_ADAPTER_INFO�ṹ��ָ��洢����������Ϣ
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	//�õ��ṹ���С,����GetAdaptersInfo����
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
	{
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return false;
	}
	//����GetAdaptersInfo����,���pIpAdapterInfoָ�����;����stSize��������һ��������Ҳ��һ�������
	int nRel = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	//��¼��������
	int netCardNum = 0;
	//��¼ÿ�������ϵ�IP��ַ����
	int IPnumPerNetCard = 0;
	if (ERROR_BUFFER_OVERFLOW == nRel)
	{
		//����������ص���ERROR_BUFFER_OVERFLOW
		//��˵��GetAdaptersInfo�������ݵ��ڴ�ռ䲻��,ͬʱ�䴫��stSize,��ʾ��Ҫ�Ŀռ��С
		//��Ҳ��˵��ΪʲôstSize����һ��������Ҳ��һ�������
		//�ͷ�ԭ�����ڴ�ռ�
		free(pAdapterInfo);
		//���������ڴ�ռ������洢����������Ϣ
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
		//�ٴε���GetAdaptersInfo����,���pIpAdapterInfoָ�����
		nRel = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	}
	if (NO_ERROR == nRel)
	{
		//���������Ϣ
		//�����ж�����,���ͨ��ѭ��ȥ�ж�
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			//cout<<"����������"<<++netCardNum<<endl;
			//cout<<"�������ƣ�"<<pIpAdapterInfo->AdapterName<<endl;
			//cout<<"����������"<<pIpAdapterInfo->Description<<endl;
			char mac[56] = { 0 };
			IP_ADDR_STRING *pIpAddrString = NULL;
			bool found = false;
			switch (pAdapter->Type)
			{
				//case MIB_IF_TYPE_OTHER:
				//	cout << "�������ͣ�" << "OTHER" << endl;
				//	break;
			case MIB_IF_TYPE_ETHERNET:
				cout << "����MAC��ַ��";
				for (DWORD i = 0; i < pAdapter->AddressLength; i++)
				{
					sprintf_s(mac, "%s%s%02X", mac, (i == 0) ? "" : "-", pAdapter->Address[i]);
				}
				_mac = mac;
				cout << "����IP��ַ���£�" << endl;
				//���������ж�IP,���ͨ��ѭ��ȥ�ж�
				pIpAddrString = &(pAdapter->IpAddressList);

				_ip = pIpAddrString->IpAddress.String;
				cout << _ip << " " << _mac << " " << pAdapter->GatewayList.IpAddress.String << endl;
				if (strcmp("0.0.0.0", pAdapter->GatewayList.IpAddress.String) != 0)
				{
					found = true;
				}
				break;
				//case MIB_IF_TYPE_TOKENRING:
				//	cout << "�������ͣ�" << "TOKENRING" << endl;
				//	break;
				//case MIB_IF_TYPE_FDDI:
				//	cout << "�������ͣ�" << "FDDI" << endl;
				//	break;
				//case MIB_IF_TYPE_PPP:
				//	printf("PP\n");
				//	cout << "�������ͣ�" << "PPP" << endl;
				//	break;
				//case MIB_IF_TYPE_LOOPBACK:
				//	cout << "�������ͣ�" << "LOOPBACK" << endl;
				//	break;
				//case MIB_IF_TYPE_SLIP:
				//	cout << "�������ͣ�" << "SLIP" << endl;
				//	break;
			default:
				break;
			}
			if (found)
				break;
			pAdapter = pAdapter->Next;
			cout << "--------------------------------------------------------------------" << endl;
		}
	}
	//�ͷ��ڴ�ռ�
	if (pAdapterInfo)
		free(pAdapterInfo);

	return true;
}

ClibMsgTcp::ClibMsgTcp()
{
}

ClibMsgTcp::~ClibMsgTcp()
{
	Stop();
	if (m_connected && m_sock >= 0)
	{
		nn_shutdown(m_sock, m_eid);
		if (nn_close(m_sock) != 0)
			printf("nn_close: %s\n", nn_strerror(errno));
		m_connected = false;
	}
}

const char* ClibMsgTcp::Init(const char* _name, const char* _addr, int _port)
{
	if (_name == nullptr)
		return nullptr;
	if (_addr == nullptr)
		return nullptr;
	if (_port == 0)
		return nullptr;
	if (strcmp(_name, "") == 0)
		return nullptr;
	if (strcmp(_addr, "") == 0)
		return nullptr;
	m_name = _name;
	m_addr = _addr;
	m_port = _port;
	m_sock = nn_socket(AF_SP, NN_REQ);
	if (m_sock < 0)
	{
		printf("nn_socket init failed: %s\n", nn_strerror(errno));
		return nullptr;
	}

	//120��û���յ�, �ط����һ��
	int linger = 120;
	nn_setsockopt(m_sock, NN_REP, NN_REQ_RESEND_IVL, &linger, sizeof(linger));

	string ip = "";
	string mac = "";
	if (GetIPandMac(ip, mac))
	{
		m_ip = ip;
	}
	m_guid = Create_GUID();
	printf("����ip: %s GUID: %s\n", m_ip.c_str(), m_guid.c_str());

	//�ȷ�3��ʱ��ͬ��
	GetMsg("timecheck", "timecheck");
	GetMsg("timecheck", "timecheck");
	GetMsg("timecheck", "timecheck");
	return m_guid.c_str();
}

bool ClibMsgTcp::Start()
{
	if (m_sock < 0)
		return false;

	m_th = new thread(std::bind(&ClibMsgTcp::Run, this));
	//m_th->detach();
	return true;
}

void ClibMsgTcp::Stop()
{
	if (m_th != nullptr)
	{
		//nn_term();
		m_Terminated = true;
		m_th->join();
		m_th = nullptr;
	}
}

bool ClibMsgTcp::ConnectSvr()
{
	char buf[512] = "";
	sprintf_s(buf, "tcp://%s:%d", m_addr.c_str(), m_port);
	if (m_eid = nn_connect(m_sock, buf) < 0)
	{
		printf("ConnectSvr libMsg failed: %s\n", nn_strerror(errno));
		return false;
	}
	printf("Connect libMsg Svr: %s eid:%d\n", buf, m_eid);
	m_connected = true;
	return true;
}

void ClibMsgTcp::Run()
{
	int iTimeCheck = 3;
	double iSumTimeDiff = 0;
	while (!m_Terminated)
	{
		if (!m_connected && m_sock >= 0)
		{
			ConnectSvr();
		}

		bool need_work = false;
		st_pack *one = nullptr;

		//��ȡ����
		{
			unique_lock<mutex> ulk(m_mtx);
			if (m_msgs.size() > 0)
			{
				need_work = true;
				one = &m_msgs.front();
			}
		}

		//������Ϣ
		if (need_work && one != nullptr && !one->key.empty())
		{
			SendMsg(*one, NN_DONTWAIT, m_dTimeDiff);
		}

		//���շ�������Ϣ
		st_pack one_pack;
		if (RecvMsg(one_pack))
		{
			//�ڲ�Ӧ��
			if (one_pack.key == FEEDBACK)
			{
				//ʱ��ͬ��
				if (one_pack.value == "timecheck")
				{
					m_dTimeDiff = difftime(one_pack.send_time, one_pack.recv_time);
					char create_time[26];
					char send_time[26];
					char recv_time[26];
					struct tm ttm;
					localtime_s(&ttm, &one_pack.create_time);
					strftime(create_time, sizeof(create_time), "%x %X", &ttm);
					localtime_s(&ttm, &one_pack.send_time);
					strftime(send_time, sizeof(send_time), "%x %X", &ttm);//������ʱ��
					localtime_s(&ttm, &one_pack.recv_time);
					strftime(recv_time, sizeof(recv_time), "%x %X", &ttm);//�����յ�ʱ��

					printf("%s svr time %s this time %s diff %.1f sec \n", one_pack.key.c_str(), send_time, recv_time, m_dTimeDiff);
					iSumTimeDiff += m_dTimeDiff;
					iTimeCheck--;
					one_pack.erase = true;

					if (iTimeCheck == 0)
					{
						m_dTimeDiff = iSumTimeDiff / 3;
						printf("������ʱ���뱾��ʱ��ƽ����%f \n", m_dTimeDiff);
						iTimeCheck--;
					}
				}
				else if (one_pack.value == "check")
				{
					one_pack.erase = true;
				}
				else
				{
					//�쳣�ظ�
					one_pack.erase = true;
				}
				//ȷ�ϲ�ɾ������
				unique_lock<mutex> ulk(m_mtx);
				for (auto iter = m_msgs.begin(); iter != m_msgs.end(); iter++)
				{
					if (one_pack.erase && iter->source_guid == one_pack.source_guid && iter->index == one_pack.index)
					{
						m_msgs.erase(iter);
						break;
					}
				}
			}
			//������������������Ϣ, ��Ҫ����Ӧ����
			else
			{
				//Ӧ��
				st_pack rep;
				one_pack.key == FEEDBACK;
				rep.value = "check";
				SendMsg(rep, NN_DONTWAIT, m_dTimeDiff);
			}
		}

		if (!need_work)
		{
			unique_lock<mutex> ulk(m_mtx);
			m_cv.wait_for(ulk, std::chrono::milliseconds(100));
		}
	}

	unique_lock<mutex> ulk(m_mtx);
	m_msgs.clear();
}

//_wait=NN_DONTWAIT
bool ClibMsgTcp::SendMsg(st_pack &_pack, int _wait/* = 0*/, double _timesync/* = 0*/)
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
bool ClibMsgTcp::RecvMsg(st_pack & _pack, int _wait/*=0*/)
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

void ClibMsgTcp::GetMsg(const char* _key, const char* _value)
{
	if (m_Terminated)
		return;
	st_pack one(m_name.c_str());
	one.index = m_index++;
	if (m_index > MAXUINT)
	{
		m_index = 0;
	}
	one.key = _key;
	one.value = _value;
	one.source_guid = m_guid;
	one.source_ip = m_ip;
	unique_lock<mutex> ulk(m_mtx);
	m_msgs.push_back(one);
	m_cv.notify_all();
	return;
}

LIBMSGTCP_API ClibMsg* CreateLibMsgTcp(void)
{
	return new ClibMsgTcp();
}

LIBMSGTCP_API void FreeLibMsgTcp(ClibMsg* _pClibMsg)
{
	if (_pClibMsg != nullptr)
	{
		delete _pClibMsg;
		_pClibMsg = nullptr;
	}
}