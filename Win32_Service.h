#pragma once
#include <string>
#include <Windows.h>

/*
 * The general wrapper for running as a service.
 * The subclasses need to define their virtual methods.
 */

class __declspec(dllexport) Service{
	public:
		// Because of how service work, you can only have 1 service object in your process.
		Service(const std::string &name,
			bool canStop,
			bool canShutdown,
			bool canPauseContinue);

			virtual ~Service();
			// Starts the run, return on stop.
			void run();

			// Don't use it for SERVICE_STOPPED.
			void setState(DWORD state);
			// Convenient
			void setStateRunning()
			{
				setState(SERVICE_RUNNING);
			}
			void setStatePaused()
			{
				setState(SERVICE_PAUSED);
			}

			// Setting to SERVICE_STOPPED is a lot more complicated
			void setStateStopped(DWORD exitCode);
			void setStateStoppedSpecific(DWORD exitCode);
			
			// On the lengthy operations, periodically call this to tell the
			// controller that the service is not dead.
			void bump();

			// Can be used to set the expected length of long operations. Invokes bump.
			void hintTime(DWORD msec);

			// Overwrite these in the subclass.
			virtual void onStart(
				__in DWORD argc,
				__in_ecount(argc) LPSTR *argv);
			virtual void onStop(); // sets the success exit code
			virtual void onPause();
			virtual void onContinue();
			virtual void onShutdown(); // calls onStop()

	protected:
		// The callback for the service start.
		static void WINAPI serviceMain(
			__in DWORD argc,
			__in_ecount(argc) LPSTR *argv);
		// The callback for the requests.
		static void WINAPI serviceCtrlHandler(DWORD ctrl);

		// the internal version.
		void setStateL(DWORD state);

	protected:
		static Service *instance_;

		std::string name_;
		
		SERVICE_STATUS_HANDLE statusHandle_; // Handle used to report the status.
		SERVICE_STATUS status_; // Current status

	private:
		Service();
		Service(const Service &);
		void operator=(const Service &);
};
