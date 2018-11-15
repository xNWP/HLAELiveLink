#ifndef HLL_SIMPLETHREAD_H__
#define HLL_SIMPLETHREAD_H__

#include "c4d.h"
#include "maxon/thread.h"
#include "../uWebSockets/src/uWS.h"

namespace HLL
{
	class SimpleThread : public maxon::ThreadInterfaceTemplate<SimpleThread>
	{
	public:
		SimpleThread() { this->_group = _hub.createGroup<uWS::SERVER>(); }
		~SimpleThread() { delete _group; }
		maxon::Result<void> operator ()();
		void StopListening();

	private:
		uWS::Hub _hub;
		uWS::Group<uWS::SERVER> *_group;
	};
}

#endif // !HLL_SIMPLETHREAD_H__
