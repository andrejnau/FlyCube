import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import QtQuick.Dialogs 1.1
import Render 1.0

Item {

    width: 640
    height: 480

    GLRenderItem {
        id: actionSend
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.rightMargin: 130
        clip: false
        anchors.fill: parent

        SequentialAnimation on x {
            NumberAnimation { to: 1; duration: 2500; easing.type: Easing.InQuad }
            NumberAnimation { to: 0; duration: 2500; easing.type: Easing.OutQuad }
            loops: Animation.Infinite
            running: true
        }

        MouseArea {
            id: mouse_area_input
            width: 540
            anchors.rightMargin: -29
            anchors.bottomMargin: 0
            anchors.leftMargin: 0
            anchors.topMargin: 0
            enabled: false
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton


            hoverEnabled: true

            onPositionChanged : {
                console.log(mouseX)
                console.log(mouseY)
                actionSend.doSend_nextPoint(mouseX, mouseY)
            }

            onClicked: {
                console.log(mouseX)
                console.log(mouseY)
                actionSend.doSend_Point(mouseX, mouseY)

                if (actionSend.getSize() == 3)
                {
                    swith_for_draw.checked = false
                }
            }
        }
    }

    MessageDialog {
        id: messageDialog
        title: "Draw the shape is not quadrilateral"
        text: "Draw the shape is not quadrilateral. Try again.."
        onAccepted: {
            swith_for_draw.checked = true
        }
        Component.onCompleted: visible = false
    }

    Rectangle {
        id: setting
        x: 540
        radius: 0
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.topMargin: 0

        width: 100
        height: 640
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "#F0F0F0"

        Item
        {
            Text {
                id: text_for_draw
                x: 10
                y: 15
                width: 80
                height: 15
                text: "Drawing: "
                anchors.verticalCenterOffset: 26
                anchors.horizontalCenterOffset: 33
                font.pixelSize: 12
            }

            Switch {
                id: swith_for_draw
                x: 10
                y: 36
                width: 80
                height: 15

                onCheckedChanged:
                {
                    mouse_area_input.enabled = swith_for_draw.checked
                    actionSend.doSend_Manual(swith_for_draw.checked)

                    if (!swith_for_draw.checked && actionSend.getSize() == 3)
                        actionSend.doSend_Apply()
                }

                checked:false
            }
        }
    }

    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.5)
        radius: 10
        anchors.rightMargin: -10
        anchors.bottomMargin: -10
        anchors.leftMargin: -10
        anchors.topMargin: -10
        border.width: 1
        border.color:  Qt.rgba(1, 1, 1, 0.5)
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        y: 447
        color: "black"
        wrapMode: Text.WordWrap
        text: "Computer Graphics. Task 3.45 Naumov Andrew"
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
