#include "OSCDeviceListener.h"

OSCDeviceListener::OSCDeviceListener(CDeviceControllerProvider *provider) {
	to_notify = provider;
}

void OSCDeviceListener::ProcessMessage(const osc::ReceivedMessage& m,
	const IpEndpointName& remoteEndpoint)
{
	try {
		std::string address = std::string(m.AddressPattern());

		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		arg++;
		if (arg->IsBool()) {
			to_notify->MessageReceived(address, arg->AsBool());
		}
		else if(arg->IsDouble()) {
			to_notify->MessageReceived(address, arg->AsDouble());
		}

	}
	catch (osc::Exception& e) {
		DriverLog(std::string("error while parsing message: " + std::string(e.what()) + "\n").c_str());

	}
}
