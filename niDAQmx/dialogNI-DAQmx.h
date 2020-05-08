/* ********************************************************************
    Plugin "niDAQmx" for itom software
    URL: http://www.bitbucket.org/itom/plugins
    Copyright (C) 2020, Institut fuer Technische Optik, Universitaet Stuttgart

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

#ifndef DIALOGNIDAQMX_H
#define DIALOGNIDAQMX_H

#include "common/param.h"
#include "common/retVal.h"
#include "common/sharedStructuresQt.h"
#include "common/abstractAddInConfigDialog.h"

#include "ui_dialogNI-DAQmx.h"

#include <qstring.h>
#include <qmap.h>
#include <qabstractbutton.h>
#include <qtreewidget.h>

#include "NI-DAQmx.h"


namespace ito
{
    class AddInBase; //forward declaration
}

class DialogNiDAQmx : public ito::AbstractAddInConfigDialog 
{
    Q_OBJECT

    public:
        DialogNiDAQmx(ito::AddInBase *adda);
        ~DialogNiDAQmx() {};

        ito::RetVal applyParameters();

    private:
        void enableDialog(bool enabled);
        bool m_firstRun;

        Ui::niDAQmx ui;
        QMap<QString, QTreeWidgetItem*> m_channelItems;
        bool m_channelsModified;

        //!< the m_channelItems contain additional information in user roles. These are:
        enum ChannelRole
        {
            CrPhysicalName = Qt::UserRole,
            CrConfigStr = Qt::UserRole + 1,
            CrModified = Qt::UserRole + 2
        };

        void disableChannelProps();
        void setChannelPropsAI(int terminalConfig, double minV, double maxV);
        void setChannelPropsAO(double minV, double maxV);
        void setChannelPropsDI();
        void setChannelPropsDO();

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);

    private slots:
        void on_buttonBox_clicked(QAbstractButton* btn);
        void on_comboStartTriggerMode_currentTextChanged(QString text);
        void on_treeChannels_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
        void on_treeChannels_itemChanged(QTreeWidgetItem *item, int column);
};

#endif
