#ifndef DIALOGDUMMYMOTOR_H
#define DIALOGDUMMYMOTOR_H

/**\file dialogDummyMotor.h
* \brief In this file the class of the modal dialog for the DummyMotor are specified
*
*\sa dialogDummyMotor, DummyMotor
*\author Wolfram Lyda
*\date	Oct2011
*/

#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"
#include "common/addInInterface.h"

#include "ui_dialogDummyMotor.h"	//! Header-file generated by Qt-Gui-Editor which has to be called

#include <QtGui>
#include <qdialog.h>

//----------------------------------------------------------------------------------------------------------------------------------
/** @class dialogDummyMotor
*   @brief Config dialog functionality of DummyMotor
*
*   This class is used for the modal configuration dialog. It is used for parameter setup and calibration.
*	ui_dialogDummyMotor.h is generated by the Gui-Editor.
*
*\sa DummyMotor
*/
class dialogDummyMotor : public QDialog
{
    Q_OBJECT

    private:
        Ui::dialogDummyMotor ui;	//! Handle to the dialog
        ito::AddInActuator *m_pDummyMotor;	//! Handle to the attached motor to invoke calib command
        int m_enable[10];	//! Vector with the enabled / disabled axis for calibration command
        int m_numaxis;	//!	Number of axis of this device

    public:
        dialogDummyMotor(ito::AddInActuator *motor, int axisnums);
        ~dialogDummyMotor() {};
        int setVals(QMap<QString, ito::Param> *paramVals); //!< Function called by DummyMotor::showConfDialog to set parameters values at dialog startup
        int getVals(QMap<QString, ito::Param> *paramVals);	//!< Function called by DummyMotor::showConfDialog to get back the changed parameters

    public slots:


    private slots:
        void on_pushButtonCalib_clicked();	//!< If the Botton invokes a DummyMotor::Calib of enabled Axis
        void on_checkBox_EnableX_clicked(); //!< If the Botton "pushButton_xp" is clicked a MoveRelative()-Signal is emitted
        void on_checkBox_EnableY_clicked();	//!< If the Botton "pushButton_xp" is clicked a MoveRelative()-Signal is emitted
        void on_checkBox_EnableZ_clicked();	//!< If the Botton "pushButton_xp" is clicked a MoveRelative()-Signal is emitted
};

#endif
