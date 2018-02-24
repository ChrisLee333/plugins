/* ********************************************************************
    Plugin "GenICam" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2018, Institut f�r Technische Optik (ITO),
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

#include "dataStream.h"

#include "gccommon.h"

#include <qfileinfo.h>
#include <qdebug.h>
#include <qset.h>
#include <qregexp.h>
#include <qthread.h>
#include "common/sharedStructures.h"
#include <iostream>
#define PFNC_INCLUDE_HELPERS
#include "PFNC.h"

//----------------------------------------------------------------------------------------------------------------------------------
GenTLDataStream::GenTLDataStream(QSharedPointer<QLibrary> lib, GenTL::DS_HANDLE handle, int verbose, ito::RetVal &retval) :
    m_handle(handle),
    m_lib(lib),
    m_newBufferEvent(GENTL_INVALID_HANDLE),
    m_errorEvent(GENTL_INVALID_HANDLE),
    m_acquisitionStarted(false),
	m_payloadSize(0),
	m_timeoutMS(0),
	m_usePreAllocatedBuffer(-1),
	m_endianessChanged(true),
    m_verbose(verbose)
{
    GCRegisterEvent = (GenTL::PGCRegisterEvent)m_lib->resolve("GCRegisterEvent");
    GCUnregisterEvent = (GenTL::PGCUnregisterEvent)m_lib->resolve("GCUnregisterEvent");
    DSClose = (GenTL::PDSClose)m_lib->resolve("DSClose");
    EventGetData = (GenTL::PEventGetData)m_lib->resolve("EventGetData");
    EventGetDataInfo = (GenTL::PEventGetDataInfo)m_lib->resolve("EventGetDataInfo");
    DSQueueBuffer = (GenTL::PDSQueueBuffer)m_lib->resolve("DSQueueBuffer");
	DSRevokeBuffer = (GenTL::PDSRevokeBuffer)m_lib->resolve("DSRevokeBuffer");
    DSFlushQueue = (GenTL::PDSFlushQueue)m_lib->resolve("DSFlushQueue");
    DSGetBufferInfo = (GenTL::PDSGetBufferInfo)m_lib->resolve("DSGetBufferInfo");
    DSStartAcquisition = (GenTL::PDSStartAcquisition) m_lib->resolve("DSStartAcquisition");
    DSStopAcquisition = (GenTL::PDSStopAcquisition) m_lib->resolve("DSStopAcquisition");
    DSGetInfo = (GenTL::PDSGetInfo)m_lib->resolve("DSGetInfo");
	DSAllocAndAnnounceBuffer = (GenTL::PDSAllocAndAnnounceBuffer)m_lib->resolve("DSAllocAndAnnounceBuffer");
	DSAnnounceBuffer = (GenTL::PDSAnnounceBuffer)m_lib->resolve("DSAnnounceBuffer");

    if (!GCRegisterEvent || !GCUnregisterEvent || !DSClose || \
        !EventGetData || !DSQueueBuffer || !DSFlushQueue || !DSGetBufferInfo || \
		!DSStopAcquisition || !DSStartAcquisition || !DSGetInfo || !DSAllocAndAnnounceBuffer || \
		!DSRevokeBuffer)
    {
        retval += ito::RetVal(ito::retError, 0, QObject::tr("cti file does not export all functions of the GenTL protocol.").toLatin1().constData());
    }

    if (!retval.containsError())
    {
        retval += checkGCError(GCRegisterEvent(m_handle, GenTL::EVENT_NEW_BUFFER, &m_newBufferEvent), "dataStream: register new-buffer event");
		GenTL::GC_ERROR errorEventResult = GCRegisterEvent(m_handle, GenTL::EVENT_ERROR, &m_errorEvent);

        if (m_verbose >= VERBOSE_WARNING)
        {
		    switch (errorEventResult)
		    {
		    case GenTL::GC_ERR_SUCCESS:
			    break;
		    case GenTL::GC_ERR_NOT_IMPLEMENTED:
            case GenTL::GC_ERR_NOT_AVAILABLE:
			    m_errorEvent = GENTL_INVALID_HANDLE;
			    break;
            case GenTL::GC_ERR_RESOURCE_IN_USE:
			    std::cerr << "dataStream: error event could not be registered (" << errorEventResult << ": GC_ERR_RESOURCE_IN_USE)\n" << std::endl;
			    m_errorEvent = GENTL_INVALID_HANDLE;
			    break;
		    default:
			    std::cerr << "dataStream: error event could not be registered (code: " << errorEventResult << ")\n" << std::endl;
			    m_errorEvent = GENTL_INVALID_HANDLE;
			    break;
		    }
        }
    }

    if (m_verbose >= VERBOSE_INFO)
    {
        std::cout << "Registered dataStream events: \nnew buffer event: " << m_newBufferEvent << "\nerror event: " << m_errorEvent << "\n" << std::endl;
    }

    if (!retval.containsError())
    {
        //check how much memory the event need to have and allocate it
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
GenTLDataStream::~GenTLDataStream()
{
    if (GCUnregisterEvent && m_newBufferEvent != GENTL_INVALID_HANDLE)
    {
        GCUnregisterEvent(m_handle, GenTL::EVENT_NEW_BUFFER);
    }

    if (GCUnregisterEvent && m_errorEvent != GENTL_INVALID_HANDLE)
    {
        GCUnregisterEvent(m_handle, GenTL::EVENT_ERROR);
    }

    if (DSClose && m_handle != GENTL_INVALID_HANDLE)
    {
		//to be sure, only
		flushBuffers(GenTL::ACQ_QUEUE_ALL_DISCARD);
		revokeAllBuffers();

        if (m_acquisitionStarted)
        {
            stopAcquisition(GenTL::ACQ_STOP_FLAGS_KILL);
        }

        DSClose(m_handle);
        m_handle = GENTL_INVALID_HANDLE;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::allocateAndAnnounceBuffers(int nrOfBuffers, size_t bytesPerBuffer /*= 0*/)
{
    GenTL::BUFFER_HANDLE handle;
    ito::RetVal retval;
    GenTL::INFO_DATATYPE type;
    bool definesPayloadsize = false;
    size_t size;
    size_t payloadSize = 0;
	size_t bufAnnounceMin = 1;
    int payloadType = 0;
    
    if (bytesPerBuffer == 0) //estimate payload
    {
        size = sizeof(definesPayloadsize);
        if (DSGetInfo(m_handle, GenTL::STREAM_INFO_DEFINES_PAYLOADSIZE, &type, &definesPayloadsize, &size) == GenTL::GC_ERR_SUCCESS)
        {
            if (type != GenTL::INFO_DATATYPE_BOOL8)
            {
                definesPayloadsize = false;
            }
        }

        if (definesPayloadsize)
        {
            size = sizeof(payloadSize);
            if (DSGetInfo(m_handle, GenTL::STREAM_INFO_PAYLOAD_SIZE, &type, &payloadSize, &size) == GenTL::GC_ERR_SUCCESS)
            {
                if (type != GenTL::INFO_DATATYPE_SIZET)
                {
                    payloadSize = 0;
                    retval += ito::RetVal(ito::retWarning, 0, "Payload size given by data stream has an invalid format. Use the payload size from the device parameter instead.");
                }

                payloadType = 1;
            }
        }

        if (payloadSize == 0)
        {
			payloadSize = m_payloadSize;
            payloadType = 2;
            //has to be obtained by node... TODO
        }

		if (payloadSize == 0)
		{
			retval += ito::RetVal(ito::retError, 0, "could not get a valid payload size.");
			return retval;
		}

        bytesPerBuffer = payloadSize;
    }
	else
	{
		payloadType = 3; //given by plugin parameter 'payloadSize'
	}

    size = sizeof(bufAnnounceMin);
	GenTL::GC_ERROR ret = DSGetInfo(m_handle, GenTL::STREAM_INFO_BUF_ANNOUNCE_MIN, &type, &bufAnnounceMin, &size);
	if (ret == GenTL::GC_ERR_SUCCESS)
	{
		if (type != GenTL::INFO_DATATYPE_SIZET)
		{
			retval += ito::RetVal(ito::retWarning, 0, "Data stream returns wrong data type for STREAM_INFO_BUF_ANNOUNCE_MIN. A minimum number of 1 buffer is assumed.");
			bufAnnounceMin = 1;
		}
	}
	else if (ret == GenTL::GC_ERR_NOT_AVAILABLE || ret == GenTL::GC_ERR_NOT_IMPLEMENTED)
	{
		bufAnnounceMin = 1;
	}
	else
	{
		retval += ito::RetVal(ito::retWarning, 0, "Data stream returns error for STREAM_INFO_BUF_ANNOUNCE_MIN. A minimum number of 1 buffer is assumed.");
		bufAnnounceMin = 1;
	}

    if (bufAnnounceMin > nrOfBuffers)
	{
		retval += ito::RetVal::format(ito::retError, 0, "the number of announced buffer is below the minimum number of buffers (%i), required for data acquisition by the specific device. Use the parameter 'numBuffers' to increase the number of buffers.", bufAnnounceMin);
		return retval;
	}

    //get aligment (optional)
    size_t alignment = 1;
    size = sizeof(alignment);
    ret = DSGetInfo(m_handle, GenTL::STREAM_INFO_BUF_ALIGNMENT, &type, &alignment, &size);
    if (ret != GenTL::GC_ERR_SUCCESS)
    {
        alignment = 1;
    }

    if (m_verbose >= VERBOSE_INFO)
    {
        std::cout << "Buffer allocation and announce:\n--------------------------------------------\n" << std::endl;
        std::cout << "* Number of buffers: " << nrOfBuffers << "\n" << std::endl;
        std::cout << "* Minimum number of required buffers: " << bufAnnounceMin << "\n" << std::endl;
        std::cout << "* Buffer alignment: " << alignment << " byte(s)\n" << std::endl;
		switch (payloadType)
		{
		case 0:
			std::cout << "* Source for buffer size: " << "Manually set" << "\n" << std::endl;
			break;
		case 1:
			std::cout << "* Source for buffer size: " << "Given from data stream info" << "\n" << std::endl;
			break;
		case 2:
			std::cout << "* Source for buffer size: " << "Given by device parameter" << "\n" << std::endl;
			break;
		case 3:
			std::cout << "* Source for buffer size: " << "Given by plugin parameter 'userDefinedPayloadSize'" << "\n" << std::endl;
			break;
		default:
			std::cout << "* Source for buffer size: " << "Unknown" << "\n" << std::endl;
			break;

		}
        std::cout << "* Payload size: " << bytesPerBuffer << " bytes\n" << std::endl;
        std::cout << "* Guess allocation type: " << ((m_usePreAllocatedBuffer == -1) ? "Yes" : "No") << "\n" << std::endl;
    }

    for (int i = 0; i < nrOfBuffers; ++i)
    {
		GenTL::GC_ERROR err;
		
		if (m_usePreAllocatedBuffer >= 0) //use the type of buffer (itom-allocated or camera-allocated depending on the first buffer)
		{
			if (m_usePreAllocatedBuffer > 0)
			{
				//some cameras fail if camera-internal buffer should be allocated, therefore we try to announce pre-allocated buffer here
				ito::uint8 *buffer = new ito::uint8[bytesPerBuffer];
				err = DSAnnounceBuffer(m_handle, buffer, bytesPerBuffer, this, &handle);
				if (err == GenTL::GC_ERR_SUCCESS)
				{
					m_usePreAllocatedBuffer = 1;
				}
				else
				{
					m_usePreAllocatedBuffer = -1;
					DELETE_AND_SET_NULL_ARRAY(buffer);
					retval += checkGCError(err, "DataStream: DSAnnounceBuffer (a)");
				}
			}
			else
			{
				err = DSAllocAndAnnounceBuffer(m_handle, bytesPerBuffer, this, &handle);

				if (err == GenTL::GC_ERR_RESOURCE_IN_USE)
				{
#if QT_VERSION >= 0x050000
					QThread::msleep(50);
#endif
					err = DSAllocAndAnnounceBuffer(m_handle, bytesPerBuffer, this, &handle);
					retval += checkGCError(err, "DataStream: DSAllocAndAnnounceBuffer (a2)");
				}
				m_usePreAllocatedBuffer = 0;
				retval += checkGCError(err, "DataStream: DSAllocAndAnnounceBuffer (a)");
			}
		}
		else //in the first run, guess if pre-allocated or internal-allocated memory should be used.
		{ 
			err = DSAllocAndAnnounceBuffer(m_handle, bytesPerBuffer, this, &handle);
			if (err == GenTL::GC_ERR_INVALID_PARAMETER && DSAnnounceBuffer)
			{
                if (alignment != 1)
                {
                    retval += ito::RetVal::format(ito::retWarning, 0, "DataStream: an alignment of %i bytes is requested. However this is currently not implemented", alignment);
                }

				//some cameras fail if camera-internal buffer should be allocated, therefore we try to announce pre-allocated buffer here
				ito::uint8 *buffer = new ito::uint8[bytesPerBuffer];
				err = DSAnnounceBuffer(m_handle, buffer, bytesPerBuffer, this, &handle);
				if (err == GenTL::GC_ERR_SUCCESS)
				{
					m_usePreAllocatedBuffer = 1;
				}
				else
				{
					m_usePreAllocatedBuffer = -1;
					DELETE_AND_SET_NULL_ARRAY(buffer);
					retval += checkGCError(err, "DataStream: DSAnnounceBuffer (b)");
				}
			}
			else
			{
				m_usePreAllocatedBuffer = 0;
				retval += checkGCError(err, "DataStream: DSAllocAndAnnounceBuffer (b)");
			}
		}

        if (!retval.containsError())
        {
            //directly queue allocated buffer
            retval += checkGCError(DSQueueBuffer(m_handle, handle));
            m_buffers.insert(handle);
        }
        else
        {
			m_usePreAllocatedBuffer = -1;
            break;
        }
    }

    if (m_verbose >= VERBOSE_INFO)
    {
        
        std::cout << "* Allocation type: " << ((m_usePreAllocatedBuffer == 0) ? "camera internal" : "allocated by itom") << "\n" << std::endl;
        std::cout << "* Number of buffers " << m_buffers.size() << "\n" << std::endl;
		if (retval.containsError())
		{
			std::cout << "* Success: Error (" << retval.errorMessage() << ")\n" << std::endl;
		}
		else
		{
			std::cout << "* Success: OK" << "\n" << std::endl;
		}
        std::cout << "--------------------------------------------\n" << std::endl;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::revokeAllBuffers()
{
	ito::RetVal retval;
	foreach(GenTL::BUFFER_HANDLE handle, m_buffers)
	{
		if (m_usePreAllocatedBuffer)
		{
			ito::uint8 *pBuffer = NULL;
			retval += checkGCError(DSRevokeBuffer(m_handle, handle, (void**)&pBuffer, NULL), "revoke buffer");
			if (pBuffer)
			{
				DELETE_AND_SET_NULL(pBuffer);
			}
		}
		else
		{
			retval += checkGCError(DSRevokeBuffer(m_handle, handle, NULL, NULL), "revoke buffer");
		}
	}
	m_buffers.clear();

	return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::flushBuffers(GenTL::ACQ_QUEUE_TYPE queueType /*= GenTL::ACQ_QUEUE_ALL_DISCARD*/)
{
	ito::RetVal retval;

    if (queueType == GenTL::ACQ_QUEUE_ALL_DISCARD)
    {
        GenTL::INFO_DATATYPE type;
        bool acquiring;
        size_t pSize = sizeof(acquiring);
        bool busy;

        do
        {
            busy = false;

            foreach (GenTL::BUFFER_HANDLE buf, m_buffers)
            {
	            if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buf, GenTL::BUFFER_INFO_IS_ACQUIRING, &type, &acquiring, &pSize))
                {
                    if (type == GenTL::INFO_DATATYPE_BOOL8 && acquiring)
                    {
                        busy = true;
                    }
                }
            }
        } while (busy);
    }

    retval += checkGCError(DSFlushQueue(m_handle, queueType), "discard all buffers in input and ouput buffers");
                   

	return retval;
}


//----------------------------------------------------------------------------------------------------------------------------------
void GenTLDataStream::printBufferInfo(const char* prefix, GenTL::BUFFER_HANDLE buffer)
{
	GenTL::INFO_DATATYPE type;
	bool boolValue;
	size_t sizetValue;
    uint64 uint64value;
	size_t pSize = sizeof(boolValue);

    std::cout << prefix;

	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_IS_ACQUIRING, &type, &boolValue, &pSize))
	{
		std::cout << "acquiring:" << boolValue;
	}

	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_IS_QUEUED, &type, &boolValue, &pSize))
	{
		std::cout << ", queued:" << boolValue;
	}

	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_NEW_DATA, &type, &boolValue, &pSize))
	{
		std::cout << ", newdata:" << boolValue;
	}

	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_IS_INCOMPLETE, &type, &boolValue, &pSize))
	{
		std::cout << ", incomplete:" << boolValue;
	}

	pSize = sizeof(sizetValue);
	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_SIZE_FILLED, &type, &sizetValue, &pSize))
	{
		std::cout << ", size-filled:" << sizetValue;
	}

    pSize = sizeof(uint64value);
	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_FRAMEID, &type, &uint64value, &pSize))
	{
		std::cout << ", id:" << uint64value;
	}

    pSize = sizeof(sizetValue);
	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_WIDTH, &type, &sizetValue, &pSize))
	{
		std::cout << ", size:" << sizetValue;
	}

    pSize = sizeof(sizetValue);
	if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_HEIGHT, &type, &sizetValue, &pSize))
	{
		std::cout << " x " << sizetValue;
	}

    std::cout << "\n" << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::waitForNewestBuffer(ito::DataObject &destination)
{
    ito::RetVal retval;
    GenTL::S_EVENT_NEW_BUFFER latestBuffer;
    GenTL::S_EVENT_NEW_BUFFER nextBuffer;
    size_t pSize = sizeof(latestBuffer);
    bool newData = false;
    GenTL::INFO_DATATYPE type;
    bool value;
    size_t valueSize = sizeof(value);

    QSet<GenTL::BUFFER_HANDLE> handlesToQueueAgain;

	GenTL::GC_ERROR err;

    if (m_verbose >= VERBOSE_ALL)
    {
        std::cout << "Buffer info before new-buffer-event:\n" << std::endl;
	    foreach(const GenTL::BUFFER_HANDLE &buf, m_buffers)
	    {
		    printBufferInfo(QString("* buffer 0x%1 ->").arg((size_t)buf,0,16).toLatin1().constData(), buf);
	    }
    }
    
    while (!newData)
    {
        newData = true;
        err = EventGetData(m_newBufferEvent, &latestBuffer, &pSize, m_timeoutMS);

        if (err == GenTL::GC_ERR_SUCCESS)
        {
            if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, latestBuffer.BufferHandle, GenTL::BUFFER_INFO_NEW_DATA, &type, &value, &valueSize))
            {
                if (type == GenTL::INFO_DATATYPE_BOOL8)
                {
                    newData = value;

                    if (!newData)
                    {
                        handlesToQueueAgain.insert(latestBuffer.BufferHandle);
                    }
                }
            }
        }
    }

    if (err == GenTL::GC_ERR_TIMEOUT)
    {
		foreach(GenTL::BUFFER_HANDLE buf, m_buffers)
		{
			handlesToQueueAgain.insert(buf);
		}
        //flushBuffers(GenTL::ACQ_QUEUE_ALL_TO_INPUT);

        if (checkForErrorEvent(retval, "Timeout occurred"))
        {
            //return retval;
        }
        else
        {
            retval +=  ito::RetVal(ito::retError, 0, "Timeout occurred.");
        }
    }
    else if (checkGCError(err) == ito::retOk)
    {
        while (1)
        {
            //check for further newBufferEvents (no timeout) to be sure to get latest buffer
            pSize = sizeof(nextBuffer);
            err = EventGetData(m_newBufferEvent, &nextBuffer, &pSize, 0);
            if (GenTL::GC_ERR_SUCCESS == err)
            {
                newData = true;
                if (GenTL::GC_ERR_SUCCESS == DSGetBufferInfo(m_handle, nextBuffer.BufferHandle, GenTL::BUFFER_INFO_NEW_DATA, &type, &value, &valueSize))
                {
                    if (type == GenTL::INFO_DATATYPE_BOOL8)
                    {
                        newData = value;

                        
                    }
                }

                if (latestBuffer.BufferHandle == nextBuffer.BufferHandle)
                {
                    //something strange, but happens
                }
                else if (!newData)
                {
                    handlesToQueueAgain.insert(nextBuffer.BufferHandle);
                }
                else
                {
                    handlesToQueueAgain.insert(latestBuffer.BufferHandle);
                    latestBuffer = nextBuffer;
                }
            }
            else if (err != GenTL::GC_ERR_TIMEOUT)
            {
                retval += checkGCError(err);
            }
            else //timeout
            {
                break;
            }
        }

        if (m_verbose >= VERBOSE_ALL)
        {
            std::cout << "Buffer info after new-buffer-event\n" << std::endl;
	        foreach(const GenTL::BUFFER_HANDLE &buf, m_buffers)
	        {
                if (buf == latestBuffer.BufferHandle)
                {
                    printBufferInfo(QString("* buffer 0x%1 (latest) ->").arg((size_t)buf,0,16).toLatin1().constData(), buf);
                }
                else
                {
		            printBufferInfo(QString("* buffer 0x%1 ->").arg((size_t)buf,0,16).toLatin1().constData(), buf);
                }
	        }
        }

        retval += copyBufferToDataObject(latestBuffer.BufferHandle, destination);

        //queue buffer again
        handlesToQueueAgain.insert(latestBuffer.BufferHandle);
    }
    else
    {
        handlesToQueueAgain.insert(latestBuffer.BufferHandle);
    }

    

	ito::RetVal queueRetVal;

    foreach (const GenTL::BUFFER_HANDLE &bufferHandle, handlesToQueueAgain)
    {
        //queue buffer again
		queueRetVal += checkGCError(DSQueueBuffer(m_handle, bufferHandle));
    }

    if (m_verbose >= VERBOSE_ALL)
    {
        std::cout << "Buffer info after queuing buffers\n" << std::endl;
	    foreach(const GenTL::BUFFER_HANDLE &buf, handlesToQueueAgain)
	    {
            printBufferInfo(QString("* buffer 0x%1 ->").arg((size_t)buf,0,16).toLatin1().constData(), buf);
	    }
    }

	if (queueRetVal.containsError())
	{
		std::cout << "Error queuing buffers: " << queueRetVal.errorMessage() << "\n" << std::endl;
	}

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
bool GenTLDataStream::checkForErrorEvent(ito::RetVal &retval, const QString &errorPrefix)
{
    //check if a possible error has been signaled
    if (m_errorEvent != GENTL_INVALID_HANDLE)
    {
        char bufferIn[1024];
        size_t sizeIn = sizeof(bufferIn);
        GenTL::GC_ERROR err = EventGetData(m_errorEvent, &bufferIn, &sizeIn, 0);
        if (err == GenTL::GC_ERR_SUCCESS)
        {
            QString errPrefix = (errorPrefix.isEmpty() ? "" : errorPrefix + ". ");
            if (EventGetDataInfo)
            {
                GenTL::INFO_DATATYPE piType;
                char bufferOut[512];
                size_t pSizeOut = sizeof(bufferOut);

                GenTL::GC_ERROR err2 = EventGetDataInfo(m_errorEvent, &bufferIn, sizeIn, GenTL::EVENT_DATA_VALUE, &piType, &bufferOut, &pSizeOut);

                if (err2 == GenTL::GC_ERR_SUCCESS)
                {
                    if (piType == GenTL::INFO_DATATYPE_STRING)
                    {
                        bufferOut[511] = '\0';
                        retval += ito::RetVal::format(ito::retError, 0, "%sSignaled error: %s.", bufferOut, errPrefix.toLatin1().constData());
                    }
                    else
                    {
                        retval += ito::RetVal::format(ito::retError, 0, "%sAn error has been signaled, however no more information could be fetched due to unsupported datatype returned from method 'EventGetDataInfo'", errPrefix.toLatin1().constData());
                    }
                }
                else
                {
                    retval += ito::RetVal::format(ito::retError, 0, "%sAn error has been signaled, however no more information could be fetched due to error returned from method 'EventGetDataInfo'", errPrefix.toLatin1().constData());
                }
            }
            else
            {
                retval += ito::RetVal::format(ito::retError, 0, "%sAn error has been signaled, however no more information could be fetched due to missing method 'EventGetDataInfo'", errPrefix.toLatin1().constData());
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::startAcquisition(GenTL::ACQ_START_FLAGS startFlags /*= GenTL::ACQ_START_FLAGS_DEFAULT*/)
{
    if (m_acquisitionStarted)
    {
        return ito::RetVal(ito::retWarning, 0, "acquisition already started");
    }

    ito::RetVal ret = checkGCError(DSStartAcquisition(m_handle, startFlags, GENTL_INFINITE), "start acquisition");
    if (!ret.containsError())
    {
        m_acquisitionStarted = true;
    }

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::stopAcquisition(GenTL::ACQ_STOP_FLAGS stopFlags /*= GenTL::ACQ_STOP_FLAGS_DEFAULT*/)
{
    if (!m_acquisitionStarted)
    {
        return ito::RetVal(ito::retWarning, 0, "acquisition could not be stopped since it is currently not running.");
    }

    ito::RetVal ret = checkGCError(DSStopAcquisition(m_handle, stopFlags), "stop acquisition");
    if (!ret.containsError())
    {
        m_acquisitionStarted = false;
    }

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------
bool GenTLDataStream::setPayloadSize(int payloadSize)
{
	if (m_payloadSize != payloadSize)
	{
		m_payloadSize = payloadSize;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------------------
void GenTLDataStream::setTimeoutSec(double timeout)
{
	m_timeoutMS = timeout * 1000.0;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::copyBufferToDataObject(const GenTL::BUFFER_HANDLE buffer, ito::DataObject &dobj)
{
	ito::RetVal retval;
	GenTL::INFO_DATATYPE dtype;
	size_t pSize;
	size_t sizeFilled;
	size_t size;
	size_t temp;
	uint64_t temp64;
	char* ptr;
	GenTL::PIXELENDIANNESS_IDS endianess;
	GenTL::PAYLOADTYPE_INFO_IDS payloadType;
	GenTL::PIXELFORMAT_NAMESPACE_IDS pixelformatNamespace;
	uint64_t pixelformat = 0;
	size_t height;
	size_t width;
	float bytes_pp_transferred = 0.0;

	//request mandatory parameter: size of buffer
	pSize = sizeof(size);
	retval += checkGCError(DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_SIZE, &dtype, &size, &pSize), "request size of image buffer");
	if (!retval.containsError())
	{
		if (dtype != GenTL::INFO_DATATYPE_SIZET)
		{
			retval += ito::RetVal(ito::retError, 0, "request size of image buffer: returned value is not of expected type (size_t)");
		}
	}

	//request pixelsize
	if (!retval.containsError())
	{
		//pixelformat
		pSize = sizeof(pixelformat);
		retval += checkGCError(DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_PIXELFORMAT, &dtype, &pixelformat, &pSize), "get pixelformat of image buffer");

		if (pixelformat == 0)
		{
			int dims = dobj.getDims();
			width = dobj.getSize(dims - 1);
			height = dobj.getSize(dims - 2);

			//try to guess right pixelformat
			if (dobj.getType() == ito::tUInt16 && size >= (sizeof(ito::uint16) * (width * height)))
			{
				pixelformat = PFNC_Mono16;
			}
			else if (dobj.getType() == ito::tUInt8 && size >= (sizeof(ito::uint8) * (width * height)))
			{
				pixelformat = PFNC_Mono8;
			}
		}

		bytes_pp_transferred = (float)PFNC_PIXEL_SIZE(pixelformat) / 8.0;
	}

	//request mandatory parameter: width and height of image
	pSize = sizeof(width);
	retval += checkGCError(DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_WIDTH, &dtype, &width, &pSize), "request width of image buffer");
	pSize = sizeof(height);
	retval += checkGCError(DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_HEIGHT, &dtype, &height, &pSize), "request height of image buffer");

    size_t buffer_offset = 0;

    //request optional parameter: buffer offset
    pSize = sizeof(buffer_offset);
    if (DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_IMAGEOFFSET, &dtype, &buffer_offset, &pSize) != GenTL::GC_ERR_SUCCESS)
    {
        buffer_offset = 0;
    }
    else
    {
        if (dtype != GenTL::INFO_DATATYPE_SIZET)
        {
            buffer_offset = 0;
            retval += ito::RetVal(ito::retError, 0, "request offset of image in buffer: returned value is not of expected type (size_t)");
        }
    }

	//request mandatory parameter: base pointer
	pSize = sizeof(ptr);
	retval += checkGCError(DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_BASE, &dtype, &ptr, &pSize), "request pointer of image buffer");
	if (!retval.containsError())
	{
		if (dtype != GenTL::INFO_DATATYPE_PTR)
		{
			retval += ito::RetVal(ito::retError, 0, "request pointer of image buffer: returned value is not of expected type (ptr)");
		}
	}

	if (!retval.containsError())
	{
		pSize = sizeof(sizeFilled);
		if (DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_SIZE_FILLED, &dtype, &sizeFilled, &pSize) == GenTL::GC_ERR_SUCCESS)
		{
			if (sizeFilled < size)
			{
				if (bytes_pp_transferred > 0.0)
				{
					if (sizeFilled < (width * height * bytes_pp_transferred))
					{
						retval += ito::RetVal::format(ito::retWarning, 0, "returned image buffer is only partially filled."); //this message has a dot at the end (in order to distinguish it from the message below)
					}
				}
				else
				{
					retval += ito::RetVal::format(ito::retWarning, 0, "returned image buffer is only partially filled"); //no dot here (see comment above)
				}
			}
		}
	}

	if (!retval.containsError())
	{
		//payload type -> only accept PAYLOAD_TYPE_IMAGE
		pSize = sizeof(temp);
		GenTL::GC_ERROR err = DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_PAYLOADTYPE, &dtype, &temp, &pSize);
		if (err == GenTL::GC_ERR_SUCCESS)
		{
			payloadType = (GenTL::PAYLOADTYPE_INFO_IDS)temp;

			if (payloadType != GenTL::PAYLOAD_TYPE_IMAGE)
			{
				retval += ito::RetVal(ito::retError, 0, "currently only buffers that contain image data are supported");
			}
		}
		else if (err == GenTL::GC_ERR_NOT_IMPLEMENTED)
		{
			//payload type not implemented -> no information -> hope everything is ok
		}
		else
		{
			retval += checkGCError(err, "get payload type of image buffer");
		}

		//pixelformat namespace
		pSize = sizeof(temp64);
		err = DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_PIXELFORMAT_NAMESPACE, &dtype, &temp64, &pSize);
		if (err == GenTL::GC_ERR_NOT_IMPLEMENTED)
		{
			pixelformatNamespace = GenTL::PIXELFORMAT_NAMESPACE_UNKNOWN;
		}
		else
		{
			retval += checkGCError(err, "get pixelformat namespace of image buffer");

			if (!retval.containsError())
			{
				pixelformatNamespace = (GenTL::PIXELFORMAT_NAMESPACE_IDS)temp64;
			}
		}
		
		

		//get pixel endianess
		if (pixelformat != PFNC_Mono8)
		{
			pSize = sizeof(temp);
			if (DSGetBufferInfo(m_handle, buffer, GenTL::BUFFER_INFO_PIXEL_ENDIANNESS, &dtype, &temp, &pSize) != GenTL::GC_ERR_SUCCESS)
			{
				endianess = GenTL::PIXELENDIANNESS_LITTLE; //default
				m_endianessChanged = false;
			}
			else
			{
				endianess = (GenTL::PIXELENDIANNESS_IDS)temp;

				if (endianess == GenTL::PIXELENDIANNESS_UNKNOWN)
				{
					if (m_endianessChanged)
					{
						retval += ito::RetVal(ito::retWarning, 0, "camera does not provide the pixel endianess: little endian is assumed.");
					}
					endianess = GenTL::PIXELENDIANNESS_LITTLE; //default
					m_endianessChanged = false;
				}
			}
		}
		else
		{
			endianess = GenTL::PIXELENDIANNESS_LITTLE; //default
			m_endianessChanged = false;
		}
	}

	if (!retval.containsError())
	{
		switch (pixelformat)
		{
		case PFNC_Mono8:
			retval += copyMono8ToDataObject(ptr + buffer_offset, width, height, endianess == GenTL::PIXELENDIANNESS_LITTLE, dobj);
			break;
		case PFNC_Mono10:
		case PFNC_Mono12:
		case PFNC_Mono14:
		case PFNC_Mono16:
			retval += copyMono10to16ToDataObject(ptr + buffer_offset, width, height, endianess == endianess, dobj);
			break;
		case GVSP_Mono12Packed: //GigE specific
		case PFNC_Mono12p:
			retval += copyMono12pToDataObject(ptr + buffer_offset, width, height, endianess == endianess, dobj);
			break;
		default:
			retval += ito::RetVal::format(ito::retError, 0, "Pixel format %i (%s) is not yet supported.", pixelformat, GetPixelFormatName((PfncFormat)pixelformat));
			break;
		}
	}

	return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::copyMono8ToDataObject(const char* ptr, const size_t &width, const size_t &height, bool littleEndian, ito::DataObject &dobj)
{
	//little or big endian is idle for mono8:
	return dobj.copyFromData2D<ito::uint8>((const ito::uint8*)ptr, width, height);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::copyMono10to16ToDataObject(const char* ptr, const size_t &width, const size_t &height, bool littleEndian, ito::DataObject &dobj)
{
	if (littleEndian)
	{
		return dobj.copyFromData2D<ito::uint16>((const ito::uint16*)ptr, width, height);
	}
	else
	{
		return ito::RetVal(ito::retError, 0, "big endian for mono10, mono12, mono14, mono16 currently not supported.");
	}
}

//----------------------------------------------------------------------------------------------------------------------------------
void unpack12into16lsb(ito::uint16 *dest, const ito::uint8 *source, size_t n)
{
	const ito::uint8 *end = source + n;
	ito::uint8 b0, b1, b2;

	while (source != end)
	{
		b0 = *source++;
		b1 = *source++;
		b2 = *source++;
		
		//byte 0: pixel 0, bit 11..4
		//byte 1: pixel 1, bit 3..0  FOLLOWED by pixel 0, bit 3..0
		//byte 2: pixel 1, bit 11..4
		*dest++ = (b0 << 4) + (b1 & 0x0f);
		*dest++ = ((b1 & 0xf0) >> 4) + (b2 << 4);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal GenTLDataStream::copyMono12pToDataObject(const char* ptr, const size_t &width, const size_t &height, bool littleEndian, ito::DataObject &dobj)
{
	if (littleEndian)
	{
		if (width * height % 2 != 0)
		{
			return ito::RetVal(ito::retError, 0, "invalid numbers of pixels for datatype mono12p or mono12packed (width*height must be divisible by 2).");
		}

		ito::uint16 *dest = dobj.rowPtr<ito::uint16>(0, 0);
		const ito::uint8 *source = (const ito::uint8*)ptr;
		unpack12into16lsb(dest, source, width * height * 12 / 8);
		return ito::retOk;
	}
	else
	{
		return ito::RetVal(ito::retError, 0, "data converter for mono12p or mono12packed, most significant bit, not implemented yet.");
	}
}