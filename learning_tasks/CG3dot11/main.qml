import QtQuick 2.4
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

        onInputChanged: {
            if (actionApp.input)
            {
                label.visible = true;
                label.text = "Select 3 points parallelepiped base"
            }
            else
            {
                label.visible = false
                label.text = ""
            }
        }

        onFinishInput: {
            label.visible = true
            label.text = "Select the direction of the beam"
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

    MouseArea {
        id: mouse_area_beam
        anchors.rightMargin: 0
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

    Text {
       id: label
       color: "black"
       visible: false
       wrapMode: Text.WordWrap
       anchors.topMargin: 20
       anchors.leftMargin: 20
       anchors.right: parent.right
       anchors.top:  parent.top
       anchors.margins: 20
    }
}
