//*****************************************************************************
//*****************************************************************************

#include "controllers/appcontroller.h"
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

    // TODO maybe remove custom endine?
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

    if (!parser.isSet(server) || !parser.isSet(group))
    {
        parser.showHelp();
        return 0;
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

    std::cout << "connect as " << userName.toStdString() << "-" << parser.value(group).toStdString() << std::endl;

    AppController * ac = AppController::instance();
    ac->init(userName, parser.value(group), parser.value(server));

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
