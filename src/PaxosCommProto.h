#ifndef __PAXOS_DEMO_COMM_PROTO_H_
#define __PAXOS_DEMO_COMM_PROTO_H_

#include <stdint.h>
#include <jsoncpp/json/json.h>
#include <vector>
#include <string>
#include "CommonServer/CommUtil.h"
#include "DataStructDefine.h"



namespace CommProtoUtil
{
	bool parsePrepareReq(const char* jsonData, int dataSize, ProposalID& posId);
    bool parsePrepareAck(const char* jsonData, int dataSize, ProposalID& posId, ProposalID& maxPosId, int& acceptValue);
    bool parseAcceptReq(const char* jsonData, int dataSize, ProposalID& posId, int& preposalValue);
    bool parseAcceptAck(const char* jsonData, int dataSize, ProposalID& posId, int& acceptValue);

	bool transPrepareReq(const ProposalID& posId, std::string& jsonString);
	bool transPrepareAck(const ProposalID& posId, const ProposalID& maxPosId, int acceptValue, std::string& jsonString);
    bool transAcceptReq(const ProposalID& posId, int preposeValue, std::string& jsonString);
    bool transAcceptAck(const ProposalID& posId, int acceptValue, std::string& jsonString);
};

#endif
