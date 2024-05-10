/* ********************************************************************
Plugin "NerianSceneScanPro" for itom software
URL: http://www.uni-stuttgart.de/ito
Copyright (C) 2019, Institut für Technische Optik (ITO),
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

#ifndef DIALOGNERIANSCENESCANPRO_H
#define DIALOGNERIANSCENESCANPRO_H

#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"
#include "common/abstractAddInConfigDialog.h"

#include "ui_dialogNerianSceneScanPro.h"

#include <qstring.h>
#include <qmap.h>
#include <qabstractbutton.h>

namespace ito
{
    class AddInBase; //forward declaration
}

class DialogNerianSceneScanPro : public ito::AbstractAddInConfigDialog
{
    Q_OBJECT

    public:
        DialogNerianSceneScanPro(ito::AddInBase *grabber);
        ~DialogNerianSceneScanPro() {};

        ito::RetVal applyParameters();


    private:
        void enableDialog(bool enabled);
        bool m_firstRun;

        Ui::DialogNerianSceneScanPro ui;
        QPointer<ito::AddInBase> m_pluginPointer;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);

    private slots:
        void on_buttonBox_clicked(QAbstractButton* btn);
};

#endif
