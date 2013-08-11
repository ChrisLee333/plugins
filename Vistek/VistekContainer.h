#ifndef VISTEKCONTAINER_H
#define VISTEKCONTAINER_H

#include "common/addInGrabber.h"
#include "dialogVistek.h"
#include "SVGigE.h"

#include <qsharedpointer.h>
#include <QTimerEvent>
#include <qmutex.h>

struct VistekCam
{
	public:
		QString camModel;
		QString camSerialNo;
		QString camVersion;
		QString camIP;
		QString camManufacturer;
		int sensorPixelsX;
		int sensorPixelsY;
		bool started;
};

class VistekContainer : QObject
{

	Q_OBJECT

	public:
		static VistekContainer* getInstance();
		ito::RetVal initCameraContainer();

		VistekCam getCamInfo(int CamNo);
		const int getNextFreeCam();
		const int getCameraBySN(const QString &camSerialNo);
		void freeCameraStatus(const int camnumber);
		CameraContainerClient_handle getCameraContainerHandle();

	protected:
		CameraContainerClient_handle m_camClient;
		QMutex m_mutex;
		bool m_initialized;
		QVector<VistekCam> m_cameras;


	private:
		VistekContainer(void);
		VistekContainer(VistekContainer  &/*copyConstr*/) {}
		~VistekContainer(void);

		static VistekContainer *m_pVistekContainer;

		//!< singleton nach: http://www.oop-trainer.de/Themen/Singleton.html
		class VistekContainerSingleton
		{
			public:
				~VistekContainerSingleton()
				{
					#pragma omp critical
					{
						if( VistekContainer::m_pVistekContainer != NULL)
						{
							delete VistekContainer::m_pVistekContainer;
							VistekContainer::m_pVistekContainer = NULL;
						}
					}
				}
		};
		friend class VistekContainerSingleton;
};

#endif // VISTEKCONTAINER_H