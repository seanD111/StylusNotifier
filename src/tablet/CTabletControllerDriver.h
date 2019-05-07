
#pragma once
#include <lib\openvr\openvr_driver.h>
#include <vector>
#include <map>
#include <string>

class CTabletControllerDriver : public vr::ITrackedDeviceServerDriver
{
public:
	CTabletControllerDriver();
	virtual ~CTabletControllerDriver() {};
	virtual vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);
	virtual void Deactivate();
	virtual void EnterStandby();
	virtual void *GetComponent(const char *pchComponentNameAndVersion);
	virtual void PowerOff();
	virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize);
	virtual vr::DriverPose_t CTabletControllerDriver::GetPose();
	void RunFrame();
	void ProcessEvent(const vr::VREvent_t & vrEvent);
	std::string GetSerialNumber() const { return m_sSerialNumber; }

	vr::EVRInputError UpdateBoolComponent(std::string, bool);
	vr::EVRInputError UpdateScalarComponent(std::string, float);
private:

	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::map<std::string, vr::VRInputComponentHandle_t> m_boolCompMap;
	std::map<std::string, vr::VRInputComponentHandle_t> m_scalCompMap;

	vr::VRInputComponentHandle_t m_compHaptic;
	vr::DriverPose_t m_pose;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;
};

