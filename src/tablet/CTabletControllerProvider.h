#include "driverlog.h"
#pragma once
#include <lib\openvr\openvr_driver.h>
#include "CTabletControllerDriver.h"
#include "OSCDeviceListener.h"
#include <vector>
#include <map>
#include <string>

namespace tablet_driver {
	static const std::vector<std::string> TABLET_NAMES = { "surface1", "surface2" };
	static const std::string listen_address = "127.0.0.1";
	static const unsigned short listen_port = 27015;
}



class CTabletControllerProvider : public vr::IServerTrackedDeviceProvider
{
public:
	CTabletControllerProvider();
	virtual vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
	void AddDevice(std::string);
	bool HasDevice(std::string);
	void MessageReceived(std::string, bool);
	void MessageReceived(std::string, double);
private:
	std::map<std::string, CTabletControllerDriver*> m_pController;
	//OSCDeviceListener notifier;
	//UdpListeningReceiveSocket receive_socket;
};

