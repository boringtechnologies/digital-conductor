import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"

ComboBox {
    id: comboBoxId

    delegate: ItemDelegate {
        id: myItem
        width: 300
        height: 25
        contentItem: DCText {
            verticalAlignment: Text.AlignVCenter
            label: model[index]
        }
        highlighted: comboBoxId.highlightedIndex === index
        background: Rectangle {
            height: 25
            color: myItem.highlighted ? Common.darkGray : Common.transparent
        }

        required property int index
    }

    indicator: Canvas {
        id: canvas
        x: comboBoxId.width - width - comboBoxId.rightPadding
        y: comboBoxId.topPadding + (comboBoxId.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: comboBoxId
            function onPressedChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = Common.secondaryColor;
            context.fill();
        }
    }

    contentItem: DCText {
        leftPadding: 5
        topPadding: 5
        bottomPadding: 5
        rightPadding: comboBoxId.indicator.width + comboBoxId.spacing

        label: comboBoxId.displayText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: Common.midGray
        radius: 4
    }

    popup: Popup {
        y: comboBoxId.height - 1
        width: comboBoxId.width
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: comboBoxId.popup.visible ? comboBoxId.delegateModel : null
            currentIndex: comboBoxId.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            border.color: Common.midGray
            color: Common.midGray
            radius: 2
        }
    }
}
