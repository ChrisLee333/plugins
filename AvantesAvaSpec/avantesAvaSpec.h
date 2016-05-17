/* ********************************************************************
    Plugin "AvantesAvaSpec" for itom software
    URL: http://www.bitbucket.org/itom/plugins
    Copyright (C) 2016, Institut fuer Technische Optik, Universitaet Stuttgart

    This file is part of a plugin for the measurement software itom.
  
    This itom-plugin is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.

    itom and its plugins are distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
    General Public Licence for more details.

    You should have received a copy of the GNU Library General Public License
    along with itom. If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */

#ifndef AVANTESAVASPEC_H
#define AVANTESAVASPEC_H

#include "common/addInGrabber.h"
#include "dialogAvantesAvaSpec.h"

#include <qsharedpointer.h>

#include "avantesDefines.h"


//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    AvantesAvaSpec 
  */
class AvantesAvaSpec : public ito::AddInGrabber
{
    Q_OBJECT

    protected:
        //! Destructor
        ~AvantesAvaSpec();
        //! Constructor
        AvantesAvaSpec();

    public:
        friend class AvantesAvaSpecInterface;
        const ito::RetVal showConfDialog(void);    /*!< Open the config nonmodal dialog to set camera parameters */

    protected:
        ito::RetVal retrieveData(ito::DataObject *externalDataObject = NULL);    /*! <Wait for acquired picture */
        ito::RetVal checkData(ito::DataObject *externalDataObject = NULL);
        //        ito::RetVal checkData(void);    /*!< Check if objekt has to be reallocated */

    private:
        ito::RetVal sendCommand(const char* cmd, int cmd_size, unsigned char* buf, int &buf_size);
        ito::RetVal readWithFixedLength(char* buf, int &buf_size);
        ito::RetVal checkAnswerForError(const unsigned char* buf, const unsigned char &desiredCmd, bool warningNotError = false, const char *prefix = "");
		void dummyRead();

        ito::AddInDataIO *m_pUsb;
        bool m_isGrabbing;
        sony_single_measdatatype sony_single_meas;
        sony_multi_measdatatype sony_multi_meas;
        DeviceConfigType m_deviceConfig;
        ito::RetVal m_acquisitionRetVal;

        static void idleCharDeleter(char* /*v*/) {};
        static void idleIntDeleter(int* /*v*/) {};

    signals:
        void parametersChanged(QMap<QString, ito::Param> params);    /*! Signal send changed or all parameters to listeners */

    public slots:
        
        //! returns parameter of m_params with key name.
        ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond = NULL);
        //! sets parameter of m_params with key name. 
        ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond = NULL);
        
        //! Initialise board, load dll, allocate buffer
        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        //! Free buffer, delete board, unload dll
        ito::RetVal close(ItomSharedSemaphore *waitCond);

        //! Start the camera to enable acquire-commands
        ito::RetVal startDevice(ItomSharedSemaphore *waitCond);
        //! Stop the camera to disable acquire-commands
        ito::RetVal stopDevice(ItomSharedSemaphore *waitCond);
        //! Softwaretrigger for the camera
        ito::RetVal acquire(const int trigger, ItomSharedSemaphore *waitCond = NULL);
        //! Calls retrieveData(NULL), than copy the picture to dObj of right type and size
        ito::RetVal getVal(void *dObj, ItomSharedSemaphore *waitCond);
        //! Deep copy the camera buffer to dObj. Object must be of right size and type. If liveData is running, a second deep-copy is performed to copy data to the grabber 
        ito::RetVal copyVal(void *dObj, ItomSharedSemaphore *waitCond);
        
        //! Retrieve new offset and new gain and give them to the camera dll
        void updateParameters(QMap<QString, ito::ParamBase> params);

    private slots:
        void dockWidgetVisibilityChanged(bool visible);
};

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    AS5216Interface 
  *
  *\brief    Interface-Class for AS5216-Class
  *
  *    \sa    AddInDataIO, AS5216
  *    \date    11.10.2010
  *    \author    Wolfram Lyda
  * \warning    NA
  *
  */
class AvantesAvaSpecInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
#if QT_VERSION >=  QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "ito.AddInInterfaceBase" )
#endif
    Q_INTERFACES(ito::AddInInterfaceBase)
    PLUGIN_ITOM_API

    protected:

    public:
        AvantesAvaSpecInterface();
        ~AvantesAvaSpecInterface();
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);
        //! auto-increment, static instance counter for all dummy-1394 instances
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // AVANTESAVASPEC_H
