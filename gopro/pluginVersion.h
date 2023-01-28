#ifndef PLUGINVERSION_H
#define PLUGINVERSION_H

#include "itom_sdk.h"

#define PLUGIN_VERSION_MAJOR 0
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 1
#define PLUGIN_VERSION_REVISION 0
#define PLUGIN_VERSION        CREATE_VERSION(PLUGIN_VERSION_MAJOR,PLUGIN_VERSION_MINOR,PLUGIN_VERSION_PATCH)
#define PLUGIN_VERSION_STRING CREATE_VERSION_STRING(PLUGIN_VERSION_MAJOR,PLUGIN_VERSION_MINOR,PLUGIN_VERSION_PATCH)
#define PLUGIN_COMPANY        "Institut fuer Technische Optik, University Stuttgart"
#define PLUGIN_AUTHOR         "Johann Krauter"
#define PLUGIN_COPYRIGHT      "(C) 2023, ITO, University Stuttgart"
#define PLUGIN_NAME           "GoPro"

//----------------------------------------------------------------------------------------------------------------------------------

#endif // PLUGINVERSION_H
