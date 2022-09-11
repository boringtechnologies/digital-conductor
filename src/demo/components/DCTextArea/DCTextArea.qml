import QtQuick 2.6
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts
import DC as DC
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCTextField"

TextArea {
    id: textAreaId

    selectByMouse: true
    width: parent.width
    height: parent.height
    padding: 10
    color: Common.textColor
    placeholderTextColor: Common.lightGray
    selectionColor: Common.secondaryColor
    cursorDelegate: Rectangle {
        id: cursor
        color: Common.secondaryColor
        width: 2

        SequentialAnimation {
            loops: Animation.Infinite
            running: licenseKeyArea.cursorVisible

            PropertyAction {
                target: cursor
                property: 'visible'
                value: true
            }

            PauseAnimation {
                duration: 600
            }

            PropertyAction {
                target: cursor
                property: 'visible'
                value: false
            }

            PauseAnimation {
                duration: 600
            }

            onStopped: {
                cursor.visible = false
            }
        }
    
    }
    background: Rectangle {
        color: Common.midGray
        radius: 4
        border.width: 1
        border.color: textAreaId.activeFocus ? Common.secondaryColor : Common.borderColor
    }
}
