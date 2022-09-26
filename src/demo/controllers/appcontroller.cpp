//*****************************************************************************
//*****************************************************************************

#include "appcontroller.h"

#include "licensing/LicenseManager.hpp"


//******************************************************************************
//******************************************************************************
class AppController::impl
{
    friend class AppController;

protected:
    impl(AppController * owner);

    AppController      * m_o;

    AppController::State m_state;
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
AppController::State AppController::state() const
{
    return m_p->m_state;
}
