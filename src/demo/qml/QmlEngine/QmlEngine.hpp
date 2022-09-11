#pragma once

#include <QQmlContext>
#include <QQmlEngine>

class QmlEngine
{
public:
    QmlEngine(const QmlEngine&) = delete;
    QmlEngine& operator=(const QmlEngine&) = delete;

    static void initialize();
    static void deinitialize();

    static QQmlEngine* instance();

private:
    QmlEngine();
    ~QmlEngine();

    void register_();
    void registerGadgets_();
    void registerInfos_();
    void registerControllers_();

    static QmlEngine* instance_;

    QQmlEngine* engine_ {};

    const char* moduleName_ {};
    int moduleVersion_[2] {};
};
