import QtQuick 2.0
import QtQuick.Controls 1.3
import OpenGLUnderQML 1.0

Item {

    width: 700
    height: 700

    Squircle {
        id: actionApp
        SequentialAnimation on x {
            NumberAnimation { to: 1; duration: 2500; easing.type: Easing.InQuad }
            NumberAnimation { to: 0; duration: 2500; easing.type: Easing.OutQuad }
            loops: Animation.Infinite
            running: true
        }
    }

    MouseArea {
        id: mouse_area_input
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        enabled: actionApp.input
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        hoverEnabled: true

        onPositionChanged : {
            console.log(mouseX)
            console.log(mouseY)
            actionApp.doSendFuturePoint(mouseX, mouseY)
        }
        onClicked: {
            console.log(mouseX)
            console.log(mouseY)
            actionApp.doSendPoint(mouseX, mouseY)
        }
    }

    ToolButton {
        width: contentIcon.width
        height: contentIcon.height

        Image {
            id: contentIcon
            source: "icons/ic_note_add_black_18dp.png"
        }

        onClicked: actionApp.input = true
    }
}
