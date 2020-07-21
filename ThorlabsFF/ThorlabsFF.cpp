/* ********************************************************************
    Template for a camera / grabber plugin for the software itom
    
    You can use this template, use it in your plugins, modify it,
    copy it and distribute it without any license restrictions.
*********************************************************************** */

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

#include "ThorlabsFF.h"
#include "pluginVersion.h"
#include "gitVersion.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qplugin.h>
#include <qmessagebox.h>

#include "dockWidgetThorlabsFF.h"

#include "Thorlabs.MotionControl.FilterFlipper.h"

QList<QByteArray> ThorlabsFF::openedDevices = QList<QByteArray>();

//----------------------------------------------------------------------------------------------------------------------------------
//! Constructor of Interface Class.
/*!
    \todo add necessary information about your plugin here.
*/
ThorlabsFFInterface::ThorlabsFFInterface()
{
    m_type = ito::typeDataIO | ito::typeRawIO; //any grabber is a dataIO device AND its subtype grabber (bitmask -> therefore the OR-combination).
    setObjectName("ThorlabsFF");

    m_description = QObject::tr("ThorlabsFF");

    m_detaildescription = QObject::tr("ThorlabsFF is an plugin to controll the Thorlabs Filter Flipper: \n\
\n\
* Filter Flipper (MFF101) \n\
\n\
It requires the new Kinesis driver package from Thorlabs and implements the interface Thorlabs.MotionControl.IntegratedStepperMotors.\n\
\n\
Please install the Kinesis driver package in advance with the same bit-version (32/64bit) than itom. \n\
\n\
This plugin has been tested with the flipper MFF101.");

    m_author = PLUGIN_AUTHOR;
    m_version = PLUGIN_VERSION;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = QObject::tr("licensed under LGPL");
    m_aboutThis = QObject::tr(GITVERSION); 

    m_initParamsOpt.append(ito::Param("serialNo", ito::ParamBase::String, "", tr("Serial number of the device to be loaded, if empty, the first device that can be opened will be opened").toLatin1().data()));
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Destructor of Interface Class.
/*!
    
*/
ThorlabsFFInterface::~ThorlabsFFInterface()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal ThorlabsFFInterface::getAddInInst(ito::AddInBase **addInInst)
{
    NEW_PLUGININSTANCE(ThorlabsFF) //the argument of the macro is the classname of the plugin
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal ThorlabsFFInterface::closeThisInst(ito::AddInBase **addInInst)
{
   REMOVE_PLUGININSTANCE(ThorlabsFF) //the argument of the macro is the classname of the plugin
   return ito::retOk;
}


//----------------------------------------------------------------------------------------------------------------------------------
//! Constructor of plugin.
/*!
    \todo add internal parameters of the plugin to the map m_params. It is allowed to append or remove entries from m_params
    in this constructor or later in the init method
*/
ThorlabsFF::ThorlabsFF() : AddInDataIO(),
m_opened(false)
{
    m_params.insert("name", ito::Param("name", ito::ParamBase::String | ito::ParamBase::Readonly, "ThorlabsFF", tr("name of the plugin").toLatin1().data()));
    m_params.insert("deviceName", ito::Param("deviceName", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("description of the device").toLatin1().data()));
    m_params.insert("serialNumber", ito::Param("serialNumber", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("serial number of the device").toLatin1().data()));
    
    if (hasGuiSupport())
    {
        //now create dock widget for this plugin
        DockWidgetThorlabsFF *dockWidget = new DockWidgetThorlabsFF(this);
        Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
        QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
        createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dockWidget);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
ThorlabsFF::~ThorlabsFF()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
//! initialization of plugin
ito::RetVal ThorlabsFF::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    QByteArray serialNo = paramsOpt->at(0).getVal<char*>();

    retValue += checkError(TLI_BuildDeviceList(), "build device list");
    QByteArray existingSerialNo("", 256);
    TLI_DeviceInfo deviceInfo;

    if (!retValue.containsError())
    {
        short numDevices = TLI_GetDeviceListSize();

        if (numDevices == 0)
        {
            retValue += ito::RetVal(ito::retError, 0, "no Thorlabs device detected");
        }
        else
        {
            retValue += checkError(TLI_GetDeviceListExt(existingSerialNo.data(), existingSerialNo.size()), "get device list");
        }
    }

    if (!retValue.containsError())
    {
        int idx = existingSerialNo.indexOf("\0");
        if (idx > 0)
        {
            existingSerialNo = existingSerialNo.left(idx); //serial number found 
        }

        QList<QByteArray> serialNumbers = existingSerialNo.split(','); // Thorlabs really!?

        if (serialNo == "") // no serial number input
        {
            bool found = false;
            for (int cnt = 0; cnt < serialNumbers.size(); cnt++)
            {
                if (!openedDevices.contains(serialNumbers[cnt]))
                {
                    serialNo = serialNumbers[cnt];
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                retValue += ito::RetVal(ito::retError, 0, "no free Thorlabs device found.");
            }

        }
        else // serial number input given
        {
            bool found = false;
            for each (const QByteArray &s in serialNumbers)
            {
                if (s == serialNo && s != "")
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                retValue += ito::RetVal::format(ito::retError, 0, "no device with the serial number '%s' found", serialNo.data());
            }
            else if (openedDevices.contains(serialNo))
            {
                retValue += ito::RetVal::format(ito::retError, 0, "Thorlabs device with the serial number '%s' already in use.", serialNo.data());
            }
        }
    }

    if (!retValue.containsError()) // open the device
    {
        if (TLI_GetDeviceInfo(serialNo.data(), &deviceInfo) == 0)
        {
            retValue += ito::RetVal(ito::retError, 0, "error obtaining device information.");
        }
        else
        {
            m_params["serialNumber"].setVal<char*>(serialNo.data()); // bug: deviceInfo.serialNo is sometimes wrong if more than one of the same devices are connected
            setIdentifier(QLatin1String(serialNo.data())); // bug: deviceInfo.serialNo is sometimes wrong if more than one of the same devices are connected
            m_params["deviceName"].setVal<char*>(deviceInfo.description);
        }
    }

    if (!retValue.containsError())
    {
        if (deviceInfo.isKnownType && (deviceInfo.typeID == 37 /*Filter Flipper*/))
        {
            memcpy(m_serialNo, serialNo.data(), (size_t)serialNo.size());
            retValue += checkError(FF_Open(m_serialNo), "open device");

            if (!retValue.containsError())
            {
                m_opened = true;
                openedDevices.append(m_serialNo);
                
            }
        }
        else
        {
            retValue += ito::RetVal(ito::retError, 0, "the type of the device is not among the supported devices (Long Travel Stage, Labjack, Cage Rotator)");
        }
    }
    

    if (!retValue.containsError())
    {
        emit parametersChanged(m_params);
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
//! shutdown of plugin
ito::RetVal ThorlabsFF::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    
    //todo:
    // - disconnect the device if not yet done
    // - this funtion is considered to be the "inverse" of init.

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal ThorlabsFF::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString,ito::Param>::iterator it;

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if (retValue == ito::retOk)
    {
        //gets the parameter key from m_params map (read-only is allowed, since we only want to get the value).
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if (!retValue.containsError())
    {
        //put your switch-case.. for getting the right value here

        //finally, save the desired value in the argument val (this is a shared pointer!)
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
ito::RetVal ThorlabsFF::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName( val->getName(), key, hasIndex, index, suffix );

    if (!retValue.containsError())
    {
        //gets the parameter key from m_params map (read-only is not allowed and leads to ito::retError).
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if (!retValue.containsError())
    {
        //here the new parameter is checked whether its type corresponds or can be cast into the
        // value in m_params and whether the new type fits to the requirements of any possible
        // meta structure.
        retValue += apiValidateParam(*it, *val, false, true);
    }

    if (!retValue.containsError())
    {
        if (key == "demoKey1")
        {
            //check the new value and if ok, assign it to the internal parameter
            retValue += it->copyValueFrom( &(*val) );
        }
        else if (key == "demoKey2")
        {
            //check the new value and if ok, assign it to the internal parameter
            retValue += it->copyValueFrom( &(*val) );
        }
        else
        {
            //all parameters that don't need further checks can simply be assigned
            //to the value in m_params (the rest is already checked above)
            retValue += it->copyValueFrom( &(*val) );
        }
    }

    if (!retValue.containsError())
    {
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
void ThorlabsFF::dockWidgetVisibilityChanged(bool visible)
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
const ito::RetVal ThorlabsFF::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogThorlabsFF(this));
}

ito::RetVal ThorlabsFF::checkError(short value, const char* message)
{
    if (value == 0)
    {
        return ito::retOk;
    }
    else
    {
        switch (value)
        {
        case 1:
            return ito::RetVal::format(ito::retError, 1, "%s: The FTDI functions have not been initialized.", message);
        case 2:
            return ito::RetVal::format(ito::retError, 1, "%s: The device could not be found.", message);
        case 3:
            return ito::RetVal::format(ito::retError, 1, "%s: The device must be opened before it can be accessed.", message);
        case 37:
            return ito::RetVal::format(ito::retError, 1, "%s: The device cannot perform the function until it has been homed (call calib() before).", message);
        case 38:
            return ito::RetVal::format(ito::retError, 1, "%s: The function cannot be performed as it would result in an illegal position.", message);
        case 39:
            return ito::RetVal::format(ito::retError, 1, "%s: An invalid velocity parameter was supplied. The velocity must be greater than zero. ", message);
        default:
            return ito::RetVal::format(ito::retError, value, "%s: unknown error %i.", message, value);
        }
    }
}