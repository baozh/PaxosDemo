#include <stdexcept>
#include <stdlib.h>
#include "JsonValue.h"

JsonValue::JsonValue(const Json::Value &jv)
	: jv_ (jv)
{}

int JsonValue::size() const
{
	return jv_.size();
}

uint64_t JsonValue::asUInt64() const
{
	try	
	{
		return static_cast<uint64_t>(jv_.asUInt64());
	}
	catch(std::runtime_error &)
	{
		try
		{
			return static_cast<uint64_t>(strtoull(jv_.asCString(), NULL, 10));
		}
		catch(std::runtime_error &)
		{
			return 0;
		}
	}	
}

uint32_t JsonValue::asUInt32() const
{
	try
	{
		return static_cast<uint32_t>(jv_.asUInt());
	}
	catch(std::runtime_error &)
	{
		try
		{
			return static_cast<uint32_t>(strtoul(jv_.asCString(), NULL, 10));
		}
		catch(std::runtime_error &)
		{
			return 0;
		}
	}	
}

uint16_t JsonValue::asUInt16() const
{
	return static_cast<uint16_t>(asUInt32());
}

int64_t JsonValue::asInt64() const
{
	try	
	{
		return static_cast<int64_t>(jv_.asInt64());
	}
	catch(std::runtime_error &)
	{
		try
		{
			return static_cast<int64_t>(strtoll(jv_.asCString(), NULL, 10));
		}
		catch(std::runtime_error &)
		{
			return 0;
		}
	}	
}

int32_t JsonValue::asInt32() const
{
	try
	{
		return static_cast<int32_t>(jv_.asInt());
	}
	catch(std::runtime_error &)
	{
		try
		{
			return static_cast<int32_t>(strtol(jv_.asCString(), NULL, 10));
		}
		catch(std::runtime_error &)
		{
			return 0;
		}
	}	
}

int16_t JsonValue::asInt16() const
{
	return static_cast<int16_t>(asInt32());
}

int JsonValue::asInt() const
{
	return static_cast<int>(asInt32());
}



const char * JsonValue::asCString() const
{
	return jv_.asCString();
}

std::string JsonValue::asString() const
{
	return jv_.asString();
}

JsonValue JsonValue::operator[](const char *key) const
{
	if(!jv_.isMember(key))
	{
		throw std::string(key);
	}
	return jv_[key];
}

JsonValue JsonValue::operator[](int index) const
{
	return jv_[(Json::ArrayIndex)index];
}

Json::Value JsonValue::asValue() const
{
    return jv_;
}

bool JsonValue::isMember(const char *key) const
{
    return jv_.isMember(key);
}
