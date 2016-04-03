#ifndef _JSONUTIL_H_
#define _JSONUTIL_H_

#include <stdint.h>
#include <jsoncpp/json/json.h>

class JsonValue
{
public:
	JsonValue(const Json::Value &jv);
	uint64_t asUInt64() const;
	uint32_t asUInt32() const;
	uint16_t asUInt16() const;
	int64_t asInt64() const;
	int32_t asInt32() const;
	int16_t asInt16() const;
	int asInt() const;
	const char *asCString() const;
	std::string asString() const;
    Json::Value asValue() const;
    bool isMember(const char *key) const;
	int size() const; // size of array, return 0 if not an array
	JsonValue operator[]( const char *key) const;
	JsonValue operator[]( int index) const; // for array
private:
	const Json::Value &jv_;
};
#endif
