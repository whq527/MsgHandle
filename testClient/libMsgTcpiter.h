#pragma once
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
	//初始化成功, 返回guid号
	virtual const char* Init(const char* _name, const char* _addr, int _port) = 0;
	virtual bool Start() = 0;
	virtual void GetMsg(const char* _key, const char* _value) = 0;
};

//extern LIBMSGTCP_API int nlibMsgTcp;
//
LIBMSGTCP_API ClibMsg* CreateLibMsgTcp(void);
LIBMSGTCP_API void FreeLibMsgTcp(ClibMsg* _pClibMsg);