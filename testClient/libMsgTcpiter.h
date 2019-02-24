#pragma once
// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� LIBMSGTCP_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// LIBMSGTCP_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef LIBMSGTCP_EXPORTS
#define LIBMSGTCP_API __declspec(dllexport)
#else
#define LIBMSGTCP_API __declspec(dllimport)
#endif

//���û���յ���������Ӧ��, ���һ������ÿ���ӻ��ط�һ��

class LIBMSGTCP_API ClibMsg
{
public:
	//ClibMsg(void);
	virtual ~ClibMsg() {};
	//��ʼ���ɹ�, ����guid��
	virtual const char* Init(const char* _name, const char* _addr, int _port) = 0;
	virtual bool Start() = 0;
	virtual void GetMsg(const char* _key, const char* _value) = 0;
};

//extern LIBMSGTCP_API int nlibMsgTcp;
//
LIBMSGTCP_API ClibMsg* CreateLibMsgTcp(void);
LIBMSGTCP_API void FreeLibMsgTcp(ClibMsg* _pClibMsg);