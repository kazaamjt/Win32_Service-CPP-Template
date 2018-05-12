#include "Win32_Service.h"

class Service : public WindowsService {
	using WindowsService::WindowsService;

protected:
	virtual DWORD WINAPI worker(LPVOID lpParam) {
		//  Periodically check if the service has been requested to stop
		while (WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {

			// Pause on pauseEvent
			if (WaitForSingleObject(pauseEvent, 0) != WAIT_OBJECT_0) {

				/*
				* Perform main service functionality here
				*/

				//  Simulate work
				Sleep(3000);

			}
			else {
				confirm_pause();
				// Wait for continue to be thrown
				WaitForSingleObject(continueEvent, INFINITE);
				confirm_continue();
			}
		}

		return ERROR_SUCCESS;
	}

	// Gets called during startup.
	virtual void on_startup() {
		/*
		 * Perform tasks necessary to start the service here.
		 */
	}

	// Gets called when pause is thrown and pause/continue is enabled.
	virtual void on_pause() {
		/*
		 * Perform tasks necessary to pause the service here.
		 */
	}

	// Gets called when continue is thrown and pause/continue is enabled.
	virtual void on_continue() {
		/*
		 * Perform tasks necessary to continue the service here.
		 */
	}

	// Gets called when the windows service controller tells service to stop, but BEFORE stopEvent is thrown.
	virtual void on_stop() {
		/*
		 * Perform tasks necessary to stop the service loop here.
		 */
	}

	// Gets called all the way at the end of the liecycle.
	virtual void on_exit() {
		/*
		 * Perform tasks necessary for cleanup.
		 */
	}

	// What to do when the service fails to regiser.
	// By default it runs test_startStop();
	virtual void on_failedRegistration() {
		/*
		 * what to do when registration fails...
		 */
		 test_startStop();
	}
};

int main(int argc, TCHAR *argv[]) {
	Service test("test", true);
	return test.run();
}
