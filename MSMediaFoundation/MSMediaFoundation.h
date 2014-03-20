/* ********************************************************************
    Plugin "MSMediaFoundation" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2014, Institut f�r Technische Optik (ITO),
    Universit�t Stuttgart, Germany

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

#ifndef MSMEDIAFOUNDATION_H
#define MSMEDIAFOUNDATION_H

#include "common/addInGrabber.h"
#include "opencv2/opencv.hpp"
#include <qsharedpointer.h>
#include "videoInput.h"

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    MSMediaFoundation 

  */
class MSMediaFoundation : public ito::AddInGrabber //, public MSMediaFoundationInterface
{
    Q_OBJECT

    protected:
        //! Destructor
        ~MSMediaFoundation();
        //! Constructor
        MSMediaFoundation();
 //       ito::RetVal checkData(ito::DataObject *externalDataObject = NULL);    /*!< Check if objekt has to be reallocated */
        ito::RetVal retrieveData(ito::DataObject *externalDataObject = NULL); /*!< Wait for acquired picture */


    public:
        friend class MSMediaFoundationInterface;

        int hasConfDialog(void) { return 0; }; //!< indicates that this plugin has got a configuration dialog

    private:

        VideoInput *m_pVI;    /*!< Handle to the VideoInput-Class */
        CamParameters m_camParams;
        QHash<QString, Parameter*> m_camParamsHash;

        int m_deviceID; /*!< Camera ID */
        bool m_isgrabbing; /*!< Check if acquire was called */
        bool m_timeout;

        int m_imgChannels; /*!< number of channels of the camera image due to current parameterization */
        int m_imgCols; /*!< cols of the camera image due to current parameterization */
        int m_imgRows; /*!< rows of the camera image due to current parameterization */
        int m_imgBpp; /*!< number of element size of the camera image due to current parameterization */
        bool m_camStatusChecked;

        int m_colorMode;

        cv::Mat m_pDataMatBuffer;    /*!< OpenCV DataFile to retrieve datas, this image is already filled after acquire command */

        cv::Mat m_alphaChannel; /* simple uint8, 1-channel image with 255 values filled in case of colorMode. This is the alpha plane */

        ito::RetVal checkCameraAbilities(); /*!< Funktion to check and set aviable data types */

        enum tColorMode
        {
            modeAuto,
            modeColor,
            modeRed,
            modeGreen,
            modeBlue,
            modeGray
        };

        ito::RetVal synchronizeParam(const Parameter &parameter, ito::Param &paramDbl, ito::Param &paramAutoInt);
        ito::RetVal updateCamParam(Parameter &parameter, const ito::ParamBase &paramDbl, const ito::ParamBase &paramAutoInt);
        ito::RetVal synchronizeCameraParametersToParams(bool deleteIfNotAvailable = false);

    public slots:
        //!< Get Camera-Parameter
        ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond);
        //!< Set Camera-Parameter
        ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond);
        //!< Initialise board, load dll, allocate buffer
        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        //!< Free buffer, delete board, unload dll
        ito::RetVal close(ItomSharedSemaphore *waitCond);

        //!< Start the camera to enable acquire-commands
        ito::RetVal startDevice(ItomSharedSemaphore *waitCond);
        //!< Stop the camera to disable acquire-commands
        ito::RetVal stopDevice(ItomSharedSemaphore *waitCond);
        //!< Softwaretrigger for the camera
        ito::RetVal acquire(const int trigger, ItomSharedSemaphore *waitCond = NULL);
        //!< Wait for acquired picture, copy the picture to dObj of right type and size
        ito::RetVal getVal(void *vpdObj, ItomSharedSemaphore *waitCond);

        ito::RetVal copyVal(void *vpdObj, ItomSharedSemaphore *waitCond);

        ito::RetVal checkData(ito::DataObject *externalDataObject = NULL);

    private slots:
        void MSMediaFoundation::dockWidgetVisibilityChanged(bool visible);
//        void dockWidgetValueChanged(int type, double value);
};

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    MSMediaFoundationInterface 
  *
  *\brief    Interface-Class for MSMediaFoundation-Class
  *
  *    \sa    AddInDataIO, MSMediaFoundation
  *
  */
class MSMediaFoundationInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
#if QT_VERSION >=  QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "ito.AddInInterfaceBase" )
#endif
    Q_INTERFACES(ito::AddInInterfaceBase)
    PLUGIN_ITOM_API

    protected:

    public:
        MSMediaFoundationInterface();
        ~MSMediaFoundationInterface();
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);

    signals:

    public slots:
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // OpenCVGrabber_H
