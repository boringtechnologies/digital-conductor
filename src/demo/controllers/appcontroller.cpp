//*****************************************************************************
//*****************************************************************************

#include "appcontroller.h"

#include "licensing/LicenseManager.hpp"

#include "client/grpcclient.h"

#include <QGuiApplication>
#include <QClipboard>

#include <memory>

//******************************************************************************
//******************************************************************************
namespace dc
{
} // namespace dc

//******************************************************************************
//******************************************************************************
class AppController::impl
{
    friend class AppController;

protected:
    impl(AppController * owner);

    AppController      * m_o;

    AppController::State m_state;

    std::string m_target;
    std::string m_user;
    std::string m_group;
    std::string m_type;

    std::shared_ptr<dc::GRpcClient>  m_rpcClient;
};

//******************************************************************************
//******************************************************************************
AppController::impl::impl(AppController * owner)
    : m_o(owner)
    , m_state(AppController::StateWelcome)
{

}

//*****************************************************************************
//*****************************************************************************
AppController::AppController()
    : m_p(new impl(this))
{

}

//******************************************************************************
//******************************************************************************
// static
AppController * AppController::instance(QQmlEngine *, QJSEngine *)
{
    static AppController * c = new AppController;
    return c;
}

//*****************************************************************************
//*****************************************************************************
AppController::~AppController()
{

}

//******************************************************************************
//******************************************************************************
void AppController::init()
{
    m_p->m_target = "localhost:50051";
    m_p->m_rpcClient.reset(new dc::GRpcClient(m_p->m_user, m_p->m_group, grpc::CreateChannel(m_p->m_target, grpc::InsecureChannelCredentials())));
    // m_p->m_rpcClient->subscribe(m_p->m_type, onGrpcMessage);
}

//******************************************************************************
//******************************************************************************
QString AppController::generateLicenseToken() const
{
    dc::LicenseManager licenseManager;
    return QString::fromStdString(licenseManager.GenerateDeviceToken());
}

//******************************************************************************
//******************************************************************************
void AppController::registerLicense(const QString license)
{
    dc::LicenseManager licenseManager;
    if (licenseManager.RegisterLicense(license.toStdString()))
    {
        m_p->m_state = StateCompleted;
        emit stateChanged();
    }

    // TODO emit error message or show messagebox
}

//******************************************************************************
//******************************************************************************
void AppController::checkLicense()
{
    dc::LicenseManager licenseManager;
    m_p->m_state = licenseManager.LicenseCheck() ? StateCompleted
                                                 : StateRegisterLicense;

    emit stateChanged();
}

//******************************************************************************
//******************************************************************************
void AppController::copyToClipboard(QString data)
{
    QClipboard * clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}

//******************************************************************************
//******************************************************************************
AppController::State AppController::state() const
{
    return m_p->m_state;
}
