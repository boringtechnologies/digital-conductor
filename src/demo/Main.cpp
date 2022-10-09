//*****************************************************************************
//*****************************************************************************

#include "controllers/appcontroller.h"
#include "core/media/util.h"

#include "QmlEngine/QmlEngine.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QCommandLineParser>
#include <QRandomGenerator>

//*****************************************************************************
//*****************************************************************************
void registerTypes(QQmlEngine & engine);

//*****************************************************************************
//*****************************************************************************
QString autogenerateName(const uint32_t length = 4)
{
    const static char chars[] = "";

    QString result;
    for (uint32_t i = 0; i < length; ++i)
    {
        result += static_cast<char>(QRandomGenerator::global()->bounded('a', 'z'));
    }

    return result;
}

//*****************************************************************************
//*****************************************************************************
int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("DC");
    QCoreApplication::setApplicationName("DigitalConductor");
    QCoreApplication::setApplicationVersion("0.1");

    QGuiApplication app(argc, argv);

    // init media
    // TODO use gst plugin path
    dc::media::Util & util = dc::media::Util::instance();
    util.initializeMediaModule("");

    // TODO maybe remove custom engine?
    QmlEngine::initialize();

    QQmlApplicationEngine engine;
    registerTypes(engine);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption server(QStringList() << "s" << "server",
                              QCoreApplication::translate("main", "Signalling server"),
                              QCoreApplication::translate("main", "ip:port"));
    QCommandLineOption user(QStringList() << "u" << "user",
                            QCoreApplication::translate("main", "User name"),
                            QCoreApplication::translate("main", "user"));
    QCommandLineOption group(QStringList() << "g" << "group",
                             QCoreApplication::translate("main", "User group"),
                             QCoreApplication::translate("main", "group"));

    parser.addOptions({server, user, group});
    parser.process(app);

    QString signallingServer("ec2-3-223-54-164.compute-1.amazonaws.com:50051");
    if (parser.isSet(server))
    {
        signallingServer = parser.value(server);
    }

    QString userGroup("default");
    if (parser.isSet(group))
    {
        userGroup = parser.value(group);
    }

    QString userName;
    if (parser.isSet(user))
    {
        userName = parser.value(user);
    }
    else
    {
        // generate user name
        std::cout << "generate user name" << std::endl;
        userName = autogenerateName();
    }

    std::cout << "connect to " << signallingServer.toStdString()
              << " as " << userName.toStdString()
              << "-" << userGroup.toStdString() << std::endl;

    AppController * ac = AppController::instance();
    ac->init(userName, userGroup, signallingServer);

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
