#pragma once

#include <windows.h>
#include <qmutex.h>

DWORD WINAPI MainThreadFunction( LPVOID lpParam );

class ImageGrabber;

struct IMFMediaSource;

typedef void(*emergensyStopEventCallback)(int, void *);

/// Class for controlling of thread of the grabbing raw data from video device
class ImageGrabberThread
{
	friend DWORD WINAPI MainThreadFunction( LPVOID lpParam );

public:
	~ImageGrabberThread(void);

	static HRESULT CreateInstance(ImageGrabberThread **ppIGT, IMFMediaSource *pSource, unsigned int deviceID);

	void start();

	void stop();

	void setEmergencyStopEvent(void *userData, void(*func)(int, void *));

	ImageGrabber *getImageGrabber();

	QMutex runMutex;

protected:

	virtual void run();

private:
	
	ImageGrabberThread(IMFMediaSource *pSource, unsigned int deviceID);

	HANDLE igt_Handle;
	
    DWORD   igt_ThreadIdArray;

	ImageGrabber *igt_pImageGrabber;

	emergensyStopEventCallback igt_func;

	void *igt_userData;

	bool igt_stop;

	unsigned int igt_DeviceID;

};

