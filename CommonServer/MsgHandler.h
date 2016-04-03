#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#include <assert.h>
#include <boost/function.hpp>
#include <muduo/net/TcpConnection.h>

class CommonServer;
class PacketBuffer;
typedef boost::function<void (CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)> MsgFunc;

class MsgHandler
{
public:
	MsgHandler();
	MsgHandler(const MsgFunc& func);
	MsgHandler(const MsgFunc& func, int threadType);
	int threadType() const{ return threadType_; }
	void doIt(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data) 
	{ assert(func_); func_(commServer, conn, data);}
private:
	MsgFunc func_;
	int threadType_;
};

#endif