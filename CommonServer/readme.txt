
˵����

ʹ�ã�
1. ʹ��CommonServer�������CommonServer.h
	���Ҫʹ��Server����:
		��main�����У�
		//(1)��������
		CommonServer server;

		muduo::net::EventLoop loop;
		int clientThreadNum = 5;
		//(2)��ʼ��
		if (server.init(&loop, "ThirdpartServer", gConfigInfo.systemInfo_.appId_, gConfigInfo.systemInfo_.setId_, gConfigInfo.systemInfo_.serialNo_,
						CommServUtil::SERV_THIRPART, clientThreadNum, gConfigInfo.systemInfo_.heartCheckCycle_) ==  false)
		{
			LOG_ERROR << "CommonServer init failed!";
			printf("CommonServer init failed!\n");
			return -1;
		}

		//(3)ע����Ϣ����� �ص�����
		server.regMessageHandler(MSG_THIRDPART_PUSH_MSG_CONTENT_CMD, boost::bind(&PushMsgContentCmd, _1, _2, _3));
		server.regMessageHandler(DC_THIRDPART_CHANGE_LOG_LEVEL_REQ, boost::bind(&ChangeLogLevel, _1, _2, _3));
		
		//(4)��������server
		muduo::net::InetAddress iAddr(gConfigInfo.systemInfo_.ip_, gConfigInfo.systemInfo_.port_);
		if (server.accept(iAddr, gConfigInfo.systemInfo_.netThreadNum_) == false)
		{
			//�������һ���������(core dump)���Ǻ��п��ܶ˿ڱ�ռ���ˣ�����bindʧ��
			LOG_ERROR << "CommonServer accept failed!";
			printf("CommonServer accept failed!\n");
			return -1;
		}

		//(5)����dcServer
		bool isRetry = true;
		muduo::net::InetAddress dcAddr(gConfigInfo.systemInfo_.dcServerIp_, gConfigInfo.systemInfo_.dcServerPort_);
		if (server.startConnect(dcAddr, gConfigInfo.systemInfo_.appId_, 0, 0, CommServUtil::SERV_DC, isRetry) == false)
		{
			LOG_ERROR << "CommonServer startConnect dcServer failed!";
			printf("CommonServer startConnect dcServer failed!\n");
			return -1;
		}

		loop.loop();

	���ֻʹ��Client����(��monitor):	
		������Ĺ��� ���ƣ���ͬ���ǣ����� accept�ӿڣ���ֻ�� startConnect�ӿڼ��ɡ�

2. MsgIdDefine.h ���������е���Ϣ�š�
3. CommUtil.h ������һЩ���õĽṹ�壬������ ServerContext�������Ҫ����context�Ĳ�����ֱ���ں��� ���� ���ɡ�
   �����˳��õĺ�����sendJsonMessage��forceCloseConn, printConnInfo, isDcServer, SendMsgUseStack

   



���ܣ�
1. ���������������ܣ�������������ء���ʱ������Ĭ���� Client���� ��������Server����������Client��Server��Ҫ����Ƿ�������ʱ��
   ��Ҫ���� �������ļ����Ĭ��60�룩����������ļ����Ĭ��180�룩   

2. ��Զ�Server�����ӣ�����TcpClient�����������������Server��Ϣ��

3. ʵʱ��DcServerͬ������״̬���������ӡ��Ͽ�����ʱ ��DcServer�ϱ���ʵʱ ͬ������״̬����ӦDc�� ��������ر�����֪ͨ�ȡ�

4. ע�� ��Ϣ�����������Ҫ�޸�Ĭ�ϵ���Ϣ��������ֻ�� ��regMessageHandler �滻���ɡ�

5. ����������Ϣ���ᰴservType, servIpPort, TcpClient�ȷ��������Ϣ��

6. �ṩ�˲�ͬ�ӿ� ������Ϣ��




ע�����
1. ע���msgHandler������ Լ��һ�������߼������json����ʧ�ܣ��͹ر����ӡ�
   
2. �ڵ���createThreadPoolǰ����Ҫ �ȵ� init ��ʼ����



��muduo�����޸ģ�
1. muduo/net/TcpClient.cc���ϣ�
 void disableRetry() {retry_ = false;}

2. ��muduo/net/TimeId.h�м��ϣ�   
int64_t getSeq() {return sequence_;};
Timer* getTimer() {return timer_;};

3. ��muduo/net/Connector.h�м��ϣ�
bool isConnected() {return state_== kConnected;};
��muduo/net/TcpClient.cc���ϣ�
bool TcpClient::isConnected() 
{
	return connector_->isConnected();
}

4. ��muduo/net/EventLoopThreadPool.cc���ϣ�
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




