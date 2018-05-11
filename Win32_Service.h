#pragma once
#include <string>
#include <Windows.h>

class WindowsService {
private:
	std::string name;
	LPSTR Wname;
	bool canPauseContinue;

	SERVICE_STATUS status;
	SERVICE_STATUS_HANDLE statusHandle;

	HANDLE workerPaused;
	HANDLE workerContinued;

	static void WINAPI service_main(DWORD argc, LPTSTR *argv);
	static void WINAPI control_handler(DWORD);
	static DWORD WINAPI worker_thread(LPVOID lpParam);

	// Set accepted controls
	void set_acceptedControls(bool _in);

	// Internal start/stop functions
	void startup();
	void exit();
	void on_error();

	// Service controller invoked start/stop functions
	void control_stop();
	void control_pause();
	void control_continue();
	void control_stopOnPause();

	void test_startStop();

protected:
	HANDLE stopEvent;
	HANDLE pauseEvent;
	HANDLE continueEvent;

	void set_state(DWORD state);
	static WindowsService *instance;
	virtual DWORD WINAPI worker(LPVOID lpParam) { return ERROR_SUCCESS; }

	void confirm_pause() {SetEvent(workerPaused);}
	void confirm_continue() {SetEvent(workerContinued);}

	// Functions that get called on specific events
	virtual void on_startup() {};
	virtual void on_pause() {};
	virtual void on_continue() {};
	virtual void on_stop() {};
	virtual void on_exit() {};

	// Tell the service controller we're still alive during lenghty pending operations. EG STOP_PENDING. Not for during bump.
	void bump();

public:
	WindowsService(std::string _name, bool _canPauseContinue);
	int run(int argc, TCHAR *argv[]);

};
