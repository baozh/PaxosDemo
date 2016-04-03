# PaxosDemo

利用 Paxos 协议 实现一个demo，实现 分布式选举功能。

## 实现细节

Basic Paxos协议的实现流程可参考[这篇文章](https://baozh.github.io/2016-03/paxos-learning/)。

ProposalID 使用 `当前的时间戳` + `server_id`。比较ProposalID的大小时，优先比较`时间戳`。

持久化：使用ini配置文件模拟。

消息传递：使用json格式封装消息协议。


## 异常处理

1. 某个server崩溃重启后，须再走一遍PaxosInstance，来确定 当前谁是master。

2. master server崩溃/断开连接时，其它server 须再启动Paxos 选出一个master。

3. server一个个关闭，直到少于一半的server数，这时，应 有相关提示，且 当其它server 重新启动、连接上后，须再启动Paxos 选出一个master。

4. 任意非master server的关闭/断开连接，不影响 其它server的 Paxos已提交值（即master server值）。

5. 超时重试：需设置定时器，若超时没有收集过半数的prepare_ack,accept_ack，则 重新开始下一轮的PaxosInstance。

6. 在一个PaxosInstance内，须用相同的一个ProposalID。在收到prepare_ack,accept_ack时，须检查 是否是 当前PaxosInstance的 ProposalID（因为有可能返回的是之前请求的回复）。

7. 持久化：Acceptor两次持久化的时机。当崩溃重启后，要读取这些信息，再发起PaxosInstance。




