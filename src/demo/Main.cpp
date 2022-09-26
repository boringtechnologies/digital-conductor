//*****************************************************************************
//*****************************************************************************

#include "QmlEngine/QmlEngine.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

//*****************************************************************************
//*****************************************************************************
void registerTypes(QQmlEngine & engine);

//*****************************************************************************
//*****************************************************************************
int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("DC");
    QCoreApplication::setApplicationName("DigitalConductor");

    QGuiApplication app(argc, argv);

    // TODO maybe remove custom endine?
    QmlEngine::initialize();

    QQmlApplicationEngine engine;
    registerTypes(engine);

    const QUrl url("qrc:dc/Main.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    QPixmap appIcon(":img/app_icon.svg");
    app.setWindowIcon(appIcon);

    return app.exec();
}
