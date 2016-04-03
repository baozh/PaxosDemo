
#include "CommUtil.h"


namespace CommServUtil
{
	typedef std::map<int, std::string> TEventTable;
	TEventTable gServTypeStrTable;
	void InitServTypeStrTable()
	{
#define INIT_CONNTYPE_EVENT(X) gServTypeStrTable[X] = #X
		INIT_CONNTYPE_EVENT(SERV_NONE);
		INIT_CONNTYPE_EVENT(SERV_MSG);
		INIT_CONNTYPE_EVENT(SERV_CS);
		INIT_CONNTYPE_EVENT(SERV_DC);
		INIT_CONNTYPE_EVENT(SERV_THIRPART);
		INIT_CONNTYPE_EVENT(SERV_MONITOR);
		INIT_CONNTYPE_EVENT(SERV_PRELOADER);
		INIT_CONNTYPE_EVENT(SERV_SENDER);
		INIT_CONNTYPE_EVENT(SERV_SDK);
		INIT_CONNTYPE_EVENT(SERV_OFFLINE);
		INIT_CONNTYPE_EVENT(SERV_ACCESS);
		INIT_CONNTYPE_EVENT(SERV_DBAGENT);
		INIT_CONNTYPE_EVENT(SERV_DTCLEANER);
		INIT_CONNTYPE_EVENT(SERV_ADMIN);
	}

	TEventTable gMsgStrTable;
	void InitMsgStrTable()
	{
#define INIT_MSG_EVENT(X) gMsgStrTable[X] = #X

		INIT_MSG_EVENT(PRO_SYS_HEART_REQ);
		INIT_MSG_EVENT(PRO_SYS_HEART_ACK);
		INIT_MSG_EVENT(START_CONNECT_REQ);
		INIT_MSG_EVENT(START_CONNECT_ACK);
		INIT_MSG_EVENT(TO_DC_LAUNCH_CONNECTION_NTY);
		INIT_MSG_EVENT(TO_DC_LOST_CONNNECTION_NTY);
		INIT_MSG_EVENT(TO_DC_CONN_STAT_SYNC_NTY);
		INIT_MSG_EVENT(FROM_DC_LAUNCH_CONNECTION_CMD);
		INIT_MSG_EVENT(FROM_DC_NOTIFY_CLOSE_CONNECTION_REQ);
		INIT_MSG_EVENT(TO_DC_NOTIFY_CLOSE_CONNECTION_ACK);

		INIT_MSG_EVENT(PROPOSER_ACCEPTOR_PREPARE_REQ);
		INIT_MSG_EVENT(ACCEPTOR_PROPOSER_PREPARE_ACK);
		INIT_MSG_EVENT(PROPOSER_ACCEPTOR_ACCEPT_REQ);
		INIT_MSG_EVENT(ACCEPTOR_PROPOSER_ACCEPT_ACK);
		INIT_MSG_EVENT(ACCEPTOR_PROPOSER_ACCEPT_NACK);
	}

	const char *getServTypeStr(int type)
	{
		if(gServTypeStrTable.find(type) != gServTypeStrTable.end())
		{
			return gServTypeStrTable[type].c_str();
		}
		else
		{
			LOG_DEBUG<<"[getServTypeStr] Can't find ServType Desc! ServTypeNo:" << type;
			return "NoneServTypeID";
		}
	}

	const char *getProtoTypeStr(int type)
	{
		if(gMsgStrTable.find(type) != gMsgStrTable.end())
		{
			return gMsgStrTable[type].c_str();
		}
		else
		{
			LOG_DEBUG<<"[getProtoTypeStr] Can't find Msg Desc! ProtoTypeNo:" << type;
			return "NoneMsgID";
		}
	}

	void printConnInfo(const muduo::net::TcpConnectionPtr & conn, bool isForcePrint)
	{
		if (isForcePrint)
		{
			LOG_WARN<<"[Peer_Conn_Info]";
			LOG_WARN<<"IpPort:"<<conn->peerAddress().toIpPort() << ", Fd:" << conn->name();
			CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
			if(context != NULL)
			{
				LOG_WARN << "AppId:" << context->connInfo_.appId_ << ",SetId:" << context->connInfo_.setId_ 
					<< ",SerialNo:" << context->connInfo_.serialNo_
					<< ",ServType:" << context->connInfo_.servType_ << ", ServTypeStr:" << CommServUtil::getServTypeStr(context->connInfo_.servType_);
			}
		}
		else
		{
			LOG_DEBUG<<"[Peer_Conn_Info]";
			LOG_DEBUG<<"IpPort:"<<conn->peerAddress().toIpPort() << ", Fd:" << conn->name();
			CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
			if(context != NULL)
			{
				LOG_DEBUG << "AppId:" << context->connInfo_.appId_ << ",SetId:" << context->connInfo_.setId_ 
					<< ",SerialNo:" << context->connInfo_.serialNo_
					<< ",ServType:" << context->connInfo_.servType_ << ", ServTypeStr:" << CommServUtil::getServTypeStr(context->connInfo_.servType_);
			}
		}

	}

	void forceCloseConn(const muduo::net::TcpConnectionPtr &conn, const std::string& strMsg)
	{
		LOG_WARN << "[forceCloseConn] force close conn! Reason:" << strMsg;
		CommServUtil::printConnInfo(conn, true);
		conn->forceClose();
	}

	bool isDcServer(const muduo::net::TcpConnectionPtr& conn)
	{
		CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
		assert(context);
		if (context->connInfo_.servType_ == CommServUtil::SERV_DC)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void sendJsonMessage(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		int msgLen = 0;
		if (isSendJson == true && jsonStr.size() != 0)
		{
			msgLen = static_cast<int>(DEFAULT_MSG_BASEHEAD_LENTH + jsonStr.size() + 1);
		}
		else
		{
			msgLen = static_cast<int>(DEFAULT_MSG_BASEHEAD_LENTH);
		}

		if (protoType != PRO_SYS_HEART_REQ && protoType != PRO_SYS_HEART_ACK)
		{
			LOG_DEBUG << "[SEND_MESSAGE] MsgId:" << protoType << ", MsgIdStr:" << CommServUtil::getProtoTypeStr(protoType) << ", JsonStr:" << (isSendJson == true ? jsonStr:"NoneJsonStr");
			CommServUtil::printConnInfo(conn, false);
		}

		CommServUtil::MsgBaseHead baseHead(msgLen, protoType);
		char *msg = static_cast<char *>(malloc(msgLen));
		if (msg == NULL)
		{
			LOG_ERROR << "malloc msg space failed! msgLen:" << msgLen << ", msg:" << jsonStr;
			return;
		}

		memcpy(msg, &baseHead, sizeof(baseHead));
		if (isSendJson == true && jsonStr.size() != 0)
		{
			memcpy(msg + sizeof(baseHead), jsonStr.c_str(), jsonStr.size() + 1);
		}

		conn->send(msg, msgLen);

		free(msg);
		msg = NULL;
	}

	void SendMsgUseStack::sendJsonMessage(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		int msgLen = 0;
		if (isSendJson == true && jsonStr.size() != 0)
		{
			msgLen = static_cast<int>(DEFAULT_MSG_BASEHEAD_LENTH + jsonStr.size() + 1);
		}
		else
		{
			msgLen = static_cast<int>(DEFAULT_MSG_BASEHEAD_LENTH);
		}

		if (protoType != PRO_SYS_HEART_REQ && protoType != PRO_SYS_HEART_ACK)
		{
			LOG_DEBUG << "[SEND_MESSAGE] MsgId:" << protoType << ", MsgIdStr:" << CommServUtil::getProtoTypeStr(protoType) << ", JsonStr:" << (isSendJson == true ? jsonStr:"NoneJsonStr");
			CommServUtil::printConnInfo(conn, false);
		}

		CommServUtil::MsgBaseHead baseHead(msgLen, protoType);
		char *msg = NULL;
		bool isUseNewSpace = false;
		if (msgLen < kMaxMsgDataLen)
		{
			memset(buf_, 0, kMaxMsgDataLen);
			msg = buf_;
		}
		else
		{
			isUseNewSpace = true;
			msg = static_cast<char *>(malloc(msgLen));
			if (msg == NULL)
			{
				LOG_ERROR << "malloc msg space failed! msgLen:" << msgLen << ", msg:" << jsonStr;
				return;
			}
		}
		memcpy(msg, &baseHead, sizeof(baseHead));
		if (isSendJson == true && jsonStr.size() != 0)
		{
			memcpy(msg + sizeof(baseHead), jsonStr.c_str(), jsonStr.size() + 1);
		}

		conn->send(msg, msgLen);

		if (isUseNewSpace == true)
		{
			free(msg);
			msg = NULL;
		}
	}


}





