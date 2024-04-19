/* ********************************************************************
    Plugin "FaulhaberMCS" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2024, Institut f�r Technische Optik (ITO),
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

#include "dialogFaulhaberMCS.h"

#include "common/addInInterface.h"

#include <qdialogbuttonbox.h>
#include <qsharedpointer.h>
#include <qvector.h>

//----------------------------------------------------------------------------------------------------------------------------------
DialogFaulhaberMCS::DialogFaulhaberMCS(ito::AddInActuator* actuator) :
    AbstractAddInConfigDialog(actuator), m_firstRun(true), m_pluginPointer(actuator)
{
    ui.setupUi(this);

    // disable dialog, since no parameters are known yet. Parameters will immediately be sent by the
    // slot parametersChanged.
    enableDialog(false);
    ui.ParamEditorWidget->setPlugin(m_pluginPointer);
};

//----------------------------------------------------------------------------------------------------------------------------------
DialogFaulhaberMCS::~DialogFaulhaberMCS()
{
    QSharedPointer<ito::ParamBase> p(
        new ito::ParamBase("enableConfiguration", ito::ParamBase::Int, 0));
    setPluginParameter(p, msgLevelWarningAndError);
};

//----------------------------------------------------------------------------------------------------------------------------------
void DialogFaulhaberMCS::parametersChanged(QMap<QString, ito::Param> params)
{
    // save the currently set parameters to m_currentParameters
    m_currentParameters = params;

    if (m_firstRun)
    {
        setWindowTitle(
            QString((params)["name"].getVal<char*>()) + " - " + tr("Configuration Dialog"));

        // this is the first time that parameters are sent to this dialog,
        // therefore you can add some initialization work here
        m_firstRun = false;

        // now activate group boxes, since information is available now (at startup, information is
        // not available, since parameters are sent by a signal)
        enableDialog(true);
    }

    // set the status of all widgets depending on the values of params
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DialogFaulhaberMCS::applyParameters()
{
    ito::RetVal retValue(ito::retOk);
    QVector<QSharedPointer<ito::ParamBase>> values;
    bool success = false;

    // foreach widget, do:
    //    check if the current value of the widget is different than the corresponding
    //    parameter in m_currentParameters.
    //    If so, write something like:
    //    values.append(QSharedPointer<ito::ParamBase>(new
    //    ito::ParamBase("name",ito::ParamBase::Int, newValue)))

    retValue += setPluginParameters(values, msgLevelWarningAndError);

    return retValue;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFaulhaberMCS::on_buttonBox_clicked(QAbstractButton* btn)
{
    ito::RetVal retValue(ito::retOk);

    QDialogButtonBox::ButtonRole role = ui.buttonBox->buttonRole(btn);

    if (role == QDialogButtonBox::RejectRole)
    {
        reject(); // close dialog with reject
    }
    else if (role == QDialogButtonBox::AcceptRole)
    {
        accept(); // AcceptRole
    }
    else
    {
        applyParameters(); // ApplyRole
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFaulhaberMCS::enableDialog(bool enabled)
{
}
