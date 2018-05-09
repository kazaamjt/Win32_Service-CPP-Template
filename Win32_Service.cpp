#include <assert.h>
#include "Win32_Service.h"

Service *Service::instance;

Service::Service(std::string in_name,
	bool in_canStop,
	bool in_canShutdown,
	bool in_canPauseContinue) {

	name = in_name;
	Wname = const_cast<char *>(name.c_str());

	canStop = in_canStop;
	canShutdown = in_canShutdown;
	canPauseContinue = in_canPauseContinue;
}

int Service::run() {
	instance = this;

	SERVICE_TABLE_ENTRY ServiceTable[] = {
		{ Wname,
		(LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
		return GetLastError();
	}
	return 0;
}

void WINAPI Service::service_main(DWORD argc, LPTSTR *argv) {
	assert(instance != NULL);

	// Register our controller
	instance->statusHandle = RegisterServiceCtrlHandler(instance->Wname, service_control_handler);
	if (instance->statusHandle == NULL)
	{
		std::string debugmsg = instance->name + ": service_main: Failed to register service.";
		OutputDebugString(debugmsg.c_str());
		return;
	}

	// initialize service status and tell the windows service controller we're starting
	SecureZeroMemory(&instance->status, sizeof(instance->status));
	instance->status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	instance->on_start_pending();

	if (instance->stopEvent == NULL) {
		// Tell service controller we are stopped and exit
		instance->status.dwControlsAccepted = 0;
		instance->on_stop_error();
		return;
	}

	instance->on_start();

	// Wait until our thread is done
	WaitForSingleObject(instance->workerThreadHandle, INFINITE);

	instance->on_stop_pending();
	instance->on_stop();
}

void WINAPI Service::service_control_handler(DWORD control) {
	switch (control) {
		case SERVICE_CONTROL_STOP:
			if (instance->status.dwControlsAccepted & SERVICE_ACCEPT_STOP) {
				instance->on_control_stop();
			}
			break;
		case SERVICE_CONTROL_PAUSE:
			if (instance->status.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
				instance->on_control_pause();
			}
			break;
		case SERVICE_CONTROL_CONTINUE:
			if (instance->status.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
				instance->on_control_continue();
			}
			break;
		case SERVICE_CONTROL_SHUTDOWN:
			if (instance->status.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) {
				instance->on_control_shutdown();
			}
			break;

		case SERVICE_CONTROL_INTERROGATE: // Deprecated, but you never know... let's just handle it.
			SetServiceStatus(instance->statusHandle, &instance->status);
			break;

		default:
			break;
	}
}

void Service::on_control_stop() {
	if (status.dwCurrentState != SERVICE_RUNNING)
		return;
	
	SetEvent(stopEvent);
}

void Service::on_control_pause() {
	if (status.dwCurrentState != SERVICE_RUNNING)
		return;
}

void Service::on_control_continue() {
	if (status.dwCurrentState != SERVICE_PAUSED)
		return;
}

void Service::on_control_shutdown() {
	on_control_stop();
}

void Service::on_start_pending() {
	set_state(SERVICE_START_PENDING);
	stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	status.dwWin32ExitCode = NO_ERROR;
	status.dwServiceSpecificExitCode = 0;

	status.dwControlsAccepted = 0;
	if (canStop)
		status.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	if (canShutdown)
		status.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	if (canPauseContinue)
		status.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
}

void Service::on_start() {
	service_init();
	workerThreadHandle = CreateThread(NULL, 0, &worker_thread, (LPVOID)this, 0, NULL);
	set_state(SERVICE_RUNNING);
}

void Service::on_pause() {
	set_state(SERVICE_PAUSED);
}

void Service::on_continue() {
	set_state(SERVICE_RUNNING);
}

void Service::on_stop_pending() {
	set_state(SERVICE_STOP_PENDING);
	service_cleanUp();
	CloseHandle(stopEvent);
}

void Service::on_stop() {
	set_stateStopped(NO_ERROR);
}

void Service::on_stop_error() {
	status.dwCurrentState = SERVICE_STOPPED;
	status.dwWin32ExitCode = GetLastError();
	status.dwCheckPoint = 1;
}

void Service::on_stop_pending() {
	SetEvent(stopEvent);
}

void Service::set_state(DWORD state) {
	set_stateL(state);
}

void Service::set_stateL(DWORD state) {
	status.dwCurrentState = state;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;
	SetServiceStatus(statusHandle, &status);
	if (SetServiceStatus(statusHandle, &status) == FALSE)
	{
		std::string debugmsg = name + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}
}

void Service::set_stateStopped(DWORD exitCode) {
	status.dwWin32ExitCode = exitCode;
	set_stateL(SERVICE_STOPPED);
}

void Service::set_stateStoppedSpecific(DWORD exitCode) {
	status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	status.dwServiceSpecificExitCode = exitCode;
	set_stateL(SERVICE_STOPPED);
}

void Service::bump() {
	++status.dwCheckPoint;
	::SetServiceStatus(statusHandle, &status);
}

DWORD WINAPI Service::worker_thread(LPVOID lpParam) {
	instance->worker_thread_function(lpParam);
}