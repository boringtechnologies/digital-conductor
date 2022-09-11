#include "LicenseController/LicenseController.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>

LicenseController::LicenseController()
{
}

LicenseController::~LicenseController()
{
}

bool LicenseController::licenseCheck()
{
    // return _licenseManager.LicenseCheck();
    qDebug() << "<---- License checking";
    return false;
}

QString LicenseController::generateDeviceToken()
{
    qDebug() << "<---- Generate device token";

    return "ASDASDAEEFEIWDVCKEVIRGJRVSMDVJCKSRWEFJD";
}

bool LicenseController::registerLicense(QString license)
{
    qDebug() << "<---- Register License: " << license;

    return true;
}

void LicenseController::copyToClipboard(QString data)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}
