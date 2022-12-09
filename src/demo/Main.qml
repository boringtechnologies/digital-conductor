import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import QtQuick.Dialogs

import DC.controller 1.0

import "qrc:/dc/common/common.js" as Common

import "qrc:/dc/components/DCText"
import "qrc:/dc/views/AppGui"
import "qrc:/dc/views/LicenseView"
import "qrc:/dc/views/RegisterLicenseView"

Window {

    minimumWidth: 530
    minimumHeight: 350
    visible: true
    title: "Digital Conductor Demo"
    color: Common.primaryColor

    // TODO update Qt to 6.3
//    MessageDialog {
//        buttons: MessageDialog.Ok
//        text: "The document has been modified."
//    }

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    Connections {
        target: Controller

        function onShowMessage(message) {
            // TODO use MessageDialog
        }
    }

    Loader {
        id: idLoader

        x: 0
        y: 0
        width: parent.width
        height: parent.height

        focus: true
        visible: false
        onLoaded: {
            idLoader.visible = true;
            idLoader.item.forceActiveFocus();
        }

        sourceComponent: Controller.state === Controller.StateWelcome         ? idLicenseView :
                         Controller.state === Controller.StateRegisterLicense ? idRegisterLicenseView :
                         Controller.state === Controller.StateCompleted       ? idAppGui :
                                                                                idNullView;
        Component { id: idNullView; Item {} }
        Component { id: idLicenseView; LicenseView {} }
        Component {
            id: idRegisterLicenseView;
            RegisterLicenseView {
                deviceToken: Controller.newDeviceLicense
                onRegisterLicense: {
                    Controller.registerLicense(license);
                }
            }
        }
        Component { id: idAppGui; AppGui {} }
    }

    Component.onCompleted: {
        Controller.checkLicense();
    }

}
