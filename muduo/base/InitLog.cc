#include <unistd.h>
#include <muduo/base/TimeZone.h>
#include "InitLog.h"

namespace muduo
{

static AsyncLogging *gAsyncLogging = NULL;

static void asyncOutputLog(const char *msg, int len)
{
	if(gAsyncLogging)
	{
		gAsyncLogging->append(msg, len);
	}
}

static void asyncFlushLog()
{
//	if(gAsyncLogging)
//	{
//		gAsyncLogging->stop();
//	}
}

void InitLog::init(const std::string &baseName, const std::string &logDir, int logLevel, int rollSize)
{
	asyncLogging_.reset(new AsyncLogging(logDir, baseName, rollSize));
	gAsyncLogging = asyncLogging_.get();
	muduo::Logger::setTimeZone(muduo::TimeZone(8*3600, "CST"));
	muduo::Logger::setLogLevel((muduo::Logger::LogLevel)logLevel);
	muduo::Logger::setOutput(asyncOutputLog);
	muduo::Logger::setFlush(asyncFlushLog);		
	asyncLogging_->start();
	usleep(100*1000); // wait asyncLogging to enter the threaad loop
}

InitLog::~InitLog()
{
	// wait for the last log message to be written into log file.
	// this is not accurate but practical
	LOG_TRACE << "~InitLog";
	usleep(500*1000); 
}

}