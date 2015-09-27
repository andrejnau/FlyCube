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

        onSetLabel: {
            label.visible = true;
            label.text = str;
        }

        onInputChanged: {
            if (!actionApp.input)
                label.visible = false
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

        TextField {
            id: textFieldN1
            width: 75
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10
            anchors.bottomMargin: 35
            text: actionApp.val_n1
            onTextChanged: actionApp.val_n1 = text
            placeholderText: qsTr("N1")
            validator: DoubleValidator {
                bottom: 0.01
                top: 10
                locale: "C"
            }
        }

        TextField {
            id: textFieldN2
            width: 75
            text: actionApp.val_n2
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10
            anchors.bottomMargin: 10
            onTextChanged: actionApp.val_n2 = text
            placeholderText: qsTr("N2")
            validator: DoubleValidator {
                bottom: 0.01
                top: 10
                locale: "C"
            }
        }
    }

    ToolButton {
        width: contentIcon.width
        height: contentIcon.height

        Image {
            id: contentIcon
            source: "icons/ic_note_add_black_18dp.png"
        }

        onClicked: actionApp.doButton()
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
