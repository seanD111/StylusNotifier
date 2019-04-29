#include "CTabletControllerDriver.h"

inline vr::HmdQuaternion_t HmdQuaternion_Init(double w, double x, double y, double z)
{
	vr::HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

CTabletControllerDriver::CTabletControllerDriver()
{
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;


	char buf[1024];
	vr::VRSettings()->GetString("driver_tablet", "serialNumber", buf, sizeof(buf));
	m_sSerialNumber = buf;
	vr::VRSettings()->GetString("driver_tablet", "modelNumber", buf, sizeof(buf));
	m_sModelNumber = buf;
	// note: in this context, 'surface/touch' refers to when the stylus or finger are touching the screen- the stylus 'barrel/click' refers to when the stylus button is pressed.
	// stylus/surface/value refers to the amount of pressure being placed on the screen
	// this has caveats:
	//	1) finger/#/position/x,y cannot change without finger/#/surface/touch being true
	//	2) stylus/position/x,y CAN change without stylus/surface/touch being true  (if it does, it is in air)

	//for now, fingers only support a fixed quantity
	m_boolCompMap.emplace("/input/stylus/barrel/click", vr::k_ulInvalidInputComponentHandle);
	m_boolCompMap.emplace("/input/stylus/eraser/click", vr::k_ulInvalidInputComponentHandle);
	m_boolCompMap.emplace("/input/stylus/surface/touch", vr::k_ulInvalidInputComponentHandle);
	m_boolCompMap.emplace("/input/finger/1/surface/touch", vr::k_ulInvalidInputComponentHandle);
	m_boolCompMap.emplace("/input/mouse/surface/touch", vr::k_ulInvalidInputComponentHandle);


	m_scalCompMap.emplace("/input/mouse/position/x", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/mouse/position/y", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/stylus/surface/value", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/stylus/position/x", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/stylus/position/y", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/finger/1/position/x", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/finger/1/position/y", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/finger/1/size/x", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/finger/1/size/y", vr::k_ulInvalidInputComponentHandle);

	m_scalCompMap.emplace("/input/surface/size/y", vr::k_ulInvalidInputComponentHandle);
	m_scalCompMap.emplace("/input/surface/size/x", vr::k_ulInvalidInputComponentHandle);

}

vr::EVRInputError CTabletControllerDriver::UpdateBoolComponent(std::string key, bool val) {
	vr::EVRInputError err;
	if (m_boolCompMap.find(key) != m_boolCompMap.end()) {
		vr::VRInputComponentHandle_t handle = m_boolCompMap[key];
		err = vr::VRDriverInput()->UpdateBooleanComponent(handle, val, 0);
	}
	else {
		//somethings wrong
		err = vr::VRInputError_NameNotFound;
	}
	return err;
}
vr::EVRInputError CTabletControllerDriver::UpdateScalarComponent(std::string key, float val) {
	vr::EVRInputError err;
	if (m_scalCompMap.find(key) != m_scalCompMap.end()) {
		vr::VRInputComponentHandle_t handle = m_scalCompMap[key];
		 err = vr::VRDriverInput()->UpdateScalarComponent(handle, val, 0);
	}
	else {
		//somethings wrong
		err = vr::VRInputError_NameNotFound;
	}
	return err;
}



vr::EVRInitError CTabletControllerDriver::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ModelNumber_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_RenderModelName_String, "{tablet}/rendermodels/surface");

	// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, vr::Prop_CurrentUniverseId_Uint64, 2);

	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_IsOnDesktop_Bool, false);

	// our sample device isn't actually tracked, so set this property to avoid having the icon blink in the status window
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_NeverTracked_Bool, true);


	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_OptOut);

	// this file tells the UI what to show the user for binding this controller as well as what default bindings should
	// be for legacy or other apps
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_InputProfilePath_String, "{tablet}/input/tablet_profile.json");

	for (auto & x : m_boolCompMap)
	{
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, x.first.c_str(), &x.second);
	}

	for (auto & x : m_scalCompMap)
	{
		vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, x.first.c_str(), &x.second, vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
	}

	return  vr::VRInitError_None;
}

void CTabletControllerDriver::Deactivate()
{
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void CTabletControllerDriver::EnterStandby()
{
}

void *CTabletControllerDriver::GetComponent(const char *pchComponentNameAndVersion)
{
	// override this to add a component to a driver
	return NULL;
}

void CTabletControllerDriver::PowerOff()
{
}

	/** debug request from a client */
void CTabletControllerDriver::DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

vr::DriverPose_t CTabletControllerDriver::GetPose()
{
	vr::DriverPose_t pose = { 0 };
	pose.poseIsValid = false;
	pose.result = vr::TrackingResult_Calibrating_OutOfRange;
	pose.deviceIsConnected = true;

	pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
	pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);

	return pose;
}


void CTabletControllerDriver::RunFrame()
{
#if defined( _WINDOWS )
	// Your driver would read whatever hardware state is associated with its input components and pass that
	// in to UpdateBooleanComponent. This could happen in RunFrame or on a thread of your own that's reading USB
	// state. There's no need to update input state unless it changes, but it doesn't do any harm to do so.

	//vr::VRDriverInput()->UpdateBooleanComponent( m_compA, (0x8000 & GetAsyncKeyState( 'A' )) != 0, 0 );
	//vr::VRDriverInput()->UpdateBooleanComponent( m_compB, (0x8000 & GetAsyncKeyState( 'B' )) != 0, 0 );
	//vr::VRDriverInput()->UpdateBooleanComponent( m_compC, (0x8000 & GetAsyncKeyState( 'C' )) != 0, 0 );
#endif
}

void CTabletControllerDriver::ProcessEvent(const vr::VREvent_t & vrEvent)
{
	switch (vrEvent.eventType)
	{
	case vr::VREvent_Input_HapticVibration:
	{
		//if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic)
		//{
		//	// This is where you would send a signal to your hardware to trigger actual haptic feedback
		//	DriverLog("BUZZ!\n");
		//}
	}
	break;
	}
}
