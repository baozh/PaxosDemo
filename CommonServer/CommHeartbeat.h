#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include "CommUtil.h"

class CommonServer;


class CommHeartbeat
{
public:
	CommHeartbeat(muduo::net::EventLoop *loop, CommonServer* commServer, int timeoutSec, int sendSec);
	~CommHeartbeat();
	void updateHeartbeat(const muduo::net::TcpConnectionPtr& conn);
private:
	void updateHeartbeatInLoop(const muduo::net::TcpConnectionPtr& conn);
	void checkAllHeartbeat();
	void sendAllHeartbeat();
	void checkSingleHeartbeat(const muduo::net::TcpConnectionPtr& conn);
	
	CommonServer* commServer_;
	muduo::net::EventLoop *loop_;
	int timeoutSec_;
	int sendSec_;
	muduo::net::TimerId checkTimer_;
	muduo::net::TimerId sendTimer_;
};

#endif