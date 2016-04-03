#ifndef __MODULE_SERVER_CLIENT_MANAGER_H
#define __MODULE_SERVER_CLIENT_MANAGER_H

#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include "MsgIdDefine.h"
#include "CommUtil.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

using namespace muduo;
using namespace muduo::net;

//�Զ�client(�������ܵ�����)����Ϣ ���� ���飺1) conn->context  2)ConnInfoManager
//�Զ�server(�������������)����Ϣ ���� ���飺1��conn->context  2)PeerServerInfoManager  3)ConnInfoManager
class ConnInfoManager
{
public:
	struct OneServTypeInfo
	{
		OneServTypeInfo() : lockNextIndex_()
		{
			nextConnIndex_ = 0;
			conns_.clear();
		}

		OneServTypeInfo(const OneServTypeInfo& info)
		{
			if (this == &info)
				return;
			this->conns_.assign(info.conns_.begin(), info.conns_.end());
			this->nextConnIndex_ = info.nextConnIndex_;
		}

		int getNextIndexAndIncrement()
		{
			int nextIndex = 0;

			lockNextIndex_.lock();
			nextIndex = nextConnIndex_;
			nextConnIndex_++;
			if (nextConnIndex_ == conns_.size())
			{
				nextConnIndex_ = 0;
			}
			lockNextIndex_.unlock();

			return nextIndex;
		}

		void setNextIndexZero()
		{
			lockNextIndex_.lock();
			nextConnIndex_ = 0;
			lockNextIndex_.unlock();
		}

		std::vector<muduo::net::TcpConnectionPtr> conns_;
		int nextConnIndex_;   //��һ��Ҫ���͵���������
		boost::mutex lockNextIndex_;
	};

public:
	ConnInfoManager()
	{
		clientInfos_.clear();
	}

	~ConnInfoManager()
	{
		clientInfos_.clear();
	}

	bool isEmpty(int servType)
	{
		bool ret;
		rwLockClientInfo_.lock_shared();
		if (clientInfos_.empty())
		{
			ret = true;
		}
		else
		{
			if (clientInfos_.find(servType) == clientInfos_.end())
			{
				ret = true;
			}
			else
			{
				ret = false;
			}
		}
		rwLockClientInfo_.unlock_shared();
		return ret;
	}

	void deleteConnPtr(int servType, const muduo::net::TcpConnectionPtr& conn)
	{
		rwLockDcConn_.lock();
		if (servType == CommServUtil::SERV_DC && dcConn_ == conn)
		{
			dcConn_.reset();
		}
		rwLockDcConn_.unlock();

		rwLockClientInfo_.lock();
		std::vector<muduo::net::TcpConnectionPtr>& vecConn = clientInfos_[servType].conns_;
		vecConn.erase(std::remove(vecConn.begin(), vecConn.end(),conn), vecConn.end());

		clientInfos_[servType].setNextIndexZero();

		if (clientInfos_[servType].conns_.empty() == true)
		{
			clientInfos_.erase(servType);
		}
		rwLockClientInfo_.unlock();
	}

	void addConnPtr(int servType, const muduo::net::TcpConnectionPtr& conn)
	{
		rwLockClientInfo_.lock();
		clientInfos_[servType].conns_.push_back(conn);
		rwLockClientInfo_.unlock();

		if (servType == CommServUtil::SERV_DC)
		{
			rwLockDcConn_.lock();
			dcConn_ = conn;
			rwLockDcConn_.unlock();
		}
	}

	bool sendJsonMessage(int appId, int setId, int serialNo, int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		bool ret = false;
		rwLockClientInfo_.lock_shared();
		std::map<int, OneServTypeInfo>::iterator iter = clientInfos_.find(servType);
		if (iter != clientInfos_.end() && iter->second.conns_.empty() == false)
		{
			std::vector<muduo::net::TcpConnectionPtr>::iterator iter_vec = iter->second.conns_.begin();
			for(; iter_vec != iter->second.conns_.end(); iter_vec++)
			{
				if(*iter_vec)
				{
					CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>((*iter_vec)->getMutableContext());
					if (context != NULL &&
						context->connInfo_.appId_ == appId &&
						context->connInfo_.setId_ == setId &&
						context->connInfo_.serialNo_ == serialNo &&
						context->connInfo_.servType_ == servType)
					{
						CommServUtil::sendJsonMessage((*iter_vec), protoType, isSendJson, jsonStr);
						ret = true;
						break;
					}
				}
			}
		}
		rwLockClientInfo_.unlock_shared();

		if (ret == false)
		{
			LOG_WARN << "Can't find the connection, so don't send msg. appId:" << appId << ", setId:" << setId << ", serialNo:" 
						<< serialNo << ", servType:" << servType << ", protoType:" << protoType;
		}
		return ret;
	}

	void sendJsonMessage(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		rwLockClientInfo_.lock_shared();
		std::map<int, OneServTypeInfo>::iterator iter = clientInfos_.find(servType);

		if (clientInfos_.empty()==true || iter == clientInfos_.end() || iter->second.conns_.empty())
		{
			LOG_WARN << "client info is empty or don't have this servType conn, so Can't send msg. servType" 
					<< servType << ", protoType" << protoType;
			rwLockClientInfo_.unlock_shared();
			return;
		}

		int connIndex = iter->second.getNextIndexAndIncrement();
		CommServUtil::sendJsonMessage(iter->second.conns_[connIndex], protoType, isSendJson, jsonStr);
		rwLockClientInfo_.unlock_shared();
	}

	void sendJsonMessageToDc(uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		rwLockDcConn_.lock_shared();
		if (dcConn_)
		{
			CommServUtil::sendJsonMessage(dcConn_, protoType, isSendJson, jsonStr);
		}
		else
		{
			LOG_WARN << "Don't have dc connection! protoType:" << protoType;
		}
		rwLockDcConn_.unlock_shared();
	}

	void sendJsonMsgToOneServType(int servType, uint16_t protoType, bool isSendJson, const std::string& jsonStr)
	{
		rwLockClientInfo_.lock_shared();
		std::map<int, OneServTypeInfo>::iterator iter = clientInfos_.find(servType);

		if (clientInfos_.empty()==true || iter == clientInfos_.end() || iter->second.conns_.empty())
		{
			LOG_WARN << "client info is empty or don't have this servType conn, so Can't send msg. servType" 
				<< servType << ", protoType" << protoType;
			rwLockClientInfo_.unlock_shared();
			return;
		}

		std::vector<muduo::net::TcpConnectionPtr>::iterator iter_conn = iter->second.conns_.begin();
		for (; iter_conn != iter->second.conns_.end(); iter_conn++)
		{
			CommServUtil::sendJsonMessage((*iter_conn), protoType, isSendJson, jsonStr);
		}
		rwLockClientInfo_.unlock_shared();
	}

	void getPeerInfoExceptDc(std::vector<CommServUtil::ConnInfo>& vecConnInfo)
	{
		rwLockClientInfo_.lock_shared();
		std::map<int, OneServTypeInfo>::iterator iter = clientInfos_.begin();
		for (; iter != clientInfos_.end(); iter++)
		{
			if (iter->first != CommServUtil::SERV_DC)
			{
				std::vector<muduo::net::TcpConnectionPtr>::const_iterator iter_vec = iter->second.conns_.begin();
				for (; iter_vec != iter->second.conns_.end(); iter_vec++)
				{
					CommServUtil::ServerContext* context = boost::any_cast<CommServUtil::ServerContext>((*iter_vec)->getMutableContext());
					assert(context != NULL);
					CommServUtil::ConnInfo connInfo;
					connInfo.appId_ = context->connInfo_.appId_;
					connInfo.setId_ = context->connInfo_.setId_;
					connInfo.serialNo_ = context->connInfo_.serialNo_;
					connInfo.servType_ = context->connInfo_.servType_;
					vecConnInfo.push_back(connInfo);
				}
			}
		}
		rwLockClientInfo_.unlock_shared();
	}

	void getAllClientConn(std::vector<muduo::net::TcpConnectionPtr> &connVec)
	{
		rwLockClientInfo_.lock_shared();
		std::map<int, OneServTypeInfo>::iterator iter = clientInfos_.begin();
		for(; iter != clientInfos_.end(); iter++)
		{
			if (iter->second.conns_.empty() == false)
			{
				std::copy(iter->second.conns_.begin(),  iter->second.conns_.end(),  std::back_inserter(connVec));
			}
		}
		rwLockClientInfo_.unlock_shared();
	}
public:
	std::map<int, OneServTypeInfo> clientInfos_;   //map<servType, OneServTypeInfo>
	boost::shared_mutex rwLockClientInfo_;

	muduo::net::TcpConnectionPtr dcConn_;
	boost::shared_mutex rwLockDcConn_;
};


class PeerServerInfoManager
{
public:
#define   MAX_ONE_SERVER_CLIENTS     100000
	typedef boost::shared_ptr<muduo::net::TcpClient> TcpClientPtr;

	struct PeerServerInfo  //�Զ�Server����Ϣ
	{
		PeerServerInfo() {clean();};

		void clean()
		{
			appId_ = -1;
			setId_ = -1;
			serialNo_ = -1;
			servType_ = -1;
			isRetry_ = false;
		};

		int servType_;
		int appId_;
		int setId_;
		int serialNo_;
		bool isRetry_;
	};

	struct VecClients  //���TcpClient������(��stl::vector�е㲻ͬ�������Լ�������һ��)
	{
		VecClients()
		{
			lastIndex = -1;
		}

		int pushClient(TcpClientPtr client)
		{
			assert(lastIndex < MAX_ONE_SERVER_CLIENTS);
			lastIndex++;
			clients_[lastIndex] = client;
			return lastIndex;
		}

		void eraseClient(int index)
		{
			clients_[index].reset();  //ֻ����ԭλ�� ��������ƶ������Ԫ��
		}

		//ɾ��TcpClient�������� ��ɾ��Ԫ�ص� ��������
		int eraseClient(const std::string& localIpPort)
		{
			for(int i=0; i <= lastIndex; i++)
			{
				if (clients_[i] && clients_[i]->connection()->localAddress().toIpPort() == localIpPort)
				{
					//ֻ���ڹر�״̬�£�����ɾ��TcpClient����������
					if (clients_[i]->isConnected() == false)
					{
						clients_[i].reset();
						return i;
					}
					else
					{
						LOG_WARN << "Can't delete this TcpClient, because the conn is connected! localIpPort:" << localIpPort
							<< ", servIpport:" << clients_[i]->connection()->peerAddress().toIpPort();
						return -1;
					}
				}
			}
			return -1;
		}


		int getLastElemIndex()
		{
			return lastIndex;
		}

		TcpClientPtr& getClient(int index)
		{
			assert(index <= lastIndex);
			return clients_[index];
		}

		TcpClientPtr clients_[MAX_ONE_SERVER_CLIENTS];
		int lastIndex;
	};

	struct InternalPeerServerInfo
	{
		InternalPeerServerInfo()
		{
			peerServerInfo_.clean();
			clientIndexs_.clear();
		}
		PeerServerInfo peerServerInfo_;
		std::vector<int> clientIndexs_;  //�洢 ��VecClients �е�����
	};

public:

	void getPeerServerConn(std::vector<muduo::net::TcpConnectionPtr> &connVec)
	{
		rwLockClients_.lock_shared();
		int lastIndex = clients_.getLastElemIndex();
		for (int i = 0; i <= lastIndex; i++)
		{
			TcpClientPtr& client = clients_.getClient(i);
			if ((client) && client->isConnected() == true)
			{
				connVec.push_back(client->connection());

			}
		}
		rwLockClients_.unlock_shared();
	}

	void getConnInfo(const muduo::net::InetAddress& addr, CommServUtil::ConnInfo& info)
	{
		std::string ipport = addr.toIpPort();
		std::map<std::string, InternalPeerServerInfo>::iterator iter;

		rwLockServInfos_.lock_shared();
		iter = servInfos_.find(ipport);
		if (iter != servInfos_.end())
		{
			info.appId_ = iter->second.peerServerInfo_.appId_;
			info.setId_ = iter->second.peerServerInfo_.setId_;
			info.serialNo_ = iter->second.peerServerInfo_.serialNo_;
			info.servType_ = iter->second.peerServerInfo_.servType_;
		}
		else
		{
			LOG_WARN << "Can't find this connInfo! ipport:" << addr.toIpPort();
		}
		rwLockServInfos_.unlock_shared();
	}

	void setConnInfo(const muduo::net::InetAddress& addr, int appId, int setId, int serialNo, int servType)
	{
		std::string ipport = addr.toIpPort();
		std::map<std::string, InternalPeerServerInfo>::iterator iter;

		rwLockServInfos_.lock();
		iter = servInfos_.find(ipport);
		if (iter != servInfos_.end())
		{
			iter->second.peerServerInfo_.appId_ = appId;
			iter->second.peerServerInfo_.setId_ = setId;
			iter->second.peerServerInfo_.serialNo_ = serialNo;
			iter->second.peerServerInfo_.servType_ = servType;
		}
		else
		{
			LOG_WARN << "Can't find this connInfo! ipport:" << addr.toIpPort();
		}
		rwLockServInfos_.unlock();
	}

	void setServerInfo(const std::string& ipport, const PeerServerInfo& info, const TcpClientPtr& client)
	{
		rwLockClients_.lock();
		int lastIndex = clients_.pushClient(client);
		rwLockClients_.unlock();

		rwLockServInfos_.lock();
		servInfos_[ipport].peerServerInfo_ = info;
		servInfos_[ipport].clientIndexs_.push_back(lastIndex);   //����client������
		rwLockServInfos_.unlock();
	}

	void disableRetry(int appId, int setId, int serialNo, int servType)
	{
		rwLockServInfos_.lock_shared();
		std::map<std::string, InternalPeerServerInfo>::iterator iter = servInfos_.begin();
		for (; iter != servInfos_.end(); iter++)
		{
			if (iter->second.peerServerInfo_.appId_ == appId &&
				iter->second.peerServerInfo_.setId_ == setId &&
				iter->second.peerServerInfo_.serialNo_ == serialNo &&
				iter->second.peerServerInfo_.servType_ == servType)
			{
				//��clientIndexs_ �ҵ� ����client����������disableRetry
				std::vector<int>::iterator iter_vec = iter->second.clientIndexs_.begin();
				for (; iter_vec != iter->second.clientIndexs_.end(); iter_vec++)
				{
					rwLockClients_.lock_shared();
					TcpClientPtr& client = clients_.getClient(*iter_vec);
					if (client)
					{
						client->disableRetry();
					}
					rwLockClients_.unlock_shared();
				}
				iter->second.peerServerInfo_.isRetry_ = false;
			}
		}
		rwLockServInfos_.unlock_shared();
	}

	void delServerInfo(const muduo::net::InetAddress& servAddr, const muduo::net::InetAddress& localAddr)
	{
		std::string servIpPort = servAddr.toIpPort();

		rwLockServInfos_.lock();
		std::map<std::string, InternalPeerServerInfo>::iterator iter = servInfos_.find(servIpPort);
		if (iter != servInfos_.end())
		{
			//ע����Ҫ dc�������ʾ������ӹرպ󣬲Ż� ����ֹͣ �������server, ��ɾ��������ݽṹ.
			if (iter->second.peerServerInfo_.isRetry_ == false)
			{
				//ɾ��TcpClient
				rwLockClients_.lock();
				int delElemIndex = clients_.eraseClient(localAddr.toIpPort());
				rwLockClients_.unlock();

				//��clientIndexs_ �� ɾ��TcpClient������
				if (delElemIndex != -1)
				{
					std::vector<int>& vec = iter->second.clientIndexs_;
					vec.erase(std::remove(vec.begin(), vec.end(),delElemIndex), vec.end());

					//��� ����vecΪ�գ���ʾ û�д�ServIpport��Ӧ��TcpClient��Ӧ��ɾ��������ݽṹ
					if (vec.empty() == true)
					{
						servInfos_.erase(iter);
					}
				}
			}
		}
		rwLockServInfos_.unlock();
	}

private:
	VecClients clients_;
	boost::shared_mutex rwLockClients_;

	std::map<std::string, InternalPeerServerInfo> servInfos_;    //���� ��������ʱ �Զ˵���Ϣ  map<serv_ipport, servInfo>
	boost::shared_mutex rwLockServInfos_;
};



#endif