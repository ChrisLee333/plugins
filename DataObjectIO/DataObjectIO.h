/* ********************************************************************
    Plugin "DataObjectIO" for itom software
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

#ifndef DATAOBJECTIO_H
#define DATAOBJECTIO_H

#include "common/addInInterface.h"
#include <qsharedpointer.h>
#include <qfile.h>
#include <qfileinfo.h>

//----------------------------------------------------------------------------------------------------------------------------------
/** @class DataObjectIOInterface
*   @brief DataIO functions implemented in former ito mcpp measurement programm
*
*   AddIn Interface for the MCPPDataIO class s. also \ref MCPPDataIO
*/
class DataObjectIOInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
#if QT_VERSION >=  QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "ito.AddInInterfaceBase" )
#endif
    Q_INTERFACES(ito::AddInInterfaceBase)
    PLUGIN_ITOM_API

    protected:

    public:
        DataObjectIOInterface();
        ~DataObjectIOInterface();
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);

    signals:

    public slots:
};

//----------------------------------------------------------------------------------------------------------------------------------
/** @class DataObjectIO
*   @brief Algorithms used to process images and dataobjects with filters provided by openCV
*
*   In this class the algorithms used for the processing of images are implemented.
*   The filters wrapp openCV-Filters to python interface. Handling of 3D-Objects differs depending on the filter.
*
*/
class DataObjectIO : public ito::AddInAlgo
{
    Q_OBJECT

    protected:
        DataObjectIO();
        ~DataObjectIO() {};

    public:
        friend class DataObjectIOInterface;

        enum ImageFormat
        {
            noFormat = 0x00,
            tiffFormat = 0x01,
            pgmFormat = 0x02,
            ppmFormat = 0x03,
            jpgFormat = 0x04,
            jp2000Format = 0x05,
            bmpFormat = 0x06,
            pngFormat = 0x07,
            sunFormat = 0x08,
            gifFormat = 0x09,
            xpmFormat = 0x0a
        };

        enum
        {
            invWrite = 0x00,
            invIgnor = 0x01,
            invChange = 0x02,
            invBAD = 0x03,
            invHandlingMask = 0x0F
        } tInvalidHandling;

        static const char *saveDataObjectDoc;
        static ito::RetVal saveDataObject(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveDataObjectParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *loadDataObjectDoc;
        static ito::RetVal loadDataObject(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal loadDataObjectParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *loadDataFromTxtDoc;
        static ito::RetVal loadDataFromTxt(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal loadDataFromTxtParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveNistSDFDoc;
        static ito::RetVal saveNistSDF(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveNistSDFParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveItomIDODoc;
        static ito::RetVal saveItomIDO(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveItomIDOParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveTiffDoc;
        static ito::RetVal saveTiff(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveTiffParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveJPGDoc;
        static ito::RetVal saveJPG(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveJPGParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *savePNGDoc;
        static ito::RetVal savePNG(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal savePNGParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveGIFDoc;
        static ito::RetVal saveGIF(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveGIFParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveXPMDoc;
        static ito::RetVal saveXPM(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveXPMParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveBMPDoc;
        static ito::RetVal saveBMP(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveBMPParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *savePPMDoc;
        static ito::RetVal savePPM(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal savePPMParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *savePGMDoc;
        static ito::RetVal savePGM(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal savePGMParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *saveRASDoc;
        static ito::RetVal saveRAS(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal saveRASParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *loadImageDoc;
        static ito::RetVal loadImage(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal loadImageParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *loadNistSDFDoc;
        static ito::RetVal loadNistSDF(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal loadNistSDFParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

        static const char *loadItomIDODoc;
        static ito::RetVal loadItomIDO(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut);
        static ito::RetVal loadItomIDOParams(QVector<ito::Param> *paramsMand, QVector<ito::Param> *paramsOpt, QVector<ito::Param> *paramsOut);

    private:
        static ito::RetVal saveDataObjectOpenCV(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut, const ImageFormat &imageFormat);
        static ito::RetVal saveDataObjectQt(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, QVector<ito::ParamBase> *paramsOut, const ImageFormat &imageFormat);

        static ito::RetVal analyseTXTData(QFile &inFile, ito::DataObject &newObject, QChar &sperator, QChar &decimalSign, const int flags, const int ignoreLines);
        static ito::RetVal readTXTDataBlock(QFile &inFile, ito::DataObject &newObject, const QChar &sperator, const QChar &decimalSign, const int flags, const int ignoreLines);

        template<typename _Tp> static ito::RetVal writeDataBlock(QFile &outFile, const ito::DataObject *scrObject, const double zScale, const int decimals, const int flags, const char seperator, const double nanValue);
        template<typename _Tp> static ito::RetVal readDataBlock(QFile &inFile, ito::DataObject &newObject, const double zScale, const int flags, const QByteArray &nanString);
        static ito::RetVal readNistHeader(QFile &inFile, ito::DataObject &newObject, double &zscale, const int flags, const std::string &xyUnit, const std::string &valueUnit, QByteArray &nanString);

        static void checkAndModifyFilenameSuffix(QFileInfo &file, const QString &desiredAndAllowedSuffix, const QString &allowedSuffix2 = QString(), const QString &allowedSuffix3 = QString());

    public slots:
        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal close(ItomSharedSemaphore *waitCond);
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // DATAOBJECTIO_H
