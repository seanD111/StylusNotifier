#include "CDeviceControllerProvider.h"
#include "OSCDeviceListener.h"


//CDeviceControllerProvider::CDeviceControllerProvider()
//{	
//	//for (std::vector<int>::size_type i = 0; i < tablet_driver::TABLET_NAMES.size() - 1; i++) {
//	//	m_pController.emplace(tablet_driver::TABLET_NAMES[i], new CTabletControllerDriver());
//	//}
//
//	OSCDeviceListener listener;
//	UdpListeningReceiveSocket s(
//		IpEndpointName(IpEndpointName::ANY_ADDRESS, tablet_driver::listen_port),
//		&listener);
//	s.Run();
//}

void CDeviceControllerProvider::ListenerThread()
{
	OSCDeviceListener listener(this);

	UdpListeningReceiveSocket s(
		IpEndpointName(IpEndpointName::ANY_ADDRESS, tablet_driver::listen_port),
		&listener);
	s.Run();
}

void CDeviceControllerProvider::AddDevice(std::string name) {
	m_pController.emplace(name, new CTabletControllerDriver());
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pController[name]->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_Controller,
		m_pController[name]);
}

bool CDeviceControllerProvider::HasDevice(std::string name) {
	bool to_return;
	if (m_pController.find(name) == m_pController.end()) {
		to_return = false;
	}
	else {
		to_return = true;
	}
	return to_return;
}

void CDeviceControllerProvider::MessageReceived(std::string address, bool value) {
	std::lock_guard<std::mutex> guard(lock);
	std::string delimiter = "/";
	std::string device = address.substr(1, address.find(delimiter,1) -1 );
	if (!HasDevice(device)) {
		AddDevice(device);
	}
	else {
		std::string key = address.substr(address.find(delimiter, 1));
		m_pController[device]->UpdateBoolComponent(key, value);
	}

	DriverLog(std::string("Provider: " +address +" " + std::to_string(value)).c_str());
}

void CDeviceControllerProvider::MessageReceived(std::string address, double value) {
	std::lock_guard<std::mutex> guard(lock);
	std::string delimiter = "/";
	std::string device = address.substr(1, address.find(delimiter, 1)-1);
	if (!HasDevice(device)) {
		AddDevice(device);
	}
	else {
		std::string key = address.substr(address.find(delimiter, 1));
		m_pController[device]->UpdateScalarComponent(key, value);
	}
	DriverLog(std::string("Provider: " + address + " " + std::to_string(value)).c_str());
}


vr::EVRInitError CDeviceControllerProvider::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	listener_thread = std::thread(&CDeviceControllerProvider::ListenerThread, this);

	//m_pNullHmdLatest = new CSampleDeviceDriver();
	//vr::VRServerDriverHost()->TrackedDeviceAdded( m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pNullHmdLatest );


	//vr::VRServerDriverHost()->TrackedDeviceAdded( m_pController->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, m_pController );

	return vr::VRInitError_None;
}

void CDeviceControllerProvider::Cleanup()
{
	CleanupDriverLog();
	//delete m_pNullHmdLatest;
	//m_pNullHmdLatest = NULL;
	for (auto &x: m_pController) {
		delete x.second;
		x.second = NULL;
	}

}

void CDeviceControllerProvider::RunFrame()
{


	for (auto &x : m_pController) {
		if (x.second)
		{
			x.second->RunFrame();
		}
	}

	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
	{

		for (auto &x : m_pController) {
			if (x.second)
			{
				x.second->ProcessEvent(vrEvent);
			}
		}
	}
}
