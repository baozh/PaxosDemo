#ifndef __PROCESS_MSG_HANDLER_H
#define __PROCESS_MSG_HANDLER_H

#include <cassert>
#include <set>
#include <map>
#include <vector>
#include<stdio.h>
#include<stdlib.h>
#include "CommonServer/CommUtil.h"
#include "CommonServer/CommonServer.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Mutex.h>
#include "CommonServer/PacketBuffer.h"
#include "CommonServer/MsgHandler.h"
#include "muduo/net/TimerId.h"
#include "muduo/net/Timer.h"
#include "muduo/base/Atomic.h"
#include "DataStructDefine.h"


extern ConfigInfo gConfigInfo;


class MessageHandler
{
public:
	class PaxosInstance
	{
	public:
		PaxosInstance()
        {
            isShouldSendPrepareReq_ = true;

            timeId_ = muduo::net::TimerId();
            isSetTimer_ = false;

            isSetPosId_ = false;
            recentSendPosId_ = ProposalID();

            maxPosIdInRecvPrepareAck_ = ProposalID();

            recvValueInRecvPrepareAck_.getAndSet(INVALID_ACCEPT_VALUE);
            recvPrepareAckNum_.getAndSet(0);

            ipportInRecvPrepareAck_.clear();
            recvAcceptAckNum_.getAndSet(0);
            recvAcceptValue_.getAndSet(INVALID_ACCEPT_VALUE);
		}

        bool isShouldSendPrepareReq()
        {
            return isShouldSendPrepareReq_;
        }

        void setShouldSendPrepareReq(bool is)
        {
            isShouldSendPrepareReq_ = is;
        }


		ProposalID getRecentPosID()
		{
			ProposalID posId;
			lockIsSetPosId_.lock();
			if (isSetPosId_ == true)
			{
				lockPosId_.lock();
				posId = recentSendPosId_;
				lockPosId_.unlock();
			}
			else
			{
				lockPosId_.lock();
				recentSendPosId_.time_ = muduo::Timestamp::now().microSecondsSinceEpoch();
				recentSendPosId_.servId_ = gConfigInfo.myServId_;
				posId = recentSendPosId_;
				lockPosId_.unlock();
				isSetPosId_ = true;
			}
			lockIsSetPosId_.unlock();

            printf("[getRecentPosID] recentProposeId_time:%" PRIu64 ", recentProposeId_servId:%d\n",
                   posId.time_, posId.servId_);
			return posId;
		}

        bool isOriginalPosId(const ProposalID& posId)
        {
            bool ret;
            lockIsSetPosId_.lock();
            if (isSetPosId_ == true)
            {
                lockPosId_.lock();
                if (recentSendPosId_.time_ == posId.time_ &&
                    recentSendPosId_.servId_ == posId.servId_)
                {
                    printf("Equal the recent posId, recentProposeId_time:%" PRIu64 ", recentProposeId_servId:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
                           recentSendPosId_.time_, recentSendPosId_.servId_, posId.time_, posId.servId_);
                    ret = true;
                }
                else
                {
                    printf("Isn't the recent posId, recentProposeId_time:%" PRIu64 ", recentProposeId_servId:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
                           recentSendPosId_.time_, recentSendPosId_.servId_, posId.time_, posId.servId_);
                    ret = false;
                }
                lockPosId_.unlock();
            }
            else
            {
                ret = false;
            }
            lockIsSetPosId_.unlock();

            return ret;
        }

		void resetTimer()
		{
			lockIsSetTimer_.lock();
			if (isSetTimer_ == true)
			{
				lockTimerId_.lock();
				timeId_ = muduo::net::TimerId();
				lockTimerId_.unlock();
				isSetTimer_ = false;
			}
			lockIsSetTimer_.unlock();
		}

		void setNextSendTimer(CommonServer* commServ, MessageHandler* handler)
		{
			lockIsSetTimer_.lock();
			if (isSetTimer_ == true)
			{
				lockTimerId_.lock();
				if (timeId_.getTimer()->expiration().microSecondsSinceEpoch() > muduo::Timestamp::now().microSecondsSinceEpoch())
				{
					//如果 定时器 还在当前时间之后，则不处理.
					//说明之前已经设置定时发送了，现在不用设置
				}
				else
				{
					//取消定时器
                    commServ->getMainLoop()->cancel(timeId_);
					double timeInterval = rand() % 2000 + 1000;   //定时时间 1~3秒
					timeId_ = commServ->getMainLoop()->runAfter(timeInterval/1000, boost::bind(&MessageHandler::prepareReqTimerFun, handler));
				}
				lockTimerId_.unlock();
			}
			else
			{
				double timeInterval = rand() % 2000 + 1000;   //定时时间 1~3秒
				timeId_ = commServ->getMainLoop()->runAfter(timeInterval/1000, boost::bind(&MessageHandler::prepareReqTimerFun, handler));
				isSetTimer_ = true;
			}
			lockIsSetTimer_.unlock();
		}

		int getRecvPrepareAckNum()
		{
			return recvPrepareAckNum_.get();
		}

        bool isRecvPrepareAck(const std::string& peerIpPort)
        {
            bool ret;
            lockIpPortInRecvPrepareAck_.lock();
            if (ipportInRecvPrepareAck_.find(peerIpPort) != ipportInRecvPrepareAck_.end())
            {
                ret = true;
            }
            else
            {
                ret = false;
            }
            lockIpPortInRecvPrepareAck_.unlock();

            return ret;
        }

		int increPrepareAckNum(const ProposalID& recvPosId, int recvValue, const std::string& peerIpPort)
		{
			int recvNum = recvPrepareAckNum_.incrementAndGet();

            lockIpPortInRecvPrepareAck_.lock();
            ipportInRecvPrepareAck_.insert(peerIpPort);
            lockIpPortInRecvPrepareAck_.unlock();

            lockMaxPosIdInRecvPrepareAck_.lock();
            if(ProposalID::compareProposalID(recvPosId, maxPosIdInRecvPrepareAck_) >= 0)
            {
                maxPosIdInRecvPrepareAck_ = recvPosId;
                recvValueInRecvPrepareAck_.getAndSet(recvValue);
            }
            lockMaxPosIdInRecvPrepareAck_.unlock();
            return recvNum;
		}

        int getMaxRecvPrepareAckValue()
        {
            return recvValueInRecvPrepareAck_.get();
        }

        int getAcceptAckNum()
        {
            return recvAcceptAckNum_.get();
        }

        int getAcceptValue()
        {
            return recvAcceptValue_.get();
        }

        void increAcceptAckNum(const ProposalID& posId, int recvValue)
        {
            ProposalID oriPosId = getRecentPosID();
            if (oriPosId.time_ == posId.time_ &&
                oriPosId.servId_ == posId.servId_)
            {
                int recvNum = recvAcceptAckNum_.incrementAndGet();
                recvAcceptValue_.getAndSet(recvValue);
                printf("[increAcceptAckNum] equal originalProposeId, oriProposeId_time:%" PRIu64 ", oriProposeId_servId:%d,"\
                        "posId_time:%" PRIu64 ", posId_servId:%d, commit value:%d, recvAcceptAckNum:%d\n",
                       oriPosId.time_, oriPosId.servId_, posId.time_, posId.servId_, recvValue, recvNum);
            }
            else
            {
                printf("[increAcceptAckNum] Don't equal originalProposeId, oriProposeId_time:%" PRIu64 ", oriProposeId_servId:%d,"\
                        "posId_time:%" PRIu64 ", posId_servId:%d, commit value:%d\n",
                       oriPosId.time_, oriPosId.servId_, posId.time_, posId.servId_, recvValue);
            }
        }

        void resetPaxosVairable()
        {
            resetTimer();

            lockIsSetPosId_.lock();
            isSetPosId_ = false;
            lockIsSetPosId_.unlock();

            lockPosId_.lock();
            recentSendPosId_ = ProposalID();
            lockPosId_.unlock();

            lockMaxPosIdInRecvPrepareAck_.lock();
            maxPosIdInRecvPrepareAck_ = ProposalID();
            lockMaxPosIdInRecvPrepareAck_.unlock();

            recvValueInRecvPrepareAck_.getAndSet(INVALID_ACCEPT_VALUE);
            recvPrepareAckNum_.getAndSet(0);

            lockIpPortInRecvPrepareAck_.lock();
            ipportInRecvPrepareAck_.clear();
            lockIpPortInRecvPrepareAck_.unlock();

            recvAcceptAckNum_.getAndSet(0);
            recvAcceptValue_.getAndSet(INVALID_ACCEPT_VALUE);
        }

	private:
        volatile bool isShouldSendPrepareReq_;  /*atomic*/ //在连接建立成功时，是否应该发送prepare_req

		volatile bool isSetTimer_;
		muduo::MutexLock lockIsSetTimer_;
		TimerId timeId_;                //统计 收到prepare_ack个数的 定时器
		muduo::MutexLock lockTimerId_;

		volatile bool isSetPosId_;
		muduo::MutexLock lockIsSetPosId_;
		ProposalID recentSendPosId_;   //这一轮paxosInstance的 proposeID
		muduo::MutexLock lockPosId_;

        ProposalID maxPosIdInRecvPrepareAck_;    //收到prepare_ack中 最大的proposeID(acceptor返回的已提交的proposeID)
        muduo::MutexLock lockMaxPosIdInRecvPrepareAck_;
        muduo::AtomicInt32 recvValueInRecvPrepareAck_;  //收到prepare_ack中 最大的proposeID的 value
		muduo::AtomicInt32 recvPrepareAckNum_;          //收到prepare_ack的个数

        std::set<std::string> ipportInRecvPrepareAck_;  //收到prepare_ack的对端server的ipport
        muduo::MutexLock lockIpPortInRecvPrepareAck_;

        muduo::AtomicInt32 recvAcceptAckNum_;  //收到 accept_ack的个数
        muduo::AtomicInt32 recvAcceptValue_;   //收到 accept_ack的带来的value值(即本server发起的,已成功提交的value)
	};

public:
	MessageHandler();

	void regAllMsgHandler(CommonServer* commServ);

	void OnConnection(CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn, bool isConnected);

	//消息处理函数
	void ProposerAcceptorPrepareReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);

	void AcceptorProposerPrepareAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);

	void ProposerAcceptorAcceptReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);

	void AcceptorProposerAcceptAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data);

private:
	void sendPrepareReq(const muduo::net::TcpConnectionPtr& conn, const ProposalID& posId);
	void sendAllPrepareReq();
	void prepareReqTimerFun();
    void beginNextPaxosInstance();
    void checkAcceptAckTimerFun();
    void competeNextMasterShip(int servIdInLostConn);

private:
	PaxosInstance paxosInstance_;
	CommonServer* pCommServer_;
};


#endif



