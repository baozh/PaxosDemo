#ifndef MUDUO_INIT_LOG_H
#define MUDUO_INIT_LOG_H

#include <string>
#include <boost/scoped_ptr.hpp>
#include <muduo/base/AsyncLogging.h>
#include <muduo/base/Logging.h>

namespace muduo
{

class InitLog
{
public:
	void init(const std::string &baseName, const std::string &logDir, int logLevel, int rollSize);
	~InitLog();
	//
private:
	boost::scoped_ptr<muduo::AsyncLogging> asyncLogging_;
};

}// end namespace muduo

#endif