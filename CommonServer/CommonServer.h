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
	//ע��Լ�� json��������� ��������false, ������� ������true
	typedef boost::function<void (CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn, bool isConnected)> ConnCallBack;
	typedef boost::function<void (CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn)> WriteCompleteCallBack;

	CommonServer();
	~CommonServer();

public:
	//�������ӿ�
	//not thread safe, should call in main thread.
	bool init(muduo::net::EventLoop* loop, const std::string& name, int appId, int setId, int serialNo, int servType, int clientThreadNum,
			int heartCheckInterval = CommServUtil::kDefaultHeartCheckInterval, 
			int heartSendInterval = CommServUtil::kDefaultHeartSendInterval);   //��ʼ����������new Heartbeat
	
	//thread safe
	bool accept(const muduo::net::InetAddress& addr, int thread_num);
	
	//not thread safe, should call in main thread. �������ӣ����Ϻ�ᷢstart_connect_req. 
	//�豣֤ Ҫ���ӵ�server ���Լ��� ��ͬһ��set��(appid, setid��ͬ)������ ˫���������Ͽ�����
	bool startConnect(const InetAddress& serverAddr, int appId, int setId, int serialNo, int servType, bool isRetry);
	
	//not thread safe, should call in main thread.  //ע����Ϣ������
	void regMessageHandler(uint16_t msgNo, const MsgFunc &fun);
	//not thread safe, should call in main thread.
	void regMessageHandler(uint16_t msgNo, const MsgFunc &fun, int threadType);

	//thread safe  //�ص���ʱ����1)�������ӳɹ� �� ��Զ�Server start_connect ������ɹ�  2)���ӶϿ�
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

	//thread safe. ��� �Ҳ��� ��Ӧ��TcpConnPtr, �ͷ���false.
	//�����һ��servType�µ�����conn, �ٶ���һ��.
	bool sendJsonMessage(int appId, int setId, int serialNo, int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr);

	//thread safe. �� �������� ������Ϣ (����ѯ�ķ�ʽ) 
	//�ٶȿ�
	void sendJsonMessage(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr);
	//thread safe. �ٶȸ���
	void sendJsonMessageToDc(uint16_t protoType, bool isSendJson, const std::string& jsonStr);
	//thread safe. ��˷������͵�����server ������Ϣ 
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

	muduo::net::EventLoop *loop_;   //readonly. ���̵߳�loop����
	muduo::net::InetAddress addr_;  //��server�ļ�����ַ
	int servType_;
	int appId_;
	int setId_;
	int serialNo_;
	std::string name_;
	
	int heartCheckInterval_;  //Ĭ��TcpServer����check������TcpClient����������
	int heartSendInterval_;
	
	boost::shared_ptr<CommHeartbeat> heartbeat_;
	boost::shared_ptr<muduo::net::TcpServer> server_;
	
	ConnInfoManager connsManager_;   //�� servType ������е�����
	PeerServerInfoManager peerServerManager_;  //������е�TcpClient�����Ϣ

	muduo::net::EventLoopThreadPool* clientThreadPool_;   //�̳߳أ�ר������ ����tcpclient //ֻ�������̷߳���

	ConnCallBack connCb_;   //���ӳɹ���Ͽ�����ص����û�����
	WriteCompleteCallBack writeCompleteCb_;
	//
	int autoThreadType_;
};


#endif




