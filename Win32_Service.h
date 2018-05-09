#pragma once
#include <string>
#include <Windows.h>

/*
 * Just subclass this class really...
 */

 // Meant for running an exe as a service on windows platforms.
class Service {
	private:
		std::string name;
		LPSTR Wname;
		bool canStop;
		bool canShutdown;
		bool canPauseContinue;

		SERVICE_STATUS_HANDLE statusHandle;
		SERVICE_STATUS status;
		HANDLE stopEvent;
		HANDLE pauseEvent;
		HANDLE continueEvent;
		HANDLE workerThreadHandle;

		static void WINAPI service_main(DWORD argc, LPTSTR *argv);
		static void WINAPI service_control_handler(DWORD control);
		static DWORD WINAPI worker_thread(LPVOID lpParam);

		// Convenience
		void set_stateL(DWORD state);
		void set_stateStopped(DWORD exitCode);
		void set_stateStoppedSpecific(DWORD exitCode);
		void set_stateRunning() {
			set_state(SERVICE_RUNNING);
		}

		void set_statePaused() {
			set_state(SERVICE_PAUSED);
		}

		void on_control_stop();
		void on_control_pause();
		void on_control_continue();
		void on_control_shutdown();

		void on_start_pending();
		void on_start();
		void on_pause();
		void on_continue();
		void on_stop_pending();
		void on_stop();
		void on_stop_error();

	protected:
		static Service *instance;
		virtual DWORD WINAPI worker_thread_function(LPVOID lpParam);
		virtual void service_init();
		virtual void service_cleanUp();

	public:
		Service(std::string name,
			bool in_canStop,
			bool in_canShutdown,
			bool in_canPauseContinue);
		
		int run();
		void set_state(DWORD state);
		void bump(); // increments status.dwCheckPoint, see:https://msdn.microsoft.com/en-us/library/windows/desktop/ms685996%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
};
