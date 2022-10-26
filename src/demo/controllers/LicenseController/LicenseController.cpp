#include "LicenseController/LicenseController.hpp"

#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>

LicenseController::LicenseController()
{
}

LicenseController::~LicenseController()
{
}

bool LicenseController::licenseCheck()
{
    qDebug() << "<---- License checking";
    return _licenseManager.LicenseCheck();
}

QString LicenseController::generateDeviceToken()
{
    qDebug() << "<---- Generate device token";

    return QString::fromStdString(_licenseManager.GenerateDeviceToken());
}

bool LicenseController::registerLicense(QString license)
{
    qDebug() << "<---- Register License: " << license;

    if (_licenseManager.RegisterLicense(license.toStdString()))
    {
        return true;
    }
    else
    {
        return true;
    }
}

void LicenseController::copyToClipboard(QString data)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}
