#include <string>
#include <Windows.h>

/*
* TODO: make this whole thing in to a class header, but...
* Due to the Windows-api expecting C-style callbacks, E.G. not compatible with member functions...
* we can't easily make this in to a class...
*/
std::string NAME = "ExampleService";
LPSTR WNAME = const_cast<char *>(NAME.c_str());

SERVICE_STATUS STATUS = { 0 };
SERVICE_STATUS_HANDLE STATUSHANDLE = NULL;
HANDLE STOPEVENT = INVALID_HANDLE_VALUE;

void WINAPI service_main(DWORD argc, LPTSTR *argv);
void WINAPI control_handler(DWORD);
DWORD WINAPI worker_thread(LPVOID lpParam);

// main
int main(int argc, TCHAR *argv[]) {
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ WNAME, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}

	return 0;
}

void WINAPI service_main(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;

	// Register our service control handler with the SCM
	STATUSHANDLE = RegisterServiceCtrlHandler(WNAME, control_handler);

	if (STATUSHANDLE == NULL)
	{
		return;
	}

	// Tell the service controller we are starting
	ZeroMemory(&STATUS, sizeof(STATUS));
	STATUS.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	STATUS.dwControlsAccepted = 0;
	STATUS.dwCurrentState = SERVICE_START_PENDING;
	STATUS.dwWin32ExitCode = 0;
	STATUS.dwServiceSpecificExitCode = 0;
	STATUS.dwCheckPoint = 0;

	if (SetServiceStatus(STATUSHANDLE, &STATUS) == FALSE)
	{
	std::string debugmsg = NAME + ": service_main: SetServiceStatus returned an error";
	OutputDebugString(debugmsg.c_str());
	}

	/*
	* Perform tasks necessary to start the service here
	*/

	// Create a service stop event to wait on later
	STOPEVENT = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (STOPEVENT == NULL)
	{
		// Error creating event
		// Tell service controller we are stopped and exit
		STATUS.dwControlsAccepted = 0;
		STATUS.dwCurrentState = SERVICE_STOPPED;
		STATUS.dwWin32ExitCode = GetLastError();
		STATUS.dwCheckPoint = 1;

		if (SetServiceStatus(STATUSHANDLE, &STATUS) == FALSE)
		{
			std::string debugmsg = NAME + ": service_main: SetServiceStatus returned an error";
			OutputDebugString(debugmsg.c_str());
		}
		return;
	}

	// Tell the service controller we are running
	STATUS.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	STATUS.dwCurrentState = SERVICE_RUNNING;
	STATUS.dwWin32ExitCode = 0;
	STATUS.dwCheckPoint = 0;

	if (SetServiceStatus(STATUSHANDLE, &STATUS) == FALSE)
	{
		std::string debugmsg = NAME + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}

	// Start a thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, worker_thread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);


	/*
	* Perform any cleanup tasks
	*/

	CloseHandle(STOPEVENT);

	// Tell the service controller we are stopped
	STATUS.dwControlsAccepted = 0;
	STATUS.dwCurrentState = SERVICE_STOPPED;
	STATUS.dwWin32ExitCode = 0;
	STATUS.dwCheckPoint = 3;

	if (SetServiceStatus(STATUSHANDLE, &STATUS) == FALSE)
	{
		std::string debugmsg = NAME + ": service_main: SetServiceStatus returned an error";
		OutputDebugString(debugmsg.c_str());
	}
}

void WINAPI control_handler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		if (STATUS.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks necessary to stop the service here
		*/

		STATUS.dwControlsAccepted = 0;
		STATUS.dwCurrentState = SERVICE_STOP_PENDING;
		STATUS.dwWin32ExitCode = 0;
		STATUS.dwCheckPoint = 4;

		if (SetServiceStatus(STATUSHANDLE, &STATUS) == FALSE)
		{
			std::string debugmsg = NAME + ": service_main: SetServiceStatus returned an error";
			OutputDebugString(debugmsg.c_str());
		}

		// Signal the worker thread to start shutting down
		SetEvent(STOPEVENT);
		break;

	default:
		break;
	}
}

DWORD WINAPI worker_thread(LPVOID lpParam)
{
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(STOPEVENT, 0) != WAIT_OBJECT_0)
	{
		/*
		* Perform main service function here
		*/

		//  Simulate work
		Sleep(3000);
	}

	return ERROR_SUCCESS;
}
