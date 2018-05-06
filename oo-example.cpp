#include <string>
#include <memory>
#include <Windows.h>
#include "Win32_Service.h"

DWORD serviceMainFunction(LPVOID lpdwThreadParam) {
	Sleep(30000);
	return NO_ERROR;
}

class ExampleService : public Service {
	protected:
		// The background thread that will be executing the application.
		HANDLE appThread_;

	public:
		// The exit code that will be set by the application thread on exit.
		DWORD exitCode_;

		// name - service name
		ExampleService(__in const std::string &name)
		: Service(name, true, true, false),
		appThread_(INVALID_HANDLE_VALUE),
		exitCode_(1) {}// be pessimistic

		~ExampleService() {
			if (appThread_ != INVALID_HANDLE_VALUE) {
				CloseHandle(appThread_);
			}
		}

		virtual void onStart(
			__in DWORD argc,
			__in_ecount(argc) LPWSTR *argv)	{
			setStateRunning();

			// Start the thread that will execute the application
			appThread_ = CreateThread(NULL,
				0, // Need to change the stack size or nah??
				&serviceMainFunction,
				(LPVOID)this,
				0, NULL);

			if (appThread_ == INVALID_HANDLE_VALUE) {
				std::string debugmsg = name_ + ": service_main: Failed to execute the app thread.";
				OutputDebugString(debugmsg.c_str());
				setStateStopped(1);
				return;
			}
		}

	virtual void onStop()
	{
		// Tell thread to stop.

			DWORD status = WaitForSingleObject(appThread_, INFINITE);
		if (status == WAIT_FAILED) {
			std::string debugmsg = name_ + ": service_main: Failed waiting for the app thread.";
			OutputDebugString(debugmsg.c_str());
			// presumably exitCode_ already contains some reason at this point
		}

		// exitCode_ should be set by the application thread on exit
		setStateStopped(exitCode_);
	}
};

int __cdecl main(
	__in long argc,
	__in_ecount(argc) PSTR argv[]) {
	auto svc = std::make_shared<ExampleService>("ExampleService");
	svc->run();
	return 0;
}
