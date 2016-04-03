#ifndef _COMM_COMMPROTO_H_
#define _COMM_COMMPROTO_H_

#include <stdint.h>
#include <jsoncpp/json/json.h>
#include <vector>
#include <string>

#include "CommUtil.h"


namespace CommProtoUtil
{
	bool transStartConnectReq(int servType, int servPort, int appId, int setId, int serialNo, std::string& jsonString);
	bool transStartConnectAck(int servType, int appId, int setId, int serialNo, std::string& jsonString);
	bool transNotifyCloseConnAck(int servType, int appId, int setId, int serialNo, std::string& jsonString);
	bool transLaunchConnNty(int sevType, int appId, int setId, int serialNo, std::string& jsonString);
	bool transSyncConnStat(std::vector<CommServUtil::ConnInfo>& verConnInfo, std::string& jsonString);

	bool parseStartConnectReq(const char *jsonData, int size, int& servType, int& appId, int& setId, int& serialNo, int& servPort);
	bool parseNotifyCloseConnReq(const char *jsonData, int size, int& servType, int& appId, int& setId, int& serialNo);
	bool parseStartConnectAck(const char *jsonData, int size, int& servType, int& appId, int& setId, int& serialNo);
	bool parseLauchConnCmd(const char* jsonData, int size, std::vector<CommServUtil::ExtraConnInfo>& vecConnInfo);
};

namespace JSONTAG
{
#define STAT_STR static const char* 
	STAT_STR UUID					 = "UUID";
	STAT_STR STATUS					 = "STATUS";
	STAT_STR DEVTYPE				 = "DEVTYPE";
	STAT_STR APPID					 = "APPID";
	STAT_STR SETID					 = "SETID";
	STAT_STR SERIAL_NO				 = "SERIAL_NO";
	STAT_STR MSGSEQ				 = "MSGSEQ";
	STAT_STR RESULT				 = "RESULT";
	STAT_STR DEVTOKEN				 = "DEVTOKEN";
	STAT_STR MSG				 = "MSG";
	STAT_STR UUIDVERSION				 = "UUIDVERSION";
	STAT_STR MSGTYPE				 = "MSGTYPE";
	STAT_STR CLIENTID				 = "CLIENTID";
	STAT_STR SERVTYPE				 = "SERVTYPE";
	STAT_STR SERVPORT				 = "SERVPORT";
	STAT_STR OPTION				    = "OPTION";
	STAT_STR LOGLEVEL			    = "LOGLEVEL";
	STAT_STR RET_LOG_LEVEL			    = "RET_LOG_LEVEL";
	STAT_STR RET_LOG_STAT			    = "RET_LOG_STAT";
	STAT_STR UPDATE_TIME			    = "UPDATE_TIME";
	STAT_STR IPLIST			         = "IPLIST";
	STAT_STR IP		                 = "IP";
	STAT_STR PORT			         = "PORT";
};


#endif
