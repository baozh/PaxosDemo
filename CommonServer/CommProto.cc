#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include <muduo/base/Logging.h>
#include <JsonValue.h>
#include "CommProto.h"
#include <fstream>
#include <iostream>

using namespace std;


bool CommProtoUtil::transStartConnectAck(int servType, int appId, int setId, int serialNo, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;

		jRoot[JSONTAG::APPID] = appId;
		jRoot[JSONTAG::SETID] = setId;
		jRoot[JSONTAG::SERIAL_NO] = serialNo;
		jRoot[JSONTAG::SERVTYPE] = servType;
		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}

bool CommProtoUtil::transNotifyCloseConnAck(int servType, int appId, int setId, int serialNo, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;

		jRoot[JSONTAG::APPID] = appId;
		jRoot[JSONTAG::SETID] = setId;
		jRoot[JSONTAG::SERIAL_NO] = serialNo;
		jRoot[JSONTAG::SERVTYPE] = servType;
		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}



bool CommProtoUtil::transSyncConnStat(std::vector<CommServUtil::ConnInfo>& verConnInfo, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;

		Json::Value connList;
		Json::Value connInfo;
		for(std::vector<CommServUtil::ConnInfo>::iterator iter = verConnInfo.begin();iter != verConnInfo.end(); ++iter)
		{
			connInfo[JSONTAG::APPID] = iter->appId_;
			connInfo[JSONTAG::SETID] = iter->setId_;
			connInfo[JSONTAG::SERIAL_NO] = iter->serialNo_;
			connInfo[JSONTAG::SERVTYPE] = iter->servType_;
			connList.append(connInfo);
		}
		jRoot[JSONTAG::IPLIST] = connList;

		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}



bool CommProtoUtil::transLaunchConnNty(int sevType, int appId, int setId, int serialNo, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;

		jRoot[JSONTAG::APPID] = appId;
		jRoot[JSONTAG::SETID] = setId;
		jRoot[JSONTAG::SERIAL_NO] = serialNo;
		jRoot[JSONTAG::SERVTYPE] = sevType;
		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}

bool CommProtoUtil::transStartConnectReq(int servType, int servPort, int appId, int setId, int serialNo, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;
	
		jRoot[JSONTAG::APPID] = appId;
		jRoot[JSONTAG::SETID] = setId;
		jRoot[JSONTAG::SERIAL_NO] = serialNo;
		jRoot[JSONTAG::SERVTYPE] = servType;
		jRoot[JSONTAG::SERVPORT] = servPort;
		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}









bool CommProtoUtil::parseLauchConnCmd(const char* jsonData, int size, std::vector<CommServUtil::ExtraConnInfo>& vecConnInfo)
{
	if (jsonData == NULL) return false;

	Json::Value jRoot;
	Json::Reader jReader;
	if(!jReader.parse(jsonData, jsonData+size, jRoot))
	{
		LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
		return false;
	}
	try
	{
		// json will throw a c++ exception when parseing a bad formatted json string
		JsonValue jv(jRoot);
		vecConnInfo.clear();
		CommServUtil::ExtraConnInfo info;
		JsonValue jvIpList(jv[JSONTAG::IPLIST]);
		for(int index = 0; index < jvIpList.size(); ++index)
		{
			info.clean();
			info.servType_ = jvIpList[index][JSONTAG::SERVTYPE].asInt();
			info.port_ = jvIpList[index][JSONTAG::PORT].asInt();
			info.serialNo_ = jvIpList[index][JSONTAG::SERIAL_NO].asInt();
			info.appId_ = jvIpList[index][JSONTAG::APPID].asInt();
			info.setId_ = jvIpList[index][JSONTAG::SETID].asInt();
			info.ip_ = jvIpList[index][JSONTAG::IP].asString();
			vecConnInfo.push_back(info);
		}
	}
	catch(const std::string &key)
	{
		LOG_ERROR << "json need " << key;
		return false;
	}
	catch(...)
	{
		LOG_ERROR << "[parseLauchConnCmd] parse json data failed";
		return false;
	}
	return true;
}

bool CommProtoUtil::parseStartConnectAck(const char *jsonData, int size,  int& servType, int& appId, int& setId, int& serialNo)
{
	if (jsonData == NULL) return false;

	Json::Value jRoot;
	Json::Reader jReader;
	if(!jReader.parse(jsonData, jsonData+size, jRoot))
	{
		LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
		return false;
	}
	try
	{
		JsonValue jv(jRoot);
		// json will throw a c++ exception when parseing a bad formatted json string
		appId = jv[JSONTAG::APPID].asInt();
		setId = jv[JSONTAG::SETID].asInt();
		serialNo = jv[JSONTAG::SERIAL_NO].asInt();
		servType= jv[JSONTAG::SERVTYPE].asInt();
	}
	catch(const std::string &key)
	{
		LOG_ERROR << "json need " << key;
		return false;
	}
	catch(...)
	{
		LOG_ERROR << "[parseStartConnectReq] parse json data failed";
		return false;
	}
	return true;
}

bool CommProtoUtil::parseNotifyCloseConnReq(const char *jsonData, int size, int& servType, int& appId, int& setId, int& serialNo)
{
	if (jsonData == NULL) return false;

	Json::Value jRoot;
	Json::Reader jReader;
	if(!jReader.parse(jsonData, jsonData+size, jRoot))
	{
		LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
		return false;
	}
	try
	{
		JsonValue jv(jRoot);
		// json will throw a c++ exception when parseing a bad formatted json string
		appId = jv[JSONTAG::APPID].asInt();
		setId = jv[JSONTAG::SETID].asInt();
		serialNo = jv[JSONTAG::SERIAL_NO].asInt();
		servType= jv[JSONTAG::SERVTYPE].asInt();
	}
	catch(const std::string &key)
	{
		LOG_ERROR << "json need " << key;
		return false;
	}
	catch(...)
	{
		LOG_ERROR << "[parseStartConnectReq] parse json data failed";
		return false;
	}
	return true;
}

bool CommProtoUtil::parseStartConnectReq(const char *jsonData, int size, int& servType, int& appId, int& setId, int& serialNo, int& servPort)
{
	if (jsonData == NULL) return false;
	
	Json::Value jRoot;
	Json::Reader jReader;
	if(!jReader.parse(jsonData, jsonData+size, jRoot))
	{
		LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
		return false;
	}
	try
	{
		JsonValue jv(jRoot);
		// json will throw a c++ exception when parseing a bad formatted json string
		appId = jv[JSONTAG::APPID].asInt();
		setId = jv[JSONTAG::SETID].asInt();
		serialNo = jv[JSONTAG::SERIAL_NO].asInt();
		servType= jv[JSONTAG::SERVTYPE].asInt();
		servPort = jv[JSONTAG::SERVPORT].asInt();
	}
	catch(const std::string &key)
	{
		LOG_ERROR << "json need " << key;
		return false;
	}
	catch(...)
	{
		LOG_ERROR << "[parseStartConnectReq] parse json data failed";
		return false;
	}
	return true;
}




