#ifndef GWINSTEKPSP_H
#define GWINSTEKPSP_H

#include "common/addInInterface.h"

#include "dialogGWInstekPSP.h"
#include "dockWidgetGWInstekPSP.h"

#include <qsharedpointer.h>

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class	GWInstekPSP 
  *\brief	This class can be used to communicate with PSP-405, PSP-603 and PSP-2010
  *
  *         This class can be used to work with PSP-405, PSP-603 and PSP-2010.
  *			This system needs a serial port which following settings:
  *			baud = 2400
  *			bits = 8
  *			parity = 0
  *			stopbits = 1
  *			flow = 16 (dtr = enabled)
  *			endline = "\r"
  *
  * \todo write driver
  *	\sa	SerialIO, AddInActuator, DummyMotor, dialogGWInstekPSP, DockWidgetGWInstekPSP
  *	\date	25.11.2011
  *	\author	Heiko Bieger
  * \warning	NA
  *
  */
class GWInstekPSP : public ito::AddInDataIO 
{
    Q_OBJECT

    protected:
        ~GWInstekPSP();
        GWInstekPSP();

    public:
        friend class GWInstekPSPInterface;
        friend class dialogGWInstekPSP;
		const ito::RetVal showConfDialog(void);	/*!<shows the configuration dialog*/
        int hasConfDialog(void) { return 1; } //!< indicates that this plugin has got a configuration dialog

    private:
        ito::AddInDataIO *m_pSer;
		char m_status[38];		//! Contains status string [Vvv.vvAa.aaaWwww.wUuuIi.iiPpppFffffff]
//                                                          0000000000111111111122222222223333333
//                                                          0123456789012345678901234567890123456

        const ito::RetVal SetParams();
        const ito::RetVal ReadFromSerial(bool *state);
        const ito::RetVal WriteToSerial(const char *text);
        static void doNotDelSharedPtr(char * /*ptr*/) {} /*!<workaround for deleter for QSharedPointer, such that the pointer is NOT deleted if shared-pointer's reference drops towards zero.*/

    public slots:
        void setParamVoltageFromWgt(double voltage);

        ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond = NULL);

        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal close(ItomSharedSemaphore *waitConde = NULL);

        ito::RetVal getVal(char *data, int *len, ItomSharedSemaphore *waitCond);
        ito::RetVal setVal(const void *data, const int length, ItomSharedSemaphore *waitCond);

    private slots:
};

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class	GWInstekPSPInterface 
  *
  *\brief	Interface-Class for GWInstekPSPInterface-Class
  *
  *	\sa	AddInActuator, GWInstekPSP
  *	\date	11.10.2010
  *	\author	Wolfram Lyda
  * \warning	NA
  *
  */
class GWInstekPSPInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
        Q_INTERFACES(ito::AddInInterfaceBase)

    protected:

    public:
        GWInstekPSPInterface();
        ~GWInstekPSPInterface();
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);


    signals:

    public slots:
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // LEICAMF_H
