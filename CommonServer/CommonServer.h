#ifndef __MODULE_COMMONSERVER_H
#define __MODULE_COMMONSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Callbacks.h>
#include <muduo/base/Types.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Mutex.h>

#include <boost/bind.hpp>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <vector>
#include <string>
#include <map>

#include "MsgIdDefine.h"
#include "CommUtil.h"
#include "ServerClientManager.h"
#include "PacketBuffer.h"
#include "MsgHandler.h"

class CommHeartbeat;


class CommonServer
{
public:
	//注：约定 json解析、打包 出错，返回false, 其它情况 均返回true
	typedef boost::function<void (CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn, bool isConnected)> ConnCallBack;
	typedef boost::function<void (CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn)> WriteCompleteCallBack;

	CommonServer();
	~CommonServer();

public:
	//对外服务接口
	//not thread safe, should call in main thread.
	bool init(muduo::net::EventLoop* loop, const std::string& name, int appId, int setId, int serialNo, int servType, int clientThreadNum,
			int heartCheckInterval = CommServUtil::kDefaultHeartCheckInterval, 
			int heartSendInterval = CommServUtil::kDefaultHeartSendInterval);   //初始化变量，并new Heartbeat
	
	//thread safe
	bool accept(const muduo::net::InetAddress& addr, int thread_num);
	
	//not thread safe, should call in main thread. 主动连接，连上后会发start_connect_req. 
	//需保证 要连接的server 与自己是 在同一个set内(appid, setid相同)，否则 双方会主动断开连接
	bool startConnect(const InetAddress& serverAddr, int appId, int setId, int serialNo, int servType, bool isRetry);
	
	//not thread safe, should call in main thread.  //注册消息处理函数
	void regMessageHandler(uint16_t msgNo, const MsgFunc &fun);
	//not thread safe, should call in main thread.
	void regMessageHandler(uint16_t msgNo, const MsgFunc &fun, int threadType);

	//thread safe  //回调的时机：1)建立连接成功 且 与对端Server start_connect 信令交互成功  2)连接断开
	void setConnCallback(ConnCallBack cb) {connCb_ = cb;};
	//thread safe
	void setCompleteCallback(WriteCompleteCallBack cb) {writeCompleteCb_ = cb;};

	//not thread safe, should call in main thread, need call 'init' function before call this fun.
	//return unique thread type
	int createThreadPool(int threadNum);
	//not thread safe, should call in main thread.
	//if Can't find the threadpool, return null pointer.
	boost::shared_ptr<muduo::ThreadPool> getThreadPool(int threadPoolType);

	//thread safe
	muduo::net::EventLoop* getMainLoop() {return loop_;};

	//thread safe. 如果 找不到 对应的TcpConnPtr, 就返回false.
	//会遍历一个servType下的所有conn, 速度慢一点.
	bool sendJsonMessage(int appId, int setId, int serialNo, int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr);

	//thread safe. 按 服务类型 发送消息 (以轮询的方式) 
	//速度快
	void sendJsonMessage(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr);
	//thread safe. 速度更快
	void sendJsonMessageToDc(uint16_t protoType, bool isSendJson, const std::string& jsonStr);
	//thread safe. 向此服务类型的所有server 发送消息 
	void sendJsonMsgToOneServType(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr);

	//thread safe.
	std::string getServName() {return name_;};

public:
	//not thread safe, should call in main thread.
	//attention: the conn->context may not initialized.
	void getAllConn(std::vector<TcpConnectionPtr> &connVec);

	//thread safe.
	void getPeerServerConn(std::vector<TcpConnectionPtr> &connVec);

	//thread safe
	void getAllClientConn(std::vector<muduo::net::TcpConnectionPtr> &connVec);

	//thread safe.
	void updateHeartStatInConnLoop(const muduo::net::TcpConnectionPtr &conn);

private:
	void onConnection(const muduo::net::TcpConnectionPtr& conn);
	void onMessage(const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf,
		muduo::Timestamp time);
	void onClientConnection(const muduo::net::TcpConnectionPtr& conn);
	void onClientMessage(const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf,
		muduo::Timestamp time);

	void defaultStartConnectReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void defaultStartConnectAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void defaultHeartReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void defaultHeartAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void defaultLaunchConnCmd(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);
	void defaultNotifyCloseConnReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);

	void syncConnStatWithDc();
	void notifyLaunchConnOrLostConnWithDc(const muduo::net::TcpConnectionPtr& conn, MessageType type);

	bool analyse(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, const char *data, int size);
	void WriteCompleteCB(const muduo::net::TcpConnectionPtr& conn);

	//use in main thread.
	muduo::net::EventLoop* getNextClientLoop()
	{
		return clientThreadPool_->getNextLoop();
	}

private:
	typedef std::map<uint16_t, MsgHandler> MsgHandlerMap;
	MsgHandlerMap msgHandlers_;      //map<msgType, Handler>

	typedef boost::shared_ptr<muduo::ThreadPool> ThreadPoolPtr;
	typedef std::vector<ThreadPoolPtr> ThreadPoolVec;
	ThreadPoolVec msgHandlerThreadPools_;     

	muduo::net::EventLoop *loop_;   //readonly. 主线程的loop对象
	muduo::net::InetAddress addr_;  //本server的监听地址
	int servType_;
	int appId_;
	int setId_;
	int serialNo_;
	std::string name_;
	
	int heartCheckInterval_;  //默认TcpServer被动check心跳，TcpClient主动发心跳
	int heartSendInterval_;
	
	boost::shared_ptr<CommHeartbeat> heartbeat_;
	boost::shared_ptr<muduo::net::TcpServer> server_;
	
	ConnInfoManager connsManager_;   //按 servType 存放所有的连接
	PeerServerInfoManager peerServerManager_;  //存放所有的TcpClient相关信息

	muduo::net::EventLoopThreadPool* clientThreadPool_;   //线程池，专门用于 创建tcpclient //只能在主线程访问

	ConnCallBack connCb_;   //连接成功或断开，会回调的用户函数
	WriteCompleteCallBack writeCompleteCb_;
	//
	int autoThreadType_;
};


#endif




