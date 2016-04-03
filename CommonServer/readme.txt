
说明：

使用：
1. 使用CommonServer，需包含CommonServer.h
	如果要使用Server功能:
		在main函数中：
		//(1)声明变量
		CommonServer server;

		muduo::net::EventLoop loop;
		int clientThreadNum = 5;
		//(2)初始化
		if (server.init(&loop, "ThirdpartServer", gConfigInfo.systemInfo_.appId_, gConfigInfo.systemInfo_.setId_, gConfigInfo.systemInfo_.serialNo_,
						CommServUtil::SERV_THIRPART, clientThreadNum, gConfigInfo.systemInfo_.heartCheckCycle_) ==  false)
		{
			LOG_ERROR << "CommonServer init failed!";
			printf("CommonServer init failed!\n");
			return -1;
		}

		//(3)注册消息处理的 回调函数
		server.regMessageHandler(MSG_THIRDPART_PUSH_MSG_CONTENT_CMD, boost::bind(&PushMsgContentCmd, _1, _2, _3));
		server.regMessageHandler(DC_THIRDPART_CHANGE_LOG_LEVEL_REQ, boost::bind(&ChangeLogLevel, _1, _2, _3));
		
		//(4)创建监听server
		muduo::net::InetAddress iAddr(gConfigInfo.systemInfo_.ip_, gConfigInfo.systemInfo_.port_);
		if (server.accept(iAddr, gConfigInfo.systemInfo_.netThreadNum_) == false)
		{
			//如果在这一步程序崩溃(core dump)，那很有可能端口被占用了，导致bind失败
			LOG_ERROR << "CommonServer accept failed!";
			printf("CommonServer accept failed!\n");
			return -1;
		}

		//(5)连接dcServer
		bool isRetry = true;
		muduo::net::InetAddress dcAddr(gConfigInfo.systemInfo_.dcServerIp_, gConfigInfo.systemInfo_.dcServerPort_);
		if (server.startConnect(dcAddr, gConfigInfo.systemInfo_.appId_, 0, 0, CommServUtil::SERV_DC, isRetry) == false)
		{
			LOG_ERROR << "CommonServer startConnect dcServer failed!";
			printf("CommonServer startConnect dcServer failed!\n");
			return -1;
		}

		loop.loop();

	如果只使用Client功能(如monitor):	
		与上面的过程 类似，不同的是：不调 accept接口，而只调 startConnect接口即可。

2. MsgIdDefine.h 定义了所有的消息号。
3. CommUtil.h 定义了一些常用的结构体，尤其是 ServerContext，如果需要增加context的参数，直接在后面 加上 即可。
   定义了常用的函数：sendJsonMessage，forceCloseConn, printConnInfo, isDcServer, SendMsgUseStack

   



功能：
1. 自主处理心跳功能：心跳信令的来回、超时断链。默认是 Client主动 发心跳，Server回心跳。而Client、Server均要检测是否心跳超时。
   需要传入 发心跳的间隔（默认60秒）、检测心跳的间隔（默认180秒）   

2. 与对端Server的连接：创建TcpClient、处理连接信令、交换Server信息。

3. 实时与DcServer同步连接状态：建立连接、断开连接时 向DcServer上报，实时 同步连接状态，响应Dc的 连接命令、关闭连接通知等。

4. 注册 消息处理函数。如果要修改默认的消息处理函数，只需 调regMessageHandler 替换即可。

5. 管理连接信息，会按servType, servIpPort, TcpClient等分类管理信息。

6. 提供了不同接口 发送消息。




注意事项：
1. 注册的msgHandler函数中 约定一个处理逻辑：如果json解析失败，就关闭连接。
   
2. 在调用createThreadPool前，需要 先调 init 初始化。



对muduo做的修改：
1. muduo/net/TcpClient.cc加上：
 void disableRetry() {retry_ = false;}

2. 在muduo/net/TimeId.h中加上：   
int64_t getSeq() {return sequence_;};
Timer* getTimer() {return timer_;};

3. 在muduo/net/Connector.h中加上：
bool isConnected() {return state_== kConnected;};
在muduo/net/TcpClient.cc加上：
bool TcpClient::isConnected() 
{
	return connector_->isConnected();
}

4. 在muduo/net/EventLoopThreadPool.cc加上：
EventLoop* EventLoopThreadPool::getNextLoopInOtherThread()
{
	assert(started_);
	EventLoop* loop = baseLoop_;

	if (!loops_.empty())
	{
		// round-robin
		lockMutex_.lock();
		loop = loops_[next_];
		++next_;
		if (implicit_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
		lockMutex_.unlock();
	}
	return loop;
}




