#ifndef DOCKWIDGETDUMMYGRABBER_H
#define DOCKWIDGETDUMMYGRABBER_H

#include "common/sharedStructures.h"

#include <QtGui>
#include <qwidget.h>
#include <qmap.h>
#include <qstring.h>

#include "ui_dockWidgetDummyGrabber.h"

class DockWidgetDummyGrabber : public QWidget
{
    Q_OBJECT

    public:
        DockWidgetDummyGrabber(QMap<QString, ito::Param> params, int uniqueID);
        ~DockWidgetDummyGrabber() {};

    private:
        Ui::DockWidgetDummyGrabber ui;

    signals:
        void GainOffsetPropertiesChanged(double gain, double offset);
        void IntegrationPropertiesChanged(double integrationtime);

    public slots:
        void valuesChanged(QMap<QString, ito::Param> params);


    private slots:
        void on_spinBox_offset_valueChanged(int d);
        void on_spinBox_gain_valueChanged(int d);
        void on_doubleSpinBox_integration_time_valueChanged(double d);
};

#endif
