
#include <stdio.h>
#include <vector>
#include "CommonServer.h"
#include "CommProto.h"
#include "CommHeartbeat.h"


using namespace CommServUtil;

bool CommonServer::sendJsonMessage(int appId, int setId, int serialNo, int servType, 
					uint16_t protoType, bool isSendJson, const std::string& jsonStr)
{
	return connsManager_.sendJsonMessage(appId, setId, serialNo, servType, protoType, isSendJson, jsonStr);
}

void CommonServer::sendJsonMessage(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
{
	connsManager_.sendJsonMessage(servType, protoType, isSendJson, jsonStr);
}

void CommonServer::sendJsonMsgToOneServType(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
{
	connsManager_.sendJsonMsgToOneServType(servType, protoType, isSendJson, jsonStr);
}

void CommonServer::sendJsonMessageToDc(uint16_t protoType, bool isSendJson, const std::string& jsonStr)
{
	connsManager_.sendJsonMessageToDc(protoType, isSendJson, jsonStr);
}

CommonServer::CommonServer()
{
	InitServTypeStrTable();
	InitMsgStrTable();

	loop_ = NULL;
	servType_ = CommServUtil::SERV_NONE;
	appId_ = -1;
	setId_ = -1;
	serialNo_ = -1;
	name_ = "";
	heartSendInterval_ = CommServUtil::kDefaultHeartSendInterval;
	heartCheckInterval_ = CommServUtil::kDefaultHeartCheckInterval;
	clientThreadPool_ = NULL;
	connCb_ = NULL;
	writeCompleteCb_ = NULL;
	autoThreadType_ = CommServUtil::kDefaultThreadType;
}

CommonServer::~CommonServer()
{


}

void CommonServer::updateHeartStatInConnLoop(const muduo::net::TcpConnectionPtr &conn)
{
	heartbeat_->updateHeartbeat(conn);
}

bool CommonServer::init(muduo::net::EventLoop* loop, const std::string& name, int appId, int setId, int serialNo, int servType, 
						int clientThreadNum, int heartCheckInterval, int heartSendInterval)
{
#define DEFAULT_CLIENT_THREAD_NUM    3

	name_ = name;
	loop_ = loop;
	appId_ = appId;
	setId_ = setId;
	serialNo_ =  serialNo;
	servType_ = servType;
	heartCheckInterval_ = heartCheckInterval;
	heartSendInterval_ = heartSendInterval;
	//
	//注册回调
	regMessageHandler(PRO_SYS_HEART_REQ, boost::bind(&CommonServer::defaultHeartReq, this, _1, _2, _3));
	regMessageHandler(PRO_SYS_HEART_ACK, boost::bind(&CommonServer::defaultHeartAck, this, _1, _2, _3));

	regMessageHandler(START_CONNECT_REQ, boost::bind(&CommonServer::defaultStartConnectReq, this, _1, _2, _3));
	regMessageHandler(START_CONNECT_ACK, boost::bind(&CommonServer::defaultStartConnectAck, this, _1, _2, _3));

	regMessageHandler(FROM_DC_LAUNCH_CONNECTION_CMD, boost::bind(&CommonServer::defaultLaunchConnCmd, this, _1, _2, _3));
	regMessageHandler(FROM_DC_NOTIFY_CLOSE_CONNECTION_REQ, boost::bind(&CommonServer::defaultNotifyCloseConnReq, this, _1, _2, _3));
	//
	heartbeat_.reset(new CommHeartbeat(loop, this, heartCheckInterval, heartSendInterval));
	clientThreadPool_ = new muduo::net::EventLoopThreadPool(loop, "clientpool");

	loop_->assertInLoopThread();

	if (clientThreadNum > 0)
	{
		clientThreadPool_->setThreadNum(clientThreadNum);
	}
	else
	{
		clientThreadPool_->setThreadNum(DEFAULT_CLIENT_THREAD_NUM);
	}
	clientThreadPool_->start();
	//
	return true;
}

bool CommonServer::accept(const muduo::net::InetAddress& addr, int thread_num)
{
	addr_ = addr;
	muduo::net::TcpServer* pTcpServer = new muduo::net::TcpServer(loop_, addr_, name_);
	if (pTcpServer == NULL)
	{
		LOG_ERROR<<"create server failed! addr:" << addr_.toIpPort();
		return false;
	}

	server_.reset(pTcpServer);

	server_->setConnectionCallback(
		boost::bind(&CommonServer::onConnection, this, _1));
	server_->setMessageCallback(
		boost::bind(&CommonServer::onMessage, this, _1, _2, _3));

	server_->setThreadNum(thread_num);
	server_->start();

	return true;
}

bool CommonServer::startConnect(const InetAddress& serverAddr, int appId, int setId, int serialNo, int servType, bool isRetry)
{
	loop_->assertInLoopThread();

	boost::shared_ptr<muduo::net::TcpClient> client(new muduo::net::TcpClient(getNextClientLoop(), serverAddr, "tcp_client"));

	PeerServerInfoManager::PeerServerInfo peerInfo;
	peerInfo.appId_ = appId;
	peerInfo.setId_ = setId;
	peerInfo.serialNo_ = serialNo;
	peerInfo.servType_ = servType;
	peerInfo.isRetry_ = isRetry;

	if (isRetry)
	{
		client->enableRetry();
	}
	client->setConnectionCallback(
		boost::bind(&CommonServer::onClientConnection, this, _1));
	client->setMessageCallback(
		boost::bind(&CommonServer::onClientMessage, this, _1, _2, _3));

	client->connect();

	//保存信息
	peerServerManager_.setServerInfo(serverAddr.toIpPort(), peerInfo, client);
	
	if (servType == CommServUtil::SERV_DC)
	{
		//默认每5分钟 向Dc同步连接状态
		loop_->runEvery(5*60, boost::bind(&CommonServer::syncConnStatWithDc,this));
	}
	return true;
}

void CommonServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
	LOG_WARN << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "TCP_UP" : "TCP_DOWN");

	if (conn->connected())
	{
		conn->setTcpNoDelay(true);
		conn->setWriteCompleteCallback(boost::bind(&CommonServer::WriteCompleteCB, this, _1));

		//插入conn->context
		CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
		if(NULL == context)
		{
			conn->setContext(CommServUtil::ServerContext());
			context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
			assert(context);
		}

		//更新心跳
		updateHeartStatInConnLoop(conn);
	}
	else
	{
		//调用回调
		if (connCb_)
		{
			connCb_(this, conn, false);
		}

		if (CommServUtil::isDcServer(conn) == false)
		{
			//通知DC 断开连接
			notifyLaunchConnOrLostConnWithDc(conn, TO_DC_LOST_CONNNECTION_NTY);
		}

		//删除peerClientInfo信息
		LOG_WARN << "delete PeerServerInfo!";
		CommServUtil::printConnInfo(conn, true);

		CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
		assert(context);
		connsManager_.deleteConnPtr(context->connInfo_.servType_, conn);
	}
}

void CommonServer::notifyLaunchConnOrLostConnWithDc(const muduo::net::TcpConnectionPtr& conn, MessageType type)
{
	CommServUtil::ServerContext *context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context);

	std::string strJsonNew = "";
	if(CommProtoUtil::transLaunchConnNty(context->connInfo_.servType_, 
		context->connInfo_.appId_, context->connInfo_.setId_, context->connInfo_.serialNo_, strJsonNew) == false)
	{
		LOG_ERROR << "transLaunchConnNty failed.";
	}
	sendJsonMessageToDc(type, true, strJsonNew);
}

void CommonServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp time)
{
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context != NULL);
	
	int readableBytes;
	int packetLen;
	while(((readableBytes = buf->readableBytes()) >= DEFAULT_MSG_BASEHEAD_LENTH)
		&& ((packetLen = buf->peekInt16()) <= readableBytes ))
	{
		if(packetLen < DEFAULT_MSG_BASEHEAD_LENTH)
		{
			LOG_ERROR << "invalid packet length: " << packetLen;
			conn->forceClose();
			return;
		}
		//
		packetLen = buf->readInt16();
		uint16_t protoType =  buf->readInt16();
		int dateLen = packetLen - DEFAULT_MSG_BASEHEAD_LENTH;
		//
		analyse(conn, protoType, buf->peek(), dateLen);
		//
		buf->retrieve(dateLen);
	}
}

bool CommonServer::analyse(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, const char *data, int size)
{
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context != NULL);
	//
	//打印信息
	if (protoType != PRO_SYS_HEART_REQ && protoType != PRO_SYS_HEART_ACK)
	{
		printConnInfo(conn, false);
		LOG_DEBUG << "[RECV_MESSAGE] MsgId:" << protoType << ", MsgIdStr:" << CommServUtil::getProtoTypeStr(protoType) << ", data:" << std::string(data, size);
	}

	//
	MsgHandlerMap::iterator iter = msgHandlers_.find(protoType);
	if (iter != msgHandlers_.end())
	{
		int threadType = iter->second.threadType();
		if(CommServUtil::kDefaultThreadType == threadType)
		{
			PacketBuffer pb(data, size);
			iter->second.doIt(this, conn, pb);
		}
		else
		{
			assert(threadType >= 0 && threadType < msgHandlerThreadPools_.size());
			boost::shared_array<char> sa(new char[size]);
			memcpy(sa.get(), data, size);
			PacketBuffer pb(sa, size);
			msgHandlerThreadPools_[threadType]->run(boost::bind(&MsgHandler::doIt, &(iter->second), this, conn, pb));
		}
	}
	else
	{
		LOG_WARN << "Can't find the msgHandler! msgID:" << protoType;
	}
	return true;
}

void CommonServer::onClientConnection(const muduo::net::TcpConnectionPtr& conn)
{
	LOG_WARN << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "TCP_UP" : "TCP_DOWN");

	if (conn->connected())
	{
		conn->setTcpNoDelay(true);
		conn->setWriteCompleteCallback(boost::bind(&CommonServer::WriteCompleteCB, this, _1));

		//插入conn->context
		CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
		if(NULL == context)
		{
			conn->setContext(CommServUtil::ServerContext());
			context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
			assert(context);

			//设置connInfo的值
			peerServerManager_.getConnInfo(conn->peerAddress(), context->connInfo_);
		}
		
		//发送连接请求
		std::string strJson = "";
		if(CommProtoUtil::transStartConnectReq(servType_, addr_.toPort(), appId_, setId_, serialNo_, strJson) == false)
		{
			LOG_ERROR << "tranStartConnectReq failed.";
			return;
		}
		CommServUtil::sendJsonMessage(conn, START_CONNECT_REQ, true, strJson);

		//更新心跳 
		updateHeartStatInConnLoop(conn);
	}
	else
	{
		//调用回调
		if (connCb_)
		{
			connCb_(this, conn, false);
		}

		if (CommServUtil::isDcServer(conn) == false)
		{
			//通知DC 断开连接
			notifyLaunchConnOrLostConnWithDc(conn, TO_DC_LOST_CONNNECTION_NTY);
		}

		//删除peerServerInfo信息
		CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
		assert(context);
		connsManager_.deleteConnPtr(context->connInfo_.servType_, conn);

		peerServerManager_.delServerInfo(conn->peerAddress(), conn->localAddress());
	}
}

void CommonServer::onClientMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp time)
{
	onMessage(conn, buf, time);
}

//作为 服务端收到 对端的start_connect_req 
void CommonServer::defaultStartConnectReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context != NULL);

	int servType = 0;
	int appId = 0;
	int setId = 0;
	int serialNo = 0;
	int servPort = 0;

	if(CommProtoUtil::parseStartConnectReq(data.data(), data.size(), servType, appId, setId, serialNo, servPort) == false)
	{
		LOG_ERROR<<"parseStartConnectReq failed.";
		CommServUtil::forceCloseConn(conn, "parse json string failed!");
		return ;
	}

	LOG_DEBUG<<"servType:"<<servType << ", servTypeStr:" << CommServUtil::getServTypeStr(servType)
		<< ", appId:" << appId << ", setId:" << setId << ", serialNo:" << serialNo;

	//如果不在同一个set内，则断开连接
	if (servType_ != CommServUtil::SERV_DC &&
		(appId != appId_ || setId != setId_))
	{
		LOG_WARN << "Not in same set! so close conn.";
		LOG_WARN << "Peer Server Info:  servType:"<<servType << ", servTypeStr:" << CommServUtil::getServTypeStr(servType)
			<< ", appId:" << appId << ", setId:" << setId << ", serialNo:" << serialNo;
		LOG_WARN << "local appId:" << appId_ << ", setId:" << setId_;
		forceCloseConn(conn, "Not in same set.");
		return ;
	}

	//更新context->connInfo
	context->connInfo_.appId_ = appId;
	context->connInfo_.setId_ = setId;
	context->connInfo_.serialNo_ = serialNo;
	context->connInfo_.servType_ = servType;
	context->connInfo_.servPort_ = servPort;
	context->isExchangeInfo_ = true;

	//保存到map中
	connsManager_.addConnPtr(servType, conn);

	//调用回调
	if (connCb_)
	{
		connCb_(this, conn, true);
	}

	//返回响应
	std::string strJson = "";
	if(CommProtoUtil::transStartConnectAck(servType_, appId_, setId_, serialNo_, strJson) == false)
	{
		LOG_ERROR << "transStartConnectAck failed.";
		return ;
	}
	CommServUtil::sendJsonMessage(conn, START_CONNECT_ACK, true, strJson);

	//通知Dc，已经与 对端Server 连上
	if (servType_ != CommServUtil::SERV_DC)
	{
		notifyLaunchConnOrLostConnWithDc(conn, TO_DC_LAUNCH_CONNECTION_NTY);
	}
	return ;
}

//作为客户端 收到start_connect_ack
void CommonServer::defaultStartConnectAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context != NULL);

	int servType = 0;
	int appId = 0;
	int setId = 0;
	int serialNo = 0;

	if(CommProtoUtil::parseStartConnectAck(data.data(), data.size(), servType, appId, setId, serialNo) == false)
	{
		LOG_ERROR<<"parseStartConnectAck failed.";
		CommServUtil::forceCloseConn(conn, "parse json string failed!");
		return ;
	}

	LOG_DEBUG<<"[defaultStartConnectAck] connType:"<< servType << ", appId:" << appId << ", setId:" << setId << ", serialNo:" << serialNo;

	//如果不在同一个set内，则断开连接
	if (servType_ != CommServUtil::SERV_DC &&
		(appId != appId_ || setId != setId_))
	{
		LOG_WARN << "Not in same set! so close conn.";
		LOG_WARN << "Peer Server Info:  servType:"<<servType << ", servTypeStr:" << CommServUtil::getServTypeStr(servType)
			<< ", appId:" << appId << ", setId:" << setId << ", serialNo:" << serialNo;
		LOG_WARN << "local appId:" << appId_ << ", setId:" << setId_;
		forceCloseConn(conn, "Not in same set.");
		return ;
	}

	context->connInfo_.appId_ = appId;
	context->connInfo_.setId_ = setId;
	context->connInfo_.serialNo_ = serialNo;
	context->connInfo_.servType_ = servType;
	context->connInfo_.servPort_ = conn->peerAddress().toPort();
	context->isExchangeInfo_ = true;

	//保存到map中
	connsManager_.addConnPtr(servType, conn);
	peerServerManager_.setConnInfo(conn->peerAddress(), appId, setId, serialNo, servType);

	//调用回调
	if (connCb_)
	{
		connCb_(this, conn, true);
	}

	//通知Dc，已经与 对端Server 连上
	if (servType_ != CommServUtil::SERV_DC)
	{
		notifyLaunchConnOrLostConnWithDc(conn, TO_DC_LAUNCH_CONNECTION_NTY);
	}
	return;
}

void CommonServer::defaultLaunchConnCmd(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	std::vector<CommServUtil::ExtraConnInfo> vecConnInfo;
	if(CommProtoUtil::parseLauchConnCmd(data.data(), data.size(), vecConnInfo) == false)
	{
		LOG_ERROR << "parseLauchConnCmd failed.";
		CommServUtil::forceCloseConn(conn, "parse json string failed!");
		return ;
	}

	bool isRetry = true;
	std::vector<CommServUtil::ExtraConnInfo>::iterator iter = vecConnInfo.begin();
	for (; iter != vecConnInfo.end(); iter++)
	{
		InetAddress servAddr(iter->ip_, iter->port_);
		loop_->runInLoop(boost::bind(&CommonServer::startConnect, this, servAddr, iter->appId_, iter->setId_, iter->serialNo_, iter->servType_, isRetry));
	}
	return ;
}

void CommonServer::defaultNotifyCloseConnReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	int appId = 0;
	int setId = 0;
	int serialNo = 0;
	int servType = 0;

	if(CommProtoUtil::parseNotifyCloseConnReq(data.data(), data.size(), servType, appId, setId, serialNo) == false)
	{
		LOG_ERROR <<" parseNotifyCloseConnReq failed.";
		CommServUtil::forceCloseConn(conn, "parse json string failed!");
		return;
	}
	
	//设置停止重连
	peerServerManager_.disableRetry(appId, setId, serialNo, servType);

	//回复 ack
	std::string strJson = "";
	if(CommProtoUtil::transNotifyCloseConnAck(servType, appId, setId, serialNo, strJson) == false)
	{
		LOG_ERROR << "transNotifyCloseConnAck failed.";
		return ;
	}
	CommServUtil::sendJsonMessage(conn, TO_DC_NOTIFY_CLOSE_CONNECTION_ACK, true, strJson);
	return;
}

void CommonServer::syncConnStatWithDc()
{
	std::vector<CommServUtil::ConnInfo> verConnInfo;
	connsManager_.getPeerInfoExceptDc(verConnInfo);

	std::string strJson = "";
	if(CommProtoUtil::transSyncConnStat(verConnInfo, strJson) == false)
	{
		LOG_ERROR << "transStartConnectAck failed.";
		return;
	}
	sendJsonMessageToDc(TO_DC_CONN_STAT_SYNC_NTY, true, strJson);
}

void CommonServer::regMessageHandler(uint16_t msgNo, const MsgFunc &fun)
{
	loop_->assertInLoopThread();
	msgHandlers_[msgNo] = MsgHandler(fun, CommServUtil::kDefaultThreadType);
}

void CommonServer::regMessageHandler(uint16_t msgNo, const MsgFunc &fun, int threadType)
{
	loop_->assertInLoopThread();
	assert(threadType >= 0 && threadType < msgHandlerThreadPools_.size());
	msgHandlers_[msgNo] = MsgHandler(fun, threadType);
}

int CommonServer::createThreadPool(int threadNum)
{
	loop_->assertInLoopThread();
	//
	++autoThreadType_;
	//
	char poolName[48] = {0};
	snprintf(poolName, sizeof(poolName), "threadpool-%d", autoThreadType_);
	//
	//
	ThreadPoolPtr threadPool(new muduo::ThreadPool(poolName));
	threadPool->start(threadNum);

	assert(msgHandlerThreadPools_.size() == autoThreadType_);
	msgHandlerThreadPools_.push_back(threadPool);
	LOG_WARN << "Create threadpool, poolName:" << poolName << ", threadNum:" << threadNum << ", generate thread_type:" << autoThreadType_;
	return autoThreadType_;
}

boost::shared_ptr<muduo::ThreadPool> CommonServer::getThreadPool(int threadPoolType)
{
	if (threadPoolType >= 0 && threadPoolType <= autoThreadType_)
	{
		return msgHandlerThreadPools_[threadPoolType];
	}
	else
	{
		return boost::shared_ptr<muduo::ThreadPool>();
	}
}

void CommonServer::defaultHeartReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	LOG_DEBUG<<"[defaultHeartReq] receive heart_req. peerIpPort:" << conn->peerAddress().toIpPort();
	CommServUtil::sendJsonMessage(conn, PRO_SYS_HEART_ACK, false, "");

	//更新心跳时间
	updateHeartStatInConnLoop(conn);
}

void CommonServer::defaultHeartAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	LOG_DEBUG<<"[defaultHeartAck] receive heart_ack. peerIpPort:" << conn->peerAddress().toIpPort();
	CommServUtil::printConnInfo(conn, false);
	
	//更新心跳时间
	updateHeartStatInConnLoop(conn);
}

void CommonServer::getPeerServerConn(std::vector<TcpConnectionPtr> &connVec)
{
	peerServerManager_.getPeerServerConn(connVec);
}

void CommonServer::getAllClientConn(std::vector<muduo::net::TcpConnectionPtr> &connVec)
{
	connsManager_.getAllClientConn(connVec);
}

void CommonServer::getAllConn(std::vector<TcpConnectionPtr> &connVec)
{
	loop_->assertInLoopThread();
	if (server_)
	{
		server_->getAllConnections(connVec);
	}
}

void CommonServer::WriteCompleteCB(const muduo::net::TcpConnectionPtr& conn)
{
	if (writeCompleteCb_)
	{
		writeCompleteCb_(this, conn);
	}
}



