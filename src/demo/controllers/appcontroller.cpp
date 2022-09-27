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


    void onGrpcMessage(const std::string & from, const std::string & to, const std::string & msg);
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
void AppController::impl::onGrpcMessage(const std::string & from, const std::string & to, const std::string & msg)
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
void AppController::init(const QString & userName, const QString & group, const QString & target)
{
    m_p->m_user   = userName.toStdString();
    m_p->m_group  = group.toStdString();
    m_p->m_target = target.toStdString();

    m_p->m_rpcClient.reset(new dc::GRpcClient(m_p->m_user, m_p->m_group, grpc::CreateChannel(m_p->m_target, grpc::InsecureChannelCredentials())));

    uint32_t type = 3;
    if (!m_p->m_type.empty())
    {
        type = m_p->m_type == "r"  ? 1 :
               m_p->m_type == "w"  ? 2 :
               m_p->m_type == "rw" ? 3 : 0;
    }

    m_p->m_rpcClient->subscribe(type, std::bind(&impl::onGrpcMessage, m_p.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3));
}

//******************************************************************************
//******************************************************************************
std::shared_ptr<dc::GRpcClient> AppController::rpcClient()
{
    return m_p->m_rpcClient;
}

//******************************************************************************
//******************************************************************************
AppController::State AppController::state() const
{
    return m_p->m_state;
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
