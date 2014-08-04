/* ********************************************************************
    Plugin "PGRFlyCapture" for itom software
    URL: http://www.twip-os.com
    Copyright (C) 2014, twip optical solutions GmbH
	Copyright (C) 2014, Institut f�r Technische Optik, Universit�t Stuttgart

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

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

#include "PGRFlyCapture.h"
#include "pluginVersion.h"
#define _USE_MATH_DEFINES  // needs to be defined to enable standard declartions of PI constant
#include "math.h"
#include <bitset>

#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>
#include <qmetaobject.h>

#include <qdockwidget.h>
#include <qmetaobject.h>
#include "dockWidgetPGRFlyCapture.h"

#include "common/helperCommon.h"

static signed char InitList[MAXPGR + 1];
static char Initnum = 0;

#define EVALSPEED 0

Q_DECLARE_METATYPE(ito::DataObject)


//----------------------------------------------------------------------------------------------------------------------------------

/*!
    \class PGRFlyCaptureInterface
    \brief Small interface class for class PGRFlyCapture. This class contains basic information about PGRFlyCapture as is able to
        create one or more new instances of PGRFlyCapture.
*/

//----------------------------------------------------------------------------------------------------------------------------------
//! creates new instance of PGRFlyCapture and returns the instance-pointer.
/*!
    \param [in,out] addInInst is a double pointer of type ito::AddInBase. The newly created PGRFlyCapture-instance is stored in *addInInst
    \return retOk
    \sa PGRFlyCapture
*/
ito::RetVal PGRFlyCaptureInterface::getAddInInst(ito::AddInBase **addInInst)
{
    NEW_PLUGININSTANCE(PGRFlyCapture)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! deletes instance of PGRFlyCapture. This instance is given by parameter addInInst.
/*!
    \param [in] double pointer to the instance which should be deleted.
    \return retOk
    \sa PGRFlyCapture
*/
ito::RetVal PGRFlyCaptureInterface::closeThisInst(ito::AddInBase **addInInst)
{
   REMOVE_PLUGININSTANCE(PGRFlyCapture)
   return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for interace
/*!
    defines the plugin type (dataIO and grabber) and sets the plugins object name. If the real plugin (here: PGRFlyCapture) should or must
    be initialized (e.g. by a Python call) with mandatory or optional parameters, please initialize both vectors m_initParamsMand
    and m_initParamsOpt within this constructor.
*/
PGRFlyCaptureInterface::PGRFlyCaptureInterface()
{
    m_autoLoadPolicy = ito::autoLoadKeywordDefined;
    m_autoSavePolicy = ito::autoSaveAlways;

    m_type = ito::typeDataIO | ito::typeGrabber;
    setObjectName("PGRFlyCapture");

    FlyCapture2::FC2Version fc2Version;
    FlyCapture2::Utilities::GetLibraryVersion( &fc2Version );

        char docstring[] = \
"This plugin supports Point Grey cameras (currently USB models only) that can be run by the FlyCapture2 interface from Point Grey Research. \n\
The development of this plugin has mainly be done using the USB 3.0 Flea3 camera. \n\
\n\
The plugin has been compiled using the FlyCapture2 library version %1.%2.%3.%4. \n\
\n\
In order to compile the plugin by yourself, you need to install the FlyCapture2 SDK in 32bit or 64bit (depending on itom) and make \n\
sure that the development files (include files and libraries) are installed as well (option Cross Platform Dev Files). Then set the CMake variable PGRFLYCAP_INCLUDE_DIR \n\
to the include directory of the FlyCapture2 SDK. The depending variable PGRFYLCAP_API_DIR is then automatically set (if not, delete it and \n\
re-configure CMake). \n\
\n\
This plugin automatically copies the necessary FlyCapture2 DLLs to the lib-folder of itom.";
    m_detaildescription = QObject::tr(docstring).arg(fc2Version.major).arg(fc2Version.minor).arg(fc2Version.type).arg(fc2Version.build);

    m_description = QObject::tr("Point Grey FlyCapture2 Cameras");

    m_author = "W. Lyda, M. Gronle, ITO, University Stuttgart";
    m_version = (PLUGIN_VERSION_MAJOR << 16) + (PLUGIN_VERSION_MINOR << 8) + PLUGIN_VERSION_PATCH;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = QObject::tr("GPL / this plugin needs to link agains the FlyCapture2 SDK from Point Grey Research, that comes with its own license. The FlyCapture2 SDK contains components that are licensed under GPL.");
    m_aboutThis = QObject::tr("N.A.");    
    
    m_initParamsMand.clear();

    ito::Param param("cameraNumber", ito::ParamBase::Int, -1, 10, -1, QObject::tr("Continuous camera number [0,10], default: -1 uses the next free camera.").toAscii().data());
    m_initParamsOpt.append(param);
    param = ito::Param("bppLimit", ito::ParamBase::Int, -1, 16, -1, QObject::tr("Limits the bitdepth to the given level [8, 12, 16]. As default the maximum level is used (-1)").toAscii().data());
    m_initParamsOpt.append(param);
    param = ito::Param("forceSync", ito::ParamBase::Int, 0, 1, 0, QObject::tr("Direct enable software sync if present").toAscii().data());
    m_initParamsOpt.append(param);

    memset(InitList, -1, (MAXPGR + 1) * sizeof(signed char));

    return;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    clears both vectors m_initParamsMand and m_initParamsOpt.
*/
PGRFlyCaptureInterface::~PGRFlyCaptureInterface()
{
    m_initParamsMand.clear();
    m_initParamsOpt.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
// this makro registers the class PGRFlyCaptureInterface with the name PGRFlyCaptureinterface as plugin for the Qt-System (see Qt-DOC)
Q_EXPORT_PLUGIN2(PGRFlyCaptureinterface, PGRFlyCaptureInterface)

//----------------------------------------------------------------------------------------------------------------------------------

/*!
    \class PGRFlyCapture
    \brief Class for the PGRFlyCapture. The PGRFlyCapture is a dataIO-Plugin for the FlyCapture2 Camera-Interface by Point Grey Research. It can be used e.g. with the USB 3.0 Flea3 camera device.

    Usually every method in this class can be executed in an own thread. Only the constructor, destructor, showConfDialog will be executed by the
    main (GUI) thread.
*/

//----------------------------------------------------------------------------------------------------------------------------------
//! shows the configuration dialog. This method must be executed in the main (GUI) thread and is usually called by the addIn-Manager.
/*!
    creates new instance of dialogPGRFlyCapture, calls the method setVals of dialogPGRFlyCapture, starts the execution loop and if the dialog
    is closed, reads the new parameter set and deletes the dialog.

    \return retOk
    \sa dialogPGRFlyCapture
*/
const ito::RetVal PGRFlyCapture::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogPGRFlyCapture(this));
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for PGRFlyCapture
/*!
    In this constructor the m_params-vector with all parameters, which are accessible by getParam or setParam, is built.
    Additionally the optional docking widget for the PGRFlyCapture's toolbar is instantiated and created by createDockWidget.

    \param [in] uniqueID is an unique identifier for this PGRFlyCapture-instance
    \sa ito::tParam, createDockWidget, setParam, getParam
*/
PGRFlyCapture::PGRFlyCapture() :
    AddInGrabber(),
    m_isgrabbing(false),
    m_camIdx(-1),
    m_colorCam(false),
    m_RunSync(true),
    m_RunSoftwareSync(false),
    m_hasFormat7(false),
    m_gainMax(1.0),
    m_gainMin(0.0),
    m_offsetMax(1.0),
    m_offsetMin(0.0),
    m_extendedShutter(UNINITIALIZED)
{
    //qRegisterMetaType<QMap<QString, ito::Param> >("QMap<QString, ito::Param>");
    //qRegisterMetaType<ito::DataObject>("ito::DataObject");

    ito::Param paramVal("name", ito::ParamBase::String | ito::ParamBase::Readonly, "PGRFlyCapture", "GrabberName");
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("integration_time", ito::ParamBase::Double, 0.000006, 1/1.875, 1/1.875, tr("Integrationtime of CCD programmed in seconds.").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("frame_time", ito::ParamBase::Double, 1/240.0, 1/1.875, 1/1.875, tr("Frame rate in seconds. This is only considered if the camera is not in an extended shutter mode. The frame_time might influence the integration time.").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("extended_shutter", ito::ParamBase::Int, 0, 1, 1, tr("1 (default): extended shutter is on (long integration times are supported and frame_time becomes invalid), 0: frames are only acquired in the pulse given by frame_time.").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("gain", ito::ParamBase::Double, 0.0, 1.0, 0.5, tr("gain (normalized value 0..1)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("offset", ito::ParamBase::Double, 0.0, 1.0, 0.05, tr("offset (normalized value 0..1, mapped to PG-parameter BRIGHTNESS)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("exposureEV", ito::ParamBase::Int, 0, 1023, 480, tr("Camera brightness control (EV)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sharpness", ito::ParamBase::Int, 0, 4095, 0, tr("Sharpness").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("gamma", ito::ParamBase::Int, 500, 4095, 1024, tr("Gamma adjustment").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //binning is GigE only

    paramVal = ito::Param("sizex", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in x (cols)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizey", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in y (rows)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("x0", ito::ParamBase::Int, 0, 2047, 0, tr("Pixelsize in x (cols)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y0", ito::ParamBase::Int, 0, 2047, 0, tr("Pixelsize in y (rows)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("x1", ito::ParamBase::Int, 0, 2047, 2047, tr("Pixelsize in x (cols)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y1", ito::ParamBase::Int, 0, 2047, 2047, tr("Pixelsize in y (rows)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("bpp", ito::ParamBase::Int, 8, 16, 16, tr("bitdepth of each pixel").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("timeout", ito::ParamBase::Double, 0.001, 60.0, 2.0, tr("Timeout for acquiring images in seconds").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("trigger_mode", ito::ParamBase::Int, -1, 2, 0, tr("-1: Complete free run, 0: Disable trigger, 1: enable trigger mode, 2: enable software-trigger").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("videoMode", ito::ParamBase::Int | ito::ParamBase::Readonly, 0, FlyCapture2::NUM_VIDEOMODES - 1, FlyCapture2::VIDEOMODE_FORMAT7, tr("Current video mode, default is Mode7").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //paramVal = ito::Param("supported_frame_time", ito::ParamBase::IntArray | ito::ParamBase::Readonly, NULL, tr("Possible valued for the frame_time in frames per second").toAscii().data());
    //m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camSerialNumber", ito::ParamBase::Int | ito::ParamBase::Readonly, 0, tr("Serial number of the attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camModel", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Model identifier of the attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camVendor", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Name of the attachted camera vendor").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camSensor", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Identifier of the chip in attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camResolution", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Resolution of the chip in attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camFirmwareVersion", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Serial number of the firmware used in the attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camFirmwareBuildTime", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Built time of the firmware used in the attachted camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camInterface", ito::ParamBase::String | ito::ParamBase::Readonly, "n.a.", tr("Interface of camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

	paramVal = ito::Param("timestamp", ito::ParamBase::Double | ito::ParamBase::Readonly, 0.0, 10000000.0, 0.0, tr("Time in ms since last image (end of exposure)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //now create dock widget for this plugin
    DockWidgetPGRFlyCapture *dw = new DockWidgetPGRFlyCapture(this);

    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dw);

    checkData();
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    \sa ~AddInBase
*/
PGRFlyCapture::~PGRFlyCapture()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
//! returns parameter of m_params with key name.
/*!
    This method copies the string of the corresponding parameter to val with a maximum length of maxLen.

    \param [in] name is the key name of the parameter
    \param [in,out] val is a shared-pointer of type char*.
    \param [in] maxLen is the maximum length which is allowed for copying to val
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal PGRFlyCapture::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString,ito::Param>::iterator it;

    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if(retValue == ito::retOk)
    {
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if(!retValue.containsError())
    {
        *val = it.value();
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! sets parameter of m_params with key name.
/*!
    This method copies the given value  to the m_params-parameter.

    \param [in] name is the key name of the parameter
    \param [in] val is the double value to set.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal PGRFlyCapture::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;
    int running = 0;

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName( val->getName(), key, hasIndex, index, suffix );

    if(!retValue.containsError())
    {
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if(!retValue.containsError())
    {
        retValue += apiValidateParam(*it, *val, false, true);
    }

    if(!retValue.containsError())
    {
        // Adapted parameters and send out depending parameter
        if (key == "x0")
        {
            retValue += flyCapChangeFormat7(false, true, -1, val->getVal<int>(), -1, -1, -1);
        }
        else if (key == "x1")
        {
            retValue += flyCapChangeFormat7(false, true, -1, -1, -1, val->getVal<int>(), -1);
        }
        else if (key == "y0")
        {
            retValue += flyCapChangeFormat7(false, true, -1, -1, val->getVal<int>(), -1, -1);
        }
        else if (key == "y1")
        {
            retValue += flyCapChangeFormat7(false, true, -1, -1, -1, -1, val->getVal<int>());
        }
        else if (key == "extended_shutter")
        {
            retValue += flyCapSetExtendedShutter(val->getVal<int>() > 0);
            retValue += flyCapSynchronizeFrameRateShutter();
        }
        else if(key == "frame_time")
        {
            FlyCapture2::Property prop;
            prop.type = FlyCapture2::FRAME_RATE;
            retValue += checkError(m_myCam.GetProperty( &prop ));
            
            if (retValue != ito::retError && prop.onOff == true) //if off, extended shutter is on
            {
                prop.absControl = true; //floating point
                prop.autoManualMode = false; //manual
                prop.onOff = true;
                prop.absValue = 1.0 / val->getVal<double>();
                retValue += checkError(m_myCam.SetProperty( &prop ));
            }

            retValue += flyCapSynchronizeFrameRateShutter();
        }
        else if (key == "integration_time")
        {
            FlyCapture2::Property prop;
            prop.type = FlyCapture2::SHUTTER;
            retValue += checkError(m_myCam.GetProperty( &prop ));
            
            if (retValue != ito::retError)
            {
                prop.absControl = true; //floating point
                prop.autoManualMode = false; //manual
                prop.onOff = true;
                prop.absValue = val->getVal<double>() * 1000.0;
                retValue += checkError(m_myCam.SetProperty( &prop ));
            }

            retValue += flyCapSynchronizeFrameRateShutter();            
        }
        else if (key == "gain")
        {
            unsigned int value = (uint)(val->getVal<double>() * (m_gainMax - m_gainMin) + m_gainMin + 0.5);
            retValue += flyCapSetAndGetParameter("gain", value, FlyCapture2::GAIN, false, true);
            
            if (!retValue.containsError())
            {
                double gain = (value - m_gainMin)/(m_gainMax - m_gainMin);
                m_params["gain"].setVal<double>(gain);
            }
        }
        else if (key == "offset")
        {
            unsigned int value = (uint)(val->getVal<double>() * (m_offsetMax - m_offsetMin) + m_offsetMin + 0.5);
            retValue += flyCapSetAndGetParameter("offset", value, FlyCapture2::BRIGHTNESS, false, true);

            if (!retValue.containsError())
            {
                double offset = (value - m_offsetMin)/(m_offsetMax - m_offsetMin);
                m_params["offset"].setVal<double>(offset);
            }
        }
        else if (key == "gamma")
        {
            unsigned int value = (uint)(val->getVal<int>());
            retValue += flyCapSetAndGetParameter("gamma", value, FlyCapture2::GAMMA, false, true);

            if (!retValue.containsError())
            {
                m_params["gamma"].setVal<int>(value);
            }
        }
        else if (key == "sharpness")
        {
            unsigned int value = (uint)(val->getVal<int>());
            retValue += flyCapSetAndGetParameter("sharpness", value, FlyCapture2::SHARPNESS, false, true);

            if (!retValue.containsError())
            {
                m_params["sharpness"].setVal<int>(value);
            }

        }
        else if (key =="exposureEV")
        {
            unsigned int value = (uint)(val->getVal<int>());
            retValue += flyCapSetAndGetParameter("exposureEV", value, FlyCapture2::AUTO_EXPOSURE, false, true);

            if (!retValue.containsError())
            {
                m_params["exposureEV"].setVal<int>(value);
            }
        }
        else if (key == "trigger_mode")
        {

            if (grabberStartedCount() > 0)
            {
                running = grabberStartedCount();
                setGrabberStarted(1);
                retValue += stopDevice(NULL);
            }

            // The trigger must be present --> because the m_param is not readonly
            FlyCapture2::TriggerMode triggerModeSetup;
            m_myCam.GetTriggerMode(&triggerModeSetup);

            int triggerMode = val->getVal<int>();
                    
            switch(triggerMode)
            {
                case 0: //Basic Mode
                    m_RunSync = true;   // Force a synchronisation by delay
                    m_RunSoftwareSync = false;  // Force synchronisation by software trigger during aquire
                    triggerModeSetup.onOff = false;
                    break;
                case 1: //Hardware Trigger
                    m_RunSync = false;
                    m_RunSoftwareSync = false;
                    triggerModeSetup.onOff = true; 
                    break;
                case 2: //Software Trigger
                    m_RunSync = false;
                    m_RunSoftwareSync = true;
                    triggerModeSetup.onOff = true;
                    break;
                case -1: //Free run
                    m_RunSync = false;
                    m_RunSoftwareSync = false;
                    triggerModeSetup.onOff = false;
                    break;
            }

            FlyCapture2::Error retError = m_myCam.SetTriggerMode(&triggerModeSetup);
            if (retError != FlyCapture2::PGRERROR_OK)
            {
                retValue += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in setParam-function: %s", retError.GetDescription());
            }
            else
            {
                m_params["trigger"].setVal<int>(triggerMode);
            }
        }
        else if (key == "timeout")
        {
            FlyCapture2::FC2Config pCamConfig;
            retValue += checkError(m_myCam.GetConfiguration(&pCamConfig));

            if (retValue != ito::retError)
            {
                pCamConfig.grabTimeout = val->getVal<double>() * 1000;
                retValue += checkError(m_myCam.SetConfiguration(&pCamConfig));

                if (retValue != ito::retError)
                {
                    retValue += it->copyValueFrom( &(*val) );
                }
            }
        }
        else if (key == "bpp")
        {
            retValue += flyCapChangeFormat7(true, false, val->getVal<int>());
        }
        else
        {
            //all parameters that don't need further checks can simply be assigned
            //to the value in m_params (the rest is already checked above)
            retValue += it->copyValueFrom( &(*val) );
        }
    }

    if(!retValue.containsError())
    {
        //restart camera if it has been started as was stopped by this method
        if (running)
        {
            retValue += startDevice(NULL);
            setGrabberStarted(running);
        }

        emit parametersChanged(m_params); //send changed parameters to any connected dialogs or dock-widgets
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapSetAndGetParameter(const QString &name, unsigned int &value, FlyCapture2::PropertyType type, bool autoManualMode /*= false*/, bool onOff /*= true*/)
{
    FlyCapture2::Property prop(type);
    prop.absControl = false; //integer based
    prop.autoManualMode = autoManualMode;
    prop.onOff = onOff;
    prop.valueA = value;
    FlyCapture2::Error retErr = m_myCam.SetProperty( &prop );

    if (retErr != FlyCapture2::PGRERROR_OK)
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error setting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    retErr = m_myCam.GetProperty( &prop );
    if (retErr == FlyCapture2::PGRERROR_OK)
    {
        value = prop.valueA;
    }
    else
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error getting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapSetAndGetParameter(const QString &name, float &value, FlyCapture2::PropertyType type, bool autoManualMode /*= false*/, bool onOff /*= true*/)
{
    FlyCapture2::Property prop(type);
    prop.absControl = true; //float based
    prop.autoManualMode = autoManualMode;
    prop.onOff = onOff;
    prop.absValue = value;
    FlyCapture2::Error retErr = m_myCam.SetProperty( &prop );

    if (retErr != FlyCapture2::PGRERROR_OK)
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error setting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    retErr = m_myCam.GetProperty( &prop );
    if (retErr == FlyCapture2::PGRERROR_OK)
    {
        value = prop.absValue;
    }
    else
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error getting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapGetParameter(const QString &name, unsigned int &value, FlyCapture2::PropertyType type)
{
    FlyCapture2::Property prop(type);

    FlyCapture2::Error retErr = m_myCam.GetProperty( &prop );
    if (retErr == FlyCapture2::PGRERROR_OK)
    {
        value = prop.valueA;
    }
    else
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error getting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapGetParameter(const QString &name, float &value, FlyCapture2::PropertyType type)
{
    FlyCapture2::Property prop(type);

    FlyCapture2::Error retErr = m_myCam.GetProperty( &prop );
    if (retErr == FlyCapture2::PGRERROR_OK)
    {
        value = prop.absValue;
    }
    else
    {
        return ito::RetVal::format(ito::retError, (int)retErr.GetType(), "Error getting parameter %s: %s", name.toLatin1().data(), retErr.GetDescription());
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! init method which is called by the addInManager after the initiation of a new instance of PGRFlyCapture.
/*!
    This init method gets the mandatory and optional parameter vectors of type tParam and must copy these given parameters to the
    internal m_params-vector. Notice that this method is called after that this instance has been moved to its own (non-gui) thread.

    \param [in] paramsMand is a pointer to the vector of mandatory tParams.
    \param [in] paramsOpt is a pointer to the vector of optional tParams.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
*/
ito::RetVal PGRFlyCapture::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);

    FlyCapture2::Error retError;
    FlyCapture2::BusManager busMgr;
    FlyCapture2::CameraInfo camInfo;
    FlyCapture2::PixelFormat pixelFormat;
    FlyCapture2::FrameRate frameRate;
    FlyCapture2::VideoMode videoMode;
    FlyCapture2::FC2Config pCamConfig; 
    int maxBpp = 0;
    int minBpp = 1000;
    unsigned int numberOfCam = 0;
    ito::RetVal retVal;

    m_camIdx = (*paramsOpt)[0].getVal<int>();    // the first parameter in optional list is for the camera index
    int limitBPP = (*paramsOpt)[1].getVal<int>();    // the second parameter in optional list is for the max BPP
    bool startSyncronized = (*paramsOpt)[2].getVal<int>();    // the third parameter in optional list handels synchronisation

    Initnum++;
        
    retVal += checkError(busMgr.GetNumOfCameras(&numberOfCam));
    
    if(m_camIdx < 0)
    {
        bool found;
        signed char curCamIdx;
        
        //take the first free camera
        for(curCamIdx = 0; curCamIdx < (signed char)numberOfCam; curCamIdx++)
        {
            //check whether this camera is not used yet
            found = false;
            for(int i = 0; i < MAXPGR; i++)
            {
                if(InitList[i] == curCamIdx)
                {
                    found = true;
                    break;
                }
            }

            if (!found) //curCamIdx has not been used yet
            {
                m_camIdx = curCamIdx;
                break;
            }

        }
        if ( m_camIdx < 0)
        {
            retVal += ito::RetVal(ito::retError, 0, "No free camera found.");
        }
    }
    else
    {
        if(m_camIdx < numberOfCam)
        {
            for(int i = 0; i < MAXPGR; i++)
            {
                if(InitList[i] == m_camIdx)
                {
                    m_camIdx = -1;
                    retVal += ito::RetVal(ito::retError, 0, "Camera already initialized");
                }
            }
        }
        else
        {
            m_camIdx = -1;
            retVal += ito::RetVal(ito::retError, 0, "Camera index out of range");
        }
    }

    if(!retVal.containsError())
    {   
        retVal += checkError(busMgr.GetCameraFromIndex(m_camIdx, &m_myGUID));
        if (retVal.containsError())
        {
            m_camIdx = -1;       
            retVal += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in init-function: %s", retError.GetDescription());
        }
        else
        {
            for(int i = 0; i < MAXPGR; i++)
            {
                if(InitList[i] == -1)
                {
                    InitList[i] = m_camIdx;
                    break;
                }
            }  

            FlyCapture2::InterfaceType m_interfaceType;
            busMgr.GetInterfaceTypeFromGuid(&m_myGUID, &m_interfaceType);
            
            switch (m_interfaceType)
            {
                case FlyCapture2::INTERFACE_USB2:
                    m_params["camInterface"].setVal<char*>("USB2");
                    break;
                case FlyCapture2::INTERFACE_USB3:
                    m_params["camInterface"].setVal<char*>("USB3");
                    break;
                default:
                    retVal += ito::RetVal(ito::retError, 0, "unsupported interface (GigE, IEEE1394...)");
                    break;
            }
        }
    }

    if(!retVal.containsError())
    {
        retVal += checkError(m_myCam.Connect(&m_myGUID));
    }

    if(!retVal.containsError())
    {
        retVal += checkError(m_myCam.GetCameraInfo(&camInfo));
        if (!retVal.containsError())
        {
            m_colorCam = camInfo.isColorCamera;
        }
    }

    m_format7Info.mode = FlyCapture2::MODE_0;
    
    if(!retVal.containsError())
    {
        retVal += checkError(m_myCam.GetFormat7Info(&m_format7Info, &m_hasFormat7));
        retVal += checkError(m_myCam.GetVideoModeAndFrameRate(&videoMode, &frameRate));
    }
            
    if(!retVal.containsError())
    {
        if(m_hasFormat7)    // camera has mode mode 7
        {
            if(videoMode != FlyCapture2::VIDEOMODE_FORMAT7)
            {
                retError = m_myCam.SetVideoModeAndFrameRate(FlyCapture2::VIDEOMODE_FORMAT7, FlyCapture2::FRAMERATE_FORMAT7);
                if (retError != FlyCapture2::PGRERROR_OK)
                {
                    retVal += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in init-function: %s", retError.GetDescription());
                }
            }

            if(!retVal.containsError())
            {
                //get defaults from info struct
                ito::IntMeta *im;
                im = static_cast<ito::IntMeta*>( m_params["sizex"].getMeta() );
                im->setMax(m_format7Info.maxWidth);
                im->setStepSize(m_format7Info.imageHStepSize);
                im = static_cast<ito::IntMeta*>( m_params["x0"].getMeta() );
                im->setStepSize(m_format7Info.offsetHStepSize);
                im = static_cast<ito::IntMeta*>( m_params["x1"].getMeta() );
                im->setMax(m_format7Info.maxWidth - 1);
                im->setStepSize(m_format7Info.offsetHStepSize);

                im = static_cast<ito::IntMeta*>( m_params["sizey"].getMeta() );
                im->setMax(m_format7Info.maxHeight);
                im->setStepSize(m_format7Info.imageVStepSize);
                im = static_cast<ito::IntMeta*>( m_params["y0"].getMeta() );
                im->setStepSize(m_format7Info.offsetVStepSize);
                im = static_cast<ito::IntMeta*>( m_params["y1"].getMeta() );
                im->setMax(m_format7Info.maxHeight - 1);
                im->setStepSize(m_format7Info.offsetVStepSize);

                //set bpp
                int desiredBpp;
                m_params["videoMode"].setVal<int>(FlyCapture2::VIDEOMODE_FORMAT7);
                m_currentFormat7Settings.mode = FlyCapture2::MODE_0;

                if((FlyCapture2::PIXEL_FORMAT_MONO8 | FlyCapture2::PIXEL_FORMAT_RAW8) & m_format7Info.pixelFormatBitField)
                {
                    minBpp = std::min(minBpp, 8);
                    maxBpp = std::max(maxBpp, 8);
                }

                if((FlyCapture2::PIXEL_FORMAT_MONO12 | FlyCapture2::PIXEL_FORMAT_RAW12) & m_format7Info.pixelFormatBitField)
                {
                    minBpp = std::min(minBpp, 12);
                    maxBpp = std::max(maxBpp, 12);
                }

                if((FlyCapture2::PIXEL_FORMAT_MONO16 | FlyCapture2::PIXEL_FORMAT_RAW16) & m_format7Info.pixelFormatBitField)
                {
                    minBpp = std::min(minBpp, 16);
                    maxBpp = std::max(maxBpp, 16);
                }
                    
                if(limitBPP <= 8 || maxBpp <= 8)
                {
                    desiredBpp = 8;
                }
                else if (limitBPP <= 12)
                {
                    desiredBpp = 12;
                }
                else if (limitBPP <= 16 || maxBpp <= 16)
                {
                    desiredBpp = 16;
                }

                flyCapChangeFormat7(true, false, desiredBpp);
                pixelFormat = m_currentFormat7Settings.pixelFormat;
            }
        }
        else
        {
            int width;
            int height;
            bool isStippled = false;
            
            FlyCapture2::PropertyInfo propInfo;
            propInfo.type = FlyCapture2::WHITE_BALANCE;

            m_myCam.GetPropertyInfo(&propInfo);

            if (propInfo.present)
            {
                isStippled = true;
            }

            if (!GetResolutionFromVideoMode(videoMode, width, height))
            {
                retVal += ito::RetVal(ito::retError, 0, "Get resolution failed");
            }
            else
            {
                m_params["videoMode"].setVal<int>(videoMode);

                m_params["sizex"].setVal<int>(width);
                static_cast<ito::IntMeta*>( m_params["sizex"].getMeta() )->setMax(height);
                static_cast<ito::IntMeta*>( m_params["x0"].getMeta() )->setMax(height-1);
                static_cast<ito::IntMeta*>( m_params["x1"].getMeta() )->setMax(height-1);
                m_params["x1"].setVal<int>(width-1);

                m_params["sizey"].setVal<int>(height);
                static_cast<ito::IntMeta*>( m_params["sizey"].getMeta() )->setMax(height);
                static_cast<ito::IntMeta*>( m_params["y0"].getMeta() )->setMax(height-1);
                static_cast<ito::IntMeta*>( m_params["y1"].getMeta() )->setMax(height-1);
                m_params["y1"].setVal<int>(height-1);          
            }

            if (!GetPixelFormatFromVideoMode(videoMode, isStippled, &pixelFormat))
            {
                pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
            }
        }
    }

    if(!retVal.containsError())
    {
        retVal += flyCapSetExtendedShutter( m_params["extended_shutter"].getVal<int>() > 0 );
    }

    if(!retVal.containsError())
    {
        FlyCapture2:: PropertyInfo propInfo;
        propInfo.type = FlyCapture2::SHUTTER;

        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.absValSupported && propInfo.manualSupported)
            {
                FlyCapture2::Property prop;
                prop.type = FlyCapture2::SHUTTER;
                prop.absControl = true;
                prop.autoManualMode = false;
                prop.onOff = true;
                retVal += checkError(m_myCam.SetProperty( &prop )); 
            }
            
            retVal += flyCapSynchronizeFrameRateShutter();
        }
    }

    if(!retVal.containsError())
    {
        FlyCapture2::PropertyInfo propInfo;

        propInfo.type = FlyCapture2::GAIN;
        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.manualSupported)
            {
                m_gainMax = propInfo.max;
                m_gainMin = propInfo.min;   

                unsigned int valA;
                retVal += flyCapGetParameter("gain", valA, FlyCapture2::GAIN); 

                if (!retVal.containsError())
                {
                    double gain = (valA - m_gainMin) / (m_gainMax-m_gainMin) ;
                    m_params["gain"].setVal<double>(gain);

                    retVal += flyCapSetAndGetParameter("gain", valA, FlyCapture2::GAIN, false, true);
                }
            }
            else
            {
                m_params["gain"].setFlags(ito::ParamBase::Readonly);
            }
        }
    }

    if(!retVal.containsError())
    {
        FlyCapture2::PropertyInfo propInfo;

        propInfo.type = FlyCapture2::BRIGHTNESS;
        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.manualSupported)
            {
                m_offsetMax = propInfo.max;
                m_offsetMin = propInfo.min;   

                unsigned int valA;
                retVal += flyCapGetParameter("offset", valA, FlyCapture2::BRIGHTNESS); 

                if (!retVal.containsError())
                {
                    double offset = (valA - m_offsetMin) / (m_offsetMax - m_offsetMin) ;
                    m_params["offset"].setVal<double>(offset);

                    retVal += flyCapSetAndGetParameter("offset", valA, FlyCapture2::BRIGHTNESS, false, true);
                }
            }
            else
            {
                m_params["offset"].setFlags(ito::ParamBase::Readonly);
            }
        }
    }

    if(!retVal.containsError())
    {

        FlyCapture2:: PropertyInfo propInfo;

        propInfo.type = FlyCapture2::GAMMA;
        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.manualSupported)
            {

                FlyCapture2::Property prop;
                prop.type = FlyCapture2::GAMMA;

                unsigned int valA;
                retVal += flyCapGetParameter("gamma", valA, FlyCapture2::GAMMA); 

                if (!retVal.containsError())
                {
                    m_params["gamma"].setVal<int>(valA);
                    m_params["gamma"].setMeta( new ito::IntMeta((int)propInfo.min, (int)propInfo.max), true );
                    retVal += flyCapSetAndGetParameter("gamma", valA, FlyCapture2::GAMMA, false, true);
                }
            }
            else
            {
                m_params["gamma"].setFlags(ito::ParamBase::Readonly);
            }
        }
    }

    if(!retVal.containsError())
    {

        FlyCapture2:: PropertyInfo propInfo;

        propInfo.type = FlyCapture2::SHARPNESS;
        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.manualSupported)
            {

                FlyCapture2::Property prop;
                prop.type = FlyCapture2::SHARPNESS;

                unsigned int valA;
                retVal += flyCapGetParameter("sharpness", valA, FlyCapture2::SHARPNESS); 

                if (!retVal.containsError())
                {
                    m_params["sharpness"].setVal<int>(valA);
                    m_params["sharpness"].setMeta( new ito::IntMeta((int)propInfo.min, (int)propInfo.max), true );
                    retVal += flyCapSetAndGetParameter("sharpness", valA, FlyCapture2::SHARPNESS, false, true);
                }
            }
            else
            {
                m_params["sharpness"].setFlags(ito::ParamBase::Readonly);
            }
        }
    }

    if(!retVal.containsError())
    {

        FlyCapture2:: PropertyInfo propInfo;

        propInfo.type = FlyCapture2::AUTO_EXPOSURE;
        retVal += checkError( m_myCam.GetPropertyInfo( &propInfo ) );

        if (!retVal.containsError())
        {
            if(propInfo.present && propInfo.manualSupported)
            {

                FlyCapture2::Property prop;
                prop.type = FlyCapture2::AUTO_EXPOSURE;

                unsigned int valA;
                retVal += flyCapGetParameter("exposureEV", valA, FlyCapture2::AUTO_EXPOSURE); 

                if (!retVal.containsError())
                {
                    m_params["exposureEV"].setVal<int>(valA);
                    m_params["exposureEV"].setMeta( new ito::IntMeta((int)propInfo.min, (int)propInfo.max), true );
                    retVal += flyCapSetAndGetParameter("exposureEV", valA, FlyCapture2::AUTO_EXPOSURE, false, true);
                }
            }
            else
            {
                m_params["exposureEV"].setFlags(ito::ParamBase::Readonly);
            }
        }
    }

    //set the camera configuration
    if(!retVal.containsError())
    {
        retError = m_myCam.GetConfiguration(&pCamConfig);
        pCamConfig.grabTimeout = m_params["timeout"].getVal<double>() * 1000;
        retVal += checkError(m_myCam.SetConfiguration(&pCamConfig));
    }

    if(!retVal.containsError())
    {
        FlyCapture2::TriggerModeInfo pTriggerInfo;
        FlyCapture2::TriggerMode triggerModeSetup;

        m_myCam.GetTriggerModeInfo(&pTriggerInfo);
        m_myCam.GetTriggerMode(&triggerModeSetup);
        
        m_RunSync = true;
        m_RunSoftwareSync = false;

        if(pTriggerInfo.present && pTriggerInfo.softwareTriggerSupported)
        {
            triggerModeSetup.onOff = false;
            m_myCam.SetTriggerMode(&triggerModeSetup);
            m_params["trigger_mode"].setVal<int>(0);  

            if(pTriggerInfo.softwareTriggerSupported)
            {
                //m_params["trigger_mode"].setMax(2); 
                static_cast<ito::IntMeta*>( m_params["trigger_mode"].getMeta() )->setMax(2);
            }
            else
            {
                //m_params["trigger_mode"].setMax(1);
                static_cast<ito::IntMeta*>( m_params["trigger_mode"].getMeta() )->setMax(1);
            }
        }
        else
        {
            m_params["trigger_mode"].setVal<int>(0);
            m_params["trigger_mode"].setFlags(ito::ParamBase::Readonly);
            //m_params["trigger_mode"].setMax(0);
            static_cast<ito::IntMeta*>( m_params["trigger_mode"].getMeta() )->setMax(0);
        }

        if(startSyncronized && (m_params["trigger_mode"].getMax() == 2))
        {
            retVal += setParam(QSharedPointer<ito::ParamBase>(new ito::ParamBase("trigger_mode", ito::ParamBase::Int, 2)), 0);
            if(!retVal.containsError())
            {
                m_myCam.GetTriggerMode(&triggerModeSetup);
                triggerModeSetup.onOff = false;
                m_myCam.SetTriggerMode(&triggerModeSetup);
                m_params["trigger_mode"].setVal<int>(0);
                //m_params["trigger_mode"].setMax(1); 
                static_cast<ito::IntMeta*>( m_params["trigger_mode"].getMeta() )->setMax(1);
            }
        }

    }

    if(!retVal.containsError())
    {
        int curBpp = GetBppFromPixelFormat(pixelFormat);
        if(curBpp < 8)
        {
            retVal += ito::RetVal(ito::retError, 0, "Get bits per pixel failed");
        }
        else
        {
            if(m_colorCam)
            {
                if(curBpp <25)
                {
                    m_params["bpp"].setVal<int>(24);
                    m_params["bpp"].setMeta( new ito::IntMeta(24,24), true );
                }
                else if (curBpp <33)
                {
                    m_params["bpp"].setVal<int>(30);  
                    m_params["bpp"].setMeta( new ito::IntMeta(30,30), true );
                }
                else
                {
                    m_params["bpp"].setVal<int>(48); 
                    m_params["bpp"].setMeta( new ito::IntMeta(48,48), true );
                }
            }
            else
            {
                m_params["bpp"].setVal<int>(curBpp);   
                m_params["bpp"].setMeta( new ito::IntMeta(minBpp,maxBpp,4), true );
            }
        
        }
    }

    if(!retVal.containsError())
    {
          
        m_params["camSerialNumber"].setVal<int>((int)camInfo.serialNumber);
        setIdentifier(QString("%1 (%2)").arg( camInfo.modelName ).arg( camInfo.serialNumber ));
        m_params["camModel"].setVal<char*>(camInfo.modelName, (int)strlen(camInfo.modelName));
        m_params["camVendor"].setVal<char*>(camInfo.vendorName, (int)strlen(camInfo.vendorName));
        m_params["camSensor"].setVal<char*>(camInfo.sensorInfo, (int)strlen(camInfo.sensorInfo));
        m_params["camResolution"].setVal<char*>(camInfo.sensorResolution, (int)strlen(camInfo.sensorResolution));
        m_params["camFirmwareVersion"].setVal<char*>(camInfo.firmwareVersion, (int)strlen(camInfo.firmwareVersion));
        m_params["camFirmwareBuildTime"].setVal<char*>(camInfo.firmwareBuildTime, (int)strlen(camInfo.firmwareBuildTime));
    }


    if(!retVal.containsError())
    {
        checkData(); //check if image must be reallocated
    }

    if(waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    setInitialized(true); //init method has been finished (independent on retval)
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! close method which is called before that this instance is deleted by the PGRFlyCaptureInterface
/*!
    notice that this method is called in the actual thread of this instance.

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
    \sa ItomSharedSemaphore
*/
ito::RetVal PGRFlyCapture::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    FlyCapture2::Error retError;
    ito::RetVal retValue(ito::retOk, 0,"");

    if(m_camIdx > -1)
    {
        retError = m_myCam.Disconnect();
        if (retError != FlyCapture2::PGRERROR_OK)
        {
                retValue += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in close-function: %s", retError.GetDescription());
        }

        for(int i = 0; i < MAXPGR; i++)
        {
            if(InitList[i] == m_camIdx)
            {
                InitList[i] = -1;
                break;
            }
        }
    }
    Initnum--;

    if(Initnum < 1)
    {
        for(int i = 0; i < MAXPGR; i++)
        {
            if(InitList[i] > -1)
            {
                retValue += ito::RetVal(ito::retWarning, 0, "Found some uninitilized but still present grabber");
            }
        }        
    }

    if(m_timerID > 0)
    {
        killTimer(m_timerID);
        m_timerID = 0;
    }

    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();

        return retValue;
    }
    else
    {
        return retValue;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
// function moved into addInGrabber.cpp -> standard for all cameras / ADDA


//----------------------------------------------------------------------------------------------------------------------------------
//! With startDevice this camera is initialized.
/*!
    In the PGRFlyCapture, this method does nothing. In general, the hardware camera should be intialized in this method and necessary memory should be allocated.

    \note This method is similar to VideoCapture::open() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if starting was successfull, retWarning if startDevice has been calling at least twice.
*/
ito::RetVal PGRFlyCapture::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;
    FlyCapture2::Error retError;

    checkData(); //this will be reallocated in this method.

    incGrabberStarted();

    if(grabberStartedCount() == 1)
    {
        retError = m_myCam.StartCapture();
        if (retError != FlyCapture2::PGRERROR_OK)
        {
            retValue += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in startDevice-function: %s", retError.GetDescription());
        }
    }

    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! With stopDevice the camera device is stopped (opposite to startDevice)
/*!
    In this PGRFlyCapture, this method does nothing. In general, the hardware camera should be closed in this method.

    \note This method is similar to VideoCapture::release() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError if camera wasn't started before
    \sa startDevice
*/
ito::RetVal PGRFlyCapture::stopDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;
    FlyCapture2::Error retError;

    decGrabberStarted();
    if(grabberStartedCount() == 0)
    {
        retValue += checkError(m_myCam.StopCapture());
    }
    else if(grabberStartedCount() < 0)
    {
        retValue += ito::RetVal(ito::retError, 1001, tr("StopDevice of PGRFlyCapture can not be executed, since camera has not been started.").toAscii().data());
        setGrabberStarted(0);
    }


    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Call this method to trigger a new image.
/*!
    By this method a new image is trigger by the camera, that means the acquisition of the image starts in the moment, this method is called.
    The new image is then stored either in internal camera memory or in internal memory of this class.

    \note This method is similar to VideoCapture::grab() of openCV

    \param [in] trigger may describe the trigger parameter (unused here)
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError if camera has not been started or an older image lies in memory which has not be fetched by getVal, yet.
    \sa getVal
*/
ito::RetVal PGRFlyCapture::acquire(const int trigger, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    if(grabberStartedCount() <= 0)
    {
        retValue += ito::RetVal(ito::retError, 1002, tr("Acquire of PGRFlyCapture can not be executed, since camera has not been started.").toAscii().data());
    }
    else
    {
		m_last_acquireTime = m_acquireTime;
        m_acquireTime = (double)(cv::getTickCount())/cv::getTickFrequency();
		m_params["timestamp"].setVal<double>((m_acquireTime-m_last_acquireTime)*1000);
        FlyCapture2::Error retError;
        this->m_isgrabbing = true;
        if(m_RunSoftwareSync)
        {
            retValue += checkError(m_myCam.FireSoftwareTrigger());
        }
        else
        {
            /*FlyCapture2::Image pImage;
            retError = m_myCam.RetrieveBuffer( &pImage );
            if (retError != FlyCapture2::PGRERROR_OK)
            {
                _snprintf(errBuff, 255, "Error in acquire-function: %s", retError.GetDescription());
                retValue += ito::RetVal(ito::retError, (int)retError.GetType(), errBuff);
            }*/

            
        }
        /*
        retError = m_myCam.( CameraStats .GetCameraInfo.GetCycleTime(&m_timeStamp);
        if (retError != FlyCapture2::PGRERROR_OK)
        {
            _snprintf(errBuff, 255, "Error in acquire-function: %s", retError.GetDescription());
            retValue += ito::RetVal(ito::retError, (int)retError.GetType(), errBuff);
        }
        */
#if EVALSPEED
        double tempTime = (double)(cv::getTickCount())/cv::getTickFrequency();
        std::cout << "\nAcquire in camera thread\t" << tempTime-m_acquireTime << "\n";
#endif
    }


    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed camera frame as a shallow copy.
/*!
    This method copies the recently grabbed camera frame to the given DataObject-handle

    \note This method is similar to VideoCapture::retrieve() of openCV

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired image is shallow-copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is camera has not been started or no image has been acquired by the method acquire.
    \sa DataObject, acquire
*/
ito::RetVal PGRFlyCapture::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    ito::RetVal retValue(ito::retOk);

    retValue += retrieveData();

    if(!retValue.containsError())
    {
        if(dObj == NULL)
        {
            retValue += ito::RetVal(ito::retError, 1004, tr("data object of getVal is NULL or cast failed").toAscii().data());
        }
        else
        {
            retValue += sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.

            (*dObj) = this->m_data;
        }
    }

    if (waitCond)
    {
        waitCond->returnValue=retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed camera frame as a deep copy.
/*!
    This method copies the recently grabbed camera frame to the given DataObject. Therefore this camera size must fit to the data structure of the
    DataObject.

    \note This method is similar to VideoCapture::retrieve() of openCV

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired image is deep copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is camera has not been started or no image has been acquired by the method acquire.
    \sa DataObject, acquire
*/
ito::RetVal PGRFlyCapture::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    if(!dObj)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Empty object handle retrieved from caller").toAscii().data());
    }
    else
    {
        retValue += checkData(dObj);  
    }

    if(!retValue.containsError())
    {
        retValue += retrieveData(dObj);  
    }

    if(!retValue.containsError())
    {
        sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::retrieveData(ito::DataObject *externalDataObject)
{
    ito::RetVal retValue(ito::retOk);
    FlyCapture2::Error retError;

    unsigned long imglength = 0;
    long lcopysize = 0;
    long lsrcstrpos = 0;
    int y  = 0;
    int maxxsize = (int)m_params["sizex"].getMax();
    int maxysize = (int)m_params["sizey"].getMax();
    int curxsize = m_params["sizex"].getVal<int>();
    int curysize = m_params["sizey"].getVal<int>();
    int x0 = m_params["x0"].getVal<int>();
    int y0 = m_params["y0"].getVal<int>();

    bool hasListeners = false;
    bool copyExternal = false;
    if(m_autoGrabbingListeners.size() > 0)
    {
        hasListeners = true;
    }
    if(externalDataObject != NULL)
    {
        copyExternal = true;
    }
        
    if(grabberStartedCount() <= 0 || m_isgrabbing != true)
    {
        retValue += ito::RetVal(ito::retError, 0, "Camera not started or triggered");
        return retValue;
    }

    FlyCapture2::Image pImage, convertedImage;
    
    // time to sleep

#if EVALSPEED    
    double intermTime1 = (double)(cv::getTickCount())/cv::getTickFrequency();

    double getValTime = (m_params["frame_time"].getVal<double>() - ((double)(cv::getTickCount())/cv::getTickFrequency() - m_acquireTime)) * 1000.0;
    
    if(m_RunSync)
    {
        double getValTime2 = getValTime;
        while (getValTime2 > 0.0)
        {
            getValTime2 = (m_params["frame_time"].getVal<double>() - ((double)(cv::getTickCount())/cv::getTickFrequency() - m_acquireTime)) * 1000.0;
        }
    }
    double intermTime2 = (double)(cv::getTickCount())/cv::getTickFrequency();
#endif

    retError = m_myCam.RetrieveBuffer( &pImage );
    
#if EVALSPEED
    double totalTime = (double)(cv::getTickCount())/cv::getTickFrequency();
#endif

    if (retError != FlyCapture2::PGRERROR_OK)
    {
        retValue += ito::RetVal::format(ito::retError, (int)retError.GetType(), "Error in retrieveData-function: %s", retError.GetDescription());
    }
    else
    {
        //int bpp = m_params["bpp"].getVal<int>();
        int bpp = pImage.GetBitsPerPixel();
        //int cols = pImage.GetCols();
        //int rows = pImage.GetRows();
        //int buffer = pImage.GetDataSize();
        //FlyCapture2::TimeStamp cur_timeStamp = pImage.GetTimeStamp();

        //if(cur_timeStamp.seconds < m_timeStamp.seconds && cur_timeStamp.microSeconds < m_timeStamp.microSeconds)
        //{
        //    retError = m_myCam.RetrieveBuffer( &pImage );
        //    if (retError != FlyCapture2::PGRERROR_OK)
        //    {
        //        _snprintf(errBuff, 255, "Error in retrieveData-function: %s", retError.GetDescription());
        //        retValue += ito::RetVal(ito::retError, (int)retError.GetType(), errBuff);
        //    }
        //}

        if(bpp <= 8)
        {
            retValue += checkError(pImage.Convert(FlyCapture2::PIXEL_FORMAT_MONO8, &convertedImage));

            if (!retValue.containsError())
            {
                ito::uint8 *cbuf=(ito::uint8*)pImage.GetData();
                if(cbuf == NULL)
                {
                    retValue += ito::RetVal(ito::retError, 1002, tr("getVal of PGRFlyCapture failed, since retrived NULL-Pointer.").toAscii().data());
                }
                else 
                {
                    if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint8>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                    if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint8>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                }
            }
        }
        else if(bpp <= 12)
        {
            retValue += checkError(pImage.Convert(FlyCapture2::PIXEL_FORMAT_MONO16, &convertedImage));

            if (!retValue.containsError())
            {
                ito::uint16 *cbuf=(ito::uint16*)convertedImage.GetData();
                if(cbuf == NULL)
                {
                    retValue += ito::RetVal(ito::retError, 1002, tr("getVal of PGRFlyCapture failed, since retrived NULL-Pointer.").toAscii().data());
                }
                else
                {
                    if(copyExternal) 
                    {
                        retValue += externalDataObject->copyFromData2D<ito::uint16>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                        *externalDataObject >>= 4;
                    }
                    if(!copyExternal || hasListeners) 
                    {
                        retValue += m_data.copyFromData2D<ito::uint16>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                        m_data >>= 4;
                    }
                }
            }
        }
        else if(bpp <= 16)
        {
            retValue += checkError(pImage.Convert(FlyCapture2::PIXEL_FORMAT_MONO16, &convertedImage));

            if (!retValue.containsError())
            {
                ito::uint16 *cbuf=(ito::uint16*)convertedImage.GetData();
                if(cbuf == NULL)
                {
                    retValue += ito::RetVal(ito::retError, 1002, tr("getVal of PGRFlyCapture failed, since retrived NULL-Pointer.").toAscii().data());
                }
                else
                {
                    if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint16>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                    if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint16>(cbuf, convertedImage.GetCols(), convertedImage.GetRows());
                }
            }
        }
        else
        {
            retValue += ito::RetVal::format(ito::retError, 1002, tr("retrieveData of PGRFlyCapture faield, since bitdepth %i not implemented.").toLatin1().data(), bpp);
        }
        this->m_isgrabbing = false;
    }

#if EVALSPEED
    double totalTime2 = (double)(cv::getTickCount())/cv::getTickFrequency();

    std::cout << "\nFramtime \t" << m_params["frame_time"].getVal<double>() * 1000.0 << "\n";
    std::cout << "\nAcquire to timer \t" << (intermTime1 - m_acquireTime) * 1000.0 << "\n";
    std::cout << "\nWait till end of frame \t" << (intermTime2 - intermTime1)* 1000.0 << " Shoudbe:" << getValTime << "\n";
    std::cout << "\nGrabbing from Buffer \t" << (totalTime - intermTime2) * 1000.0  << "\n";

    std::cout << "\nAcquire till got buffer \t" << (totalTime -  m_acquireTime) * 1000.0 << "\n";
    std::cout << "\nAcquire till copied buffer \t" << (totalTime2 -  m_acquireTime) * 1000.0 << "\n";
#endif

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
void PGRFlyCapture::dockWidgetVisibilityChanged(bool visible)
{
    if (getDockWidget())
    {
        QWidget *widget = getDockWidget()->widget();
        if (visible)
        {
            connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), widget, SLOT(parametersChanged(QMap<QString, ito::Param>)));

            emit parametersChanged(m_params);
        }
        else
        {
            disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), widget, SLOT(parametersChanged(QMap<QString, ito::Param>)));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
bool PGRFlyCapture::GetResolutionFromVideoMode( FlyCapture2::VideoMode mode, int &sizeX, int &sizeY)
{
    switch(mode)
    {
        case FlyCapture2::VIDEOMODE_160x120YUV444:
            sizeX = 160;
            sizeY = 120;
            break;

        case FlyCapture2::VIDEOMODE_320x240YUV422:
            sizeX = 320;
            sizeY = 240;
            break;

        case FlyCapture2::VIDEOMODE_640x480Y8:
        case FlyCapture2::VIDEOMODE_640x480Y16:
        case FlyCapture2::VIDEOMODE_640x480RGB:
        case FlyCapture2::VIDEOMODE_640x480YUV422:
        case FlyCapture2::VIDEOMODE_640x480YUV411:
            sizeX = 640;
            sizeY = 480;
            break;

        case FlyCapture2::VIDEOMODE_800x600Y8:
        case FlyCapture2::VIDEOMODE_800x600Y16:
        case FlyCapture2::VIDEOMODE_800x600RGB:
        case FlyCapture2::VIDEOMODE_800x600YUV422:     
            sizeX = 800;
            sizeY = 600;
            break;        

        case FlyCapture2::VIDEOMODE_1024x768Y8:
        case FlyCapture2::VIDEOMODE_1024x768Y16:
        case FlyCapture2::VIDEOMODE_1024x768RGB:
        case FlyCapture2::VIDEOMODE_1024x768YUV422:
            sizeX = 1024;
            sizeY = 768;
            break;

        case FlyCapture2::VIDEOMODE_1280x960Y8:
        case FlyCapture2::VIDEOMODE_1280x960Y16:
        case FlyCapture2::VIDEOMODE_1280x960RGB:
        case FlyCapture2::VIDEOMODE_1280x960YUV422:
            sizeX = 1280;
            sizeY = 960;
            break;

        case FlyCapture2::VIDEOMODE_1600x1200Y8:   
        case FlyCapture2::VIDEOMODE_1600x1200Y16:   
        case FlyCapture2::VIDEOMODE_1600x1200RGB:
        case FlyCapture2::VIDEOMODE_1600x1200YUV422:
            sizeX = 1600;
            sizeY = 1200;
            break;

        case FlyCapture2::VIDEOMODE_FORMAT7:
            sizeX = 0;
            sizeY = 0;
            return false;
        
        default:
            sizeX = 0;
            sizeY = 0;
            return false;
    }

    return false;    
}

//----------------------------------------------------------------------------------------------------------------------------------
bool PGRFlyCapture::GetPixelFormatFromVideoMode( FlyCapture2::VideoMode mode, bool stippled, FlyCapture2::PixelFormat* pixFormat)
{
    switch(mode)
    {
        case FlyCapture2::VIDEOMODE_640x480Y8:
        case FlyCapture2::VIDEOMODE_800x600Y8:
        case FlyCapture2::VIDEOMODE_1024x768Y8:
        case FlyCapture2::VIDEOMODE_1280x960Y8:
        case FlyCapture2::VIDEOMODE_1600x1200Y8:
            if( stippled )
            {
                *pixFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
            }
            else
            {
                *pixFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
            }
            break;
        case FlyCapture2::VIDEOMODE_640x480Y16:
        case FlyCapture2::VIDEOMODE_800x600Y16:
        case FlyCapture2::VIDEOMODE_1024x768Y16:
        case FlyCapture2::VIDEOMODE_1280x960Y16:
        case FlyCapture2::VIDEOMODE_1600x1200Y16:
            if( stippled )
            {
                *pixFormat = FlyCapture2::PIXEL_FORMAT_RAW16;
            }
            else
            {
                *pixFormat = FlyCapture2::PIXEL_FORMAT_MONO16;
            }            
            break;
        case FlyCapture2::VIDEOMODE_640x480RGB:
        case FlyCapture2::VIDEOMODE_800x600RGB:
        case FlyCapture2::VIDEOMODE_1024x768RGB:
        case FlyCapture2::VIDEOMODE_1280x960RGB:
        case FlyCapture2::VIDEOMODE_1600x1200RGB:
            *pixFormat = FlyCapture2::PIXEL_FORMAT_RGB8;
            break;

        case FlyCapture2::VIDEOMODE_320x240YUV422:
        case FlyCapture2::VIDEOMODE_640x480YUV422:
        case FlyCapture2::VIDEOMODE_800x600YUV422:       
        case FlyCapture2::VIDEOMODE_1024x768YUV422:
        case FlyCapture2::VIDEOMODE_1280x960YUV422:
        case FlyCapture2::VIDEOMODE_1600x1200YUV422:
            *pixFormat = FlyCapture2::PIXEL_FORMAT_422YUV8;
            break;      

        case FlyCapture2::VIDEOMODE_160x120YUV444:
            *pixFormat = FlyCapture2::PIXEL_FORMAT_444YUV8;
            break;

        case FlyCapture2::VIDEOMODE_640x480YUV411:
            *pixFormat = FlyCapture2::PIXEL_FORMAT_411YUV8;
            break;

        case FlyCapture2::VIDEOMODE_FORMAT7:
            return false;
        
        default:
            return false;
    }

    return false;    
}

//----------------------------------------------------------------------------------------------------------------------------------
unsigned int PGRFlyCapture::GetBppFromPixelFormat( FlyCapture2::PixelFormat pixelFormat )
{
    switch(pixelFormat)
    {
        case FlyCapture2::PIXEL_FORMAT_MONO8:
        case FlyCapture2::PIXEL_FORMAT_RAW8:
            return 8;
            break;

        case FlyCapture2::PIXEL_FORMAT_411YUV8:
        case FlyCapture2::PIXEL_FORMAT_MONO12:
        case FlyCapture2::PIXEL_FORMAT_RAW12:
            return 12;
            break;

        case FlyCapture2::PIXEL_FORMAT_MONO16:
        case FlyCapture2::PIXEL_FORMAT_S_MONO16:
        case FlyCapture2::PIXEL_FORMAT_422YUV8:
        case FlyCapture2::PIXEL_FORMAT_RAW16:
            return 16;
            break;

        case FlyCapture2::PIXEL_FORMAT_444YUV8:
        case FlyCapture2::PIXEL_FORMAT_RGB8:
        case FlyCapture2::PIXEL_FORMAT_BGR:
            return 24;
            break;

        case FlyCapture2::PIXEL_FORMAT_BGRU:
        case FlyCapture2::PIXEL_FORMAT_RGBU:
            return 32;
            break;

        case FlyCapture2::PIXEL_FORMAT_S_RGB16:
        case FlyCapture2::PIXEL_FORMAT_RGB16:
        case FlyCapture2::PIXEL_FORMAT_BGR16:
            return 48;
            break;

        default:
            return 0;
            break;
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------------------
double PGRFlyCapture::GetFrameTimeFromFrameRate( FlyCapture2::FrameRate frameRate )
{
    switch(frameRate)
    {
        case FlyCapture2::FRAMERATE_1_875:
            return 1/1.875;
        case FlyCapture2::FRAMERATE_3_75:
            return 1/3.75;
        case FlyCapture2::FRAMERATE_7_5:
            return 1/7.5;
        case FlyCapture2::FRAMERATE_15:
            return 1/15;
        case FlyCapture2::FRAMERATE_30:
            return 1/30;
        case FlyCapture2::FRAMERATE_60:
            return 1/60;
        case FlyCapture2::FRAMERATE_120:
            return 1/120;
        case FlyCapture2::FRAMERATE_240:
            return 1/240;

        default:
            return 0.0;
            break;
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------------------
FlyCapture2::FrameRate PGRFlyCapture::GetSuitAbleFrameRateFromFrameTime( double frameTime  )
{
    if(frameTime <= 1/240) return FlyCapture2::FRAMERATE_240;
    if(frameTime <= 1/120) return FlyCapture2::FRAMERATE_120;
    if(frameTime <= 1/60) return FlyCapture2::FRAMERATE_60;
    if(frameTime <= 1/30) return FlyCapture2::FRAMERATE_30;
    if(frameTime <= 1/15) return FlyCapture2::FRAMERATE_15;
    if(frameTime <= 1/7.5) return FlyCapture2::FRAMERATE_7_5;
    if(frameTime <= 1/3.75) return FlyCapture2::FRAMERATE_3_75;
    
    return FlyCapture2::FRAMERATE_1_875;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::checkError(const FlyCapture2::Error &error)
{
    if (error == FlyCapture2::PGRERROR_OK)
    {
        return ito::retOk;
    }

    return ito::RetVal(ito::retError, (int)error.GetType(), error.GetDescription());
}


//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapChangeFormat7(bool changeBpp, bool changeROI, int bpp, int x0, int y0, int x1, int y1)
{
    ito::RetVal retValue;
    int running = 0;

    if (m_hasFormat7) //set in init
    {
        FlyCapture2::Format7ImageSettings f7ImageSettings = m_currentFormat7Settings;

        if (changeBpp)
        {
            switch (bpp)
            {
            case -1:
                break; //unchanged
            case 8:
                f7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
                break;
            case 12:
                f7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO12;
                break;
            case 16:
                {
                    f7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO16;
                    // This comes from the SDK and seems to wrap the little endian / big endian problem
                    // Force the camera to PGR's Y16 endianness
                    const unsigned int k_imageDataFmtReg = 0x1048;
                    unsigned int value = 0;
                    m_myCam.ReadRegister( k_imageDataFmtReg, &value );
                    value &= ~(0x1 << 0);
                    m_myCam.WriteRegister( k_imageDataFmtReg, value );
                }                        
                break;
            default:
                retValue += ito::RetVal::format(ito::retError, 0, "Bitdepth of %i bit is not supported by this plugin.", bpp);
            }

            if ((m_format7Info.pixelFormatBitField & f7ImageSettings.pixelFormat) == 0)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "Bitdepth of %i bit not supported by this camera.", bpp);
            }
        }

        if (changeROI)
        {
            if (x0 == -1) x0 = f7ImageSettings.offsetX;
            if (y0 == -1) y0 = f7ImageSettings.offsetY;
            if (x1 == -1) x1 = f7ImageSettings.offsetX + f7ImageSettings.width - 1;
            if (y1 == -1) y1 = f7ImageSettings.offsetY + f7ImageSettings.height - 1;

            if (x0 < 0 || x0 >= (m_format7Info.maxWidth - 1) || (x0 % m_format7Info.offsetHStepSize) != 0)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "x0 must be in range [0:%i:%i]", m_format7Info.offsetHStepSize, m_format7Info.maxWidth-1);
            }

            if (y0 < 0 || y0 >= (m_format7Info.maxHeight - 1) || (y0 % m_format7Info.offsetVStepSize) != 0)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "y0 must be in range [0:%i:%i]", m_format7Info.offsetVStepSize, m_format7Info.maxHeight-1);
            }

            if (x1 <= x0 || x1 > (m_format7Info.maxWidth - 1) || ((1 + x1 - x0) % m_format7Info.imageHStepSize) != 0)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "x1 must be in range [%i:%i:%i]", x0+1, m_format7Info.imageHStepSize, m_format7Info.maxWidth-1);
            }

            if (y1 <= y0 || y1 > (m_format7Info.maxHeight - 1) || ((1 + y1 - y0) % m_format7Info.imageVStepSize) != 0)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "y1 must be in range [%i:%i:%i]", y0+1, m_format7Info.imageVStepSize, m_format7Info.maxHeight-1);
            }

            if (!retValue.containsError())
            {
                f7ImageSettings.offsetX = x0;
                f7ImageSettings.offsetY = y0;
                f7ImageSettings.width = (x1 - x0 + 1);
                f7ImageSettings.height = (y1 - y0 + 1);
            }
        }

        if (memcmp(&f7ImageSettings,&m_currentFormat7Settings,sizeof(FlyCapture2::Format7ImageSettings)) != 0) //changes
        {
            // Validate the settings to make sure that they are valid
            bool valid;
            retValue += checkError(m_myCam.ValidateFormat7Settings(&f7ImageSettings, &valid, &m_currentPacketInfo ));
                    
            if (!retValue.containsError())
            {
                if ( !valid )
                {
                    retValue += ito::RetVal(ito::retError,0,"the format options as result of the camera parameters are invalid");
                }
                else
                {
                    //stop camera if started
                    if (grabberStartedCount() > 0)
                    {
                        running = grabberStartedCount();
                        setGrabberStarted(1);
                        retValue += stopDevice(NULL);
                    }

                    retValue += checkError(m_myCam.SetFormat7Configuration(&f7ImageSettings, m_currentPacketInfo.recommendedBytesPerPacket));

                    
                }
            }
        }

        //in any cases get now the current information back
        unsigned int packetSize;
        float percentage;
        ito::RetVal ret2 = checkError(m_myCam.GetFormat7Configuration(&m_currentFormat7Settings, &packetSize, &percentage));

        if (!ret2.containsError())
        {
            ito::IntMeta *im;

            m_params["x0"].setVal<int>( m_currentFormat7Settings.offsetX );
            im = static_cast<ito::IntMeta*>(m_params["x0"].getMeta());
            im->setMax( m_currentFormat7Settings.offsetX + m_currentFormat7Settings.width - 2 );

            m_params["x1"].setVal<int>( m_currentFormat7Settings.offsetX + m_currentFormat7Settings.width - 1 );
            im = static_cast<ito::IntMeta*>(m_params["x1"].getMeta());
            im->setMin( m_currentFormat7Settings.offsetX + m_format7Info.imageHStepSize - 1);

            m_params["y0"].setVal<int>( m_currentFormat7Settings.offsetY );
            im = static_cast<ito::IntMeta*>(m_params["y0"].getMeta());
            im->setMax( m_currentFormat7Settings.offsetY + m_currentFormat7Settings.height - 2 );

            m_params["y1"].setVal<int>( m_currentFormat7Settings.offsetY + m_currentFormat7Settings.height - 1 );
            im = static_cast<ito::IntMeta*>(m_params["y1"].getMeta());
            im->setMin( m_currentFormat7Settings.offsetY + m_format7Info.imageVStepSize - 1);

            m_params["sizex"].setVal<int>(m_currentFormat7Settings.width);
            m_params["sizey"].setVal<int>(m_currentFormat7Settings.height);

            m_params["bpp"].setVal<int>(GetBppFromPixelFormat(m_currentFormat7Settings.pixelFormat));
        }

        retValue += ret2;

        retValue += checkData(); //check if image must be reallocated

        //restart camera if it has been started as was stopped by this method
        if (running)
        {
            retValue += startDevice(NULL);
            setGrabberStarted(running);
        }        
    }
    else
    {
        retValue += ito::RetVal(ito::retError, 0, "Changing the image format is only implemented if the camera provides the format7 option");
    }

    return retValue;

}


//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapSetExtendedShutter(bool enabled)
{
    ito::RetVal retVal;

    if (enabled && m_extendedShutter != DRAGONFLY_EXTENDED_SHUTTER && m_extendedShutter != GENERAL_EXTENDED_SHUTTER)
    {
        //set the extended shutter

        // Check if the camera supports the FRAME_RATE property
        FlyCapture2::PropertyInfo propInfoFrameRate;
        propInfoFrameRate.type = FlyCapture2::FRAME_RATE;
        retVal += checkError(m_myCam.GetPropertyInfo( &propInfoFrameRate ));

        if ( propInfoFrameRate.present == true )
        {
            // Turn off frame rate

            FlyCapture2::Property prop;
            prop.type = FlyCapture2::FRAME_RATE;
            retVal += checkError(m_myCam.GetProperty( &prop ));
            
            if (!retVal.containsError())
            {
                prop.autoManualMode = false;
                prop.onOff = false;

                retVal += checkError(m_myCam.SetProperty( &prop ));
                
                if (retVal != ito::retError)
                {
                    m_extendedShutter = GENERAL_EXTENDED_SHUTTER;
                }
            }
        }
        else
        {
            // Frame rate property does not appear to be supported.
            // Disable the extended shutter register instead.
            // This is only applicable for Dragonfly.

            const unsigned int k_extendedShutter = 0x1028;
            unsigned int extendedShutterRegVal = 0;

            retVal += checkError(m_myCam.ReadRegister( k_extendedShutter, &extendedShutterRegVal ));

            if (retVal != ito::retError)
            {
                std::bitset<32> extendedShutterBS((int) extendedShutterRegVal );
                if ( extendedShutterBS[31] == true )
                {
                    // Set the camera into extended shutter mode
                    retVal += checkError(m_myCam.WriteRegister( k_extendedShutter, 0x80020000 ));

                    if (retVal != ito::retError)
                    {
                        m_extendedShutter = DRAGONFLY_EXTENDED_SHUTTER;
                    }
                    
                }
                else
                {
                    retVal += ito::RetVal(ito::retWarning,0,"frame rate and extended shutter are not supported. Could not set the extended shutter");
                }
            }
        }

        if (m_extendedShutter == UNINITIALIZED)
        {
            m_extendedShutter = NO_EXTENDED_SHUTTER;
        }
    }
    else if (!enabled && m_extendedShutter != NO_EXTENDED_SHUTTER)
    {
        //disable it

        if ( m_extendedShutter == GENERAL_EXTENDED_SHUTTER )
        {
            FlyCapture2::Property prop;
            prop.type = FlyCapture2::FRAME_RATE;
            retVal += checkError(m_myCam.GetProperty( &prop ));
            
            if (retVal != ito::retError)
            {
                prop.autoManualMode = true;
                prop.onOff = true;

                retVal += checkError(m_myCam.SetProperty( &prop ));

                if (retVal != ito::retError)
                {
                    m_extendedShutter = NO_EXTENDED_SHUTTER;
                }
            }
        }
        else if ( m_extendedShutter == DRAGONFLY_EXTENDED_SHUTTER )
        {
            const unsigned int k_extendedShutter = 0x1028;
            unsigned int extendedShutterRegVal = 0;

            retVal += checkError(m_myCam.ReadRegister( k_extendedShutter, &extendedShutterRegVal ));

            if (retVal != ito::retError)
            {
                std::bitset<32> extendedShutterBS((int) extendedShutterRegVal );
                if ( extendedShutterBS[31] == true )
                {
                    // Set the camera into extended shutter mode
                    retVal += checkError(m_myCam.WriteRegister( k_extendedShutter, 0x80000000 ));
                }
            }

            if (retVal != ito::retError)
            {
                m_extendedShutter = NO_EXTENDED_SHUTTER;
            }
        }
        else
        {
            //no extended shutter, disabled per default
            m_extendedShutter = NO_EXTENDED_SHUTTER;
        }

    }

    m_params["extended_shutter"].setVal<int>(m_extendedShutter == NO_EXTENDED_SHUTTER ? 0: 1);

    return retVal;  
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PGRFlyCapture::flyCapSynchronizeFrameRateShutter()
{
    ito::RetVal retVal;

    //############################
    //  READ FRAME_RATE
    //############################
    FlyCapture2::PropertyInfo propInfoFrameRate;
    propInfoFrameRate.type = FlyCapture2::FRAME_RATE;
    retVal += checkError(m_myCam.GetPropertyInfo( &propInfoFrameRate ));

    // Check if the camera supports the FRAME_RATE property
    if (propInfoFrameRate.present)
    {
        static_cast<ito::DoubleMeta*>(m_params["frame_time"].getMeta())->setMin( 1.0 / propInfoFrameRate.absMax );
        static_cast<ito::DoubleMeta*>(m_params["frame_time"].getMeta())->setMax( 1.0 / propInfoFrameRate.absMin );

        //get prop FRAME_RATE
        FlyCapture2::Property prop;
        prop.type = FlyCapture2::FRAME_RATE;
        retVal += checkError(m_myCam.GetProperty( &prop ));

        if (retVal != ito::retError)
        {
            if (prop.onOff == false) //off -> extended shutter, no frame_rate
            {
                m_params["frame_time"].setFlags(ito::ParamBase::Readonly);
            }
            else
            {
                m_params["frame_time"].setFlags(0);
                m_params["frame_time"].setVal<double>(1.0 / prop.absValue);
            }
        }
    }
    else
    {
        m_params["frame_time"].setFlags(ito::ParamBase::Readonly);
    }


    //############################
    //  READ SHUTTER
    //############################
    FlyCapture2::PropertyInfo propInfoShutter;
    propInfoShutter.type = FlyCapture2::SHUTTER;
    retVal += checkError(m_myCam.GetPropertyInfo( &propInfoShutter ));

    if(propInfoShutter.present && propInfoShutter.absValSupported && propInfoShutter.manualSupported)
    {
        m_params["integration_time"].setFlags(0);
        static_cast<ito::DoubleMeta*>( m_params["integration_time"].getMeta() )->setMin(propInfoShutter.absMin / 1000.0);
        static_cast<ito::DoubleMeta*>( m_params["integration_time"].getMeta() )->setMax(propInfoShutter.absMax / 1000.0);

        //get prop SHUTTER
        FlyCapture2::Property prop;
        prop.type = FlyCapture2::SHUTTER;
        retVal += checkError(m_myCam.GetProperty( &prop ));

        if (retVal != ito::retError)
        {
            m_params["integration_time"].setVal<double>(prop.absValue / 1000.0);
        }
    }
    else
    {
        m_params["integration_time"].setFlags(ito::ParamBase::Readonly);
    }

    return retVal;
}
