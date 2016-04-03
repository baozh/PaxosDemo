#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include <muduo/base/Logging.h>
#include <CommonServer/JsonValue.h>
#include "PaxosCommProto.h"


using namespace std;


namespace JSONTAG
{
#define STAT_STR static const char*

	STAT_STR PROPOSAL_ID_TIME			= "PROPOSAL_ID_TIME";
	STAT_STR PROPOSAL_ID_SERV_ID		= "PROPOSAL_ID_SERV_ID";

    STAT_STR ACCEPT_VALUE		        = "ACCEPT_VALUE";
    STAT_STR MAX_PROPOSAL_ID_TIME		= "MAX_PROPOSAL_ID_TIME";
    STAT_STR MAX_PROPOSAL_ID_SERV_ID	= "MAX_PROPOSAL_ID_SERV_ID";

    STAT_STR PROPOSAL_VALUE         	= "PROPOSAL_VALUE";
}

namespace CommProtoUtil
{

bool transPrepareReq(const ProposalID& posId, std::string& jsonString)
{
	try
	{
		Json::Value jRoot;
		Json::FastWriter jWriter;
		jRoot[JSONTAG::PROPOSAL_ID_TIME] = static_cast<Json::Value::Int64>(posId.time_);
		jRoot[JSONTAG::PROPOSAL_ID_SERV_ID] = posId.servId_;
		jsonString = jWriter.write(jRoot);
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	return true;
}

bool transAcceptReq(const ProposalID& posId, int preposeValue, std::string& jsonString)
{
    try
    {
        Json::Value jRoot;
        Json::FastWriter jWriter;
        jRoot[JSONTAG::PROPOSAL_ID_TIME] = static_cast<Json::Value::Int64>(posId.time_);
        jRoot[JSONTAG::PROPOSAL_ID_SERV_ID] = posId.servId_;
        jRoot[JSONTAG::PROPOSAL_VALUE] = preposeValue;
        jsonString = jWriter.write(jRoot);
    }
    catch(...)
    {
        LOG_ERROR << "make json data failed";
        return false;
    }
    return true;
}

bool transAcceptAck(const ProposalID& posId, int acceptValue, std::string& jsonString)
{
    try
    {
        Json::Value jRoot;
        Json::FastWriter jWriter;
        jRoot[JSONTAG::PROPOSAL_ID_TIME] = static_cast<Json::Value::Int64>(posId.time_);
        jRoot[JSONTAG::PROPOSAL_ID_SERV_ID] = posId.servId_;
        jRoot[JSONTAG::ACCEPT_VALUE] = acceptValue;
        jsonString = jWriter.write(jRoot);
    }
    catch(...)
    {
        LOG_ERROR << "make json data failed";
        return false;
    }
    return true;
}

bool transPrepareAck(const ProposalID& posId, const ProposalID& maxPosId, int acceptValue, std::string& jsonString)
{
    try
    {
        Json::Value jRoot;
        Json::FastWriter jWriter;
        jRoot[JSONTAG::PROPOSAL_ID_TIME] = static_cast<Json::Value::Int64>(posId.time_);
        jRoot[JSONTAG::PROPOSAL_ID_SERV_ID] = posId.servId_;
        jRoot[JSONTAG::MAX_PROPOSAL_ID_TIME] = static_cast<Json::Value::Int64>(maxPosId.time_);
        jRoot[JSONTAG::MAX_PROPOSAL_ID_SERV_ID] = maxPosId.servId_;
        jRoot[JSONTAG::ACCEPT_VALUE] = acceptValue;
        jsonString = jWriter.write(jRoot);
    }
    catch(...)
    {
        LOG_ERROR << "make json data failed";
        return false;
    }
    return true;
}


bool parsePrepareAck(const char* jsonData, int dataSize, ProposalID& posId, ProposalID& maxPosId, int& acceptValue)
{
    if (jsonData == NULL) return false;

    Json::Value jRoot;
    Json::Reader jReader;
    if(!jReader.parse(jsonData, jsonData+dataSize, jRoot))
    {
        LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
        return false;
    }

    try
    {
        JsonValue jv(jRoot);
        // json will throw a c++ exception when parseing a bad formatted json string
        posId.time_ = jv[JSONTAG::PROPOSAL_ID_TIME].asInt64();
        posId.servId_ = jv[JSONTAG::PROPOSAL_ID_SERV_ID].asInt();
        maxPosId.time_ = jv[JSONTAG::MAX_PROPOSAL_ID_TIME].asInt64();
        maxPosId.servId_ = jv[JSONTAG::MAX_PROPOSAL_ID_SERV_ID].asInt();
        acceptValue  = jv[JSONTAG::ACCEPT_VALUE].asInt();
    }
    catch(const std::string &key)
    {
        LOG_ERROR << "json need " << key;
        return false;
    }
    catch(...)
    {
        LOG_ERROR << "[parseResultPhp] parse json data failed";
        return false;
    }
    return true;
}

bool parseAcceptAck(const char* jsonData, int dataSize, ProposalID& posId, int& acceptValue)
{
    if (jsonData == NULL) return false;

    Json::Value jRoot;
    Json::Reader jReader;
    if(!jReader.parse(jsonData, jsonData+dataSize, jRoot))
    {
        LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
        return false;
    }

    try
    {
        JsonValue jv(jRoot);
        // json will throw a c++ exception when parseing a bad formatted json string
        posId.time_ = jv[JSONTAG::PROPOSAL_ID_TIME].asInt64();
        posId.servId_ = jv[JSONTAG::PROPOSAL_ID_SERV_ID].asInt();
        acceptValue = jv[JSONTAG::ACCEPT_VALUE].asInt();
    }
    catch(const std::string &key)
    {
        LOG_ERROR << "json need " << key;
        return false;
    }
    catch(...)
    {
        LOG_ERROR << "[parseResultPhp] parse json data failed";
        return false;
    }
    return true;
}

bool parseAcceptReq(const char* jsonData, int dataSize, ProposalID& posId, int& preposalValue)
{
    if (jsonData == NULL) return false;

    Json::Value jRoot;
    Json::Reader jReader;
    if(!jReader.parse(jsonData, jsonData+dataSize, jRoot))
    {
        LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
        return false;
    }

    try
    {
        JsonValue jv(jRoot);
        // json will throw a c++ exception when parseing a bad formatted json string
        posId.time_ = jv[JSONTAG::PROPOSAL_ID_TIME].asInt64();
        posId.servId_ = jv[JSONTAG::PROPOSAL_ID_SERV_ID].asInt();
        preposalValue = jv[JSONTAG::PROPOSAL_VALUE].asInt();
    }
    catch(const std::string &key)
    {
        LOG_ERROR << "json need " << key;
        return false;
    }
    catch(...)
    {
        LOG_ERROR << "[parseResultPhp] parse json data failed";
        return false;
    }
    return true;
}

bool parsePrepareReq(const char* jsonData, int dataSize, ProposalID& posId)
{
	if (jsonData == NULL) return false;

	Json::Value jRoot;
	Json::Reader jReader;
	if(!jReader.parse(jsonData, jsonData+dataSize, jRoot))
	{
		LOG_ERROR << "jReader.parse: " << jReader.getFormatedErrorMessages();
		return false;
	}

	try
	{
		JsonValue jv(jRoot);
		// json will throw a c++ exception when parseing a bad formatted json string
		posId.time_ = jv[JSONTAG::PROPOSAL_ID_TIME].asInt64();
		posId.servId_ = jv[JSONTAG::PROPOSAL_ID_SERV_ID].asInt();
	}
	catch(const std::string &key)
	{
		LOG_ERROR << "json need " << key;
		return false;
	}
	catch(...)
	{
		LOG_ERROR << "[parseResultPhp] parse json data failed";
		return false;
	}
	return true;
}


}