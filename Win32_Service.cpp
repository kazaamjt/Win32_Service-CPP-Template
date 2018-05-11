#include "Win32_Service.h"

WindowsService *WindowsService::instance;

WindowsService::WindowsService(std::string _name, bool _canStop, bool _canShutdown, bool _canPauseContinue) {
	name = _name;
	Wname = const_cast<char*>(name.c_str());

	bool canStop = _canStop;
	bool canShutdown = _canShutdown;
	bool canPauseContinue = _canPauseContinue;

	status = { 0 };
	statusHandle = NULL;

	stopEvent = INVALID_HANDLE_VALUE;
	pauseEvent = INVALID_HANDLE_VALUE;
	continueEvent = INVALID_HANDLE_VALUE;
}

int WindowsService::run(int argc, TCHAR *argv[]) {
	instance = this;

	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ Wname, (LPSERVICE_MAIN_FUNCTION)service_main },
	{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(serviceTable) == FALSE) {
		return GetLastError();
	}

	return 0;
}

void WINAPI WindowsService::service_main(DWORD argc, LPTSTR *argv) {
	// Register our service control handler with the SCM
	instance->statusHandle = RegisterServiceCtrlHandler(instance->Wname, instance->control_handler);

	if (instance->statusHandle == NULL) {
		return;
	}

	instance->startup();

	// Start a thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, worker_thread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	instance->exit();
}

void WINAPI WindowsService::control_handler(DWORD CtrlCode)
{
	switch (CtrlCode) {
	case SERVICE_CONTROL_STOP:
		if (instance->status.dwCurrentState != SERVICE_RUNNING) { break; }
		instance->control_stop();
		break;

	case SERVICE_CONTROL_PAUSE:
		if (instance->status.dwCurrentState != SERVICE_RUNNING) { break; }
		instance->control_pause();
		break;

	case SERVICE_CONTROL_CONTINUE:
		if (instance->status.dwCurrentState != SERVICE_PAUSED) { break; }
		instance->control_continue();
		break;

	case SERVICE_CONTROL_SHUTDOWN:
		if (instance->status.dwCurrentState != SERVICE_RUNNING) { break; }
		instance->control_stop();
		break;

	case SERVICE_CONTROL_INTERROGATE: // Deprecated, but you never know... let's just handle it.
		SetServiceStatus(instance->statusHandle, &instance->status);
		break;

	default:
		break;
	}
}

DWORD WINAPI WindowsService::worker_thread(LPVOID lpParam) {
	return instance->worker(lpParam);
}

void WindowsService::startup() {
	ZeroMemory(&status, sizeof(status));
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwControlsAccepted = 0;
	status.dwServiceSpecificExitCode = 0;
	set_state(SERVICE_START_PENDING);

	on_startup();

	stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	pauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	continueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (stopEvent == NULL) {
		on_error();
		return;
	}

	status.dwControlsAccepted = 0;
	if (canStop) {
		status.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	}
	if (canShutdown) {
		status.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	}
	if (canPauseContinue) {
		status.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	}

	set_state(SERVICE_RUNNING);
}

void WindowsService::exit() {
	on_exit();
	CloseHandle(stopEvent);
	CloseHandle(pauseEvent);
	CloseHandle(continueEvent);
	set_state(SERVICE_STOPPED);
}

void WindowsService::on_error() {
	status.dwControlsAccepted = 0;
	status.dwCurrentState = SERVICE_STOPPED;
	status.dwWin32ExitCode = GetLastError();
	status.dwCheckPoint = 0;

	if (SetServiceStatus(statusHandle, &status) == FALSE)
	{
		std::string debugmsg = name + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}
}

void WindowsService::control_stop() {
	on_stop();
	status.dwControlsAccepted = 0;
	set_state(SERVICE_STOP_PENDING);
	SetEvent(stopEvent);
}

void WindowsService::control_pause() {
	set_state(SERVICE_PAUSE_PENDING);
	on_pause();
	SetEvent(pauseEvent);
}

void WindowsService::control_continue() {
	set_state(SERVICE_CONTINUE_PENDING);
	ResetEvent(pauseEvent);
	on_continue();
	SetEvent(continueEvent);
	ResetEvent(continueEvent);
}

void WindowsService::control_stopOnPause() {
	status.dwControlsAccepted = 0;
	set_state(SERVICE_STOP_PENDING);
	on_continue();
	SetEvent(stopEvent);
	SetEvent(continueEvent);
}

void WindowsService::set_state(DWORD state) {
	status.dwCurrentState = state;
	status.dwWin32ExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;
	SetServiceStatus(statusHandle, &status);
	if (SetServiceStatus(statusHandle, &status) == FALSE)
	{
		std::string debugmsg = name + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}
}

void WindowsService::bump() {
	status.dwCheckPoint++;
	if (SetServiceStatus(statusHandle, &status) == FALSE)
	{
		std::string debugmsg = name + ": service_main: SetServiceStatus dwCheckPoint operation failed";
		OutputDebugString(debugmsg.c_str());
	}
}
