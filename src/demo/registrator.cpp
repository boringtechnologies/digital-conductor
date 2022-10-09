//*****************************************************************************
//*****************************************************************************

#include "controllers/appcontroller.h"
#include "models/peermodel.h"
#include "models/devicemodel.h"

#include <QQmlApplicationEngine>

//*****************************************************************************
//*****************************************************************************
void registerTypes(QQmlEngine & engine)
{
    qmlRegisterSingletonType<AppController>("DC.controller", 1, 0, "Controller", &AppController::instance);

    qmlRegisterType<PeerModel>  ("DC.model", 1, 0, "PeerModel");
    qmlRegisterType<DeviceModel>("DC.model", 1, 0, "DeviceModel");

//    qmlRegisterSingletonType<WelcomeViewModel>     ("DC.model", 1, 0, "Welcome",      &WelcomeViewModel::instance);

//    qmlRegisterSingletonType(QUrl("qrc:/Style.qml"), "DC.style", 1, 0, "Style" );

//    qmlRegisterUncreatableMetaObject(TD::staticMetaObject, "DC.namespace", 1, 0, "TD", "Error: only enums");
}
