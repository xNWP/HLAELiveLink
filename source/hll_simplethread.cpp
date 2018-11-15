#include "hll_simplethread.h"

maxon::Result<void> HLL::SimpleThread::operator ()()
{
	ApplicationOutput("== Thread Start ==");
	
	this->_group->onMessage([this](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
	{
		ApplicationOutput("Message, closing.");
		this->_group->close();
	});

	if (this->_hub.listen("192.168.1.3", 31337, nullptr, 0, this->_group))
	{
		ApplicationOutput("== Server Listen ==");
		this->_hub.run();
	}

	return maxon::OK;
}

void HLL::SimpleThread::StopListening()
{
	this->_group->close();
	ApplicationOutput("== Server Stopped ==");
}