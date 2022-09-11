import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import DC as DC
import "qrc:/dc/Main.js" as Logic
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"
import "qrc:/dc/views/AppGui"
import "qrc:/dc/views/LicenseView"
import "qrc:/dc/views/RegisterLicenseView"

Window {
    property bool isLicenseChecked: false
    property bool isLicenseValid: false
    property string newDeviceToken: ""

    width: 500
    height: 300
    visible: true
    title: `Digital Conductor Demo`
    color: Common.primaryColor

    Timer {
        id: timer
    }

    Component.onCompleted: () => {
        licenseRegistrationView.onRegisterLicense.connect((license) => { 
            Logic.registerLicense(license)
            
            isLicenseValid = true
        })
        Logic.delay(5, () => {
            if (!!DC.LicenseController) {
                isLicenseValid = Logic.licenseCheck()
                isLicenseChecked = true

                if (!isLicenseValid) {
                    newDeviceToken = Logic.generateDeviceToken()
                }
                timer.stop()
            }
        })
    }

    LicenseView {
        height: parent.height
        width: parent.width
        visible: !isLicenseChecked
    }

    RegisterLicenseView {
        id: licenseRegistrationView
        width: parent.width
        visible: isLicenseChecked && !isLicenseValid

        deviceToken: newDeviceToken

    }

    AppGui {
        height: parent.height
        width: parent.width
        visible: isLicenseChecked && isLicenseValid
    }
}
