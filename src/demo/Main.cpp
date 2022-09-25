#include "QmlEngine/QmlEngine.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QmlEngine::initialize();

    QQmlApplicationEngine engine;
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
