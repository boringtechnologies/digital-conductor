.import DC as DC
.import "qrc:/dc/common/common.js" as Common

function delay(delayTimeMs, cb) {
    Common.setInterval(timer, false, delayTimeMs, cb)
}

function licenseCheck() {
    return DC.LicenseController.licenseCheck()
}

function generateDeviceToken() {
    return DC.LicenseController.generateDeviceToken() 
}

function registerLicense(license) {
    return DC.LicenseController.registerLicense(license)
}
