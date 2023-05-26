/* ********************************************************************
    Plugin "AerotechA3200" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2018, Institut fuer Technische Optik (ITO),
    Universitaet Stuttgart, Germany

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

/**\file aerotechA3200.h
* \brief In this file the class of the non modal dialog for the AerotechA3200 are specified
*
*/
#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"

#include "ui_dockWidgetAerotechA3200.h"    //! Header-file generated by Qt-Gui-Editor which has to be called

#include <QtGui>
#include <qwidget.h>
#include <qmap.h>
#include <qstring.h>
#include <qvector.h>
#include <qsignalmapper.h>

namespace ito //Forward-Declaration
{
    class AddInActuator;
}

//----------------------------------------------------------------------------------------------------------------------------------
class DockWidgetAerotechA3200 : public QWidget
{
    Q_OBJECT
    public:
        DockWidgetAerotechA3200(QMap<QString, ito::Param> params, int uniqueID, ito::AddInActuator *actuator); //, ito::AddInActuator * myPlugin);    //!< Constructor called by AerotechA3200::Constructor
        ~DockWidgetAerotechA3200() {};

    private:
        Ui::DockWidgetAerotechA3200 ui;    //! Handle to the dialog
        ito::AddInActuator *m_actuator;

        int m_numaxis;                    //! Number of axis
        void CheckAxisNums(QMap<QString, ito::Param> params);    //! This functions checks all axis after parameters has changed and blocks unspecified axis
        //ito::AddInActuator *m_pMyPlugin;    //! Handle to the attached motor to enable / disable connections
//        bool m_isVisible;

        void enableWidget(bool enabled);
//        void visibleWidget();

        bool m_initialized;

        QVector<QCheckBox*> m_pWidgetEnabled;
        QVector<QDoubleSpinBox*> m_pWidgetActual;
        QVector<QDoubleSpinBox*> m_pWidgetTarget;
        QVector<QPushButton*> m_pWidgetPosInc;
        QVector<QPushButton*> m_pWidgetPosDec;

        QSignalMapper *m_pSignalMapperEnabled;
        QSignalMapper *m_pSignalPosInc;
        QSignalMapper *m_pSignalPosDec;

        void DockWidgetAerotechA3200::MoveRelative(const int &axis, const double dpos);
        void Move(const QVector<int> axis, const QVector<double> dpos, const char* func);

    signals:
//        void MoveRelative(const int axis, const double pos, ItomSharedSemaphore *waitCond = NULL);    //!< This signal is connected to SetPosRel
//        void MoveAbsolute(QVector<int> axis,  QVector<double> pos, ItomSharedSemaphore *waitCond = NULL); //!< This signal is connected to SetPosAbs
        void MotorTriggerStatusRequest(bool sendActPosition, bool sendTargetPos);    //!< This signal is connected to RequestStatusAndPosition

    public slots:
        void init(QMap<QString, ito::Param> params, QStringList axisNames);
        void valuesChanged(QMap<QString, ito::Param> params);    //!< Slot to recive the valuesChanged signal
        void actuatorStatusChanged(QVector<int> status, QVector<double> actPosition); //!< slot to receive information about status and position changes.
        void targetChanged(QVector<double> targetPositions);

    private slots:
        void on_pb_Start_clicked();
        void on_pb_Stop_clicked();
        void on_pb_Refresh_clicked();

        void checkEnabledClicked(const int &index);  //!< This button disables the current GUI-Elements for the specified axis
        void posIncrementClicked(const int &index);  //!< If the Botton is clicked a MoveRelative()-Signal is emitted
        void posDecrementClicked(const int &index);  //!< If the Botton is clicked a MoveRelative()-Signal is emitted
};
