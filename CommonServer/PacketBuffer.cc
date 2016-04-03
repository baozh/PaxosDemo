#include "PacketBuffer.h"

const char* PacketBuffer::data() const
{
	if(data_.isStack_)
	{
		return data_.stackData_;
	}
	else
	{
		return data_.sharedData_.get();
	}
}

int PacketBuffer::size() const
{
	return size_;
}

PacketBuffer::PacketBuffer(boost::shared_array<char> &sharedData, int size)
{
	data_.isStack_ = false;
	data_.sharedData_ = sharedData;
	data_.stackData_ = NULL;
	size_ = size;
}

PacketBuffer::PacketBuffer(const char *stackData, int size)
{
	data_.isStack_ = true;
	data_.stackData_ = stackData;
	size_ = size;   
}
