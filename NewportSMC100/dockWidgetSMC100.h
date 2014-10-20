/* ********************************************************************
    Plugin "Newport SMC100" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2014, Institut f�r Technische Optik (ITO),
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

#ifndef DOCKWIDGETSMC100_H
#define DOCKWIDGETSMC100_H

#include "common/abstractAddInDockWidget.h"
#include "common/addInInterface.h"

#include <qmap.h>
#include <qstring.h>

#include "ui_dockWidgetSMC100.h"

class DockWidgetSMC100 : public ito::AbstractAddInDockWidget
{
    Q_OBJECT

    public:
        DockWidgetSMC100(int uniqueID, ito::AddInActuator * myPlugin);
        ~DockWidgetSMC100() {};

    private:
        void enableWidget(bool enabled);

        Ui::DockWidgetSMC100 ui;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);
        void identifierChanged(const QString &identifier) {};
        
        void actuatorStatusChanged(QVector<int> status, QVector<double> actPosition);
        void targetChanged(QVector<double> targetPositions);

    private slots:
        void on_radioLocal_clicked();
        void on_radioRemote_clicked() { on_radioLocal_clicked(); }
        void on_btnUp_clicked();
        void on_btnDown_clicked();
        void on_btnStart_clicked();
        void on_btnRefresh_clicked();
};

#endif
