/*
 * BSPMessage.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPMESSAGE_HPP_
#define BSPMESSAGE_HPP_
#include <stdint.h>

namespace BSP {

class Message {
private:
	uint64_t _senderProcID;
	uint64_t _receiverProcID;
	uint16_t _type;
	uint16_t _tagServer;
	uint16_t _tagClient;
	uint16_t _tagOperator;
	uint64_t _byteCount;
	char *_data;
	bool _releaseDataWhenFinalize;
public:
	// MESSAGE_TYPES
	const static uint16_t MESSAGE_USER_DEFINED = 0;
	const static uint16_t MESSAGE_USER_ARRAY_NAME = 1;
	const static uint16_t MESSAGE_USER_ARRAY_SHAPE = 2;
	const static uint16_t MESSAGE_GET_POINT_SEQUENCE = 3;
	const static uint16_t MESSAGE_GET_POINT_TENSOR = 4;
	const static uint16_t MESSAGE_GET_REGION_SEQUENCE = 5;
	const static uint16_t MESSAGE_GET_REGION_TENSOR = 6;
	const static uint16_t MESSAGE_GET_LINEAR_MAPPING = 7;
	const static uint16_t MESSAGE_SET_POINT_SEQUENCE = 8;
	const static uint16_t MESSAGE_SET_POINT_TENSOR = 9;
	const static uint16_t MESSAGE_SET_REGION_SEQUENCE = 10;
	const static uint16_t MESSAGE_SET_REGION_TENSOR = 11;
	const static uint16_t MESSAGE_SET_LINEAR_MAPPING = 12;
	const static uint16_t MESSAGE_DATA_REPLY = 13;
	const static uint16_t MESSAGE_DATA_UPDATE = 14;
	const static uint16_t MESSAGE_COUNT_OF_SENT_TO = 15;
	const static uint16_t MESSAGE_TYPE_MAX = 15;

	Message(uint64_t senderProcID, uint64_t receiverProcID, uint16_t type,
			uint16_t tagServer, uint16_t tagClient, uint16_t tagOperator,
			uint64_t byteCount, char *data, bool releaseDataWhenFinalize);
	Message(uint64_t senderProcID, uint64_t receiverProcID, uint64_t tsco,
			uint64_t byteCount, char *data, bool releaseDataWhenFinalize);
	virtual ~Message();
	uint64_t getSenderProcID() {
		return _senderProcID;
	}
	uint64_t getReceiverProcID() {
		return _receiverProcID;
	}
	uint16_t getType() {
		return _type;
	}
	uint16_t getServer() {
		return _tagServer;
	}
	uint16_t getClient() {
		return _tagClient;
	}
	uint16_t getOperator() {
		return _tagOperator;
	}
	inline uint64_t getTSCO() {
		uint64_t result = _type;
		result <<= 16;
		result |= _tagServer;
		result <<= 16;
		result |= _tagClient;
		result <<= 16;
		result |= _tagOperator;
		return result;
	}
	inline void setTSCO(const uint64_t tsco) {
		uint64_t temp = tsco;
		_tagOperator = temp & 0xffff;
		temp >>= 16;
		_tagClient = temp & 0xffff;
		temp >>= 16;
		_tagServer = temp & 0xffff;
		temp >>= 16;
		_type = temp & 0xffff;
	}
	inline uint64_t getByteCount() {
		return _byteCount;
	}
	char *getData() {
		return _data;
	}
};

} /* namespace BSP */
#endif /* BSPMESSAGE_HPP_ */
