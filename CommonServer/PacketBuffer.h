#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include <boost/shared_array.hpp>
//
class PacketBuffer
{
	friend class CommonServer;
public:
	const char* data() const;
	int size() const;
protected:
	PacketBuffer(boost::shared_array<char> &sharedData, int size);
	PacketBuffer(const char *stackData, int size);
private:
	//
	struct DataType
	{
		bool isStack_;
		boost::shared_array<char> sharedData_;
		const char *stackData_;
	};
	//
	DataType data_;
	int size_;
};

#endif