// MsgHandleSvr.cpp : 定义控制台应用程序的入口点。
//
#include "common.h"
#include <vector>
#include "SimpleIni.h"
#include "easylogging++.h"

#include "dbCreate.h"
#include "CMsgRecv.h"

INITIALIZE_EASYLOGGINGPP

using namespace std;
el::Logger* g_log;

void GetExePath(char* _path, int _len)
{
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	(strrchr(szFilePath, '\\'))[0] = 0; // 删除文件名，只获得路径字串
	strcpy_s(_path, _len, szFilePath);
}

//////////////////////////////////////////////////////////////////////////
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	el::Loggers::flushAll();
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		break;

		// CTRL-CLOSE: confirm that the user wants to exit.
	case CTRL_CLOSE_EVENT:
		break;
		// Pass other signals to the next handler.
	case CTRL_BREAK_EVENT:
		printf("Ctrl-Break event\n\n");
		break;

	case CTRL_LOGOFF_EVENT:
		printf("Ctrl-Logoff event\n\n");
		break;

	case CTRL_SHUTDOWN_EVENT:
		printf("Ctrl-Shutdown event\n\n");
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void SetEasyLog(const char* _path)
{
	//调用一次INITIALIZE_EASYLOGGINGPP
	//无默认日志文件
	//多线程安全支持
	//性能跟踪
	//崩溃跟踪
	//崩溃跟踪
	//define ELPP_NO_DEFAULT_LOG_FILE
	//define ELPP_THREAD_SAFE
	//define ELPP_FEATURE_PERFORMANCE_TRACKING
	//define ELPP_FEATURE_CRASH_LOG
	//define ELPP_HANDLE_SIGABRT

	char logpath[MAX_PATH] = "";
	/// 使用默认配置
	el::Configurations defaultConf;
	defaultConf.setToDefault();

	// 重新设置GLOBAL级别的配置项FORMAT的值
	defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime %level %msg");
	defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
	defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
	defaultConf.setGlobally(el::ConfigurationType::SubsecondPrecision, "3");
	defaultConf.setGlobally(el::ConfigurationType::PerformanceTracking, "false");
	defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "1048576");
	defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "3");
	int threadid = GetCurrentThreadId();
	sprintf_s(logpath, "%s\\log\\log_%%datetime{%%Y%%M%%d}.%d.log", _path, threadid);
	defaultConf.setGlobally(el::ConfigurationType::Filename, logpath);
	defaultConf.set(el::Level::Info, el::ConfigurationType::Format, "%datetime %levshort %line %msg");
	defaultConf.set(el::Level::Warning, el::ConfigurationType::Format, "%datetime %levshort %line %msg");
	defaultConf.set(el::Level::Error, el::ConfigurationType::Format, "%datetime %levshort %line %msg");
	/// 重新设置配置
	el::Loggers::reconfigureLogger("default", defaultConf);

	el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);//或者按照配置来开关级别
	el::Loggers::setLoggingLevel(el::Level::Global);//设置级别门阀值，前提是选择划分级别的日志, 修改参数可以控制日志输出 门阀值为 el::Level::Global 表示所有级别的日志都生效
	el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
	//el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);//激活MaxLogFileSize 日志文件就会自动清空文件中所有的日志记录，并重新开始写入
	//el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);//Fatal 级别，默认情况下会使程序中断,可设置标记 来阻止中断
	//el::Loggers::addFlag(el::LoggingFlag::ImmediateFlush);//性能影响
	//el::Loggers::addFlag(el::LoggingFlag::LogDetailedCrashReason);//When handling crashes by default, detailed crash reason will be logged as well

	//void MyCrashHandler(int sig) {
	//	LOG(ERROR) << "Woops! Crashed!";
	//	// FOLLOWING LINE IS ABSOLUTELY NEEDED AT THE END IN ORDER TO ABORT APPLICATION
	//	//el::Helpers::logCrashReason(sig, true);
	//	el::Loggers::flushAll();
	//	el::Helpers::crashAbort(sig);
	//}
	//el::Helpers::setCrashHandler(MyCrashHandler);

	g_log = el::Loggers::getLogger("default");
	//el::Loggers::flushAll();
}

//////////////////////////////////////////////////////////////////////////

int main()
{
	//初始化
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		printf("The Control Handler is installed.\n");
	}

	char g_FilePath[MAX_PATH];
	GetExePath(g_FilePath, MAX_PATH);
	char filename[512] = { 0 };
	SetEasyLog(g_FilePath);

	sprintf_s(filename, "%s\\%s", g_FilePath, "config.ini");
	CSimpleIniA ini;
	SI_Error rc = ini.LoadFile(filename);
	if (rc < 0)
	{
		g_log->error("获取配置文件失败%v", filename);
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//创建落地数据库
	sprintf_s(filename, "%s\\%s", g_FilePath, "msgdb.db");
	CSqlite_MemDb m_dbhdl;
	CMsgRecv recvsvr;
	//创建表结构
	CreateMemDb(filename, &m_dbhdl, &recvsvr);
	m_dbhdl.start();

	//打开网络服务
	bool isDebug = ini.GetBoolValue("APP", "debug", false);
	int	port = ini.GetLongValue("APP", "port", 30000);

	if (recvsvr.Init(port, &m_dbhdl, isDebug))
		recvsvr.Start();

	ULONGLONG tk = GetTickCount64();
	while (true)
	{
		if (GetTickCount64() - tk >= 10000)
		{
			tk = GetTickCount64();
			int do_count = 0;
			recvsvr.RecvState(do_count);
			LOG(INFO) << "recv msg " << do_count << " /10s";
		}

		Sleep(1);
	}
	return 0;
}