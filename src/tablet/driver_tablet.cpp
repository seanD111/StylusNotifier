//============ Copyright (c) Valve Corporation, All rights reserved. ============

#include <lib\openvr\openvr_driver.h>
#include "driverlog.h"

#include <vector>
#include <thread>
#include <chrono>
#include <map>
#include <string>

#if defined( _WINDOWS )
#include <windows.h>
#endif

using namespace vr;
using namespace std;


#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif

inline HmdQuaternion_t HmdQuaternion_Init( double w, double x, double y, double z )
{
	HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

inline void HmdMatrix_SetIdentity( HmdMatrix34_t *pMatrix )
{
	pMatrix->m[0][0] = 1.f;
	pMatrix->m[0][1] = 0.f;
	pMatrix->m[0][2] = 0.f;
	pMatrix->m[0][3] = 0.f;
	pMatrix->m[1][0] = 0.f;
	pMatrix->m[1][1] = 1.f;
	pMatrix->m[1][2] = 0.f;
	pMatrix->m[1][3] = 0.f;
	pMatrix->m[2][0] = 0.f;
	pMatrix->m[2][1] = 0.f;
	pMatrix->m[2][2] = 1.f;
	pMatrix->m[2][3] = 0.f;
}


// keys for use with the settings API
static const char * const k_pch_Sample_Section = "tablet";
static const char * const k_pch_Sample_SerialNumber_String = "serialNumber";
static const char * const k_pch_Sample_ModelNumber_String = "modelNumber";
static const char * const k_pch_Sample_WindowX_Int32 = "windowX";
static const char * const k_pch_Sample_WindowY_Int32 = "windowY";
static const char * const k_pch_Sample_WindowWidth_Int32 = "windowWidth";
static const char * const k_pch_Sample_WindowHeight_Int32 = "windowHeight";
static const char * const k_pch_Sample_RenderWidth_Int32 = "renderWidth";
static const char * const k_pch_Sample_RenderHeight_Int32 = "renderHeight";
static const char * const k_pch_Sample_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char * const k_pch_Sample_DisplayFrequency_Float = "displayFrequency";

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

class CWatchdogDriver_Sample : public IVRWatchdogProvider
{
public:
	CWatchdogDriver_Sample()
	{
		m_pWatchdogThread = nullptr;
	}

	virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
	virtual void Cleanup() ;

private:
	std::thread *m_pWatchdogThread;
};

CWatchdogDriver_Sample g_watchdogDriverNull;


bool g_bExiting = false;

void WatchdogThreadFunction(  )
{
	while ( !g_bExiting )
	{
#if defined( _WINDOWS )
		// on windows send the event when the Y key is pressed.
		if ( (0x01 & GetAsyncKeyState( 'Y' )) != 0 )
		{
			// Y key was pressed. 
			vr::VRWatchdogHost()->WatchdogWakeUp();
		}
		std::this_thread::sleep_for( std::chrono::microseconds( 500 ) );
#else
		// for the other platforms, just send one every five seconds
		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
		vr::VRWatchdogHost()->WatchdogWakeUp();
#endif
	}
}

EVRInitError CWatchdogDriver_Sample::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_WATCHDOG_DRIVER_CONTEXT( pDriverContext );
	InitDriverLog( vr::VRDriverLog() );

	// Watchdog mode on Windows starts a thread that listens for the 'Y' key on the keyboard to 
	// be pressed. A real driver should wait for a system button event or something else from the 
	// the hardware that signals that the VR system should start up.
	g_bExiting = false;
	m_pWatchdogThread = new std::thread( WatchdogThreadFunction );
	if ( !m_pWatchdogThread )
	{
		DriverLog( "Unable to create watchdog thread\n");
		return VRInitError_Driver_Failed;
	}

	return VRInitError_None;
}


void CWatchdogDriver_Sample::Cleanup()
{
	g_bExiting = true;
	if ( m_pWatchdogThread )
	{
		m_pWatchdogThread->join();
		delete m_pWatchdogThread;
		m_pWatchdogThread = nullptr;
	}

	CleanupDriverLog();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CTabletControllerDriver : public vr::ITrackedDeviceServerDriver
{
public:
	CTabletControllerDriver()
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
		m_boolCompMap.emplace("/input/stylus/barrel/click", k_ulInvalidInputComponentHandle);
		m_boolCompMap.emplace("/input/stylus/eraser/click", k_ulInvalidInputComponentHandle);
		m_boolCompMap.emplace("/input/stylus/surface/touch", k_ulInvalidInputComponentHandle);
		m_boolCompMap.emplace("/input/finger/1/surface/touch", k_ulInvalidInputComponentHandle);
		m_boolCompMap.emplace("/input/mouse/surface/touch", k_ulInvalidInputComponentHandle);
		

		m_scalCompMap.emplace("/input/mouse/position/x", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/mouse/position/y", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/stylus/surface/value", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/stylus/position/x", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/stylus/position/y", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/finger/1/position/x", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/finger/1/position/y", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/finger/1/size/x", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/finger/1/size/y", k_ulInvalidInputComponentHandle);

		m_scalCompMap.emplace("/input/surface/size/y", k_ulInvalidInputComponentHandle);
		m_scalCompMap.emplace("/input/surface/size/x", k_ulInvalidInputComponentHandle);
		
	}

	virtual ~CTabletControllerDriver()
	{
	}


	virtual EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId )
	{
		m_unObjectId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer( m_unObjectId );

		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str() );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_RenderModelName_String, "{tablet}/rendermodels/surface");

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property( m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2 );

		// avoid "not fullscreen" warnings from vrmonitor
		vr::VRProperties()->SetBoolProperty( m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false );

		// our sample device isn't actually tracked, so set this property to avoid having the icon blink in the status window
		vr::VRProperties()->SetBoolProperty( m_ulPropertyContainer, Prop_NeverTracked_Bool, true );


		vr::VRProperties()->SetInt32Property( m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_OptOut);

		// this file tells the UI what to show the user for binding this controller as well as what default bindings should
		// be for legacy or other apps
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_InputProfilePath_String, "{tablet}/input/tablet_profile.json" );


		


		for (auto & x : m_boolCompMap)
		{
			vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, x.first.c_str(), &x.second);
		}

		for (auto & x : m_scalCompMap)
		{
			vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, x.first.c_str(), &x.second, VRScalarType_Absolute, VRScalarUnits_NormalizedOneSided);
		}

		return VRInitError_None;
	}

	virtual void Deactivate()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby()
	{
	}

	void *GetComponent( const char *pchComponentNameAndVersion )
	{
		// override this to add a component to a driver
		return NULL;
	}

	virtual void PowerOff()
	{
	}

	/** debug request from a client */
	virtual void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
	{
		if ( unResponseBufferSize >= 1 )
			pchResponseBuffer[0] = 0;
	}

	virtual DriverPose_t GetPose()
	{
		DriverPose_t pose = { 0 };
		pose.poseIsValid = false;
		pose.result = TrackingResult_Calibrating_OutOfRange;
		pose.deviceIsConnected = true;

		pose.qWorldFromDriverRotation = HmdQuaternion_Init( 1, 0, 0, 0 );
		pose.qDriverFromHeadRotation = HmdQuaternion_Init( 1, 0, 0, 0 );

		return pose;
	}


	void RunFrame()
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

	void ProcessEvent( const vr::VREvent_t & vrEvent )
	{
		switch ( vrEvent.eventType )
		{
		case vr::VREvent_Input_HapticVibration:
		{
			if ( vrEvent.data.hapticVibration.componentHandle == m_compHaptic )
			{
				// This is where you would send a signal to your hardware to trigger actual haptic feedback
				DriverLog( "BUZZ!\n" );
			}
		}
		break;
		}
	}


	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::map<std::string, vr::VRInputComponentHandle_t> m_boolCompMap;
	std::map<std::string, vr::VRInputComponentHandle_t> m_scalCompMap;

	vr::VRInputComponentHandle_t m_compHaptic;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;


};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CServerDriver_Sample: public IServerTrackedDeviceProvider
{
public:
	virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
	virtual void Cleanup() ;
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame() ;
	virtual bool ShouldBlockStandbyMode()  { return false; }
	virtual void EnterStandby()  {}
	virtual void LeaveStandby()  {}

private:
	//CSampleDeviceDriver *m_pNullHmdLatest = nullptr;
	CTabletControllerDriver *m_pController = nullptr;
};

CServerDriver_Sample g_serverDriverNull;


EVRInitError CServerDriver_Sample::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_SERVER_DRIVER_CONTEXT( pDriverContext );
	InitDriverLog( vr::VRDriverLog() );

	//m_pNullHmdLatest = new CSampleDeviceDriver();
	//vr::VRServerDriverHost()->TrackedDeviceAdded( m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pNullHmdLatest );

	m_pController = new CTabletControllerDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded( m_pController->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, m_pController );

	return VRInitError_None;
}

void CServerDriver_Sample::Cleanup() 
{
	CleanupDriverLog();
	//delete m_pNullHmdLatest;
	//m_pNullHmdLatest = NULL;
	delete m_pController;
	m_pController = NULL;
}


void CServerDriver_Sample::RunFrame()
{
	//if ( m_pNullHmdLatest )
	//{
	//	m_pNullHmdLatest->RunFrame();
	//}
	if ( m_pController )
	{
		m_pController->RunFrame();
	}

	vr::VREvent_t vrEvent;
	while ( vr::VRServerDriverHost()->PollNextEvent( &vrEvent, sizeof( vrEvent ) ) )
	{
		if ( m_pController )
		{
			m_pController->ProcessEvent( vrEvent );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory( const char *pInterfaceName, int *pReturnCode )
{
	if( 0 == strcmp( IServerTrackedDeviceProvider_Version, pInterfaceName ) )
	{
		return &g_serverDriverNull;
	}
	if( 0 == strcmp( IVRWatchdogProvider_Version, pInterfaceName ) )
	{
		return &g_watchdogDriverNull;
	}

	if( pReturnCode )
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}
