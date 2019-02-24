#pragma once
#include <ctime>
#include <string>
#include "msgpack.hpp"

struct st_pack
{
	st_pack(const char* _source = nullptr)
	{
		if (_source != nullptr)
			source = _source;
		create_time = time(NULL);
	};
	unsigned long index = 0;
	std::string key = "";				//symbol.donevolume
	std::string value = "";				//10000

	std::string source = "";			//drv
	std::string source_guid = "";		//drv guid
	std::string source_ip = "";			//drv 本地ip
	std::time_t create_time = 0;		//生成包时间
	std::time_t send_time = 0;			//本地发送时间
	std::time_t recv_time = 0;			//服务器接收时间
	bool erase = false;
	MSGPACK_DEFINE(index, key, value, source, source_guid, source_ip, create_time, send_time);

	std::string str_create_time = "";
	std::string str_send_time = "";
	std::string str_recv_time = "";
};

struct st_Compare
{
	std::string Header;
	int Delay;
	int type;
	std::string Base;
};