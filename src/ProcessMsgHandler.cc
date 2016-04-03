#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <muduo/base/Logging.h>
#include "muduo/base/TimeZone.h"
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Timestamp.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "ProcessMsgHandler.h"
#include "CommonServer/MsgIdDefine.h"
#include "PaxosCommProto.h"


extern ConfigInfo gConfigInfo;


MessageHandler::MessageHandler()
{

}

void MessageHandler::regAllMsgHandler(CommonServer* commServ)
{
	if (commServ == NULL)
		return;

	pCommServer_ = commServ;
	commServ->setConnCallback(boost::bind(&MessageHandler::OnConnection, this, _1, _2, _3));

	commServ->regMessageHandler(PROPOSER_ACCEPTOR_PREPARE_REQ, boost::bind(&MessageHandler::ProposerAcceptorPrepareReq, this, _1, _2, _3));
	commServ->regMessageHandler(ACCEPTOR_PROPOSER_PREPARE_ACK, boost::bind(&MessageHandler::AcceptorProposerPrepareAck, this, _1, _2, _3));
	commServ->regMessageHandler(PROPOSER_ACCEPTOR_ACCEPT_REQ, boost::bind(&MessageHandler::ProposerAcceptorAcceptReq, this, _1, _2, _3));
	commServ->regMessageHandler(ACCEPTOR_PROPOSER_ACCEPT_ACK, boost::bind(&MessageHandler::AcceptorProposerAcceptAck, this, _1, _2, _3));
}

void MessageHandler::OnConnection(CommonServer* commServer, const muduo::net::TcpConnectionPtr& conn, bool isConnected)
{
	CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
	assert(context != NULL);

	printf("%s, peerAddr:%s, peerServId:%d, localAddr:%s\n", (isConnected ? "[TCP_UP]" : "[TCP_DOWN]"),
				conn->peerAddress().toIpPort().c_str(), context->connInfo_.serialNo_, conn->localAddress().toIpPort().c_str());

	if (isConnected == true)
	{
        if (paxosInstance_.getAcceptValue() == INVALID_ACCEPT_VALUE &&
            paxosInstance_.isShouldSendPrepareReq() == true)
        {
            //����prepare req
            ProposalID posId = paxosInstance_.getRecentPosID();
            sendPrepareReq(conn, posId);
            //���� ��һ��check �Ķ�ʱ��
            paxosInstance_.setNextSendTimer(pCommServer_, this);
        }
	}
    else
    {
        pCommServer_->getMainLoop()->runInLoop(boost::bind(&MessageHandler::competeNextMasterShip, this, context->connInfo_.serialNo_));
    }
}

void getAllConn(CommonServer* commServ, std::vector<TcpConnectionPtr> &connVec)
{
    assert(commServ!=NULL);
    commServ->getAllClientConn(connVec);
}

//���߳���
void MessageHandler::competeNextMasterShip(int servIdInLostConn)
{
    std::vector<TcpConnectionPtr> connVec;
    getAllConn(pCommServer_, connVec);
    //�۳� ���ڶϿ����Ǹ�����
    std::vector<TcpConnectionPtr>::iterator iter = connVec.begin();
    for(; iter != connVec.end(); )
    {
        CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>((*iter)->getMutableContext());
        assert(context != NULL);
        if (context->connInfo_.serialNo_ == servIdInLostConn)
        {
            iter = connVec.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    if(static_cast<double>(connVec.size())+1 > (static_cast<double>(gConfigInfo.servNum_)/2))
    {
        //����� master server�Ͽ������ˣ����ٴξ���mastership
        if(paxosInstance_.getAcceptValue() == servIdInLostConn)
        {
            printf("lost master server connection!, masterServId:%d, current servNum:%f, half num:%f\n",
                   paxosInstance_.getAcceptValue(), static_cast<double>(connVec.size())+1, static_cast<double>(gConfigInfo.servNum_)/2);

            //������һ�ε� mastership
            gConfigInfo.resetCompeteInfo();
            beginNextPaxosInstance();
        }
    }
    else   //���е�server����  �Ѿ� ���ڹ����������
    {
        printf("Small than half server number, lost master server!, current servNum:%f, half num:%f\n",
               static_cast<double>(connVec.size())+1, static_cast<double>(gConfigInfo.servNum_)/2);

        //�������������ٷ��� prepare req
        paxosInstance_.resetPaxosVairable();
        gConfigInfo.resetCompeteInfo();
        paxosInstance_.setShouldSendPrepareReq(true);
    }
}

void MessageHandler::sendPrepareReq(const muduo::net::TcpConnectionPtr& conn, const ProposalID& posId)
{
	std::string strJson = "";
	if(CommProtoUtil::transPrepareReq(posId, strJson) == false)
	{
		LOG_ERROR << "transPrepareReq failed.";
		return;
	}
	CommServUtil::sendJsonMessage(conn, PROPOSER_ACCEPTOR_PREPARE_REQ, true, strJson);
}

//���߳���
void MessageHandler::sendAllPrepareReq()
{
	std::vector<TcpConnectionPtr> connVec;
    getAllConn(pCommServer_, connVec);

	ProposalID posId = paxosInstance_.getRecentPosID();
	for(std::vector<TcpConnectionPtr>::iterator iter = connVec.begin();
		iter != connVec.end(); iter++)
	{
        sendPrepareReq((*iter), posId);

        CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>((*iter)->getMutableContext());
        assert(context != NULL);
        printf("[MessageHandler::sendAllPrepareReq] send prepare_req(to servId:%d). posId_time:%" PRIu64 ", posId_servId:%d\n",
               context->connInfo_.serialNo_, posId.time_, posId.servId_);
	}

    paxosInstance_.setShouldSendPrepareReq(true);
	//���� ��һ��check �Ķ�ʱ��
	paxosInstance_.setNextSendTimer(pCommServer_, this);
}

void MessageHandler::prepareReqTimerFun()
{
	//���յ�parepare ack�ĸ��������� һ���server����
	if ((static_cast<double>(paxosInstance_.getRecvPrepareAckNum()) + 1) > (static_cast<double>(gConfigInfo.servNum_)/2))
	{
        paxosInstance_.setShouldSendPrepareReq(false);

		//����accept req
        ProposalID posId = paxosInstance_.getRecentPosID();
        int proposeValue = paxosInstance_.getMaxRecvPrepareAckValue();
        if (proposeValue == INVALID_ACCEPT_VALUE)
        {
            proposeValue = gConfigInfo.myServId_;
        }

        printf("-------------------------[MessageHandler::prepareReqTimerFun] recv more half prepare ack(ackNum:%d), so send accept req. posId_time:%" PRIu64 ", posId_servId:%d, proposeValue:%d\n",
               paxosInstance_.getRecvPrepareAckNum() + 1, posId.time_, posId.servId_, proposeValue);

        std::string strJson = "";
        if(CommProtoUtil::transAcceptReq(posId, proposeValue, strJson) == false)
        {
            LOG_ERROR << "transAcceptReq failed.";
            return;
        }

        std::vector<TcpConnectionPtr> connVec;
        getAllConn(pCommServer_, connVec);
        for(std::vector<TcpConnectionPtr>::iterator iter = connVec.begin();
            iter != connVec.end(); iter++)
        {
            if(paxosInstance_.isRecvPrepareAck((*iter)->peerAddress().toIpPort()) == true)
            {
                CommServUtil::sendJsonMessage((*iter), PROPOSER_ACCEPTOR_ACCEPT_REQ, true, strJson);
            }
        }

        //����check ��ʱ��
        double timeInterval = rand() % 2000 + 1000;   //��ʱʱ�� 1~3��
        pCommServer_->getMainLoop()->runAfter(timeInterval/1000, boost::bind(&MessageHandler::checkAcceptAckTimerFun, this));
	}
	else
	{
        printf("-------------------------------[MessageHandler::prepareReqTimerFun] recv smaller half prepare ack(ackNum:%d), so begin next paxos instance.\n",
               paxosInstance_.getRecvPrepareAckNum() + 1);

        beginNextPaxosInstance();
	}
}

void MessageHandler::checkAcceptAckTimerFun()
{
    //���յ�accept ack�ĸ��������� һ���server����
    if ((static_cast<double>(paxosInstance_.getAcceptAckNum()) + 1) > (static_cast<double>(gConfigInfo.servNum_)/2))
    {
        //�ύ�ɹ�
        printf("----------------------------[MessageHandler::checkAcceptAckTimerFun] commit succeed! acceptNum:%d, acceptValue:%d\n",
               paxosInstance_.getAcceptAckNum()+1, paxosInstance_.getAcceptValue());

        paxosInstance_.setShouldSendPrepareReq(false);
        //paxosInstance_.resetPaxosVairable();
    }
    else
    {
        //�ύʧ��
        printf("------------------------------[MessageHandler::checkAcceptAckTimerFun] commit failed! acceptNum:%d, acceptValue:%d\n",
               paxosInstance_.getAcceptAckNum()+1, paxosInstance_.getAcceptValue());

        beginNextPaxosInstance();
    }
}

void MessageHandler::beginNextPaxosInstance()
{
    //��ʼ������һ�ֵ�paxosInstance
    double timeInterval = rand() % 2000 + 1000;   //��ʱʱ�� 1~3��
    pCommServer_->getMainLoop()->runAfter(timeInterval/1000, boost::bind(&MessageHandler::sendAllPrepareReq, this));
    paxosInstance_.setShouldSendPrepareReq(false);
    paxosInstance_.resetPaxosVairable();
}

void MessageHandler::ProposerAcceptorPrepareReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
	ProposalID posId;
	if(CommProtoUtil::parsePrepareReq(data.data(), data.size(), posId) == false)
	{
		LOG_ERROR<<"parsePrepareReq failed.";
		CommServUtil::forceCloseConn(conn, "parse json string failed!");
		return;
	}

    CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
    assert(context != NULL);
    printf("recv prepare_req(from servId:%d). posId_time:%" PRIu64 ", podId_servId:%d\n",
           context->connInfo_.serialNo_, posId.time_, posId.servId_);

	if (gConfigInfo.compareAndSetMaxPosID(posId) == true)
	{
		//�ظ����󣬲����� ��Accept���᰸�� ProposalID ���� value
        ProposalID maxAcceptPosId;
		int maxAcceptPosIdValue = gConfigInfo.getMaxAcceptPosIdValue(maxAcceptPosId);
        std::string strJson = "";

        printf("send prepare_ack, MaxAcceptProposeId_time:%" PRIu64 ", MaxAcceptProposeId_servId:%d, value:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
               maxAcceptPosId.time_, maxAcceptPosId.servId_, maxAcceptPosIdValue, posId.time_, posId.servId_);

        if(CommProtoUtil::transPrepareAck(posId, maxAcceptPosId, maxAcceptPosIdValue, strJson) == false)
        {
            LOG_ERROR << "transPrepareAck failed.";
            return;
        }
        CommServUtil::sendJsonMessage(conn, ACCEPTOR_PROPOSER_PREPARE_ACK, true, strJson);
	}
}

void MessageHandler::AcceptorProposerPrepareAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
    ProposalID posId, maxPosId;
    int acceptValue = 0;
    if(CommProtoUtil::parsePrepareAck(data.data(), data.size(), posId, maxPosId, acceptValue) == false)
    {
        LOG_ERROR<<"parsePrepareAck failed.";
        CommServUtil::forceCloseConn(conn, "parse json string failed!");
        return;
    }

    CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
    assert(context != NULL);
    printf("recv prepare_ack(from servId:%d), MaxAcceptProposeId_time:%" PRIu64 ", MaxAcceptProposeId_servId:%d, value:%d, posId_time:%" PRIu64 ", posId_servId:%d\n",
           context->connInfo_.serialNo_, maxPosId.time_, maxPosId.servId_, acceptValue, posId.time_, posId.servId_);

    //��� ���ַ���ProposalID
    if (paxosInstance_.isOriginalPosId(posId) == true)
    {
        //ͳ�Ƹ�����1, ����value
        int recvNum = paxosInstance_.increPrepareAckNum(maxPosId, acceptValue, conn->peerAddress().toIpPort());
        printf("recv %d prepareAck\n", recvNum);
    }
}

void MessageHandler::ProposerAcceptorAcceptReq(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
    ProposalID posId;
    int preposalValue = 0;
    if(CommProtoUtil::parseAcceptReq(data.data(), data.size(), posId, preposalValue) == false)
    {
        LOG_ERROR << "parsePrepareAck failed.";
        CommServUtil::forceCloseConn(conn, "parse json string failed!");
        return;
    }

    CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
    assert(context != NULL);
    printf("recv accept_req(from servId:%d), ProposeId_time:%" PRIu64 ", ProposeId_servId:%d, value:%d\n",
           context->connInfo_.serialNo_, posId.time_, posId.servId_, preposalValue);

    //�յ���ProposalID >= Max_ProposalID (һ��������� ����)���� �ظ� �ύ�ɹ����� �־û� ProposalID ��value��
    if(gConfigInfo.compareAndSetAcceptValue(posId, preposalValue) == true)
    {
        std::string strJson = "";
        if(CommProtoUtil::transAcceptAck(posId, preposalValue, strJson) == false)
        {
            LOG_ERROR << "transAcceptAck failed.";
            return;
        }
        CommServUtil::sendJsonMessage(conn, ACCEPTOR_PROPOSER_ACCEPT_ACK, true, strJson);
    }
}

void MessageHandler::AcceptorProposerAcceptAck(CommonServer* commServer, const muduo::net::TcpConnectionPtr & conn, const PacketBuffer& data)
{
    ProposalID posId;
    int acceptValue = 0;
    if(CommProtoUtil::parseAcceptAck(data.data(), data.size(), posId, acceptValue) == false)
    {
        LOG_ERROR << "parseAcceptAck failed.";
        CommServUtil::forceCloseConn(conn, "parse json string failed!");
        return;
    }

    CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>(conn->getMutableContext());
    assert(context != NULL);
    printf("recv accept_ack(from servId:%d), ProposeId_time:%" PRIu64 ", ProposeId_servId:%d, value:%d\n",
           context->connInfo_.serialNo_, posId.time_, posId.servId_, acceptValue);

    paxosInstance_.increAcceptAckNum(posId, acceptValue);
}

