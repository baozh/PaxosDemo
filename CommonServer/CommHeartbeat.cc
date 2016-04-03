#include <vector>
#include <boost/bind.hpp>
#include <boost/any.hpp>
#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>

#include "CommHeartbeat.h"
#include "CommonServer.h"

using namespace std;
using namespace muduo::net;




CommHeartbeat::CommHeartbeat(muduo::net::EventLoop *loop, CommonServer* commServer, int timeoutSec, int sendSec)
	: loop_ (loop)
	, commServer_ (commServer)
	, timeoutSec_ (timeoutSec)
	, sendSec_ (sendSec)
{
	checkTimer_ = loop_->runEvery(static_cast<double>(timeoutSec_), boost::bind(&CommHeartbeat::checkAllHeartbeat, this));
	sendTimer_ = loop_->runEvery(static_cast<double>(sendSec_), boost::bind(&CommHeartbeat::sendAllHeartbeat, this));
}

CommHeartbeat::~CommHeartbeat()
{
	loop_->cancel(checkTimer_);
	loop_->cancel(sendTimer_);
}

void CommHeartbeat::updateHeartbeat(const muduo::net::TcpConnectionPtr& conn)
{
	assert(conn);
	conn->getLoop()->runInLoop(boost::bind(&CommHeartbeat::updateHeartbeatInLoop, this, conn));
}

void CommHeartbeat::updateHeartbeatInLoop(const muduo::net::TcpConnectionPtr& conn)
{
	conn->getLoop()->assertInLoopThread();
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context);
	context->heartTime_ = muduo::Timestamp::now().secondsSinceEpoch();
}

void CommHeartbeat::checkAllHeartbeat()
{
	loop_->assertInLoopThread();
	assert(commServer_);
	
	vector<TcpConnectionPtr> connVec;
	commServer_->getAllConn(connVec);
	for(vector<TcpConnectionPtr>::iterator it = connVec.begin(); it != connVec.end(); ++it)
	{
		if(*it)
		{
			(*it)->getLoop()->runInLoop(boost::bind(&CommHeartbeat::checkSingleHeartbeat, this, *it));
		}
	}
}

void CommHeartbeat::checkSingleHeartbeat(const muduo::net::TcpConnectionPtr& conn)
{
	assert(conn);
	conn->getLoop()->assertInLoopThread();

	LOG_DEBUG << "check hearbeat of connection: " << conn->peerAddress().toIpPort();

	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context);
	if(context->heartTime_ < muduo::Timestamp::now().secondsSinceEpoch() - timeoutSec_)
	{
		// timeout, close the connection
		LOG_ERROR << "[HeartExpired] timeout of connection, close it: " << conn->peerAddress().toIpPort() 
			<< ", heartTime:" << context->heartTime_ << ", Require time: " << (muduo::Timestamp::now().secondsSinceEpoch() - timeoutSec_);
		CommServUtil::forceCloseConn(conn, "Heart_Expired!");
	}
}


void CommHeartbeat::sendAllHeartbeat()
{
	loop_->assertInLoopThread();
	assert(commServer_);

	vector<TcpConnectionPtr> connVec;
	commServer_->getPeerServerConn(connVec);   //只需对 主动连接的server 发送心跳
	for(vector<TcpConnectionPtr>::iterator it = connVec.begin(); it != connVec.end(); ++it)
	{
		if((*it))
		{
			CommServUtil::sendJsonMessage((*it), PRO_SYS_HEART_REQ, false, "");
		}
	}
}


