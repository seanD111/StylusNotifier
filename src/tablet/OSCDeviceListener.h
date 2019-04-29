#pragma once
#include <lib/openvr/openvr_driver.h>
#include <src/osc/OscReceivedElements.h>
#include <src/osc/OscPacketListener.h>
#include <src/ip/UdpSocket.h>
#include "CTabletControllerProvider.h"

class OSCDeviceListener: public osc::OscPacketListener
{
protected:
	virtual void ProcessMessage(const osc::ReceivedMessage& m,
		const IpEndpointName& remoteEndpoint);
	//CTabletControllerProvider to_notify;
};

