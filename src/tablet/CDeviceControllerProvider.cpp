#include "CDeviceControllerProvider.h"
#include <src/osc/OscPacketListener.h>
#include <windows.h>


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


class OSCDeviceListener : public osc::OscPacketListener
{
public:
	OSCDeviceListener(CDeviceControllerProvider *provider);

protected:
	virtual void ProcessMessage(const osc::ReceivedMessage& m,
		const IpEndpointName& remoteEndpoint);
private:
	CDeviceControllerProvider *to_notify;
};


namespace tablet_driver {
	static const std::vector<std::string> TABLET_NAMES = { "surface1", "surface2" };
	static const std::string listen_address = "127.0.0.1";
	static const unsigned short listen_port = 8338;

}

OSCDeviceListener::OSCDeviceListener(CDeviceControllerProvider *provider) {
	to_notify = provider;
}

void OSCDeviceListener::ProcessMessage(const osc::ReceivedMessage& m,
	const IpEndpointName& remoteEndpoint)
{
	DriverLog("OSCDeviceListener::ProcessMessage call \n");
	try {
		std::string address = std::string(m.AddressPattern());
		DriverLog(std::string("Got " + address).c_str());

		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		arg++;

		

		if (arg->IsBool()) {
			to_notify->MessageReceived(address, arg->AsBool());
		}
		else if (arg->IsFloat()) {
			to_notify->MessageReceived(address, arg->AsFloat());
		}

		else if (arg->IsDouble()) {
			to_notify->MessageReceived(address, arg->AsDouble());
		}

	}
	catch (osc::Exception& e) {
		DriverLog(std::string("error while parsing message: " + std::string(e.what()) + "\n").c_str());

	}
}

CDeviceControllerProvider::CDeviceControllerProvider() {

}

void CDeviceControllerProvider::ListenerThread()
{
#ifdef _WIN32
	HRESULT hr = SetThreadDescription(GetCurrentThread(), L"tablet listen thread");
#endif
	DriverLog("CDeviceControllerProvider Starting listener thread\n");
	OSCDeviceListener listener(this);
	UdpListeningReceiveSocket socket_listener(IpEndpointName(IpEndpointName::ANY_ADDRESS, tablet_driver::listen_port), &listener);
	receive_socket = &socket_listener;
	socket_listener.RunUntilSigInt();
}

void CDeviceControllerProvider::AddDevice(std::string name) {
	m_pController.emplace(name, CTabletControllerDriver());
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pController[name].GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_Controller,
		&m_pController[name]);
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
	DriverLog(std::string("Bool Message Received from: \n"+ address).c_str());
	std::string delimiter = "/";
	std::string device = address.substr(1, address.find(delimiter,1) -1 );
	if (!HasDevice(device)) {
		AddDevice(device);
	}
	else {
		std::string key = address.substr(address.find(delimiter, 1));
		m_pController[device].UpdateBoolComponent(key, value);
	}

	DriverLog(std::string("Provider: " +address +" " + std::to_string(value)).c_str());
}

void CDeviceControllerProvider::MessageReceived(std::string address, double value) {
	std::lock_guard<std::mutex> guard(lock);
	DriverLog(std::string("double Message Received from: \n" + address).c_str());
	std::string delimiter = "/";
	std::string device = address.substr(1, address.find(delimiter, 1)-1);
	if (!HasDevice(device)) {
		AddDevice(device);
	}
	else {
		std::string key = address.substr(address.find(delimiter, 1));
		m_pController[device].UpdateScalarComponent(key, value);
	}
	DriverLog(std::string("Provider: " + address + " " + std::to_string(value)).c_str());
}


vr::EVRInitError CDeviceControllerProvider::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	listener_thread = std::thread(&CDeviceControllerProvider::ListenerThread, this);

	DriverLog("DeviceProvider: Init called\n");
	//m_pNullHmdLatest = new CSampleDeviceDriver();
	//vr::VRServerDriverHost()->TrackedDeviceAdded( m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pNullHmdLatest );


	//vr::VRServerDriverHost()->TrackedDeviceAdded( m_pController->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, m_pController );

	return vr::VRInitError_None;
}

void CDeviceControllerProvider::Cleanup()
{
	CleanupDriverLog();
	if (receive_socket != NULL) {
		receive_socket->Break();
	}
	
	listener_thread.join();
	//delete m_pNullHmdLatest;
	//m_pNullHmdLatest = NULL;
	for (auto &x: m_pController) {
		x.second.Deactivate();
	}

}

void CDeviceControllerProvider::RunFrame()
{


	for (auto &x : m_pController) {
		x.second.RunFrame();

	}

	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
	{

		for (auto &x : m_pController) {

			x.second.ProcessEvent(vrEvent);

		}
	}
}
