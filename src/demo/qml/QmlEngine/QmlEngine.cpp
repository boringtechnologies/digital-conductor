// gadgets
#include "DeviceDirection/DeviceDirection.hpp"
#include "DeviceInfo/DeviceInfo.hpp"
#include "PeerInfo/PeerInfo.hpp"

// controllers
#include "PeerController/PeerController.hpp"

// qml
#include "QmlEngine/QmlEngine.hpp"

namespace
{

QString decapitalize(QString s)
{
    s[0] = s[0].toLower();
    return s;
}

} // namespace

QmlEngine *QmlEngine::instance_ = nullptr;

void QmlEngine::initialize()
{
    if (!instance_)
    {
        instance_ = new QmlEngine;
        instance_->register_();
    }
}

void QmlEngine::deinitialize()
{
    delete instance_;
    instance_ = nullptr;
}

QQmlEngine *QmlEngine::instance()
{
    if (instance_)
    {
        return instance_->engine_;
    }
    else
    {
        return nullptr;
    }
}

QmlEngine::QmlEngine()
{
    engine_ = new QQmlEngine;

    moduleName_       = "DC";
    moduleVersion_[0] = 0;
    moduleVersion_[1] = 1;
}

QmlEngine::~QmlEngine()
{
    delete engine_;
}

void QmlEngine::register_()
{
    registerGadgets_();
    registerControllers_();
}

void QmlEngine::registerGadgets_()
{
#define DC_REGISTER_GADGET(T)                                                                                          \
    qmlRegisterType<T>(moduleName_, moduleVersion_[0], moduleVersion_[1], decapitalize(#T).toStdString().c_str())

    DC_REGISTER_GADGET(PeerInfo);
    DC_REGISTER_GADGET(DeviceDirection);
    DC_REGISTER_GADGET(DeviceInfo);

#undef DC_REGISTER_GADGET
}

void QmlEngine::registerControllers_()
{
#define DC_REGISTER_CONTROLLER(T)                                                                                      \
    qmlRegisterSingletonInstance(moduleName_, moduleVersion_[0], moduleVersion_[1], #T, &T::instance());

    DC_REGISTER_CONTROLLER(PeerController);

#undef DC_REGISTER_CONTROLLER
}
