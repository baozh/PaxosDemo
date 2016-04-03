// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/FastTcpServer.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

FastTcpServer::FastTcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
  : loop_(CHECK_NOTNULL(loop)),
    hostport_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(
      boost::bind(&FastTcpServer::newConnection, this, _1, _2));
  connections_.resize(10000); // initialize to 10000 empty connections
}

FastTcpServer::~FastTcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "FastTcpServer::~FastTcpServer [" << name_ << "] destructing";

//  for (ConnectionMap::iterator it(connections_.begin());
//      it != connections_.end(); ++it)
//  {
//    TcpConnectionPtr conn = it->second;
//    it->second.reset();
//    conn->getLoop()->runInLoop(
//      boost::bind(&TcpConnection::connectDestroyed, conn));
//    conn.reset();
//  }
  for (ConnectionVec::iterator it(connections_.begin());
      it != connections_.end(); ++it)
  {
    TcpConnectionPtr conn = *it;
	if(conn)
	{
      it->reset();
      conn->getLoop()->runInLoop(
        boost::bind(&TcpConnection::connectDestroyed, conn));
      conn.reset();
	}
  }

}

void FastTcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

void FastTcpServer::start()
{
  if (started_.getAndSet(1) == 0)
  {
    threadPool_->start(threadInitCallback_);

    assert(!acceptor_->listenning());
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void FastTcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  EventLoop* ioLoop = threadPool_->getNextLoop();
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  LOG_INFO << "FastTcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
  // the place must be released before
  assert(!connections_[sockfd]);
  // ensure enough capacity
  while(connections_.size() <= sockfd) 
  {
	connections_.resize(connections_.size()*2);
  }
  connections_[sockfd] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      boost::bind(&FastTcpServer::removeConnection, this, _1)); // FIXME: unsafe
  ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void FastTcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  loop_->runInLoop(boost::bind(&FastTcpServer::removeConnectionInLoop, this, conn));
}

void FastTcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "FastTcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
//  size_t n = connections_.erase(conn->name());
//  (void)n;
//  assert(n == 1);
  int fd = conn->fd();
  assert(connections_[fd]);
  connections_[fd].reset();
  //
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
}

TcpConnectionPtr FastTcpServer::getConnection(int fd)
{
	// run in main loop
	loop_->assertInLoopThread();
	//
	if(fd >= connections_.size())
		return TcpConnectionPtr();
	return connections_[fd];
}

void FastTcpServer::getAllConnections(std::vector<TcpConnectionPtr> &connVec)
{
	// run in main loop
	loop_->assertInLoopThread();
	//
	int connNum = connections_.size();
	connVec.reserve(connNum);
	for(int i = 0; i < connNum; ++i)
	{
		if(connections_[i])
		{
			connVec.push_back(connections_[i]);
		}
	}
}