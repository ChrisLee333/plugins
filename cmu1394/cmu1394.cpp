/*! \file cmu1394.cpp
   \brief   file for a generic firewire support based on the CMU-driver
   \detailed This is the file for the generic firewire suppoert based on the free CMU-driver.
   This driver can be downloaded from http://www.cs.cmu.edu/~iwan/1394/. Current version is 6.4.6.

   \author ITO
   \date 02.2012
*/

#include "cmu1394.h"
#include "dockWidgetcmu1394.h"

#include <QFile>
#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>

#include "pluginVersion.h"

//#include <windows.h>

#pragma comment(lib, "1394camera.lib")
#pragma comment(linker, "/delayload:1394camera.dll")

Q_DECLARE_METATYPE(ito::DataObject)

static signed char InitList[MAX1394 + 1];

static char Initnum=0;
static HINSTANCE dll = NULL;

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394Interface::getAddInInst(ito::AddInBase **addInInst)
{
    CMU1394* newInst = new CMU1394();
    newInst->setBasePlugin(this);
    *addInInst = qobject_cast<ito::AddInBase*>(newInst);

    m_InstList.append(*addInInst);

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394Interface::closeThisInst(ito::AddInBase **addInInst)
{
   if (*addInInst)
   {
      delete ((CMU1394 *)*addInInst);
      int idx = m_InstList.indexOf(*addInInst);
      m_InstList.removeAt(idx);
   }

   return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
CMU1394Interface::CMU1394Interface() : AddInInterfaceBase()
{
   m_type = ito::typeDataIO | ito::typeGrabber;
   setObjectName("CMU1394");

    m_description = QObject::tr("Firewire via generic CMU-Driver");

    //for the docstring, please don't set any spaces at the beginning of the line.
    char* docstring = \
"This plugins provides generic firewire camera support based on CMU-Driver version 6.4.6. CMU 6.4.6 works for firewire specifications <= v1.30.\
This library is currently developed and tested under Windows only. Tested with PointGrayResearch Firefly and AVT Marlin.\n\
\n\
In order to run this plugin you also need to install the CMU1394 drivers that can be obtained as installer from http://www.cs.cmu.edu/~iwan/1394/. \
Together with this plugin parts of the drivers (some header files and static libraries for 32 and 64bit) in version 6.4.6 are shipped and linked to this plugin \
at comile time. Therefore you need to install the drivers for the same version as well. Otherwise you can also change the files in the corresponding source folder \
of this plugin.";
    m_detaildescription = QObject::tr(docstring);
    m_author            = "W. Lyda, M. Gronle, ITO, University Stuttgart";
    m_license           = QObject::tr("itom-plugin under LGPL / CMU1394 driver under LGPL");
    m_version           = (PLUGIN_VERSION_MAJOR << 16) + (PLUGIN_VERSION_MINOR << 8) + PLUGIN_VERSION_PATCH;
    m_minItomVer        = MINVERSION;
    m_maxItomVer        = MAXVERSION;
    m_aboutThis         = tr("N.A."); 

   ito::Param paramVal = ito::Param("Format", ito::ParamBase::Int, 0, 2, 0, tr("Formattype for the camera, first index of struct VIDEO_MODE_DESCRIPTOR. See CMU documentation.").toAscii().data());
   m_initParamsMand.append(paramVal);

   paramVal = ito::Param("Mode", ito::ParamBase::Int, 0, 5, 0, tr("Modetype for the camera, second index of struct VIDEO_MODE_DESCRIPTOR. See CMU documentation.").toAscii().data());
   m_initParamsMand.append(paramVal);

   paramVal = ito::Param("Rate", ito::ParamBase::Int, 0, 8, 0, tr("Rate (fps) for the camera (1: 3.75, 2: 7.5, 3: 15, 4: 30, 5: 60, 6: 120). For more information see tableQPP of CMU.").toAscii().data());
   m_initParamsMand.append(paramVal);

   paramVal = ito::Param("CameraNumber", ito::ParamBase::Int, -1, MAX1394 - 1, -1, tr("Camera number (-1 for auto)").toAscii().data());
   m_initParamsOpt.append(paramVal);

   return;

   /**\brief width, height and color code for the core formats (0-2) */
    //static VIDEO_MODE_DESCRIPTOR tableModeDesc[3][8] = 
    //{
       // {
          //  {160 ,120 ,COLOR_CODE_YUV444},
          //  {320 ,240 ,COLOR_CODE_YUV422},
          //  {640 ,480 ,COLOR_CODE_YUV411},
          //  {640 ,480 ,COLOR_CODE_YUV422},
          //  {640 ,480 ,COLOR_CODE_RGB8},
          //  {640 ,480 ,COLOR_CODE_Y8},
          //  {640 ,480 ,COLOR_CODE_Y16},
          //  {0   ,0   ,COLOR_CODE_MAX}
       // },{
          //  {800 ,600 ,COLOR_CODE_YUV422},
          //  {800 ,600 ,COLOR_CODE_RGB8},
          //  {800 ,600 ,COLOR_CODE_Y8},
          //  {1024,768 ,COLOR_CODE_YUV422},
          //  {1024,768 ,COLOR_CODE_RGB8},
          //  {1024,768 ,COLOR_CODE_Y8},
          //  {800 ,600 ,COLOR_CODE_Y16},
          //  {1024,768 ,COLOR_CODE_Y16}
       // },{
          //  {1280,960 ,COLOR_CODE_YUV422},
          //  {1280,960 ,COLOR_CODE_RGB8},
          //  {1280,960 ,COLOR_CODE_Y8},
          //  {1600,1200,COLOR_CODE_YUV422},
          //  {1600,1200,COLOR_CODE_RGB8},
          //  {1600,1200,COLOR_CODE_Y8},
          //  {1280,960 ,COLOR_CODE_Y16},
          //  {1600,1200,COLOR_CODE_Y16}
       // }
    //};
}

//----------------------------------------------------------------------------------------------------------------------------------
CMU1394Interface::~CMU1394Interface()
{
    m_initParamsMand.clear();
    m_initParamsOpt.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(CMU1394interface, CMU1394Interface)

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
const ito::RetVal CMU1394::showConfDialog(void)
{
    ito::RetVal retValue(ito::retOk);
    int bitppix_old = 12;
    int binning_old = 0;
    int bitppix_new = 12;
    int binning_new = 0;
    double offset_new = 0.0;

    dialogCMU1394 *confDialog = new dialogCMU1394();

    if(grabberStartedCount() > 0)
        return ito::RetVal(ito::retWarning, 0, tr("Please run stopDevice() and shut down live data before configuration").toAscii().data());

    connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));
    connect(confDialog, SIGNAL(changeParameters(QMap<QString, ito::ParamBase>)), this , SLOT(updateParameters(QMap<QString, ito::ParamBase>)));

    confDialog->setVals(&m_params);
    if (confDialog->exec())
    {
        confDialog->getVals(&m_params);
        
        foreach(const ito::ParamBase &param1, m_params)
        {    
            retValue += setParam(QSharedPointer<ito::ParamBase>(new ito::ParamBase(param1)), NULL);
        }
    }
    delete confDialog;

    return retValue;
}
//----------------------------------------------------------------------------------------------------------------------------------
CMU1394::CMU1394() :  AddInGrabber(), m_saveParamsOnClose(false), 
                                        m_ptheCamera(NULL), m_pC1394gain(NULL), m_pC1394offset(NULL), 
                                        m_pC1394autoexp(NULL), m_pC1394trigger(NULL), m_isgrabbing(0),
                                        m_iFireWire_VideoMode(0), m_iFireWire_VideoFormat(0), m_iFireWire_VideoRate(0),
                                        m_iCamNumber(0)
{
    //qRegisterMetaType<ito::DataObject>("ito::DataObject");
    //qRegisterMetaType<QMap<QString, ito::Param> >("QMap<QString, ito::Param>");
    
    ito::Param paramVal("name", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::NoAutosave, "CMU1394", NULL);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("integration_time", ito::ParamBase::Double, 0.0, 0.0, 0.0, tr("Integrationtime of CCD programmed in s").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("gain", ito::ParamBase::Double, 0.0, 1.0, 0.0, tr("Gain").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("offset", ito::ParamBase::Double, 0.0, 1.0, 0.0, tr("Offset").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);


    paramVal = ito::Param("x0", ito::ParamBase::Int, 0, 1391, 0, tr("Startvalue for ROI").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y0", ito::ParamBase::Int, 0, 1023, 0, tr("Stoppvalue for ROI").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizex", ito::ParamBase::Int, 1, 1392, 1392, tr("ROI-Size in x").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizey", ito::ParamBase::Int, 1, 1024, 1024, tr("ROI-Size in y").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("bpp", ito::ParamBase::Int, 8, 12, 12, tr("Grabdepth in bpp").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("timeout", ito::ParamBase::Double, 0.001, 4095.0, 1.0, tr("Timeout for grabbing in s").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("binning", ito::ParamBase::Int, 0, 0, 0, tr("Activate nxn binning (not implemented yet)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("trigger_enable", ito::ParamBase::Int, 0, 1, 0, tr("Enable triggermode").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal= ito::Param("CamNumber", ito::ParamBase::Int | ito::ParamBase::Readonly | ito::ParamBase::NoAutosave, 0, MAX1394 - 1, 0, tr("Number of this Camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("trigger_mode", ito::ParamBase::Int, 0, 2, 0, tr("Set Triggermode").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //now create dock widget for this plugin
    DockWidgetCMU1394 *cmuwdg = new DockWidgetCMU1394(m_params, getID());
    connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), cmuwdg, SLOT(valuesChanged(QMap<QString, ito::Param>)));
    connect(cmuwdg, SIGNAL(changeParameters(QMap<QString, ito::ParamBase>)), this, SLOT(updateParameters(QMap<QString, ito::ParamBase>)));

    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, cmuwdg);

}

//----------------------------------------------------------------------------------------------------------------------------------
CMU1394::~CMU1394()
{
   m_params.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \details This method copies the complete tparam of the corresponding parameter to val

    \param [in,out] val  is a input of type::tparam containing name, value and further informations
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal CMU1394::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key = val->getName();

    if(key == "")
    {
        retValue += ito::RetVal(ito::retError, 0, tr("name of requested parameter is empty.").toAscii().data());
    }
    else
    {
        QMap<QString, ito::Param>::iterator paramIt = m_params.find(key);
        if (paramIt != m_params.end())
        {
            if(!key.compare("gain"))
            {
                unsigned short gainMin, gainMax, gainVal, dummy;
                m_pC1394gain->GetRange(&gainMin, &gainMax);
                m_pC1394gain->GetValue(&gainVal, &dummy);
                double gain = (gainVal-gainMin) / (double)(gainMax-gainMin);
                paramIt.value().setVal<double>(gain);
                *val = paramIt.value();
            }
            else if(!key.compare("offset"))
            {
                unsigned short offsMin, offsVal, offsMax, dummy;
                m_pC1394offset->GetRange(&offsMin, &offsMax);
                m_pC1394offset->GetValue(&offsVal, &dummy);
                double offset = (offsVal-offsMin) / (double)(offsMax-offsMin);
                paramIt.value().setVal<double>(offset);
                *val = paramIt.value();
            }
            else
                *val = paramIt.value();
        }
        else
        {
            retValue += ito::RetVal(ito::retError, 0, tr("parameter not found in m_params.").toAscii().data());
        }
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

   return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \detail This method copies the value of val to to the m_params-parameter and sets the corresponding camera parameters.

    \param [in] val  is a input of type::tparam containing name, value and further informations
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal CMU1394::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    int ret = 0;
    int i = 0;
    int maxxsize = 1;
    int maxysize = 1;

    int runningANDstopped = 0;

    int vbin = 0;
    int hbin = 0;
    double gain = 0.0;
    double offset = 0.0;
    
    int trigger_mode = 0;
    int trigger_on = 0;
    
    QString key = val->getName();

    if(key == "")    // Check if the key is valied
    {
        retValue += ito::RetVal(ito::retError, 0, tr("name of given parameter is empty.").toAscii().data());
    }
    else    // key valid so go on
    {
        QMap<QString, ito::Param>::iterator paramIt = m_params.find(key);    // try to find the parameter in the parameter list

        if (paramIt != m_params.end()) // Okay the camera has this parameter so go on
        {
    
            if(paramIt->getFlags() & ito::ParamBase::Readonly)
            {
                retValue += ito::RetVal(ito::retWarning, 0, tr("Parameter is read only, input ignored").toAscii().data());
                goto end;
            }
            else if(val->isNumeric() && paramIt->isNumeric())
            {
                double curval = val->getVal<double>();
                if( curval > paramIt->getMax())
                {
                    retValue += ito::RetVal(ito::retError, 0, tr("New value is larger than parameter range, input ignored").toAscii().data());
                    goto end;
                }
                else if(curval < paramIt->getMin())
                {
                    retValue += ito::RetVal(ito::retError, 0, tr("New value is smaller than parameter range, input ignored").toAscii().data());
                    goto end;
                }
                else 
                {
                    paramIt.value().setVal<double>(curval);
                }
            }
            else if (paramIt->getType() == val->getType())
            {
                retValue += paramIt.value().copyValueFrom( &(*val) );
            }
            else
            {
                retValue += ito::RetVal(ito::retError, 0, tr("Parameter type conflict").toAscii().data());
                goto end;
            }
        
            Sleep(5);

            trigger_mode = m_params["trigger_mode"].getVal<int>();
            trigger_on = m_params["trigger_enable"].getVal<int>();
            gain = (double)(m_params["gain"].getVal<double>()); 
            offset = (double)(m_params["offset"].getVal<double>());    
            int timeout_ms = (int)(m_params["timeout"].getVal<double>()*1000);    
            //vbin = m_params["binning"].getVal<int>();
            //hbin = m_params["binning"].getVal<int>();

            if(!key.compare("gain"))
            {
                unsigned short  gainMin = 0, gainMax = 0;
                m_pC1394gain->GetRange(&gainMin, &gainMax);
                m_pC1394gain->SetValue((unsigned short)((gainMax-gainMin)*gain)+gainMin, 0);
            }
            else if(!key.compare("offset"))
            {
                unsigned short offsMin, offsMax, offsoldVal, dummy;
                m_pC1394offset->GetRange(&offsMin, &offsMax);
                m_pC1394offset->GetValue(&offsoldVal, &dummy);
                unsigned short offsVal = (unsigned short)((offsMax-offsMin)*offset)+offsMin;
                ret = m_pC1394offset->SetValue(offsVal, 0);
                ret = m_ptheCamera->CaptureImage();
                if (ret)
                {
                    m_pC1394offset->SetValue(offsoldVal, 0);
                }
            }
            else 
            {
                if (grabberStartedCount())
                {
                    runningANDstopped = grabberStartedCount();
                    setGrabberStarted(1);
                    retValue += this->stopDevice(0);
                }
                if(!key.compare("trigger_mode"))
                {
                    if (m_ptheCamera->HasFeature(FEATURE_TRIGGER_MODE))
                    {
                        ret=m_pC1394trigger->SetMode((unsigned short)trigger_mode);
                    }
                    else
                        retValue = ito::RetVal(ito::retError, 0, tr("Camera has no trigger feature").toAscii().data());
                }
                else if(!key.compare("trigger_enable"))
                {
                    if (m_ptheCamera->HasFeature(FEATURE_TRIGGER_MODE))
                    {
                        if(trigger_on > 0)
                        {
                            m_pC1394trigger->SetOnOff(true);
                            if ( (ret = m_ptheCamera->StartImageAcquisitionEx( 6, timeout_ms, ACQ_START_VIDEO_STREAM)))  
                            {
                                if (ret == -14)
                                    retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed,\nmaybe video rate too high!").toAscii().data()); 
                                else if (ret==-15)
                                    retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed,\ntime out!").toAscii().data());
                                return -1;
                            }
                            m_ptheCamera->StopImageAcquisition();

                            ret = m_ptheCamera->GetNode();
                        }
                        else
                        {
                            m_pC1394trigger->SetOnOff(false);
                            if ( (ret = m_ptheCamera->StartImageAcquisition() ) )  
                            {
                                if (ret == -14)
                                    retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed,\nmaybe video rate too high!").toAscii().data()); 
                                else if (ret==-15)
                                    retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed,\ntime out!").toAscii().data());
                                return -1;
                            }
                            ret = m_ptheCamera->GetNode();
                            m_ptheCamera->StopImageAcquisition();
                        }
                    }
                    else
                        retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed,\ntime out!").toAscii().data());
                }
            }
        }
        else
        {
            retValue = ito::RetVal(ito::retWarning, 0, tr("Parameter not found").toAscii().data());
        }
    }

end:

    retValue += checkData();

    if (runningANDstopped)
    {
        retValue += this->startDevice(0);
        setGrabberStarted(runningANDstopped);
    }

    if (!retValue.containsWarningOrError())
    {
        emit parametersChanged(m_params);
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

   return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    
    ito::RetVal retValue(ito::retOk);

    int ret = 0;
    int i = 0;
    int bpp = 0;
    int numcameras = 0;
    unsigned long maxxsize = 0;
    unsigned long maxysize = 0;

    int vbin = 0;
    int hbin = 0;

    int trigger_mode = 0;
    
    double gain = 0;
    double offset = 0;

    QFile paramFile = NULL;

    m_iFireWire_VideoFormat = (*paramsMand)[0].getVal<int>();
    m_iFireWire_VideoMode = (*paramsMand)[1].getVal<int>();
    m_iFireWire_VideoRate = (*paramsMand)[2].getVal<int>();

    m_iCamNumber = (*paramsOpt)[0].getVal<int>();
    m_params["CamNumber"].setVal<int>(m_iCamNumber);

    Initnum++;  // So count up the init-number

    if(Initnum > MAX1394)
    {
        retValue = ito::RetVal(ito::retError, 0, tr("Maximal number of CMU1394 grabber already running").toAscii().data());
    }
    else if(m_iCamNumber > 127)
    {
        retValue = ito::RetVal(ito::retError, 0, tr("Invalid input for camera number").toAscii().data());
    }
    else
    {
        //Get the Initnumber / check if camera is already running  
        if(m_iCamNumber < 0) // find next free camera automatically
        {
            int freeplace = MAX1394;
            m_iCamNumber = 0;
            for(i = 0; i < MAX1394; i++)
            {
                if(InitList[i] == m_iCamNumber + 1)
                {
                    m_iCamNumber++;
                    i = 0;
                }
            }
            InitList[Initnum-1] = m_iCamNumber + 1;
            
        }
        else    // Set the next camera by user input
        {
            for(i = 0; i < MAX1394; i++)
            {
                if(InitList[i] == m_iCamNumber + 1)
                {
                    retValue = ito::RetVal(ito::retError, 0, tr("Camera already initialized").toAscii().data());
                    m_iCamNumber = -1;
                    break;
                }
            }
            InitList[Initnum-1] = m_iCamNumber + 1;
        }
    }

    if (!retValue.containsError())
    {
        if (m_ptheCamera != NULL)
        {
            delete(m_ptheCamera);
        }
        m_ptheCamera = new(C1394Camera);
    }

    if (!retValue.containsError())
    {
        if (!(m_ptheCamera->RefreshCameraList()))
            retValue = ito::RetVal(ito::retError, 0, tr("Refresh camera list failed").toAscii().data());
    }

    if (!retValue.containsError())
    {
        if ((numcameras = m_ptheCamera->GetNumberCameras())==0)
            retValue = ito::RetVal(ito::retError, 0, tr("Get number of cameras failed").toAscii().data());
    }

    if (!retValue.containsError())
    {
        if (m_ptheCamera->SelectCamera(m_iCamNumber))
            retValue = ito::RetVal(ito::retError, 0, tr("Select camera failed").toAscii().data());
    }

    m_identifier = QString("CamNum: %1").arg(m_iCamNumber);

    if (!retValue.containsError())
    {
        m_ptheCamera->InitCamera(1);
        m_ptheCamera->GetNode();
        if(m_ptheCamera->Has1394b())
            m_ptheCamera->Set1394b(1);
        m_ptheCamera->GetNode();
    }

    if (!retValue.containsError())
    {
        switch(m_iFireWire_VideoFormat)
        {
            case 0:
            {
                switch(m_iFireWire_VideoMode)
                {
                    case 4:
                        maxxsize = 640; maxysize = 480; bpp = 16;
                        break;
                    case 5:
                        maxxsize = 640; maxysize = 480; bpp = 8;
                        break;
                }
                break;
            }
            case 1:
            {
                switch(m_iFireWire_VideoMode)
                {
                    case 1:
                        maxxsize = 800; maxysize = 600; bpp = 16;
                        break;
                    case 2:
                        maxxsize = 800; maxysize = 600; bpp = 8;
                        break;
                    case 4:
                        maxxsize = 1024; maxysize = 768; bpp = 16;
                        break;
                    case 5:
                        maxxsize = 1024; maxysize = 768; bpp = 8;
                        break;
                }
                break;
            }

            case 2:
            {
                switch(m_iFireWire_VideoMode)
                {
                    case 1:
                        maxxsize = 1280; maxysize = 960; bpp = 8;
                        break;
                    case 2:
                        maxxsize = 1280; maxysize = 960; bpp = 8;
                        break;
                    case 4:
                        maxxsize = 1600; maxysize = 1200; bpp = 16;
                        break;
                    case 5:
                        maxxsize = 1600; maxysize = 1200; bpp = 8;
                        break;
                }
                break;
            }

            case 7: //basler Mode 7 geht nur mit der a602f die anderen mit a602 und 601
                //achtung! a602 nur f�r Mahr, und die benutzen einen modifizierten s->FireWire-Controllertreiber
                //und einen modifizierten Kameratreiber
                //der Standard-Treiber vom Windows f�r den Controller und der spezialtreiber vom M machen nur Mode 0 und 1
                //ansonsten ist der Cam-Mode benutzerdefiniert!
                //die Kamera hat Defaultwerte f�r Mode 7, unterst�tzt aber auch andere Gr��en wenn
                //in der entsprechenden TReiberdll ein SetROI-Befehl exportiert wird.
                {    
    /*
                    switch(s->FireWire_VideoMode)
                    {
                        case 0: //Standardmode 30 fps
                            s->xsize=640;s->ysize=480;s->depth=8;
                            break;
                        case 1: //Standardmode 60fps
                            s->xsize=640;s->ysize=480;s->depth=8;
                            break;
                        case 7: //spezialmode abhaengig vom Shutter und roi bis zu 120 fps
                            s->xsize=659;s->ysize=491;s->depth=8;
                            break;
                        default:
                                AddErrorMessage("Basler Kamera:illegaler Camera Mode! Erlaubt: 0,1,7)\n");
                            return -EHARDWARE;                
                    }
    */
                    switch(m_iFireWire_VideoMode)
                    {
                        case 0: //Standardmode 15 fps
                            maxxsize = 1624; maxysize = 1224; bpp = 8;
                            break;
                        default:
                            retValue = ito::RetVal(ito::retError, 0, tr("PtGrey Kamera:illegaler Camera Mode! Only: 0)\n").toAscii().data());
                            break;            
                    }
                }
            break;
            default:
                maxxsize = 0; maxysize = 0; bpp = 0;
                retValue = ito::RetVal(ito::retError, 0, tr("This Video/Camera mode is not implemented in this version of the firewire driver!\n").toAscii().data());
            break;
        }
        m_params["bpp"].setVal<int>(bpp);
    }

    if (!retValue.containsError())
    {
        if (m_ptheCamera->HasVideoFormat(m_iFireWire_VideoFormat))
        {
            m_ptheCamera->SetVideoFormat(m_iFireWire_VideoFormat);
        }
        else
        {
            retValue = ito::RetVal(ito::retError, 0, tr("Unable to set video format!\n").toAscii().data());
        }
    }
    if (!retValue.containsError())
    {
        if (m_ptheCamera->HasVideoMode(m_iFireWire_VideoFormat, m_iFireWire_VideoMode))
        {
            m_ptheCamera->SetVideoMode(m_iFireWire_VideoMode);
        }
        else
        {
            retValue = ito::RetVal(ito::retError, 0, tr("Unable to set video mode!\n").toAscii().data());
        }
    }
    if (!retValue.containsError())
    {
        if (m_ptheCamera->HasVideoFrameRate(m_iFireWire_VideoFormat, m_iFireWire_VideoMode, m_iFireWire_VideoRate))
        {
            m_ptheCamera->SetVideoFrameRate(m_iFireWire_VideoRate);
        }
        else
        {
            retValue = ito::RetVal(ito::retError, 0, tr("Unable to set video frame rate!\n").toAscii().data());
        }
    }

    if (!retValue.containsError())
    {
        m_ptheCamera->GetVideoFrameDimensions(&maxxsize, &maxysize);
        
        static_cast<ito::IntMeta*>( m_params["sizex"].getMeta() )->setMax(maxxsize);
        static_cast<ito::IntMeta*>( m_params["sizey"].getMeta() )->setMax(maxysize);
        m_params["sizex"].setVal<int>(maxxsize);
        m_params["sizey"].setVal<int>(maxysize);
        static_cast<ito::IntMeta*>( m_params["x0"].getMeta() )->setMax(maxxsize-1);
        static_cast<ito::IntMeta*>( m_params["y0"].getMeta() )->setMax(maxysize-1);
        m_params["x0"].setVal<int>(0);
        m_params["y0"].setVal<int>(0);
    }

    if (!retValue.containsError())
    {
        if (m_ptheCamera->HasFeature(FEATURE_GAIN))
        {
            m_pC1394gain = m_ptheCamera->GetCameraControl(FEATURE_GAIN);
            m_pC1394gain->SetAutoMode(0);
        }
        if (m_ptheCamera->HasFeature(FEATURE_SHUTTER))
        {
            m_pC1394offset = m_ptheCamera->GetCameraControl(FEATURE_SHUTTER);
            m_pC1394offset->SetAutoMode(0);
        }
        if (m_ptheCamera->HasFeature(FEATURE_AUTO_EXPOSURE))
        {
            m_pC1394autoexp = m_ptheCamera->GetCameraControl(FEATURE_AUTO_EXPOSURE);
            m_pC1394autoexp->SetAutoMode(0);
            m_pC1394autoexp->SetOnOff(0);
        }

        if (m_ptheCamera->HasFeature(FEATURE_TRIGGER_MODE))
        {
            m_pC1394trigger = m_ptheCamera->GetCameraControlTrigger();
            m_pC1394trigger->SetOnOff(false);
        }
    }
    if (!retValue.containsError())
    {
        ret = m_ptheCamera->GetNode();
    }
    if (!retValue.containsError())
    {
        if ( (ret = m_ptheCamera->StartImageAcquisition() ) )  
        {
            if (ret == -14)
            {
                retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed, maybe video rate too high!\n").toAscii().data());
            }
            else if (ret==-15)
            {
                retValue = ito::RetVal(ito::retError, 0, tr("FireWire: StartDataCapture failed, time out!\n").toAscii().data());
            }
            else
            {
                retValue = ito::RetVal(ito::retError, 0, tr("FireWire: Unknown!\n").toAscii().data());
            }
        }
        else
        {
            ret = m_ptheCamera->GetNode();
            m_ptheCamera->StopImageAcquisition();    
        }
    }

       if (!retValue.containsError())
    {
        // Camera-exposure is set in �sec, itom uses s
        trigger_mode = m_params["trigger_mode"].getVal<int>();

        unsigned short Min, Max, Val, dummy;
        m_pC1394gain->GetRange(&Min, &Max);
        m_pC1394gain->GetValue(&Val, &dummy);
        double gain = (Val-Min) / (double)(Max-Min);
        m_params["gain"].setVal<double>(offset);

        m_pC1394offset->GetRange(&Min, &Max);
        m_pC1394offset->GetValue(&Val, &dummy);
        double offset = (Val-Min) / (double)(Max-Min);
        m_params["offset"].setVal<double>(offset);
    }

//end:
    
    if (!retValue.containsError())
    {
        retValue += checkData();
    }
    if (!retValue.containsWarningOrError())
    {
        emit parametersChanged(m_params);
    }
    if (retValue.containsError())
    {

    }
    else
    {
        m_saveParamsOnClose = true;
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
        
    setInitialized(true); //init method has been finished (independent on retval)
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    
    if (m_timerID > 0)
    { 
        killTimer(m_timerID);
        m_timerID=0;
    }

    int i = 0;
    int camnum = -1;
    
    Initnum--;  // So count down the init-number
    
    if (m_ptheCamera != NULL)
    {
        camnum = m_ptheCamera->GetNode();
        this->stopDevice(0);
        Sleep(50);    
        delete(m_ptheCamera);
        m_ptheCamera = NULL;
    }

    if (m_iCamNumber!=-1)
    {
        int n = 0;
        for (n = 0; n < MAX1394; n++)
        {
            if (InitList[n] == m_iCamNumber + 1)
            {
                InitList[n] = 0;
                break;
            }
        }
        for(; n < MAX1394; n++)
        {
            InitList[n] = InitList[n+1]; 
        }
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    int num = 0;
    int ret = 0;

    if (num == 0)
    {
        int camnum = m_ptheCamera->GetNode();
        if(grabberStartedCount() < 1)
            ret = m_ptheCamera->StartImageAcquisition();
        if (ret != 0)
        {
            retValue += ito::RetVal(ito::retError, 0, tr("StartImageAcquisition failed").toAscii().data());
        }
        else
        {

            if (!retValue.containsError())
            {
                incGrabberStarted();
            }
        }
    }
    else
    {
        retValue = ito::retError;
    }
    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::stopDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    int num = 0; // In case we wand num back
    int ret = 0;
    decGrabberStarted();

    if(grabberStartedCount() < 1)
    {
        if (num == 0)
        {
            int camnum = m_ptheCamera->GetNode();
            m_ptheCamera->StopImageAcquisition();
            if (ret != 0)
            {
                retValue += ito::RetVal(ito::retError, 0, tr("StopImageAcquisition failed").toAscii().data());
            }
        }
        else
        {
            retValue = ito::retError;
        }
    }
    if(grabberStartedCount() < 0)
    {
        retValue += ito::RetVal(ito::retWarning, 0, tr("Cameraflag was < 0").toAscii().data());
        setGrabberStarted(0);
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return ito::retOk;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::acquire(const int trigger, ItomSharedSemaphore *waitCond)
{ItomSharedSemaphoreLocker locker(waitCond);

    ito::RetVal retValue(ito::retOk);
    int triggermode = m_params["trigger_mode"].getVal<int>();

    if (grabberStartedCount() <= 0)
    {
        retValue = ito::RetVal(ito::retError, 0, tr("Tried to acquire without starting device").toAscii().data());
    }
    else
    {
        this->m_isgrabbing = true;
        
        if ( m_ptheCamera->AcquireImage() )  
        {
            retValue = ito::RetVal(ito::retError, 0, tr("FireWireDll: CaptureImage failed!").toAscii().data());
        }
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();  
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::retrieveData(ito::DataObject *externalDataObject)
{
    ito::RetVal retValue(ito::retOk);

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

    if (this->m_isgrabbing == false)
    {
        retValue += ito::RetVal(ito::retWarning, 0, tr("Tried to get picture without triggering exposure").toAscii().data());
    }
    else
    {
        //here we wait until the Event is set to signaled state 
        //or the timeout runs out
    
        if (retValue != ito::retError)
        {// Now we shoud have a picture in the camera buffer

            switch (m_params["bpp"].getVal<int>())
            {
                case 8:
                {
                    unsigned char *cbuf=m_ptheCamera->GetRawData(&imglength);
                    if (curxsize == maxxsize)
                    {
                        lsrcstrpos = y0 * maxxsize;
                        if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint8>((ito::uint8*)cbuf+lsrcstrpos, maxxsize, curysize);
                        if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint8>((ito::uint8*)cbuf+lsrcstrpos, maxxsize, curysize);
                    }
                    else
                    {
                        if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint8>((ito::uint8*)cbuf, maxxsize, maxysize, x0, y0, curxsize, curysize);
                        if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint8>((ito::uint8*)cbuf, maxxsize, maxysize, x0, y0, curxsize, curysize);
                    }
                    break;
                }
                case 16:
                case 12:
                {
                    unsigned short *cbuf=(unsigned short*)m_ptheCamera->GetRawData(&imglength);
                    if (curxsize == maxxsize)
                    {
                        lsrcstrpos = y0 * maxxsize;
                        if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint16>((ito::uint16*)cbuf+lsrcstrpos, maxxsize, curysize);
                        if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint16>((ito::uint16*)cbuf+lsrcstrpos, maxxsize, curysize);
                    }
                    else
                    {
                        if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint16>((ito::uint16*)cbuf, maxxsize, maxysize, x0, y0, curxsize, curysize);
                        if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint16>((ito::uint16*)cbuf, maxxsize, maxysize, x0, y0, curxsize, curysize);
                    }
                    break;
                }
                default:
                    retValue += ito::RetVal(ito::retError, 0, tr("F Wrong picture Type").toAscii().data());
                    break;
            }
        
        }
        this->m_isgrabbing = false;
    }

    return retValue;
}


//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    retValue += retrieveData();

    if(!retValue.containsError())
    {
        sendDataToListeners(0); //don't wait for live data, since user should get the data as fast as possible.

        if(dObj)
        {
            (*dObj) = this->m_data;
        }
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal CMU1394::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
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
        sendDataToListeners(0); //don't wait for live data, since user should get the data as fast as possible.
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
// Moved to addInGrabber.cpp, equal for all grabbers / ADDA


//----------------------------------------------------------------------------------------------------------------------------------
void CMU1394::updateParameters(QMap<QString, ito::ParamBase> params)
{ 
    foreach(const ito::ParamBase &param1, params)
    {    
        setParam(QSharedPointer<ito::ParamBase>(new ito::ParamBase(param1)), NULL);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
// 1394
