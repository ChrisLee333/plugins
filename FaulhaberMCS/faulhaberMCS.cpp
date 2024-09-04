/* ********************************************************************
    Plugin "FaulhaberMCS" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2024, Institut für Technische Optik (ITO),
    Universität Stuttgart, Germany

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

#define NOMINMAX

#include "faulhaberMCS.h"
#include "common/helperCommon.h"
#include "gitVersion.h"
#include "pluginVersion.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qplugin.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qwaitcondition.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include "dockWidgetFaulhaberMCS.h"

QList<ito::uint8> FaulhaberMCS::openedNodes = QList<ito::uint8>();

//----------------------------------------------------------------------------------------------------------------------------------
FaulhaberMCSInterface::FaulhaberMCSInterface()
{
    m_type = ito::typeActuator;
    setObjectName("FaulhaberMCS");

    m_description = QObject::tr("FaulhaberMCS");

    m_detaildescription =
        QObject::tr("This plugin is an actuator plugin to control servo motors from Faulhaber.\n\
\n\
It was implemented for RS232 communication and tested with:\n\
\n\
* Serie MCS 3242: https://www.faulhaber.com/de/produkte/serie/mcs-3242bx4-et/");

    m_author = PLUGIN_AUTHOR;
    m_version = PLUGIN_VERSION;
    m_minItomVer = PLUGIN_MIN_ITOM_VERSION;
    m_maxItomVer = PLUGIN_MAX_ITOM_VERSION;
    m_license = QObject::tr(PLUGIN_LICENCE);
    m_aboutThis = QObject::tr(GITVERSION);

    ito::Param paramVal(
        "serialIOInstance",
        ito::ParamBase::HWRef | ito::ParamBase::In,
        nullptr,
        tr("An opened serial port of 'SerialIO' plugin instance.").toLatin1().data());
    paramVal.setMeta(new ito::HWMeta("SerialIO"), true);
    m_initParamsMand.append(paramVal);

    paramVal = ito::Param(
        "Node",
        ito::ParamBase::Int | ito::ParamBase::In,
        1,
        new ito::IntMeta(1, std::numeric_limits<int>::max(), 1, "Communication"),
        tr("Node number of device.").toLatin1().data());
    m_initParamsMand.append(paramVal);
}

//----------------------------------------------------------------------------------------------------------------------------------
FaulhaberMCSInterface::~FaulhaberMCSInterface()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCSInterface::getAddInInst(ito::AddInBase** addInInst)
{
    NEW_PLUGININSTANCE(FaulhaberMCS)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCSInterface::closeThisInst(ito::AddInBase** addInInst)
{
    REMOVE_PLUGININSTANCE(FaulhaberMCS)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
FaulhaberMCS::FaulhaberMCS() :
    AddInActuator(), m_delayAfterSendCommandMS(50), m_async(0), m_numOfAxes(1), m_node(1),
    m_statusWord(0), m_requestTimeOutMS(5000), m_waitForDoneTimeout(60000),
    m_waitForMCSTimeout(3000), m_nodeAppended(false)
{
    ito::Param paramVal(
        "name", ito::ParamBase::String | ito::ParamBase::Readonly, "FaulhaberMCS", nullptr);
    m_params.insert(paramVal.getName(), paramVal);

    //------------------------------- category general device parameter
    //---------------------------//
    paramVal = ito::Param(
        "serialNumber",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Serial number of device.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "deviceName",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Name of device.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "vendorID",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Vendor ID of device.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "productCode",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Product code number.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "revisionNumber",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Revision number.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "firmware",
        ito::ParamBase::String | ito::ParamBase::Readonly,
        "",
        tr("Firmware version.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::String, "", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "ambientTemperature",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        tr("Ambient Temperature.").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, std::numeric_limits<int>::max(), 1, "General"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "operationMode",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        -4,
        10,
        1,
        tr("Operation Mode. -4: ATC, -3: AVC, -2: APC, -1: Voltage mode, 0: Controller not "
           "activated, 1: PP (default), 3: PV, 6: Homing, 8: CSP, 9: CSV, 10: CST.")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(-4, 10, 1, "General"));
    m_params.insert(paramVal.getName(), paramVal);

    //------------------------------- category movement ---------------------------//
    paramVal = ito::Param(
        "async",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        m_async,
        tr("Asynchronous move (1), synchronous (0) [default]. Only synchronous operation is "
           "implemented.")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Movement"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "operation",
        ito::ParamBase::Int,
        0,
        1,
        m_async,
        tr("Enable (1) or Disable (0) operation.").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Movement"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "power",
        ito::ParamBase::Int,
        0,
        1,
        m_async,
        tr("Enable (1) or Disable (0) device power.").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Movement"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "homed",
        ito::ParamBase::Int,
        0,
        1,
        m_async,
        tr("homed (1) or not homed (0).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Movement"));
    m_params.insert(paramVal.getName(), paramVal);

    //------------------------------- category Statusword ---------------------------//
    paramVal = ito::Param(
        "statusWord",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        65535,
        0,
        tr("16bit statusWord to CiA 402. It indicates the status of the drive unit.")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 65535, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "readyToSwitchOn",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Ready to switch ON, 0: Not ready to switch ON (Bit 0).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "switchedOn",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Drive is in the 'Switched ON' state, 0: No voltage present (Bit 1).")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "operationEnabled",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Operation enabled, 0: Operation disabled (Bit 2).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "fault",
        ito::ParamBase::Int,
        0,
        1,
        0,
        tr("1: Error present, 0: No error present (Bit 3).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "voltageEnabled",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Power supply enabled, 0: Power supply disabled (Bit 4).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "quickStop",
        ito::ParamBase::Int,
        0,
        1,
        0,
        tr("1: Quick stop enabled, Quick stop disabled (Bit 5).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "switchOnDisabled",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Switch on disabled, 0: Switch on enabled (Bit 6).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "warning",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: One of the monitored temperatures has exceeded at least the warning threshold, 0: "
           "No raised temperatures (Bit 7).")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "targetReached",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Target has reached, 0: is moving (Bit 10).").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "internalLimitActive",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Internal range limit (e.g. limit switch reached), 0: Internal range limit not "
           "reached (Bit 11).")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "setPointAcknowledged",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: New set-point has been loaded, 0: Previous set-point being changed or already "
           "reached (Bit 12).")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "followingError",
        ito::ParamBase::Int | ito::ParamBase::Readonly,
        0,
        1,
        0,
        tr("1: Permissible range for the following error exceeded, 0: The actual position follows "
           "the instructions without a following error (Bit 13).")
            .toLatin1()
            .data());
    paramVal.setMeta(new ito::IntMeta(0, 1, 1, "Status"));
    m_params.insert(paramVal.getName(), paramVal);


    //------------------------------- category Motion control ---------------------------//
    paramVal = ito::Param(
        "maxMotorSpeed", ito::ParamBase::Int, 0, tr("Max motor speed in 1/min.").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(1, 32767, 1, "Motion control"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "profileVelocity",
        ito::ParamBase::Int,
        0,
        tr("Profile velocity in 1/min.").toLatin1().data());
    paramVal.setMeta(new ito::IntMeta(1, 32767, 1, "Motion control"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "acceleration", ito::ParamBase::Int, 0, tr("Acceleration in 1/s².").toUtf8().data());
    paramVal.setMeta(new ito::IntMeta(1, 30000, 1, "Motion control"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "deceleration", ito::ParamBase::Int, 0, tr("Deceleration in 1/s².").toUtf8().data());
    paramVal.setMeta(new ito::IntMeta(1, 30000, 1, "Motion control"));
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param(
        "quickStopDeceleration",
        ito::ParamBase::Int,
        0,
        tr("Quickstop deceleration in 1/s².").toUtf8().data());
    paramVal.setMeta(new ito::IntMeta(1, 32750, 1, "Motion control"));
    m_params.insert(paramVal.getName(), paramVal);

    int limits[] = {6000, 6000};
    paramVal = ito::Param(
        "torqueLimits",
        ito::ParamBase::IntArray,
        2,
        limits,
        tr("Torque limit values (negative, positive).").toLatin1().data());
    paramVal.setMeta(new ito::IntArrayMeta(0, 6000, 1, "Torque control"), true);
    m_params.insert(paramVal.getName(), paramVal);

    //------------------------------------------------- EXEC FUNCTIONS
    QVector<ito::Param> pMand = QVector<ito::Param>();
    QVector<ito::Param> pOpt = QVector<ito::Param>();
    QVector<ito::Param> pOut = QVector<ito::Param>();

    paramVal = ito::Param(
        "method",
        ito::ParamBase::Int | ito::ParamBase::In,
        -4,
        37,
        0,
        tr("Homing method. Methods 1…34: A limit switch or an additional reference switch is used "
           "as reference. Method 37: The position is set to 0 without reference run. Methods "
           "–1…–4: A mechanical limit stop is set as reference.")
            .toLatin1()
            .data());
    pMand.append(paramVal);

    paramVal = ito::Param(
        "offset",
        ito::ParamBase::Int | ito::ParamBase::In,
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max(),
        0,
        tr("Offset of the zero position relative to the position of the reference switch in "
           "userdefined units.")
            .toLatin1()
            .data());
    pOpt.append(paramVal);

    paramVal = ito::Param(
        "switchSeekVelocity",
        ito::ParamBase::Int | ito::ParamBase::In,
        1,
        32767,
        400,
        tr("Speed during search for switch.").toLatin1().data());
    pOpt.append(paramVal);

    paramVal = ito::Param(
        "homingSpeed",
        ito::ParamBase::Int | ito::ParamBase::In,
        1,
        32767,
        50,
        tr("Speed during search for zero.").toLatin1().data());
    pOpt.append(paramVal);

    paramVal = ito::Param(
        "acceleration",
        ito::ParamBase::Int | ito::ParamBase::In,
        1,
        30000,
        400,
        tr("Speed during search for zero.").toLatin1().data());
    pOpt.append(paramVal);

    paramVal = ito::Param(
        "limitCheckDelayTime",
        ito::ParamBase::Int | ito::ParamBase::In,
        0,
        32750,
        10,
        tr("Delay time until blockage detection [ms].").toLatin1().data());
    pOpt.append(paramVal);

    int torqueLimits[] = {1000, 1000};
    paramVal = ito::Param(
        "torqueLimits",
        ito::ParamBase::IntArray | ito::ParamBase::In,
        2,
        torqueLimits,
        new ito::IntArrayMeta(0, 300, 1, "Torque control"),
        tr("Homing torque limit values (negative, positive).").toLatin1().data());
    pOpt.append(paramVal);

    registerExecFunc(
        "homing",
        pMand,
        pOpt,
        pOut,
        tr("In most of the cases before position control is to be used, the drive must perform a "
           "reference run to align the position used by the drive to the mechanic setup.")
            .toLatin1()
            .data());
    pMand.clear();
    pOpt.clear();
    pOut.clear();

    // initialize the current position vector, the status vector and the target position vector
    m_currentPos.fill(0.0, m_numOfAxes);
    m_currentStatus.fill(0, m_numOfAxes);
    m_targetPos.fill(0.0, m_numOfAxes);

    // the following lines create and register the plugin's dock widget. Delete these lines if the
    // plugin does not have a dock widget.
    DockWidgetFaulhaberMCS* dw = new DockWidgetFaulhaberMCS(getID(), this);

    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char*>()), features, areas, dw);
}

//----------------------------------------------------------------------------------------------------------------------------------
FaulhaberMCS::~FaulhaberMCS()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::init(
    QVector<ito::ParamBase>* paramsMand,
    QVector<ito::ParamBase>* paramsOpt,
    ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (reinterpret_cast<ito::AddInBase*>((*paramsMand)[0].getVal<void*>())
            ->getBasePlugin()
            ->getType() &
        (ito::typeDataIO | ito::typeRawIO))
    {
        // Us given SerialIO instance
        m_pSerialIO = (ito::AddInDataIO*)(*paramsMand)[0].getVal<void*>();

        QSharedPointer<ito::Param> val(new ito::Param("port"));
        retValue += m_pSerialIO->getParam(val, NULL);

        if (!retValue.containsError())
        {
            m_port = val->getVal<int>();
        }
        m_node = (uint8_t)paramsMand->at(1).getVal<int>();
        if (openedNodes.contains(m_node))
        {
            retValue += ito::RetVal(
                ito::retError,
                0,
                tr("An instance of noder number '%1' is already open.")
                    .arg(m_node)
                    .toLatin1()
                    .data());
        }
        else
        {
            ito::uint8 node;
            retValue += getNodeID(node);
            if (!retValue.containsError())
            {
                if (node != m_node)
                {
                    retValue = ito::RetVal(
                        ito::retError,
                        0,
                        tr("The node number of the device is '%1' and not '%2'.")
                            .arg(node)
                            .arg(m_node)
                            .toLatin1()
                            .data());
                }
                else
                {
                    openedNodes.append(m_node);
                    m_nodeAppended = true;
                }
            }
            else
            {
                retValue = ito::RetVal(
                    ito::retError,
                    999,
                    tr("No device found for serialIO port '%1' and node '%2'.")
                        .arg(m_port)
                        .arg(m_node)
                        .toLatin1()
                        .data());
            }
        }
    }
    else
    {
        retValue += ito::RetVal(
            ito::retError,
            0,
            tr("Input parameter is not a dataIO instance of the SerialIO Plugin!")
                .toLatin1()
                .data());
    }

    if (!retValue.containsError())
    {
        QSharedPointer<QVector<ito::ParamBase>> _dummy;
        m_pSerialIO->execFunc("clearInputBuffer", _dummy, _dummy, _dummy, nullptr);
        m_pSerialIO->execFunc("clearOutputBuffer", _dummy, _dummy, _dummy, nullptr);
    }


    // ENABLE
    if (!retValue.containsError())
    {
        int EnState = 0; // Reset the local step counter

        // Initial check of the status word
        retValue += updateStatusMCS();
        const ito::uint16 statusOperationEnabled = 0x27;
        const ito::uint16 statusSwitchOnDisabled = 0x40;
        const ito::uint16 statusQuickStop = 0x07;

        if (retValue.containsError())
        {
            return retValue;
        }

        const ito::uint16 statusMask = 0x6F;
        ito::uint16 status = m_statusWord & statusMask;

        // Check for being in stopped mode
        if (status == statusQuickStop)
        {
            enableOperation(); // Enable Operation
            EnState = 1;
        }
        else if (status == statusOperationEnabled) // Drive is already
                                                   // enabled
        {
            EnState = 2;
        }
        else if (status != statusSwitchOnDisabled) // Otherwise, it's safe to
                                                   // disable first
        {
            // We need to send a shutdown first
            disableVoltage(); // Controlword = CiACmdDisableVoltage
        }

        QElapsedTimer timer;
        bool timeout = false;
        QMutex waitMutex;
        QWaitCondition waitCondition;

        // Loop until the drive is enabled
        timer.start();
        while (EnState != 2 && !timeout)
        {
            retValue += updateStatusMCS();
            if (retValue.containsError())
            {
                return retValue;
            }

            status = m_statusWord & statusMask; // Cyclically check the status word

            if (EnState == 0)
            {
                if (status == 0x40)
                {
                    // Send the enable signature
                    shutDown(); // CiACmdShutdown
                    enableOperation(); // CiACmdEnableOperation
                    EnState = 1;
                }
            }
            else if (EnState == 1)
            {
                // Wait for enabled
                if (status == statusOperationEnabled)
                {
                    EnState = 2;
                }
            }

            // short delay
            waitMutex.lock();
            waitCondition.wait(&waitMutex, 10);
            waitMutex.unlock();

            if (timer.hasExpired(5000)) // timeout during movement
            {
                timeout = true;
                retValue +=
                    ito::RetVal(ito::retError, 9999, "Timeout occurred during initialization.");
            }
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getSerialNumber(answerString);
        if (!retValue.containsError())
#
        {
            m_params["serialNumber"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getDeviceName(answerString);
        if (!retValue.containsError())
        {
            m_params["deviceName"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getVendorID(answerString);
        if (!retValue.containsError())
        {
            m_params["vendorID"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getProductCode(answerString);
        if (!retValue.containsError())
        {
            m_params["productCode"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getRevisionNumber(answerString);
        if (!retValue.containsError())
        {
            m_params["revisionNumber"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        QString answerString;
        retValue += getFirmware(answerString);
        if (!retValue.containsError())
        {
            m_params["firmware"].setVal<char*>(answerString.toUtf8().data());
        }
    }

    if (!retValue.containsError())
    {
        ito::uint8 temp;
        retValue += getAmbientTemperature(temp);
        if (!retValue.containsError())
        {
            m_params["ambientTemperature"].setVal<int>(temp);
        }
    }

    if (!retValue.containsError())
    {
        ito::int8 mode;
        retValue +=
            setOperationMode(static_cast<ito::int8>(m_params["operationMode"].getVal<int>()));
        if (!retValue.containsError())
        {
            retValue += getOperationMode(mode);
            if (!retValue.containsError())
            {
                m_params["operationMode"].setVal<int>(mode);
            }
        }
    }

    if (!retValue.containsError())
    {
        ito::uint32 speed;
        retValue += getMaxMotorSpeed(speed);
        if (!retValue.containsError())
        {
            m_params["maxMotorSpeed"].setVal<int>(speed);
        }
    }

    if (!retValue.containsError())
    {
        ito::uint32 acceleration;
        retValue += getAcceleration(acceleration);
        if (!retValue.containsError())
        {
            m_params["acceleration"].setVal<int>(acceleration);
        }
    }

    if (!retValue.containsError())
    {
        ito::uint32 deceleration;
        retValue += getDeceleration(deceleration);
        if (!retValue.containsError())
        {
            m_params["deceleration"].setVal<int>(deceleration);
        }
    }

    if (!retValue.containsError())
    {
        ito::uint32 speed;
        retValue += getProfileVelocity(speed);
        if (!retValue.containsError())
        {
            m_params["profileVelocity"].setVal<int>(speed);
        }
    }

    if (!retValue.containsError())
    {
        ito::uint32 quick;
        retValue += getQuickStopDeceleration(quick);
        if (!retValue.containsError())
        {
            m_params["quickStopDeceleration"].setVal<int>(quick);
        }
    }

    if (!retValue.containsError())
    {
        ito::uint16 limits[] = {0, 0};
        retValue += getTorqueLimits(limits);

        int intLimits[sizeof(limits) / sizeof(limits[0])];

        for (int i = 0; i < sizeof(limits) / sizeof(limits[0]); ++i)
        {
            intLimits[i] = (int)limits[i];
        }

        if (!retValue.containsError())
        {
            m_params["torqueLimits"].setVal<int*>(intLimits, 2);
        }
    }

    if (!retValue.containsError())
    {
        retValue += updateStatusMCS();

        ito::int32 pos;
        retValue += getPosMCS(pos);

        m_currentPos[0] = static_cast<double>(pos);
        retValue += getTargetPosMCS(pos);
        m_targetPos[0] = static_cast<double>(pos);
        m_currentStatus[0] = ito::actuatorAtTarget | ito::actuatorEnabled | ito::actuatorAvailable;
        retValue += updateStatusMCS();
        sendStatusUpdate(false);
        emit parametersChanged(m_params);
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    setInitialized(true);
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::close(ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (m_nodeAppended)
    {
        openedNodes.removeOne(m_node);
        if (openedNodes.isEmpty())
        {
            int DiState = 0; // Reset the local step counter
            const ito::uint16 statusOperationEnabled = 0x27;

            retValue += updateStatusMCS();
            const ito::uint16 CiAStatusMask = 0x6F;
            ito::uint16 status = m_statusWord & CiAStatusMask;

            if (status == statusOperationEnabled)
            {
                // Send a shutdown command first to stop the motor
                disable(); // CiACmdDisable
                DiState = 1;
            }
            else
            {
                // Otherwise, the disable voltage is the next command
                // Out of quick-stop or switched on.
                DiState = 2;
            }

            QElapsedTimer timer;
            QMutex waitMutex;
            QWaitCondition waitCondition;
            bool timeout = false;

            // Loop until the disable operation is complete
            timer.start();
            while (DiState != 4 && !timeout)
            {
                retValue += updateStatusMCS();
                if (retValue.containsError())
                {
                    return retValue;
                }

                status = m_statusWord & CiAStatusMask; // Cyclically check the status

                if (DiState == 1)
                {
                    if (status == 0x23)
                    {
                        // Only now it's safe to send the disable voltage command
                        DiState = 2;
                    }
                }
                else if (DiState == 2)
                {
                    // Send the disable voltage command
                    disableVoltage(); // CiACmdDisableVoltage
                    DiState = 3;
                }
                else if (DiState == 3)
                {
                    // Wait for final state
                    if (status == 0x40)
                    {
                        DiState = 4;
                    }
                }

                // Optional: Include a small delay to prevent busy looping
                waitMutex.lock();
                waitCondition.wait(&waitMutex, 10);
                waitMutex.unlock();

                if (timer.hasExpired(5000)) // timeout during movement
                {
                    timeout = true;
                    retValue +=
                        ito::RetVal(ito::retError, 9999, "Timeout occurred during closing.");
                }
            }
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
ito::RetVal FaulhaberMCS::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;

    // parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if (retValue == ito::retOk)
    {
        // gets the parameter key from m_params map (read-only is allowed, since we only want to
        // get the value).
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if (!retValue.containsError())
    {
        int answerInteger = 0;
        if (key == "operationMode")
        {
            ito::int8 mode;
            retValue += getOperationMode(mode);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(mode);
            }
        }
        else if (key == "ambientTemperature")
        {
            ito::uint8 temp;
            retValue += getAmbientTemperature(temp);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(temp);
            }
        }
        else if (key == "maxMotorSpeed")
        {
            ito::uint32 speed;
            retValue += getMaxMotorSpeed(speed);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(speed);
            }
        }
        else if (key == "profileVelocity")
        {
            ito::uint32 speed;
            retValue += getProfileVelocity(speed);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(speed);
            }
        }
        else if (key == "acceleration")
        {
            ito::uint32 acceleration;
            retValue += getAcceleration(acceleration);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(acceleration);
            }
        }
        else if (key == "deceleration")
        {
            ito::uint32 deceleartion;
            retValue += getDeceleration(deceleartion);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(deceleartion);
            }
        }
        else if (key == "quickStopDeceleration")
        {
            ito::uint32 quick;
            retValue += getQuickStopDeceleration(quick);
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(quick);
            }
        }
        else if (key == "statusWord")
        {
            retValue += updateStatusMCS();
            if (!retValue.containsError())
            {
                retValue += it->setVal<int>(m_statusWord);
            }
        }
        else if (key == "torqueLimits")
        {
            ito::uint16 limits[] = {0, 0};
            retValue += getTorqueLimits(limits);

            int int_limits[sizeof(limits) / sizeof(limits[0])];

            // Convert from uint16_t to int
            for (int i = 0; i < sizeof(limits) / sizeof(limits[0]); ++i)
            {
                int_limits[i] = (int)limits[i];
            }

            if (!retValue.containsError())
            {
                retValue += it->setVal<int*>(int_limits, 2);
            }
        }

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
ito::RetVal FaulhaberMCS::setParam(
    QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;

    // parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if (isMotorMoving()) // this if-case is for actuators only.
    {
        retValue += ito::RetVal(
            ito::retError,
            0,
            tr("any axis is moving. Parameters cannot be set.").toLatin1().data());
    }

    if (!retValue.containsError())
    {
        // gets the parameter key from m_params map (read-only is not allowed and leads to
        // ito::retError).
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if (!retValue.containsError())
    {
        // here the new parameter is checked whether its type corresponds or can be cast into
        // the
        //  value in m_params and whether the new type fits to the requirements of any possible
        //  meta structure.
        retValue += apiValidateParam(*it, *val, false, true);
    }

    if (!retValue.containsError())
    {
        if (key == "operationMode")
        {
            retValue += setOperationMode(val->getVal<int>());
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "operation")
        {
            int operation = val->getVal<int>();
            if (operation == 0)
            {
                disableOperation();
            }
            else if (operation == 1)
            {
                enableOperation();
            }
            else
            {
                retValue += ito::RetVal(
                    ito::retError,
                    0,
                    tr("Value (%1) of parameter 'operation' must be 0 or 1.")
                        .arg(operation)
                        .toLatin1()
                        .data());
            }

            retValue += it->copyValueFrom(&(*val));
        }
        else if (key == "power")
        {
            int power = val->getVal<int>();
            if (power == 0)
            {
                shutDown();
                updateStatusMCS();
            }
            else if (power == 1)
            {
                switchOn();
                updateStatusMCS();
            }
            else
            {
                retValue += ito::RetVal(
                    ito::retError,
                    0,
                    tr("Value (%1) of parameter 'power' must be 0 or 1.")
                        .arg(power)
                        .toLatin1()
                        .data());
            }
            retValue += it->copyValueFrom(&(*val));
        }
        else if (key == "fault")
        {
            if (val->getVal<int>())
            {
                faultReset();
                updateStatusMCS();
            }
            else
            {
                retValue += ito::RetVal(
                    ito::retError,
                    0,
                    tr("Set the parameter value to 1 for fault reset.").toLatin1().data());
            }
        }
        else if (key == "maxMotorSpeed")
        {
            retValue += setMaxMotorSpeed(static_cast<ito::uint32>(val->getVal<int>()));
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "profileVelocity")
        {
            retValue += setProfileVelocity(static_cast<ito::uint32>(val->getVal<int>()));
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "acceleration")
        {
            retValue += setAcceleration(static_cast<ito::uint32>(val->getVal<int>()));
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "deceleration")
        {
            retValue += setDeceleration(static_cast<ito::uint32>(val->getVal<int>()));
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "quickStopDeceleration")
        {
            retValue += setQuickStopDeceleration(static_cast<ito::uint32>(val->getVal<int>()));
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else if (key == "torqueLimits")
        {
            int posLimits = val->getVal<int*>()[0];
            int negLimits = val->getVal<int*>()[1];

            ito::uint16 limits[] = {
                static_cast<ito::uint16>(posLimits), static_cast<ito::uint16>(negLimits)};

            retValue += setTorqueLimits(limits);
            if (!retValue.containsError())
            {
                retValue += it->copyValueFrom(&(*val));
            }
        }
        else
        {
            // all parameters that don't need further checks can simply be assigned
            // to the value in m_params (the rest is already checked above)
            retValue += it->copyValueFrom(&(*val));
        }
    }

    if (!retValue.containsError())
    {
        emit parametersChanged(
            m_params); // send changed parameters to any connected dialogs or dock-widgets
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! calib
/*!
    the given axis should be calibrated (e.g. by moving to a reference switch).
*/
ito::RetVal FaulhaberMCS::calib(const int axis, ItomSharedSemaphore* waitCond)
{
    return calib(QVector<int>(1, axis), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! calib
/*!
    the given axes should be calibrated (e.g. by moving to a reference switch).
*/
ito::RetVal FaulhaberMCS::calib(const QVector<int> axis, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;


    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::readRegister(
    const ito::uint16& address, const ito::uint8& subindex, QByteArray& response)
{
    ito::RetVal retValue = ito::retOk;

    std::vector<ito::uint8> command = {
        m_node,
        m_GET,
        static_cast<ito::uint8>(address & 0xFF),
        static_cast<ito::uint8>(address >> 8),
        subindex};
    std::vector<ito::uint8> fullCommand = {static_cast<ito::uint8>(command.size() + 2)};
    fullCommand.insert(fullCommand.end(), command.begin(), command.end());

    QByteArray CRC(reinterpret_cast<const char*>(fullCommand.data()), fullCommand.size());

    fullCommand.push_back(calculateChecksum(CRC));
    fullCommand.insert(fullCommand.begin(), m_S);
    fullCommand.push_back(m_E);

    QByteArray data(reinterpret_cast<char*>(fullCommand.data()), fullCommand.size());

    return sendCommandAndGetResponse(data, response);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::setControlWord(const ito::uint16 word)
{
    setRegister<ito::uint16>(0x6040, 0x00, word, 2);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::start()
{
    setControlWord(0x0A);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::stop()
{
    setControlWord(0x0B);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::resetCommunication()
{
    setControlWord(0x06);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::startAll()
{
    setControlWord(0x0F);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::shutDown()
{
    setControlWord(0x06);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::switchOn()
{
    setControlWord(0x07);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::enableOperation()
{
    setControlWord(0x0F);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::disable()
{
    setControlWord(0x07);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::disableOperation()
{
    setControlWord(0x14);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::disableVoltage()
{
    setControlWord(0x00);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::quickStop()
{
    setControlWord(0x13);
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::faultReset()
{
    setControlWord(0x16);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::homingCurrentPosToZero(const int& axis)
{
    ito::RetVal retValue = ito::retOk;
    QElapsedTimer timer;
    timer.start();
    QMutex waitMutex;
    QWaitCondition waitCondition;
    retValue += setOperationMode(static_cast<ito::uint8>(6));

    if (retValue.containsError())
    {
        retValue += ito::RetVal(
            ito::retError, 0, tr("Could not set operation mode to Homing (6).").toLatin1().data());
    }

    if (!retValue.containsError())
    {
        retValue += setHomingMode(static_cast<ito::int8>(37));

        // Start homing
        setControlWord(0x000F);
        setControlWord(0x001F);

        while (!retValue.containsWarningOrError())
        {
            waitMutex.lock();
            waitCondition.wait(&waitMutex, m_delayAfterSendCommandMS);
            waitMutex.unlock();
            setAlive();

            retValue += updateStatusMCS();

            if (m_statusWord && m_params["setPointAcknowledged"].getVal<int>()) // homing successful
            {
                break;
            }
            else if (m_statusWord && m_params["followingError"].getVal<int>()) // error during
                                                                               // homing
            {
                retValue += ito::RetVal(
                    ito::retError, 0, tr("Error occurs during homing").toLatin1().data());
            }

            // Timeout during movement
            if (timer.hasExpired(m_waitForMCSTimeout))
            {
                retValue += ito::RetVal(
                    ito::retError,
                    9999,
                    QString("Timeout occurred during homing").toLatin1().data());
                break;
            }
        }

        ito::int32 currentPos;
        ito::int32 targetPos;
        retValue += getPosMCS(currentPos);
        m_currentPos[axis] = static_cast<double>(currentPos);

        retValue += getTargetPosMCS(targetPos);
        m_targetPos[axis] = static_cast<double>(targetPos);

        if (retValue.containsError())
            return retValue;
    }

    retValue += setOperationMode(static_cast<ito::uint8>(1));
    retValue += updateStatusMCS();
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setOrigin
/*!
    the given axis should be set to origin. That means (if possible) its current position should
   be considered to be the new origin (zero-position). If this operation is not possible, return
   a warning.
*/
ito::RetVal FaulhaberMCS::setOrigin(const int axis, ItomSharedSemaphore* waitCond)
{
    return setOrigin(QVector<int>(1, axis), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setOrigin
/*!
    the given axes should be set to origin. That means (if possible) their current position
   should be considered to be the new origin (zero-position). If this operation is not possible,
   return a warning.
*/
ito::RetVal FaulhaberMCS::setOrigin(QVector<int> axis, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (isMotorMoving())
    {
        retValue +=
            ito::RetVal(ito::retError, 0, tr("Any motor axis is already moving").toLatin1().data());

        if (waitCond)
        {
            waitCond->release();
            waitCond->returnValue = retValue;
        }
    }
    else
    {
        foreach (const int& i, axis)
        {
            retValue += homingCurrentPosToZero(i);
            setStatus(
                m_currentStatus[i],
                ito::actuatorAtTarget,
                ito::actSwitchesMask | ito::actStatusMask);
        }

        if (waitCond)
        {
            waitCond->returnValue = retValue;
            waitCond->release();
        }

        sendStatusUpdate();
        sendTargetUpdate();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! getStatus
/*!
    re-checks the status (current position, available, end switch reached, moving, at target...)
   of all axes and returns the status of each axis as vector. Each status is an or-combination
   of the enumeration ito::tActuatorStatus.
*/
ito::RetVal FaulhaberMCS::getStatus(
    QSharedPointer<QVector<int>> status, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    retValue += updateStatus();
    *status = m_currentStatus;

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! getPos
/*!
    returns the current position (in mm or degree) of the given axis
*/
ito::RetVal FaulhaberMCS::getPos(
    const int axis, QSharedPointer<double> pos, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    QSharedPointer<QVector<double>> pos2(new QVector<double>(1, 0.0));
    ito::RetVal retValue =
        getPos(QVector<int>(1, axis), pos2, nullptr); // forward to multi-axes version
    *pos = (*pos2)[0];

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! getPos
/*!
    returns the current position (in mm or degree) of all given axes
*/
ito::RetVal FaulhaberMCS::getPos(
    QVector<int> axis, QSharedPointer<QVector<double>> pos, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    foreach (const int i, axis)
    {
        if (i >= 0 && i < m_numOfAxes)
        {
            ito::int32 intPos;
            retValue += getPosMCS(intPos);
            if (!retValue.containsError())
            {
                m_currentPos[i] =
                    static_cast<double>(intPos); // set m_currentPos[i] to the obtained position
                (*pos)[i] = m_currentPos[i];
            }
        }
        else
        {
            retValue += ito::RetVal::format(
                ito::retError, 1, tr("axis %i not available").toLatin1().data(), i);
        }
    }

    sendStatusUpdate();

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setPosAbs
/*!
    starts moving the given axis to the desired absolute target position

    depending on m_async this method directly returns after starting the movement (async = 1) or
    only returns if the axis reached the given target position (async = 0)

    In some cases only relative movements are possible, then get the current position, determine
   the relative movement and call the method relatively move the axis.
*/
ito::RetVal FaulhaberMCS::setPosAbs(const int axis, const double pos, ItomSharedSemaphore* waitCond)
{
    return setPosAbs(QVector<int>(1, axis), QVector<double>(1, pos), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setPosAbs
/*!
    starts moving all given axes to the desired absolute target positions

    depending on m_async this method directly returns after starting the movement (async = 1) or
    only returns if all axes reached their given target positions (async = 0)

    In some cases only relative movements are possible, then get the current position, determine
   the relative movement and call the method relatively move the axis.
*/
ito::RetVal FaulhaberMCS::setPosAbs(
    QVector<int> axis, QVector<double> pos, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    bool released = false;

    if (isMotorMoving())
    {
        retValue += ito::RetVal(
            ito::retError,
            0,
            tr("motor is running. Additional actions are not possible.").toLatin1().data());
    }
    else
    {
        // check if axis is available TODO for MCS
        foreach (const int i, axis)
        {
            if (i < 0 || i >= m_numOfAxes)
            {
                retValue += ito::RetVal::format(
                    ito::retError, 1, tr("axis %i not available").toLatin1().data(), i);
            }
            else
            {
                m_targetPos[i] = pos[i];
            }
        }

        if (retValue.containsError())
        {
            if (waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
        else
        {
            setStatus(axis, ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
            sendStatusUpdate();

            foreach (const int i, axis)
            {
                retValue += setPosAbsMCS(pos[i]);
                m_targetPos[i] = pos[i];
            }

            sendTargetUpdate();

            if (m_async && waitCond) // async disabled
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }

            retValue += waitForDone(m_waitForDoneTimeout, axis); // drops into timeout

            replaceStatus(axis, ito::actuatorMoving, ito::actuatorAtTarget);
            sendStatusUpdate();

            if (!m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
    }


    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setPosRel
/*!
    starts moving the given axis by the given relative distance

    depending on m_async this method directly returns after starting the movement (async = 1) or
    only returns if the axis reached the given target position (async = 0)

    In some cases only absolute movements are possible, then get the current position, determine
   the new absolute target position and call setPosAbs with this absolute target position.
*/
ito::RetVal FaulhaberMCS::setPosRel(const int axis, const double pos, ItomSharedSemaphore* waitCond)
{
    return setPosRel(QVector<int>(1, axis), QVector<double>(1, pos), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! setPosRel
/*!
    starts moving the given axes by the given relative distances

    depending on m_async this method directly returns after starting the movement (async = 1) or
    only returns if all axes reached the given target positions (async = 0)

    In some cases only absolute movements are possible, then get the current positions,
   determine the new absolute target positions and call setPosAbs with these absolute target
   positions.
*/
ito::RetVal FaulhaberMCS::setPosRel(
    QVector<int> axis, QVector<double> pos, ItomSharedSemaphore* waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    bool released = false;

    if (isMotorMoving())
    {
        retValue += ito::RetVal(
            ito::retError,
            0,
            tr("motor is running. Additional actions are not possible.").toLatin1().data());
    }
    else
    {
        // check if axis is available TODO for MCS
        foreach (const int i, axis)
        {
            if (i < 0 || i >= m_numOfAxes)
            {
                retValue += ito::RetVal::format(
                    ito::retError, 1, tr("axis %i not available").toLatin1().data(), i);
            }
            else
            {
                m_targetPos[i] = m_currentPos[i] + pos[i];
            }
        }

        if (retValue.containsError())
        {
            if (waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
        else
        {
            setStatus(axis, ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
            sendStatusUpdate();

            foreach (const int i, axis)
            {
                retValue += setPosRelMCS(pos[i]);
                m_currentPos[i] = pos[i];
            }

            sendTargetUpdate();

            if (m_async && waitCond) // async disabled
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }

            retValue += waitForDone(m_waitForDoneTimeout, axis); // drops into timeout

            replaceStatus(axis, ito::actuatorMoving, ito::actuatorAtTarget);
            sendStatusUpdate();

            if (!m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::execFunc(
    const QString funcName,
    QSharedPointer<QVector<ito::ParamBase>> paramsMand,
    QSharedPointer<QVector<ito::ParamBase>> paramsOpt,
    QSharedPointer<QVector<ito::ParamBase>> paramsOut,
    ItomSharedSemaphore* waitCond)
{
    ito::RetVal retValue = ito::retOk;

    if (funcName == "homing")
    {
        retValue += updateStatusMCS();

        if (!m_params["operationEnabled"].getVal<int>())
        {
            retValue += ito::RetVal(
                ito::retError,
                0,
                tr("Operation is not enabled. Please enable operation first.").toLatin1().data());
            return retValue;
        }

        ito::int8 method = static_cast<ito::int8>((*paramsMand)[0].getVal<int>());
        ito::int32 offset = static_cast<ito::int32>((*paramsOpt)[0].getVal<int>());
        ito::uint32 switchSeekVelocity = static_cast<ito::uint32>((*paramsOpt)[1].getVal<int>());
        ito::uint32 homingSpeed = static_cast<ito::uint32>((*paramsOpt)[2].getVal<int>());
        ito::uint32 acceleration = static_cast<ito::uint32>((*paramsOpt)[3].getVal<int>());
        ito::uint16 limitCheckDelayTime = static_cast<ito::uint16>((*paramsOpt)[4].getVal<int>());

        ito::uint16 negativeLimit = static_cast<ito::uint16>((*paramsOpt)[5].getVal<int*>()[0]);
        ito::uint16 positiveLimit = static_cast<ito::uint16>((*paramsOpt)[5].getVal<int*>()[1]);
        ito::uint16 torqueLimits[] = {negativeLimit, positiveLimit};

        ito::int8 currentOperation;
        retValue += getOperationMode(currentOperation);

        retValue += setOperationMode(static_cast<ito::uint8>(6));
        retValue += setHomingMode(method);

        retValue += setHomingOffset(offset);
        retValue += setHomingSpeed(homingSpeed);
        retValue += setHomingSeekVelocity(switchSeekVelocity);
        retValue += setHomingAcceleration(acceleration);
        retValue += setHomingLimitCheckDelayTime(limitCheckDelayTime);
        retValue += setHomingTorqueLimits(torqueLimits);

        if (!retValue.containsError())
        {
            setControlWord(0x10);
            setControlWord(0x001F);
            retValue += updateStatusMCS();

            int setPoint = m_params["setPointAcknowledged"].getVal<int>();
            int target = m_params["targetReached"].getVal<int>();

            if ((m_params["setPointAcknowledged"].getVal<int>() == 0) &&
                (m_params["targetReached"].getVal<int>() == 0))
            {
                // homing started
                int a = 1;
            }
            else if (m_params["followingError"].getVal<int>() != 0)
            {
                retValue += ito::RetVal(
                    ito::retError, 0, tr("Error occurred during homing").toLatin1().data());
            }
            else
            {
                retValue += ito::RetVal(
                    ito::retError,
                    0,
                    tr("Homing could not be started. Please check the parameters.")
                        .toLatin1()
                        .data());
            }
        }

        retValue += setOperationMode(currentOperation);
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
        waitCond->deleteSemaphore();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getSerialNumber(QString& serialNum)
{
    ito::uint32 c;
    ito::RetVal retVal = readRegisterWithParsedResponse<ito::uint32>(0x1018, 0x04, c);
    serialNum = QString::number(c);
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getDeviceName(QString& name)
{
    return readRegisterWithParsedResponse<QString>(0x1008, 0x00, name);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getVendorID(QString& id)
{
    ito::uint32 c;
    ito::RetVal retVal = readRegisterWithParsedResponse<ito::uint32>(0x1018, 0x01, c);
    id = QString::number(c);
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getProductCode(QString& code)
{
    ito::uint32 c;
    ito::RetVal retVal = readRegisterWithParsedResponse<ito::uint32>(0x1018, 0x02, c);
    code = QString::number(c);
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getRevisionNumber(QString& num)
{
    ito::uint32 c;
    ito::RetVal retVal = readRegisterWithParsedResponse<ito::uint32>(0x1018, 0x03, c);
    num = QString::number(c);
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getFirmware(QString& version)
{
    return readRegisterWithParsedResponse<QString>(0x100A, 0x00, version);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getAmbientTemperature(ito::uint8& temp)
{
    return readRegisterWithParsedResponse<ito::uint8>(0x232A, 0x08, temp);
}

ito::RetVal FaulhaberMCS::getNodeID(ito::uint8& id)
{
    return readRegisterWithParsedResponse<ito::uint8>(0x2400, 0x03, id);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getPosMCS(ito::int32& pos)
{
    return readRegisterWithParsedResponse<ito::int32>(0x6064, 0x00, pos);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getTargetPosMCS(ito::int32& pos)
{
    return readRegisterWithParsedResponse<ito::int32>(0x6062, 0x00, pos);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getMaxMotorSpeed(ito::uint32& speed)
{
    return readRegisterWithParsedResponse<ito::uint32>(0x6080, 0x00, speed);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setMaxMotorSpeed(const ito::uint32& speed)
{
    return setRegister<ito::uint32>(0x6080, 0x00, speed, sizeof(speed));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getAcceleration(ito::uint32& acceleration)
{
    return readRegisterWithParsedResponse<ito::uint32>(0x6083, 0x00, acceleration);
}

ito::RetVal FaulhaberMCS::setAcceleration(const ito::uint32& acceleration)
{
    return setRegister<ito::uint32>(0x6083, 0x00, acceleration, sizeof(acceleration));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getDeceleration(ito::uint32& deceleration)
{
    return readRegisterWithParsedResponse<ito::uint32>(0x6084, 0x00, deceleration);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setDeceleration(const ito::uint32& deceleration)
{
    return setRegister<ito::uint32>(0x6084, 0x00, deceleration, sizeof(deceleration));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getQuickStopDeceleration(ito::uint32& deceleration)
{
    return readRegisterWithParsedResponse<ito::uint32>(0x6085, 0x00, deceleration);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setQuickStopDeceleration(const ito::uint32& deceleration)
{
    return setRegister<ito::uint32>(0x6085, 0x00, deceleration, sizeof(deceleration));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getProfileVelocity(ito::uint32& speed)
{
    return readRegisterWithParsedResponse<ito::uint32>(0x6081, 0x00, speed);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setProfileVelocity(const ito::uint32& speed)
{
    ito::RetVal retVal = ito::retOk;
    ito::uint32 maxSpeed = static_cast<ito::uint32>(m_params["maxMotorSpeed"].getVal<int>());
    if (speed > maxSpeed)
    {
        retVal += setRegister<ito::uint32>(0x6081, 0x00, maxSpeed, sizeof(maxSpeed));
        retVal += ito::RetVal(
            ito::retWarning,
            0,
            tr("Speed is higher than maxMotorSpeed. Speed is set to maxMotorSpeed of value '%1'.")
                .arg(maxSpeed)
                .toLatin1()
                .data());
    }
    else
    {
        retVal += setRegister<ito::uint32>(0x6081, 0x00, speed, sizeof(speed));
    }
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getOperationMode(ito::int8& mode)
{
    return readRegisterWithParsedResponse<ito::int8>(0x6060, 0x00, mode);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setOperationMode(const ito::int8& mode)
{
    return setRegister<ito::int8>(0x6060, 0x00, mode, sizeof(mode));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::getTorqueLimits(ito::uint16 limits[])
{
    ito::RetVal retVal = ito::retOk;

    retVal += readRegisterWithParsedResponse<ito::uint16>(0x60E0, 0x00, limits[0]);
    retVal += readRegisterWithParsedResponse<ito::uint16>(0x60E1, 0x00, limits[1]);
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setTorqueLimits(const ito::uint16 limits[])
{
    ito::RetVal retVal = ito::retOk;
    retVal += setRegister<ito::uint16>(0x60E0, 0x00, limits[0], sizeof(limits[0]));
    retVal += setRegister<ito::uint16>(0x60E1, 0x00, limits[1], sizeof(limits[1]));
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingOffset(const ito::int32& offset)
{
    return setRegister<ito::int32>(0x607c, 0x00, offset, sizeof(offset));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingSpeed(const ito::uint32& speed)
{
    return setRegister<ito::uint32>(0x6099, 0x02, speed, sizeof(speed));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingSeekVelocity(const ito::uint32& seek)
{
    return setRegister<ito::uint32>(0x6099, 0x01, seek, sizeof(seek));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingAcceleration(const ito::uint32& acceleration)
{
    return setRegister<ito::uint32>(0x609A, 0x00, acceleration, sizeof(acceleration));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingLimitCheckDelayTime(const ito::uint16& time)
{
    return setRegister<ito::uint16>(0x2324, 0x02, time, sizeof(time));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingTorqueLimits(const ito::uint16 limits[])
{
    ito::RetVal retVal = ito::retOk;
    retVal += setRegister<ito::uint16>(0x2350, 0x00, limits[0], sizeof(limits[0]));
    retVal += setRegister<ito::uint16>(0x2351, 0x00, limits[1], sizeof(limits[1]));
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::updateStatusMCS()
{
    ito::RetVal retVal = readRegisterWithParsedResponse<ito::uint16>(0x6041, 0x00, m_statusWord);

    if (!retVal.containsError())
    {
        QMutex waitMutex;
        QWaitCondition waitCondition;
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsedTime = 0;
        while ((m_statusWord == 0) && (elapsedTime < m_waitForMCSTimeout))
        {
            // TODO delete statudWord == 0
            //  short delay
            waitMutex.lock();
            waitCondition.wait(&waitMutex, m_delayAfterSendCommandMS);
            waitMutex.unlock();
            setAlive();

            retVal += readRegisterWithParsedResponse<ito::uint16>(0x6041, 0x00, m_statusWord);

            if (retVal.containsError())
            {
                retVal += ito::RetVal(
                    ito::retError,
                    0,
                    tr("Error occurred during reading statusword with value %1")
                        .arg(m_statusWord)
                        .toLatin1()
                        .data());
                break;
            }
            elapsedTime = QDateTime::currentMSecsSinceEpoch() - startTime;
        }

        if (!retVal.containsError())
        {
            m_params["statusWord"].setVal<int>(m_statusWord);
            std::bitset<16> statusBit = static_cast<std::bitset<16>>(m_statusWord);

            // Interpretation of bits
            bool readyToSwitchOn = statusBit[0];
            bool switchedOn = statusBit[1];
            bool operationEnabled = statusBit[2];
            bool fault = statusBit[3];
            bool voltageEnabled = statusBit[4];
            bool quickStop = statusBit[5];
            bool switchOnDisabled = statusBit[6];
            bool warning = statusBit[7];

            bool targetReached = statusBit[10];
            bool internalLimitActive = statusBit[11];
            bool setPointAcknowledged = statusBit[12];
            bool followingError = statusBit[13];

            m_params["readyToSwitchOn"].setVal<int>((readyToSwitchOn ? true : false));
            m_params["switchedOn"].setVal<int>((switchedOn ? true : false));
            m_params["operationEnabled"].setVal<int>((operationEnabled ? true : false));
            m_params["fault"].setVal<int>((fault ? true : false));
            m_params["voltageEnabled"].setVal<int>((voltageEnabled ? true : false));
            m_params["quickStop"].setVal<int>((quickStop ? true : false));
            m_params["switchOnDisabled"].setVal<int>((switchOnDisabled ? true : false));
            m_params["warning"].setVal<int>((warning ? true : false));
            m_params["targetReached"].setVal<int>((targetReached ? true : false));
            m_params["internalLimitActive"].setVal<int>((internalLimitActive ? true : false));
            m_params["setPointAcknowledged"].setVal<int>((setPointAcknowledged ? true : false));
            m_params["followingError"].setVal<int>((followingError ? true : false));

            emit parametersChanged(m_params);
        }
        sendStatusUpdate();
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setPosAbsMCS(const double& pos)
{
    ito::RetVal retVal = ito::retOk;
    int answer;
    int val = doubleToInteger(pos);

    setRegister<ito::int32>(0x607a, 0x00, val, sizeof(val));
    setControlWord(0x000f);
    setControlWord(0x003F);
    return updateStatusMCS();
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setPosRelMCS(const double& pos)
{
    ito::RetVal retVal = ito::retOk;
    int answer;
    int val = doubleToInteger(pos);

    setRegister<ito::int32>(0x607a, 0x00, val, sizeof(val));
    setControlWord(0x000f);
    setControlWord(0x007F);
    return updateStatusMCS();
}

////----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::setHomingMode(const ito::int8& mode)
{
    return setRegister<ito::int8>(0x6098, 0x00, mode, sizeof(mode));
}

//----------------------------------------------------------------------------------------------------------------------------------
int FaulhaberMCS::doubleToInteger(const double& value)
{
    return int(std::round(value * 100) / 100);
}

//----------------------------------------------------------------------------------------------------------------------------------
int FaulhaberMCS::responseVectorToInteger(const std::vector<int>& response)
{
    int answer = 0;
    for (size_t i = 0; i < response.size(); ++i)
    {
        answer |= response[i] << (8 * i);
    }
    return answer;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::waitForDone(const int timeoutMS, const QVector<int> axis, const int flags)
{
    ito::RetVal retVal(ito::retOk);
    bool done = false;
    bool timeout = false;
    char motor;
    int currentPos = 0;
    int targetPos = 0;
    QElapsedTimer timer;
    QMutex waitMutex;
    QWaitCondition waitCondition;

    QVector<int> _axis = axis;
    if (_axis.size() == 0) // all axis
    {
        for (int i = 0; i < m_numOfAxes; i++)
        {
            _axis.append(i);
        }
    }

    timer.start();
    while (!done && !timeout && !retVal.containsWarningOrError())
    {
        if (!done && isInterrupted()) // movement interrupted
        {
            replaceStatus(_axis, ito::actuatorMoving, ito::actuatorInterrupted);
            retVal += ito::RetVal(ito::retError, 0, tr("interrupt occurred").toLatin1().data());
            done = true;
            sendStatusUpdate();
            return retVal;
        }

        if (!retVal.containsError()) // short delay to reduce CPU load
        {
            // short delay of 10ms
            waitMutex.lock();
            waitCondition.wait(&waitMutex, m_delayAfterSendCommandMS);
            waitMutex.unlock();
            setAlive();
        }

        foreach (auto i, axis) // Check for completion
        {
            retVal += getPosMCS(currentPos);
            m_currentPos[i] = static_cast<double>(currentPos);

            retVal += getTargetPosMCS(targetPos);
            m_targetPos[i] = static_cast<double>(targetPos);

            retVal += updateStatusMCS();
            if ((m_statusWord && m_params["targetReached"].getVal<int>()))
            {
                setStatus(
                    m_currentStatus[i],
                    ito::actuatorAtTarget,
                    ito::actSwitchesMask | ito::actStatusMask);
                done = true;

                retVal += getPosMCS(currentPos);
                m_currentPos[i] = static_cast<double>(currentPos);

                retVal += getTargetPosMCS(targetPos);
                m_targetPos[i] = static_cast<double>(targetPos);

                break;
            }
            else
            {
                setStatus(
                    m_currentStatus[i],
                    ito::actuatorMoving,
                    ito::actSwitchesMask | ito::actStatusMask);
                done = false;
            }
        }

        sendStatusUpdate(false);

        if (timer.hasExpired(timeoutMS)) // timeout during movement
        {
            timeout = true;
            // timeout occurred, set the status of all currently moving axes to timeout
            replaceStatus(axis, ito::actuatorMoving, ito::actuatorTimeout);
            retVal += ito::RetVal(ito::retError, 9999, "timeout occurred during movement");
            sendStatusUpdate(true);
        }
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! method obtains the current position, status of all axes
/*!
    This is a helper function, it is not necessary to implement a function like this, but it
   might help.
*/
ito::RetVal FaulhaberMCS::updateStatus()
{
    ito::RetVal retVal = updateStatusMCS();

    for (int i = 0; i < m_numOfAxes; i++)
    {
        m_currentStatus[i] = m_currentStatus[i] | ito::actuatorAvailable;

        ito::int32 intPos;
        retVal += getPosMCS(intPos);

        m_currentPos[i] = static_cast<double>(intPos);

        if (m_params["targetReached"].getVal<int>())
        { // if you know that the axis i is at its target position, change from moving to
            // target if moving has been set, therefore:
            replaceStatus(m_currentStatus[i], ito::actuatorMoving, ito::actuatorAtTarget);
        }
        else
        { // if you know that the axis i is still moving, set this bit (all other moving-related
            // bits are unchecked, but the status bits and switches bits kept unchanged
            setStatus(
                m_currentStatus[i], ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
        }
    }

    emit actuatorStatusChanged(m_currentStatus, m_currentPos);
    // emit actuatorStatusChanged with m_currentStatus and m_currentPos in order to inform
    // connected slots about the current status and position
    sendStatusUpdate();

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::sendCommand(const QByteArray& command)
{
    QSharedPointer<QVector<ito::ParamBase>> _dummy;
    m_pSerialIO->execFunc("clearInputBuffer", _dummy, _dummy, _dummy, nullptr);
    m_pSerialIO->execFunc("clearOutputBuffer", _dummy, _dummy, _dummy, nullptr);

    ito::RetVal retVal = m_pSerialIO->setVal(command.data(), command.length(), nullptr);
    setAlive();
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::sendCommandAndGetResponse(const QByteArray& command, QByteArray& response)
{
    ito::RetVal retVal = ito::retOk;
    retVal += sendCommand(command);

    if (!retVal.containsError())
    {
        retVal += readResponse(response);
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal FaulhaberMCS::readResponse(QByteArray& result)
{
    ito::RetVal retValue = ito::retOk;
    QElapsedTimer timer;
    bool done = false;

    const int bufferSize = 100;
    QSharedPointer<int> curBufLen(new int(bufferSize));
    QSharedPointer<char> curBuf(new char[bufferSize]);
    result.clear();

    QMutex waitMutex;
    QWaitCondition waitCondition;

    qsizetype startIndex;
    qsizetype endIndex;

    timer.start();
    while (!done && !retValue.containsError())
    {
        waitMutex.lock();
        waitCondition.wait(&waitMutex, m_delayAfterSendCommandMS);
        waitMutex.unlock();
        setAlive();

        *curBufLen = bufferSize;
        retValue += m_pSerialIO->getVal(curBuf, curBufLen, nullptr);

        if (!retValue.containsError())
        {
            result += QByteArray(curBuf.data(), *curBufLen);
            startIndex = result.indexOf(m_S);
            endIndex = result.lastIndexOf(m_E);

            if (startIndex != -1 && endIndex != -1)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                result = result.sliced(startIndex, endIndex + 1);
#else
                result = result.mid(startIndex, endIndex + 1);
#endif
                done = true;
            }
        }

        if (!done && timer.elapsed() > m_requestTimeOutMS && m_requestTimeOutMS >= 0)
        {
            retValue += ito::RetVal(
                ito::retError,
                m_delayAfterSendCommandMS,
                tr("timeout during read string.").toLatin1().data());
            return retValue;
        }
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
template <typename T>
inline ito::RetVal FaulhaberMCS::readRegisterWithParsedResponse(
    const ito::uint16& address, const ito::uint8& subindex, T& answer)
{
    QByteArray response;
    ito::RetVal retValue = readRegister(address, subindex, response);
    if (!retValue.containsError())
        retValue += parseResponse<T>(response, answer);
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
template <typename T>
ito::RetVal FaulhaberMCS::parseResponse(QByteArray& response, T& parsedResponse)
{
    ito::RetVal retValue = ito::retOk;

    // Identify and extract the relevant portion of the response
    qsizetype startIndex = response.indexOf(m_S);
    qsizetype endIndex = response.lastIndexOf(m_E);
    if (startIndex == -1 || endIndex == -1)
    {
        return ito::RetVal(
            ito::retError,
            0,
            tr("The character 'S' or 'E' was not detected in the received bytearray.")
                .toLatin1()
                .data());
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    response = response.sliced(startIndex, endIndex + 1);
#else
    response = response.mid(startIndex, endIndex + 1);
#endif

    // Extract basic components from the response
    ito::uint8 length = static_cast<ito::uint8>(response[1]);
    ito::uint8 nodeNumber = static_cast<ito::uint8>(response[2]);
    ito::uint8 command = static_cast<ito::uint8>(response[3]);

    // Verify node number
    if (nodeNumber != m_node)
    {
        return ito::RetVal(
            ito::retError,
            0,
            tr("The node number '%1' does not match the expected node number '%2'.")
                .arg(nodeNumber)
                .arg(m_node)
                .toLatin1()
                .data());
    }

    ito::uint8 receivedCRC = static_cast<ito::uint8>(response[length]);
    ito::uint8 checkCRC = 0x00;
    QByteArray data = "";

    // Process the response based on command type
    switch (command)
    {
    case 0x01: // SDO read request/response
    case 0x05: // SDO write parameter request
        if (length > 7)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            checkCRC = calculateChecksum(response.sliced(1, length - 1));
            data = response.sliced(7, length - 7 + 1);
#else
            checkCRC = calculateChecksum(response.mid(1, length - 1));
            data = response.mid(7, length - 7 + 1);
#endif
        }
        else
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            checkCRC = calculateChecksum(response.sliced(1, length));
#else
            checkCRC = calculateChecksum(response.mid(1, length));
#endif
        }

        if (receivedCRC != checkCRC)
        {
            return ito::RetVal(
                ito::retError,
                0,
                tr("Checksum mismatch (received: '%1', calculated: '%2').")
                    .arg(receivedCRC)
                    .arg(checkCRC)
                    .toLatin1()
                    .data());
        }

        if (length <= 7)
        {
            return ito::RetVal(ito::retError, 0, tr("The length is <= 7.").toLatin1().data());
        }

        if constexpr (std::is_same<T, QString>::value)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            parsedResponse = QString::fromUtf8(data.sliced(0, data.size() - 1));
#else
            parsedResponse = QString::fromUtf8(data.mid(0, data.size() - 1));
#endif
        }
        else if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value)
        {
            if (data.size() >= sizeof(T))
            {
                std::memcpy(&parsedResponse, data.constData(), sizeof(T));
            }
            else
            {
                return ito::RetVal(ito::retError, 0, tr("Data size mismatch").toLatin1().data());
            }
        }
        else
        {
            return ito::RetVal(ito::retError, 0, tr("Unsupported type").toLatin1().data());
        }
        break;

    case 0x02: // SDO write request/response
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        checkCRC = calculateChecksum(response.sliced(1, length - 1));
#else
        checkCRC = calculateChecksum(response.mid(1, length - 1));
#endif
        if (receivedCRC != checkCRC)
        {
            return ito::RetVal(ito::retError, 0, tr("Checksum mismatch").toLatin1().data());
        }
        break;

    case 0x03: // SDO abort request of error response
        std::memcpy(&parsedResponse, response.constData() + 4, sizeof(T)); // Adjusted data offset
        return ito::RetVal(
            ito::retError,
            0,
            tr("Error during parse response with value: %1").arg(parsedResponse).toLatin1().data());

    case 0x07: // EMCY notification
        return ito::RetVal(ito::retError, 0, tr("EMCY notification").toLatin1().data());

    default:
        return ito::RetVal(
            ito::retError,
            0,
            tr("Unknown command received: %1")
                .arg(static_cast<ito::uint8>(command))
                .toLatin1()
                .data());
    }

    return retValue;
}


//----------------------------------------------------------------------------------------------------------------------------------
template <typename T>
ito::RetVal FaulhaberMCS::setRegister(
    const ito::uint16& address,
    const ito::uint8& subindex,
    const ito::uint32& value,
    const ito::uint8& length)
{
    ito::RetVal retVal = ito::retOk;
    QByteArray response;
    std::vector<uint8_t> command = {
        m_node,
        m_SET,
        static_cast<uint8_t>(address & 0xFF),
        static_cast<uint8_t>(address >> 8),
        subindex};

    for (int i = 0; i < length; i++)
    {
        command.push_back((value >> (8 * i)) & 0xFF);
    }

    std::vector<uint8_t> fullCommand = {static_cast<uint8_t>(command.size() + 2)};
    fullCommand.insert(fullCommand.end(), command.begin(), command.end());
    QByteArray CRC(reinterpret_cast<const char*>(fullCommand.data()), fullCommand.size());
    fullCommand.push_back(calculateChecksum(CRC));
    fullCommand.insert(fullCommand.begin(), m_S);
    fullCommand.push_back(m_E);

    QByteArray data(reinterpret_cast<char*>(fullCommand.data()), fullCommand.size());

    retVal += sendCommandAndGetResponse(data, response);

    if (!retVal.containsError())
    {
        T parsedResponse;
        retVal += parseResponse<T>(response, parsedResponse);
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
template <typename T>
ito::RetVal FaulhaberMCS::setRegisterWithParsedResponse(
    const ito::uint16& address,
    const ito::uint8& subindex,
    const ito::uint32& value,
    const ito::uint8& length,
    T& answer)
{
    ito::RetVal retVal = ito::retOk;

    QMutex waitMutex;
    QWaitCondition waitCondition;

    retVal += setRegister<T>(address, subindex, value, sizeof(value));
    // retVal += readRegisterWithParsedResponse<T>(address, subindex, answer);

    QElapsedTimer timer;
    timer.start();

    while (answer != value)
    {
        // retVal += updateStatusMCS();

        // short delay
        waitMutex.lock();
        waitCondition.wait(&waitMutex, m_delayAfterSendCommandMS);
        waitMutex.unlock();
        setAlive();

        retVal += readRegisterWithParsedResponse<T>(address, subindex, answer);

        if (retVal.containsError() || timer.hasExpired(m_waitForMCSTimeout))
        {
            retVal += ito::RetVal(
                ito::retError,
                0,
                tr("Timeout occurred during setting register %1 to %2")
                    .arg(address)
                    .arg(value)
                    .toLatin1()
                    .data());
            break;
        }
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::uint8 FaulhaberMCS::calculateChecksum(const QByteArray& message)
{
    ito::uint8 calcCRC = 0xFF;
    int len = message.size(); // Get the length of the QByteArray

    for (int i = 0; i < len; i++)
    {
        calcCRC = calcCRC ^
            static_cast<ito::uint8>(message[i]); // Access QByteArray data and cast to uint8_t
        for (ito::uint8 j = 0; j < 8; j++)
        {
            if (calcCRC & 0x01)
                calcCRC = (calcCRC >> 1) ^ 0xd5;
            else
                calcCRC = (calcCRC >> 1);
        }
    }

    return calcCRC;
}

//----------------------------------------------------------------------------------------------------------------------------------
bool FaulhaberMCS::verifyChecksum(QByteArray& message, ito::uint8& receivedCRC)
{
    // Calculate the CRC of the message excluding SOF, CRC, and EOF
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    uint8_t calculatedCRC = calculateChecksum(message.sliced(1, message.size() - 3));
#else
    uint8_t calculatedCRC = calculateChecksum(message.mid(1, message.size() - 3));
#endif

    // Compare the calculated CRC with the received CRC
    if (calculatedCRC != receivedCRC)
    {
        return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------
void FaulhaberMCS::dockWidgetVisibilityChanged(bool visible)
{
    if (getDockWidget())
    {
        QWidget* widget = getDockWidget()->widget();
        if (visible)
        {
            connect(
                this,
                SIGNAL(parametersChanged(QMap<QString, ito::Param>)),
                widget,
                SLOT(parametersChanged(QMap<QString, ito::Param>)));
            connect(
                this,
                SIGNAL(actuatorStatusChanged(QVector<int>, QVector<double>)),
                widget,
                SLOT(actuatorStatusChanged(QVector<int>, QVector<double>)));
            connect(
                this,
                SIGNAL(targetChanged(QVector<double>)),
                widget,
                SLOT(targetChanged(QVector<double>)));

            emit parametersChanged(m_params);
            sendTargetUpdate();
            sendStatusUpdate(false);
        }
        else
        {
            disconnect(
                this,
                SIGNAL(parametersChanged(QMap<QString, ito::Param>)),
                widget,
                SLOT(parametersChanged(QMap<QString, ito::Param>)));
            disconnect(
                this,
                SIGNAL(actuatorStatusChanged(QVector<int>, QVector<double>)),
                widget,
                SLOT(actuatorStatusChanged(QVector<int>, QVector<double>)));
            disconnect(
                this,
                SIGNAL(targetChanged(QVector<double>)),
                widget,
                SLOT(targetChanged(QVector<double>)));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
const ito::RetVal FaulhaberMCS::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogFaulhaberMCS(this));
}
