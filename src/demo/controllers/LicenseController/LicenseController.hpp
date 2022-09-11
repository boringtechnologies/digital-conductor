#pragma once

#include "ControllerBase/ControllerBase.hpp"
#include "licensing/LicenseManager.hpp"

#include <QList>

class LicenseController : public QObject,
                          public ControllerBase<LicenseController>
{
    Q_OBJECT

public:
    Q_INVOKABLE bool licenseCheck();
    Q_INVOKABLE QString generateDeviceToken();
    Q_INVOKABLE bool registerLicense(QString license);

    Q_INVOKABLE void copyToClipboard(QString data);

private:
    friend class ControllerBase;

    LicenseController();
    ~LicenseController() override;

    dc::LicenseManager _licenseManager;
};
