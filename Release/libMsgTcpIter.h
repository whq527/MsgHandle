#pragma once
#include <string>
// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 LIBMSGTCP_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// LIBMSGTCP_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef LIBMSGTCP_EXPORTS
#define LIBMSGTCP_API __declspec(dllexport)
#else
#define LIBMSGTCP_API __declspec(dllimport)
#endif

//如果没有收到服务器的应答, 最后一条数据每分钟会重发一次

class LIBMSGTCP_API ClibMsg
{
public:
	//ClibMsg(void);
	virtual ~ClibMsg() {};
	//初始化成功, 返回guid号 否则返回nullptr
	//超时默认1分钟, 3次后, GetMsg会把后续的条目扔掉
	//如果依然超时, 重新创建对象
	virtual const char* Init(const char* _name, const char* _addr, int _port, int _timeout = 60000) = 0;
	virtual bool Start() = 0;
	//数据先根据_table_key选择table, 再进行保存, key用来索引
	virtual void GetMsg(const char* _table_key, const char* _key, const char* _value) = 0;
	virtual void GetMsg(std::string _table_key, std::string _key, std::string _value) = 0;
};

//extern LIBMSGTCP_API int nlibMsgTcp;
//
LIBMSGTCP_API ClibMsg* CreateLibMsgTcp(void);
LIBMSGTCP_API void FreeLibMsgTcp(ClibMsg* _pClibMsg);