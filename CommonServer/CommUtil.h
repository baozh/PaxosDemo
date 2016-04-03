#ifndef __MODULE_COMMON_UTIL_H
#define __MODULE_COMMON_UTIL_H


#include <string>
#include <map>
#include "muduo/base/CurrentThread.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/base/Logging.h"
#include "MsgIdDefine.h"



namespace CommServUtil
{
	const int kDefaultHeartSendInterval = 60;
	const int kDefaultHeartCheckInterval = 180;
	const int kDefaultThreadType = -1;

	struct ConnInfo
	{
		//OK: �������ӳɹ���REMOVE:admin�����ر�ĳ��server, TIMEOUT:���粨����ɵ�������ʱ��ACCIDENT:�ֶ�kill��server����
		enum resultType{
			ERROR = -1, OK, FAIL, ACCIDENT, REMOVE, TIMEOUT
		};

		ConnInfo() {clean();};

		void clean()
		{
			appId_ = -1;
			setId_ = -1;
			serialNo_ = -1;
			servType_ = -1;
			servPort_ = 0;
			bkConnStat_ = ACCIDENT;
		};

		int setId_;
		int serialNo_;
		int appId_;
		int servType_;
		uint16_t servPort_;     //listen port
		uint16_t bkConnStat_;   //�Զ˶Ͽ���ԭ��REMOVE:admin�����ر�ĳ��server, TIMEOUT:���粨����ɵ�������ʱ��ACCIDENT:�ֶ�kill��server���̣�
	};

	struct ExtraConnInfo : public ConnInfo
	{
	public:
		ExtraConnInfo ()
		{
			ConnInfo();
			ip_ = "";
			port_ = 0;
		}
		void clean()
		{
			ip_ = "";
			port_ = 0;
			ConnInfo::clean();
		}
	public:
		std::string ip_;
		uint16_t port_;
	};

#define  DEFAULT_MSG_BASEHEAD_LENTH    (4)
	struct MsgBaseHead
	{
	public:
		MsgBaseHead() {len = 0; type = 0;}
		MsgBaseHead(uint16_t wLen, uint16_t wType)
		{
			len = muduo::net::sockets::hostToNetwork16(wLen);
			type = muduo::net::sockets::hostToNetwork16(wType);
		}

		void setLen(uint16_t wLen) {len = muduo::net::sockets::hostToNetwork16(wLen);}
		void setType(uint16_t wType) {type = muduo::net::sockets::hostToNetwork16(wType);}
		uint16_t getLen() {return muduo::net::sockets::networkToHost16(len);}
		uint16_t getDataLen() {return muduo::net::sockets::networkToHost16(len) - DEFAULT_MSG_BASEHEAD_LENTH;}
		uint16_t getType() {return muduo::net::sockets::networkToHost16(type);}
	private:
		//�ڲ��������򣬶���Ľӿڣ����ص�ֵ�����õ�ֵ����������
		uint16_t len;  //data size
		uint16_t type; //msg type
	};

	struct ServerContext
	{
		ServerContext()
		{
			connInfo_.clean();
			heartTime_ = 0;
			isExchangeInfo_ = false;
		}
		ConnInfo connInfo_;
		time_t heartTime_;
		bool isExchangeInfo_; //�Ƿ���ɽ�����Ϣ���Ƿ��лظ� start_connect_ack, ���յ�start_connect_req��
	};

	enum ServerType
	{
		SERV_NONE = 0,      /*the connection type is unknown */
		SERV_MSG,           /*the connection from MSG SERVER*/
		SERV_CS,            /*the connection from CS SERVER*/
		SERV_DC,            /*the connection from DC SERVER*/
		SERV_THIRPART,      /*the connection from ThirPartServer*/
		SERV_LOGIC = 6,     /*the connection from Logic server*/
		SERV_PHP = 7,       /*the connection from PHP*/
		SERV_MONITOR = 8,   /*from small programe*/
		SERV_PRELOADER = 9,  /*preloader*/
		SERV_SENDER = 10,    /*sender*/
		SERV_SDK = 11,       /*sdk server*/
		SERV_OFFLINE = 12,   /*offline server*/
		SERV_ACCESS = 13,    /*access server*/
		SERV_STATS = 14,     /*statistic server*/
		SERV_ADMIN = 15,     /* admin */
		SERV_DBAGENT = 16,   /* dbAgent */
		SERV_DTCLEANER = 17,   /*dtcleaner */
		SERV_TIMER = 501,    /*test timer*/
		SERV_MDEVICE = 1000,
		SERV_PDEVICE = 2000
	};

	void InitServTypeStrTable();
	void InitMsgStrTable();

	const char *getServTypeStr(int type);   //��ȡservType��string����
	const char *getProtoTypeStr(int type);  //��ȡ��Ϣ�ŵ�string����

	void printConnInfo(const muduo::net::TcpConnectionPtr & conn, bool isForcePrint);

	//thread safe
	void forceCloseConn(const muduo::net::TcpConnectionPtr &conn, const std::string& strMsg);

	//thread safe
	bool isDcServer(const muduo::net::TcpConnectionPtr& conn);

	//thread safe use malloc space.
	void sendJsonMessage(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, bool isSendJson, const std::string& jsonStr);

	//send msg use stack space.
	struct SendMsgUseStack
	{
		SendMsgUseStack()
		{
			memset(buf_, 0, kMaxMsgDataLen);
		}

		void sendJsonMessage(const muduo::net::TcpConnectionPtr &conn, uint16_t protoType, bool isSendJson, const std::string& jsonStr);

	private:
		const static int kMaxMsgDataLen = 32 * 1024;
		char buf_[kMaxMsgDataLen];
	};
}




#endif


