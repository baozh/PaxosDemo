#ifndef __DATASTRUCT_H
#define __DATASTRUCT_H

#include <stdint.h>
#include "ini/inifile.h"
#include "muduo/base/Logging.h"
#include <muduo/base/Mutex.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <map>
#include <utility>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define  INVALID_ACCEPT_VALUE    (-1)


struct ProposalID
{
	ProposalID()
	{
		time_ = 0;
		servId_ = 0;
	}

    ProposalID(const ProposalID& posId)
    {
        if (this == &posId)
            return;

        time_ = posId.time_;
        servId_ = posId.servId_;
    }

    ProposalID& operator =(const ProposalID& posId)
    {
        if (this == &posId)
            return *this;

        this->time_ = posId.time_;
        this->servId_ = posId.servId_;

        return *this;
    }

	bool operator <(const ProposalID& posId) const
	{
		if (this->time_ != posId.time_)
		{
			return this->time_ < posId.time_;
		}
		else
		{
			return this->servId_ < posId.servId_;
		}
	}

	static int compareProposalID(const ProposalID& left, const ProposalID& right)
	{
		if (left.time_ > right.time_)
		{
			return 1;
		}
		else if (left.time_ < right.time_)
		{
			return -1;
		}
		else  //left.time_ == right.time_
		{
			if (left.servId_ > right.servId_)
			{
				return 1;
			}
			else if (left.servId_ < right.servId_)
			{
				return -1;
			}
			else
			{
				return 0;
			}
		}
	}

	int64_t time_;
	int servId_;
};


struct ConfigInfo
{
public:
	struct ServInfo
	{
		ServInfo()
		{
			ip_ = "";
			port_ = 0;
		}
		std::string ip_;
		uint16_t port_;
	};

	ConfigInfo()
	{
		myServId_ = -1;

		servNum_ = -1;
		servInfo_.clear();

		acceptInfo_.clear();
	}

	void readSystemConf()
	{
		const std::string NODE_NAME = "SYSTEM";
		int ret = 0;
		
		myServId_ = iniFile_.getIntValue(NODE_NAME ,"MY_SERV_ID", ret);
		servNum_ = iniFile_.getIntValue(NODE_NAME ,"SERVER_NUM", ret);

		if (servNum_ > 0)
		{
			char keyIpStr[48] = {0};
			char keyPortStr[48] = {0};
			
			for (int i = 1; i <= servNum_; i++)
			{
				ServInfo info;
				snprintf(keyIpStr, sizeof(keyIpStr), "SERVER_%d_IP", i);
				snprintf(keyPortStr, sizeof(keyPortStr), "SERVER_%d_PORT", i);

				iniFile_.getValue(NODE_NAME, keyIpStr, info.ip_);
				info.port_ = iniFile_.getIntValue(NODE_NAME, keyPortStr, ret);

				servInfo_[i] = info;

				printf("serinfo: id:%d, ip:%s, port:%d\n", i, info.ip_.c_str(), info.port_);
			}
		}

		maxPropersalId_.time_ = iniFile_.getInt64Value(NODE_NAME ,"MAX_PROPERSAL_ID_TIME", ret);
		maxPropersalId_.servId_ = iniFile_.getIntValue(NODE_NAME ,"MAX_RROPERSAL_ID_SERVER_ID", ret);

		int acceptNum = iniFile_.getIntValue(NODE_NAME ,"ACCEPT_NUM", ret);
		if (acceptNum > 0)
		{
			char keyPosIdTimeStr[48] = {0};
			char keyPosIdServIdStr[48] = {0};
			char keyPosValueStr[48] = {0};

			for (int i = 1; i <= acceptNum; i++)
			{
				ProposalID posId;
				snprintf(keyPosIdTimeStr, sizeof(keyPosIdTimeStr), "ACCEPT_%d_PROPERSAL_ID_TIME", i);
				snprintf(keyPosIdServIdStr, sizeof(keyPosIdServIdStr), "ACCEPT_%d_PROPERSAL_ID_SERVER_ID", i);
				snprintf(keyPosValueStr, sizeof(keyPosValueStr), "ACCEPT_%d_PROPERSAL_VALUE", i);

				posId.time_ = iniFile_.getInt64Value(NODE_NAME ,keyPosIdTimeStr, ret);
				posId.servId_ = iniFile_.getIntValue(NODE_NAME ,keyPosIdServIdStr, ret);
				int acceptValue = iniFile_.getIntValue(NODE_NAME ,keyPosValueStr, ret);

                acceptInfo_.insert(std::make_pair(posId, acceptValue));
				//acceptInfo_[posId] = acceptValue;

				printf("[AcceptInfo]acceptPropersalId_time:%" PRIu64 ", acceptPropersalId_servId:%d, acceptPropersalValue:%d\n", 
					posId.time_, posId.servId_, acceptValue);
			}
		}
		printf("[readSystemConf] myServId:%d, servNum:%d, maxPropersalId_time:%" PRIu64 ", maxPropersalId_servId:%d\n",
			myServId_, servNum_, maxPropersalId_.time_, maxPropersalId_.servId_);
	}

	void setMaxPropersalID(const ProposalID& id)
	{
		maxPropersalId_.time_ = id.time_;
		maxPropersalId_.servId_ = id.servId_;

		const std::string NODE_NAME = "SYSTEM";
		iniFile_.setInt64Value(NODE_NAME, "MAX_PROPERSAL_ID_TIME", maxPropersalId_.time_);
		iniFile_.setIntValue(NODE_NAME, "MAX_RROPERSAL_ID_SERVER_ID", maxPropersalId_.servId_);

		iniFile_.save();
	}

	bool compareAndSetMaxPosID(const ProposalID& posId)
	{
		bool ret;
		lockMaxPosId_.lock();
		if (ProposalID::compareProposalID(posId, maxPropersalId_) > 0)
		{
			ret = true;
            printf("[compareAndSetMaxPosID] bigger than MaxProposeId, MaxProposeId_time:%" PRIu64 ","\
                    "MaxProposeId_servId:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
                    maxPropersalId_.time_, maxPropersalId_.servId_, posId.time_, posId.servId_);
			setMaxPropersalID(posId);
		}
		else
		{
            printf("[compareAndSetMaxPosID] smaller than MaxProposeId, MaxProposeId_time:%" PRIu64 ","\
                    "MaxProposeId_servId:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
                   maxPropersalId_.time_, maxPropersalId_.servId_, posId.time_, posId.servId_);
			ret = false;
		}
		lockMaxPosId_.unlock();
		return ret;
	}

	int getMaxAcceptPosIdValue(ProposalID& posId)
	{
		int ret;
		lockAcceptInfo_.lock();
		if (acceptInfo_.empty() == true)
		{
			ret = INVALID_ACCEPT_VALUE;
		}
		else
		{
			std::map<ProposalID, int>::reverse_iterator iter = acceptInfo_.rbegin();
            posId = iter->first;
			ret = iter->second;
		}
		lockAcceptInfo_.unlock();
		return ret;
	}

	void setAcceptValue(const ProposalID& posId, int value)
	{
        acceptInfo_.insert(std::make_pair(posId, value));

		//acceptInfo_[posId] = value;

		const std::string NODE_NAME = "SYSTEM";
		int acceptNum = acceptInfo_.size();
		iniFile_.setIntValue(NODE_NAME, "ACCEPT_NUM", acceptNum);

		if (acceptNum > 0)
		{
			char keyPosIdTimeStr[48] = {0};
			char keyPosIdServIdStr[48] = {0};
			char keyPosValueStr[48] = {0};

			std::map<ProposalID, int>::iterator iter = acceptInfo_.begin();
			int i = 0;
			for (; iter != acceptInfo_.end(); iter++)
			{
				i++;
				snprintf(keyPosIdTimeStr, sizeof(keyPosIdTimeStr), "ACCEPT_%d_PROPERSAL_ID_TIME", i);
				snprintf(keyPosIdServIdStr, sizeof(keyPosIdServIdStr), "ACCEPT_%d_PROPERSAL_ID_SERVER_ID", i);
				snprintf(keyPosValueStr, sizeof(keyPosValueStr), "ACCEPT_%d_PROPERSAL_VALUE", i);

				iniFile_.setInt64Value(NODE_NAME, keyPosIdTimeStr, iter->first.time_);
				iniFile_.setIntValue(NODE_NAME, keyPosIdServIdStr, iter->first.servId_);
				iniFile_.setIntValue(NODE_NAME, keyPosValueStr, iter->second);
			}
		}
		iniFile_.save();
	}

	bool compareAndSetAcceptValue(const ProposalID& posId, int value)
	{
		bool ret;
		lockMaxPosId_.lock();
		if (ProposalID::compareProposalID(posId, maxPropersalId_) >= 0)
		{
            printf("[compareAndSetAcceptValue] bigger/equal than MaxProposeId, MaxProposeId_time:%" PRIu64 ", MaxProposeId_servId:%d,"\
                    "posId_time:%" PRIu64 ", posId_servId:%d, commit value:%d\n",
                   maxPropersalId_.time_, maxPropersalId_.servId_, posId.time_, posId.servId_, value);

			ret = true;
			lockAcceptInfo_.lock();
			setAcceptValue(posId, value);
			lockAcceptInfo_.unlock();
		}
		else
		{
            printf("[compareAndSetAcceptValue] smaller than MaxProposeId, MaxProposeId_time:%" PRIu64 ", MaxProposeId_servId:%d,"\
                    "posId_time:%" PRIu64 ", posId_servId:%d, commit value:%d\n",
                   maxPropersalId_.time_, maxPropersalId_.servId_, posId.time_, posId.servId_, value);
			ret = false;
		}
		lockMaxPosId_.unlock();
		return ret;
	}

	void readConfig(std::string filepath = "./config.ini")
	{
		iniFile_.open(filepath); 
		readSystemConf();
	};

    void resetCompeteInfo()
    {
        lockMaxPosId_.lock();
        maxPropersalId_.time_ = 0;
        maxPropersalId_.servId_ = 0;
        lockMaxPosId_.unlock();

        lockAcceptInfo_.lock();
        acceptInfo_.clear();
        lockAcceptInfo_.unlock();
    }


public:
	int myServId_;
	
	int servNum_;
	std::map<int,ServInfo> servInfo_;  //map<serv_id, servInfo>

	ProposalID maxPropersalId_;
	muduo::MutexLock lockMaxPosId_;

	std::map<ProposalID, int> acceptInfo_;
	muduo::MutexLock lockAcceptInfo_;

private:
	inifile::IniFile iniFile_;
};




#endif
