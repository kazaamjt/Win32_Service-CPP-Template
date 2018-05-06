#include <string>
#include <assert.h>
#include "Win32_Service.h"

Service *Service::instance_;

Service::Service(const std::string &name,
	bool canStop,
	bool canShutdown,
	bool canPauseContinue)
	:name_(name), statusHandle_(NULL) {
	// The service runs in its own process.
	status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	// The service is starting.
	status_.dwCurrentState = SERVICE_START_PENDING;

	// The accepted commands of the service.
	status_.dwControlsAccepted = 0;
	if (canStop)
		status_.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	if (canShutdown)
		status_.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	if (canPauseContinue)
		status_.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;

	status_.dwWin32ExitCode = NO_ERROR;
	status_.dwServiceSpecificExitCode = 0;
	status_.dwCheckPoint = 0;
	status_.dwWaitHint = 0;
}

Service::~Service(){}


void Service::run() {
	instance_ = this;

	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ (LPSTR)name_.c_str(), serviceMain },
	{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(serviceTable)) {
		std::string debugmsg = name_ + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}
}

void WINAPI Service::serviceMain(
	__in DWORD argc,
	__in_ecount(argc) LPSTR *argv) {
	assert(instance_ != NULL);

	// Register the handler function for the service
	instance_->statusHandle_ = RegisterServiceCtrlHandler(
		instance_->name_.c_str(), serviceCtrlHandler);
	if (instance_->statusHandle_ == NULL)
	{
		std::string debugmsg = instance_->name_ + ": service_main: Failed to register service.";
		OutputDebugString(debugmsg.c_str());
		return;
	}

	// Start the service.
	instance_->setState(SERVICE_START_PENDING);
	instance_->onStart(argc, argv);
}

void WINAPI Service::serviceCtrlHandler(DWORD ctrl) {
	switch (ctrl)
	{
	case SERVICE_CONTROL_STOP:
		if (instance_->status_.dwControlsAccepted & SERVICE_ACCEPT_STOP) {
			instance_->setState(SERVICE_STOP_PENDING);
			instance_->onStop();
		}
		break;
	case SERVICE_CONTROL_PAUSE:
		if (instance_->status_.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
			instance_->setState(SERVICE_PAUSE_PENDING);
			instance_->onPause();
		}
		break;
	case SERVICE_CONTROL_CONTINUE:
		if (instance_->status_.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
			instance_->setState(SERVICE_CONTINUE_PENDING);
			instance_->onContinue();
		}
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		if (instance_->status_.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) {
			instance_->setState(SERVICE_STOP_PENDING);
			instance_->onShutdown();
		}
		break;
	case SERVICE_CONTROL_INTERROGATE:
		SetServiceStatus(instance_->statusHandle_, &instance_->status_);
		break;
	default:
		break;
	}
}

void Service::setState(DWORD state) {
	setStateL(state);
}

void Service::setStateL(DWORD state) {
	status_.dwCurrentState = state;
	status_.dwCheckPoint = 0;
	status_.dwWaitHint = 0;
	SetServiceStatus(statusHandle_, &status_);
}

void Service::setStateStopped(DWORD exitCode) {
	status_.dwWin32ExitCode = exitCode;
	setStateL(SERVICE_STOPPED);
}


void Service::setStateStoppedSpecific(DWORD exitCode) {
	status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	status_.dwServiceSpecificExitCode = exitCode;
	setStateL(SERVICE_STOPPED);
}

void Service::bump() {
	++status_.dwCheckPoint;
	::SetServiceStatus(statusHandle_, &status_);
}

void Service::hintTime(DWORD msec) {
	++status_.dwCheckPoint;
	status_.dwWaitHint = msec;
	::SetServiceStatus(statusHandle_, &status_);
	status_.dwWaitHint = 0; // won't apply after the next update
}

void Service::onStart(
	__in DWORD argc,
	__in_ecount(argc) LPSTR *argv) {
	setState(SERVICE_RUNNING);
}

void Service::onStop() {
	setStateStopped(NO_ERROR);
}

void Service::onPause() {
	setState(SERVICE_PAUSED);
}

void Service::onContinue() {
	setState(SERVICE_RUNNING);
}

void Service::onShutdown() {
	onStop();
}
