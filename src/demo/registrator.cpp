//*****************************************************************************
//*****************************************************************************

#include "controllers/appcontroller.h"

#include <QQmlApplicationEngine>

//*****************************************************************************
//*****************************************************************************
void registerTypes(QQmlEngine & engine)
{
    qmlRegisterSingletonType<AppController>("DC.controller", 1, 0, "Controller", &AppController::instance);

//    qmlRegisterSingletonType<WelcomeViewModel>     ("DC.model", 1, 0, "Welcome",      &WelcomeViewModel::instance);

//    qmlRegisterSingletonType(QUrl("qrc:/Style.qml"), "DC.style", 1, 0, "Style" );

//    qmlRegisterUncreatableMetaObject(TD::staticMetaObject, "DC.namespace", 1, 0, "TD", "Error: only enums");
}
