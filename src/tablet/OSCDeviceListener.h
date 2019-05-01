#pragma once
#include <lib/openvr/openvr_driver.h>
#include <src/osc/OscReceivedElements.h>
#include <src/osc/OscPacketListener.h>
#include <src/ip/UdpSocket.h>
#include <src/tablet/CDeviceControllerProvider.h>

class OSCDeviceListener: public osc::OscPacketListener
{
public:
	OSCDeviceListener(CDeviceControllerProvider *provider);

protected:
	virtual void ProcessMessage(const osc::ReceivedMessage& m,
		const IpEndpointName& remoteEndpoint);
private:
	CDeviceControllerProvider *to_notify;
};

