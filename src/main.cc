#include <cassert>
#include <cstdio>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "muduo/base/InitLog.h"
#include "muduo/base/TimeZone.h"
#include "muduo/base/Logging.h"
#include "DataStructDefine.h"
#include "ProcessMsgHandler.h"
#include "CommonServer/CommonServer.h"

using namespace muduo;


ConfigInfo gConfigInfo;
muduo::TimeZone gTimeZone(::getenv("MUDUO_TZ_PATH")?::getenv("MUDUO_TZ_PATH"):"/etc/localtime");
MessageHandler *gpMsgHandler = NULL;
CommonServer *gpCommServer = NULL;



CommonServer* creatServer(uint16_t servType, const std::string& servName, muduo::net::EventLoop* loop, 
	const std::string& ipport, uint16_t port, int appId, int setId, int serialNo)
{
	CommonServer *server = new CommonServer();

	int clientThreadNum = 1;
	//初始化
	if (server->init(loop, servName, appId, setId, serialNo,
		servType, clientThreadNum) ==  false)
	{
		LOG_ERROR << "CommonServer init failed!";
		printf("CommonServer init failed!\n");
		delete server;
		return NULL;
	}

	//注册消息处理的 回调函数
	gpMsgHandler = new MessageHandler();
	assert(gpMsgHandler != NULL);
	gpMsgHandler->regAllMsgHandler(server);

	//创建监听server
	muduo::net::InetAddress iAddr(ipport, port);
	int netThreadNum = 4;
	if (server->accept(iAddr, netThreadNum) == false)
	{
		LOG_ERROR << "CommonServer accept failed!";
		printf("CommonServer accept failed!\n");
		delete server;
		delete gpMsgHandler;
		return NULL;
	}
	return server;
}

int main()
{ 
	//读取配置
	gConfigInfo.readConfig();

	//初始化日志线程
	muduo::InitLog initLog;
	initLog.init("PaxosDemo", "./", 1, 50000000);
	
	//创建CommonServer
	if (gConfigInfo.servInfo_.find(gConfigInfo.myServId_) == gConfigInfo.servInfo_.end())
	{
		printf("Can't find this server IpPort! servId:%d\n", gConfigInfo.myServId_);
		return -1;
	}
	std::string servIp = gConfigInfo.servInfo_[gConfigInfo.myServId_].ip_;
	uint16_t servPort  = gConfigInfo.servInfo_[gConfigInfo.myServId_].port_;

	char servNameStr[48] = {0};
	snprintf(servNameStr, sizeof(servNameStr), "paxos_dc_%d", gConfigInfo.myServId_);

	muduo::net::EventLoop loop;
	gpCommServer = creatServer(CommServUtil::SERV_CS, servNameStr, &loop, servIp, 
		servPort, 550, 0, gConfigInfo.myServId_);
	assert(gpCommServer != NULL);
	
	//连接其它server
	std::map<int, ConfigInfo::ServInfo>::iterator iter = gConfigInfo.servInfo_.begin();
	for (; iter != gConfigInfo.servInfo_.end(); iter++)
	{
		//由 servId 大的 server 主动连 servId 小的server.
		if (gConfigInfo.myServId_ > iter->first)
		{
			bool isRetry = true;
			muduo::net::InetAddress servAddr(iter->second.ip_, iter->second.port_);
			if (gpCommServer->startConnect(servAddr, 550, 0, iter->first, CommServUtil::SERV_CS, isRetry) == false)
			{
				LOG_ERROR << "CommonServer startConnect failed!";
				printf("CommonServer startConnect failed!\n");
				return -1;
			}
		}
	}

	loop.loop();

	if (gpMsgHandler != NULL)
		delete gpMsgHandler;
	if (gpCommServer != NULL)
		delete gpCommServer;
}

