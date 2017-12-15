/* ********************************************************************
    Plugin "GenICam" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut f�r Technische Optik (ITO),
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

#include "deviceContainer.h"



#include "gccommon.h"

#include <qfileinfo.h>
#include <qdebug.h>
#include <qset.h>
#include <qregexp.h>
#include "common/sharedStructures.h"
#include <iostream>

/*static*/ GenTLOrganizer *GenTLOrganizer::m_pOrganizer = NULL;
//----------------------------------------------------------------------------------------------------------------------------------

/*static*/ GenTLOrganizer * GenTLOrganizer::instance(void)
{
    static GenTLOrganizerSingleton w;
    if (GenTLOrganizer::m_pOrganizer == NULL)
    {
        #pragma omp critical
        {
            if (GenTLOrganizer::m_pOrganizer == NULL)
            {
                GenTLOrganizer::m_pOrganizer = new GenTLOrganizer();
            }
        }
    }
    return GenTLOrganizer::m_pOrganizer;
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLOrganizer::GenTLOrganizer(void)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLOrganizer::~GenTLOrganizer(void)
{
}

//----------------------------------------------------------------------------------------------------------------------------------
QSharedPointer<GenTLSystem> GenTLOrganizer::getSystem(const QString &filename, ito::RetVal &retval)
{
    //check if this filename has already be opened
    foreach( const QWeakPointer<GenTLSystem> &system, m_systems)
    {
        if (system.data() && system.data()->getCtiFile() == filename)
        {
            return system.toStrongRef();
        }
    }

    //nothing found
    QSharedPointer<GenTLSystem> system(new GenTLSystem());
    retval += system->init(filename);

    if (retval.containsError())
    {
        return QSharedPointer<GenTLSystem>();
    }

    m_systems.append( system.toWeakRef() );
    return system;
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLSystem::GenTLSystem() :
    GCInitLib(NULL),
    GCCloseLib(NULL),
    GCGetInfo(NULL),
    TLOpen(NULL),
    TLClose(NULL),
    TLUpdateInterfaceList(NULL),
    m_initialized(false),
    m_systemInit(false),
    m_systemOpened(false),
    m_handle(GENTL_INVALID_HANDLE)
{
    m_lib = QSharedPointer<QLibrary>(new QLibrary());
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLSystem::~GenTLSystem()
{
    if (m_lib->isLoaded() && m_initialized)
    {
        if (TLClose && m_systemOpened)
        {
            TLClose(m_handle);
            m_handle = NULL;
        }

        if (GCCloseLib && m_systemInit)
        {
            GCCloseLib();
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLSystem::init(const QString &filename)
{
    ito::RetVal retval;
    QFileInfo info(filename);

    if (!info.exists())
    {
        retval += ito::RetVal::format(ito::retError, 0, "file '%s' does not exist", filename.toLatin1().data());
    }
    else if (filename.endsWith(".cti") == false)
    {
        retval += ito::RetVal::format(ito::retError, 0, "file '%s' is no *.cti file", filename.toLatin1().data());
    }
    else
    {
        m_lib->setFileName(filename);
        if (!m_lib->load())
        {
            retval += ito::RetVal::format(ito::retError, 0, "error loading library '%s' (%s)", filename.toLatin1().data(), m_lib->errorString().toLatin1().data());
        }
        else
        {
            m_initialized = true;

            GCInitLib = (GenTL::PGCInitLib)m_lib->resolve("GCInitLib");
            GCCloseLib = (GenTL::PGCCloseLib)m_lib->resolve("GCCloseLib");
            GCGetInfo = (GenTL::PGCGetInfo)m_lib->resolve("GCGetInfo"); //only necessary for getStringInfo. Check for existence there.
            TLOpen = (GenTL::PTLOpen)m_lib->resolve("TLOpen");
            TLClose = (GenTL::PTLClose)m_lib->resolve("TLClose");
            TLUpdateInterfaceList = (GenTL::PTLUpdateInterfaceList)m_lib->resolve("TLUpdateInterfaceList");
            TLGetNumInterfaces = (GenTL::PTLGetNumInterfaces)m_lib->resolve("TLGetNumInterfaces");
            TLGetInterfaceID = (GenTL::PTLGetInterfaceID)m_lib->resolve("TLGetInterfaceID");
            TLGetInterfaceInfo = (GenTL::PTLGetInterfaceInfo)m_lib->resolve("TLGetInterfaceInfo");
            TLOpenInterface = (GenTL::PTLOpenInterface)m_lib->resolve("TLOpenInterface");

            if (GCInitLib == NULL || GCCloseLib == NULL || \
                TLClose == NULL || TLOpen == NULL || !TLUpdateInterfaceList || \
                !TLGetNumInterfaces || !TLGetInterfaceID || !TLGetInterfaceInfo || !TLOpenInterface)
            {
                retval += ito::RetVal(ito::retError, 0, QObject::tr("cti file '%1' does not export all necessary methods of the GenTL standard.").arg(filename).toLatin1().data());
            }

            
        }
    }

    if (!retval.containsError())
    {
        retval += checkGCError(GCInitLib());

        if (retval.containsError())
        {
            GCCloseLib(); 
        }
        else
        {
            m_systemInit = true;
            m_ctiFile = filename;
        }
    }
    else
    {
        m_lib->unload();
        m_initialized = false;
    }
    

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLSystem::openSystem()
{
    ito::RetVal retval;

    if (!m_systemOpened)
    {

        if (!m_initialized || !m_systemInit || !TLOpen)
        {
            retval += ito::RetVal(ito::retError, 0, "System not initialized");
        }
        else
        {
            retval += checkGCError(TLOpen(&m_handle));
        }

        if (!retval.containsError())
        {
            //create and update interface list (done once for each system)
            //bool8_t pbChanged;
            retval += checkGCError(TLUpdateInterfaceList(m_handle, NULL, GENTL_INFINITE), "Update interface list");
            m_systemOpened = true;
        }
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
QByteArray GenTLSystem::getStringInfo(GenTL::TL_INFO_CMD_LIST cmd, ito::RetVal &retval) const
{
    
    if (!m_initialized || !GCGetInfo)
    {
        retval += ito::RetVal(ito::retError, 0, "System not initialized or method GCGetInfo in transport layer not available.");
    }
    else
    {
        GenTL::INFO_DATATYPE piType;
        char pBuffer[512];
        size_t piSize = 512;
        retval += checkGCError(GCGetInfo(cmd, &piType, &pBuffer, &piSize));

        if (!retval.containsError())
        {
            if (piType == GenTL::INFO_DATATYPE_STRING)
            {
                return QByteArray(pBuffer);
            }
            else
            {
                retval += ito::RetVal(ito::retError,0,"info type is no string");
            }
        }
    }

    return QByteArray();
}

//----------------------------------------------------------------------------------------------------------------------------------
QByteArray GenTLSystem::getInterfaceInfo(GenTL::INTERFACE_INFO_CMD_LIST cmd, const char *sIfaceID, ito::RetVal &retval) const
{
    if (!m_initialized || !m_systemInit || !TLGetInterfaceInfo)
    {
        retval += ito::RetVal(ito::retError, 0, "System not initialized");
    }
    else
    {
        GenTL::INFO_DATATYPE piType;
        char pBuffer[512];
        size_t piSize = 512;
        retval += checkGCError(TLGetInterfaceInfo(m_handle, sIfaceID, cmd, &piType, &pBuffer, &piSize));

        if (!retval.containsError())
        {
            if (piType == GenTL::INFO_DATATYPE_STRING)
            {
                return QByteArray(pBuffer);
            }
            else
            {
                retval += ito::RetVal(ito::retError,0,"info type is no string");
            }
        }
    }

    return QByteArray();
}

//----------------------------------------------------------------------------------------------------------------------------------
QSharedPointer<GenTLInterface> GenTLSystem::getInterface(const QByteArray &interfaceID, ito::RetVal &retval)
{
    if (!m_initialized || !m_systemInit)
    {
        retval += ito::RetVal(ito::retError, 0, "System not initialized");
    }
    else
    {
        //check existing, opened interfaces
        for (int i = 0; i < m_interfaces.size(); ++i)
        {
            if (m_interfaces[i].isNull() == false && m_interfaces[i].data()->getIfaceID() == interfaceID)
            {
                return m_interfaces[i].toStrongRef();
            }
        }

        uint32_t piNumIfaces = 0;
        retval += checkGCError(TLGetNumInterfaces(m_handle, &piNumIfaces));
        char sIfaceID[512];
        size_t piSize = 512;
        bool found = false;
		QByteArray id, displayname, tltype;
		ito::RetVal tlretval;
		QByteArray interfaceIDToOpen = interfaceID;

        if (!retval.containsError())
        {
			if (interfaceID == "")
			{
				std::cout << "Available interfaces\n----------------------------------------\n" << std::endl;
			}

            for (uint32_t i = 0; i < piNumIfaces; ++i)
            {
                retval += checkGCError(TLGetInterfaceID(m_handle, i, sIfaceID, &piSize));

				if (interfaceIDToOpen == "auto")
				{
					if (piNumIfaces == 1)
					{
						interfaceIDToOpen = sIfaceID;
					}
					else if (i == 0)
					{
						interfaceIDToOpen = sIfaceID;
						retval += ito::RetVal::format(ito::retWarning, 0, "The transport layer supports more than one interface. The first interface '%s' has been automatically selected.", sIfaceID);
					}
				}

				tlretval = ito::retOk;
				tltype = getInterfaceInfo(GenTL::INTERFACE_INFO_TLTYPE, sIfaceID, tlretval);

				if (interfaceID == "")
				{
					id = getInterfaceInfo(GenTL::INTERFACE_INFO_ID, sIfaceID, retval);
					displayname = getInterfaceInfo(GenTL::INTERFACE_INFO_DISPLAYNAME, sIfaceID, retval);
					std::cout << (i + 1) << ". ID: " << sIfaceID << ", name: " << displayname.data() << ", transport layer type: " << tltype.data() << "\n" << std::endl;
				}
            }

			if (interfaceID == "")
			{
				std::cout << "----------------------------------------\nUse the 'ID' value as interface parameter\n" << std::endl;
			}
        }

		if (interfaceID == "")
		{
			retval += ito::RetVal(ito::retError, 0, "no valid interface chosen");
		}

        if (!retval.containsError())
        {
            GenTL::IF_HANDLE ifHandle;
			retval += checkGCError(TLOpenInterface(m_handle, interfaceIDToOpen.data(), &ifHandle), QString("Error opening interface '%1'").arg(QLatin1String(interfaceIDToOpen)));

            if (!retval.containsError())
            {
				GenTLInterface *gtli = new GenTLInterface(m_lib, ifHandle, interfaceIDToOpen, retval);
                if (!retval.containsError())
                {
					QSharedPointer<GenTLInterface> sharedPtr(gtli);
					m_interfaces.append(sharedPtr.toWeakRef());
                    return sharedPtr;
                }
                else
                {
                    delete gtli;
                    gtli = NULL;
                }
            }
        }
        else
        {
            //try to open the interface without 
            retval += ito::RetVal(ito::retError,0,"no interface found");
        }
    }

    return QSharedPointer<GenTLInterface>();
}







//----------------------------------------------------------------------------------------------------------------------------------
GenTLInterface::GenTLInterface(QSharedPointer<QLibrary> lib, GenTL::IF_HANDLE ifHandle, QByteArray ifID, ito::RetVal &retval) :
    m_lib(lib),
    m_handle(ifHandle),
    m_IfaceID(ifID),
    IFOpenDevice(NULL),
    IFClose(NULL),
    IFGetDeviceInfo(NULL),
    IFGetDeviceID(NULL),
    IFUpdateDeviceList(NULL),
    IFGetNumDevices(NULL)
{
    IFOpenDevice = (GenTL::PIFOpenDevice)m_lib->resolve("IFOpenDevice");
    IFClose = (GenTL::PIFClose)m_lib->resolve("IFClose");
    IFGetDeviceInfo = (GenTL::PIFGetDeviceInfo)m_lib->resolve("IFGetDeviceInfo");
    IFGetDeviceID = (GenTL::PIFGetDeviceID)m_lib->resolve("IFGetDeviceID");
    IFUpdateDeviceList = (GenTL::PIFUpdateDeviceList)m_lib->resolve("IFUpdateDeviceList");
    IFGetNumDevices = (GenTL::PIFGetNumDevices)m_lib->resolve("IFGetNumDevices");

    if (IFGetDeviceID == NULL || IFGetDeviceInfo == NULL || IFClose == NULL || \
        IFOpenDevice == NULL || !IFUpdateDeviceList || !IFGetNumDevices)
    {
        retval += ito::RetVal(ito::retError, 0, QObject::tr("cti file does not export all functions of the GenTL protocol.").toLatin1().data());
    }
    else
    {
        retval += ito::RetVal(IFUpdateDeviceList(m_handle, NULL, 5000 /*timeout in ms*/));
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLInterface::~GenTLInterface()
{
    if (m_lib->isLoaded() && IFClose)
    {
        IFClose(m_handle);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
/*
\param deviceID: ID of device to be opened, if empty, the first available device is used.
*/
QSharedPointer<GenTLDevice> GenTLInterface::getDevice(const QByteArray &deviceID, bool printInfoAboutAllDevices, ito::RetVal &retval)
{
    if (!IFGetDeviceInfo)
    {
        retval += ito::RetVal(ito::retError, 0, "System not initialized");
    }
    else
    {
        QSet<QByteArray> usedDeviceIDs;

        //check if already exists, this time, this is an error
        for (int i = 0; i < m_devices.size(); ++i)
        {
            if (m_devices[i].isNull() == false)    //m_devices[i].data()->getDeviceID() == deviceID)
            {
                usedDeviceIDs.insert(m_devices[i].data()->getDeviceID());

                if (m_devices[i].data()->getDeviceID() == deviceID)
                {
                    retval += ito::RetVal::format(ito::retError,0,"device '%s' already in use", deviceID.data());
                }
            }
        }

        uint32_t piNumDevices = 0;
        retval += checkGCError(IFGetNumDevices(m_handle, &piNumDevices));
        char sDeviceID[512];
        size_t piSize = 512;
        bool found = false;

        if (!retval.containsError())
        {
			if (piNumDevices == 0)
			{
				retval += ito::RetVal(ito::retError, 0, "no devices detected for given vendor and interface type");
			}

            for (uint32_t i = 0; i < piNumDevices; ++i)
            {
                retval += checkGCError(IFGetDeviceID(m_handle, i, sDeviceID, &piSize));

                //check access status
                GenTL::DEVICE_ACCESS_STATUS accessStatus = GenTL::DEVICE_ACCESS_STATUS_UNKNOWN;

                {
                    GenTL::INFO_DATATYPE piType;
                    char pBuffer[512];
                    size_t piSize = sizeof(pBuffer);
                    GenTL::GC_ERROR err = IFGetDeviceInfo(m_handle, sDeviceID, GenTL::DEVICE_INFO_ACCESS_STATUS, &piType, &pBuffer, &piSize);
                    if (err == GenTL::GC_ERR_SUCCESS && piType == GenTL::INFO_DATATYPE_INT32)
                    {
                        accessStatus = ((ito::int32*)pBuffer)[0];
                    }
                }

                if (!retval.containsError())
                {
                    if (printInfoAboutAllDevices)
                    {
                        retval += printDeviceInfo(sDeviceID);
                    }

                    if (sDeviceID == "" && usedDeviceIDs.contains(sDeviceID)) //open next free device
                    {
                        found = true;
                        break;
                    }
                    else if (deviceID == "" && accessStatus == GenTL::DEVICE_ACCESS_STATUS_READWRITE)
                    {
                        found = true;
                        break;
                    }
                    else if (deviceID == sDeviceID)
                    {
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!retval.containsError())
        {
            if (!found)
            {
                //try to open the interface with the given interfaceID
                sDeviceID[0] = '\0';
                memcpy(sDeviceID, deviceID.data(), sizeof(char) * std::min(deviceID.size(),(int)piSize));
                sDeviceID[piSize-1] = '\0';
            }

            GenTL::DEV_HANDLE devHandle;
            retval += checkGCError(IFOpenDevice(m_handle, sDeviceID, GenTL::DEVICE_ACCESS_CONTROL, &devHandle), "Opening device");

            if (!retval.containsError())
            {
				QByteArray identifier(200, '\0');
				piSize = identifier.size();
				GenTL::INFO_DATATYPE piType;
				if (IFGetDeviceInfo(m_handle, sDeviceID, GenTL::DEVICE_INFO_DISPLAYNAME, &piType, identifier.data(), &piSize) != GenTL::GC_ERR_SUCCESS)
				{
					identifier = "unknown camera";
				}

                GenTLDevice *gtld = new GenTLDevice(m_lib, devHandle, sDeviceID, identifier, retval);
                if (!retval.containsError())
                {
                    return QSharedPointer<GenTLDevice>( gtld );
                }
                else
                {
                    DELETE_AND_SET_NULL(gtld);
                }
            }
        }
    }

    return QSharedPointer<GenTLDevice>();
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLInterface::printDeviceInfo(const char* sDeviceID) const
{
    ito::RetVal retval;

    if (!IFGetDeviceInfo)
    {
        retval += ito::RetVal(ito::retError, 0, "System not initialized");
    }
    else
    {
        GenTL::INFO_DATATYPE piType;
        char pBuffer[512];
        size_t piSize;

        GenTL::DEVICE_INFO_CMD cmds[] = { GenTL::DEVICE_INFO_ID, \
            GenTL::DEVICE_INFO_VENDOR, \
            GenTL::DEVICE_INFO_MODEL, \
            GenTL::DEVICE_INFO_TLTYPE, \
            GenTL::DEVICE_INFO_DISPLAYNAME, \
            GenTL::DEVICE_INFO_USER_DEFINED_NAME, \
            GenTL::DEVICE_INFO_SERIAL_NUMBER, \
            GenTL::DEVICE_INFO_VERSION,
            GenTL::DEVICE_INFO_ACCESS_STATUS };

        const char* names[] = { "ID:", \
            "Vendor:", \
            "Model:", \
            "TL Type:", \
            "Display Name:", \
            "User Defined Name:", \
            "Serial Number:", \
            "Device Info Version:", \
            "Access Status:" };

        std::cout << "Device information for device " << sDeviceID << "\n------------------------------------------------------------------------\n" << std::endl;

        for (int i = 0; i < sizeof(cmds) / sizeof(GenTL::DEVICE_INFO_CMD); ++i)
        {
            piSize = sizeof(pBuffer);
            GenTL::GC_ERROR err = IFGetDeviceInfo(m_handle, sDeviceID, cmds[i], &piType, &pBuffer, &piSize);

            if (err == GenTL::GC_ERR_SUCCESS)
            {
                if (piType == GenTL::INFO_DATATYPE_STRING)
                {
                    std::cout << names[i] << " " << pBuffer << "\n" << std::endl;
                }
                else if (cmds[i] == GenTL::DEVICE_INFO_ACCESS_STATUS)
                {
                    ito::int32 s = ((ito::int32*)(pBuffer))[0];
                    switch (s)
                    {
                    case GenTL::DEVICE_ACCESS_STATUS_UNKNOWN:
                        std::cout << names[i] << " unknown\n" << std::endl;
                        break;
                    case GenTL::DEVICE_ACCESS_STATUS_READWRITE:
                        std::cout << names[i] << " read/write\n" << std::endl;
                        break;
                    case GenTL::DEVICE_ACCESS_STATUS_READONLY:
                        std::cout << names[i] << " readonly\n" << std::endl;
                        break;
                    case GenTL::DEVICE_ACCESS_STATUS_NOACCESS:
                        std::cout << names[i] << " no access\n" << std::endl;
                        break;
                    case GenTL::DEVICE_ACCESS_STATUS_BUSY:
                        std::cout << names[i] << " busy\n" << std::endl;
                        break;
                    case GenTL::DEVICE_ACCESS_STATUS_OPEN_READWRITE:
                    case GenTL::DEVICE_ACCESS_STATUS_OPEN_READONLY:
                        std::cout << names[i] << " resource in use\n" << std::endl;
                        break;
                    default:
                        std::cout << names[i] << " other (" << s << ")\n" << std::endl;
                        break;
                    }
                }
                else if (piType == GenTL::INFO_DATATYPE_INT32)
                {
                    std::cout << names[i] << " " << ((ito::int32*)(pBuffer))[0] << "\n" << std::endl;
                }
            }
            else if (err == GenTL::GC_ERR_INVALID_HANDLE || err == GenTL::GC_ERR_NOT_INITIALIZED)
            {
                retval += checkGCError(err, "Device Info");
                break;
            }
        }

		std::cout << "Interface: " << m_IfaceID.data() << "\n" << std::endl;
    }

    return retval;
}