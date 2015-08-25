/*
 * BSPMessage.cpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#include "BSPMessage.hpp"
#include "BSPException.hpp"
namespace BSP {

Message::Message(uint64_t senderProcID, uint64_t receiverProcID, uint16_t type,
		uint16_t tagServer, uint16_t tagClient, uint16_t tagOperator,
		uint64_t byteCount, char *data, bool releaseDataWhenFinalize) {
	if (type > MESSAGE_TYPE_MAX || data == NULL)
		throw EInvalidArgument();
	_senderProcID = senderProcID;
	_receiverProcID = receiverProcID;
	_type = type;
	_tagServer = tagServer;
	_tagClient = tagClient;
	_tagOperator = tagOperator;
	_byteCount = byteCount;
	_data = data;
	_releaseDataWhenFinalize = releaseDataWhenFinalize;
}

Message::Message(uint64_t senderProcID, uint64_t receiverProcID, uint64_t tsco,
		uint64_t byteCount, char *data, bool releaseDataWhenFinalize) {
	_senderProcID = senderProcID;
	_receiverProcID = receiverProcID;
	setTSCO(tsco);
	_byteCount = byteCount;
	_data = data;
	_releaseDataWhenFinalize = releaseDataWhenFinalize;

	if (_type > MESSAGE_TYPE_MAX || _data == NULL)
		throw EInvalidArgument();
}

Message::~Message() {
	if (_releaseDataWhenFinalize)
		delete[] _data;
}

} /* namespace BSP */
