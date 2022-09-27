import QtQuick 2.6
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts

import DC.controller 1.0

import DC as DC
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCTextField"
import "qrc:/dc/components/DCLinkButton"
import "qrc:/dc/components/DCTextArea"
import "qrc:/dc/components/DCButton"

ColumnLayout {
    property string deviceToken: ""

    anchors.leftMargin: 15
    anchors.topMargin: 15
    anchors.rightMargin: 15
    anchors.bottomMargin: 15

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom

    spacing: 5 

    signal registerLicense(string license)

    DCText {
        Layout.alignment: Qt.AlignLeft
        text: "New Device Token"
        fontSize: 18
    }

    Rectangle { 
        Layout.fillWidth: true
        color: Common.transparent
        height: 30

        DCTextField {
            id: deviceTokenField
            text: deviceToken
            color: deviceTokenField.enabled ? Common.textColor : Common.borderColor
            height: parent.height
            width: parent.width - copyButton.width - 5

            readOnly: true
            selectByMouse: true

            bottomPadding: 0
            topPadding: 0
        }

        DCButton {
            id: copyButton
            width: 50
            anchors.topMargin: 0
            anchors.bottomMargin: 0
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            buttonText: "Copy!"

            onClicked: () => {
                Controller.copyToClipboard(deviceToken)
            }
        }
    }

    RowLayout {
        spacing: 10

        DCText {
            Layout.alignment: Qt.AlignLeft
            text: "Please send this token to "
        }

        DCLinkButton {
            url: `mailto:sam@boring.tech?subject=Device Token&body=${deviceTokenField.text}`
            title: "sam@boring.tech"
        }

        DCText {
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            text: " to receive a license key."
        }   
    }

    Rectangle {
        height: 1
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.bottomMargin: 10
        color: Common.midGray
    }

    DCText {
        Layout.alignment: Qt.AlignLeft
        text: "If you already have the license key, paste it into the field below."
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        color: Common.transparent

        ScrollView {
            height: parent.height
            width: parent.width
            contentWidth: parent.width
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            DCTextArea {
                id: licenseKeyArea
                placeholderText: "License Key"
            }
        }
    }

    DCButton {
        id: submitButton
        height: deviceTokenField.height
        buttonText: "Submit License"
        buttonColor: Common.secondaryColor
        onClicked: () => {
            registerLicense(licenseKeyArea.text)
        }
    }
}

