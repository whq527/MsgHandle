// testClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "libMsgTcpiter.h"
#include <iostream>
#include <windows.h>
using namespace std;

int main()
{
	printf("enter client name: \n");
	char clientname[512];
	cin >> clientname;
	//strcpy_s(clientname, "aaa");
	printf("enter table: \n");
	char table[32] = "";
	cin >> table;
	printf("enter key: \n");
	char key[32] = "";
	cin >> key;
	//strcpy_s(key, "bbb");
	ClibMsg *cl = CreateLibMsgTcp();
	string svrip = "127.0.0.1";
	//string svrip = "10.200.66.110";
	if (cl->Init(clientname, svrip.c_str(), 30000, 5000))
	{
		cl->Start();
	}
	Sleep(100);

	char msg[512] = "";
	bool if_reload = false;
	printf_s("y:测试重启 n不测试\n");
	cin >> msg;
	if_reload = msg[0] == 'y';

	while (true)
	{
		printf_s("输入数字判定为循环,中间次会测试一次重启, 其他为人工消息\n");
		cin >> msg;
		if (cl != nullptr)
		{
			if (strspn(msg, "0123456789") == strlen(msg))
			{
				int num = atoi(msg);

				DWORD tk1 = GetTickCount();
				for (size_t i = 1; i <= num; i++)
				{
					if (i == num / 2 && if_reload)
					{
						Sleep(100);
						FreeLibMsgTcp(cl);
						cl = nullptr;
					}

					if (cl == nullptr)
					{
						cl = CreateLibMsgTcp();
						if (cl->Init(clientname, svrip.c_str(), 30000))
						{
							if (!cl->Start())
							{
								printf("重启失败\n");
							}
						}
					}

					sprintf_s(msg, "%d", rand());
					//cout << "send->" << msg << endl;
					if (cl != nullptr)
						cl->GetMsg(table, key, msg);
				}
				cout << num << " msgs time use " << (GetTickCount() - tk1) << endl;
				cout << "last send->" << msg << endl;
			}
			else
			{
				cl->GetMsg(table, key, msg);
			}
		}

		Sleep(1000);
	}
	return 0;
}