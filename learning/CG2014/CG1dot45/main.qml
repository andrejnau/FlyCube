import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
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
            anchors.rightMargin: 0
            anchors.bottomMargin: 0
            anchors.leftMargin: 0
            anchors.topMargin: 0
            enabled: false
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                console.log(mouseX)
                console.log(mouseY)
                actionSend.doSend_Point(mouseX, mouseY)
            }
        }
    }

    Rectangle {
        id: setting
        x: 504
        radius: 0
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.topMargin: 0

        width: 130
        height: 640
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "#F0F0F0"

        Item
        {
            Text {
                id: text_for_manual
                x: 8
                y: 15
                width: 50
                height: 15
                text: "Manual: "
                anchors.verticalCenterOffset: 26
                anchors.horizontalCenterOffset: 33
                font.pixelSize: 12
            }

            Switch {
                id: swith_for_manual
                x: 65
                y: 15
                width: 50
                height: 15

                onCheckedChanged:
                {
                    text_for_slider_vertex.visible = !checked
                    slider_vertex.visible = !checked

                    item_manual_input.visible = checked
                    if (checked)
                    {
                       swith_for_polygon.checked = !checked
                       swith_for_template.checked = !checked
                    }
                    else
                    {
                        actionSend.doSend_Manual(checked)
                    }
                }

                checked:false
            }
        }

        Item
        {
            id: item_manual_input
            visible: false

            Text {
                id: text_for_polygon
                x: 8
                y: 35
                width: 50
                height: 15
                text: "Polygon: "
                anchors.verticalCenterOffset: 26
                anchors.horizontalCenterOffset: 33
                font.pixelSize: 12
            }

            Switch {
                id: swith_for_polygon
                x: 65
                y: 35
                width: 50
                height: 15

                onCheckedChanged:
                {
                    if (checked && swith_for_template.checked)
                    {
                        console.log("error")
                        checked = false
                    }
                    mouse_area_input.enabled = swith_for_polygon.checked ^ swith_for_template.checked
                    if (swith_for_polygon.checked ^ swith_for_template.checked == false)
                        actionSend.doSend_Apply(0)
                    actionSend.doSend_Manual(swith_for_polygon.checked ^ swith_for_template.checked)
                }

                checked:false
            }

            Text {
                id: text_for_template
                x: 8
                y: 55
                width: 50
                height: 15
                text: "Template: "
                anchors.verticalCenterOffset: 26
                anchors.horizontalCenterOffset: 33
                font.pixelSize: 12
            }

            Switch {
                id: swith_for_template
                x: 65
                y: 55
                width: 50
                height: 15

                onCheckedChanged:
                {
                    if (checked && swith_for_polygon.checked)
                    {
                        console.log("error")
                        checked = false
                    }
                    mouse_area_input.enabled = swith_for_polygon.checked ^ swith_for_template.checked
                    if (swith_for_polygon.checked ^ swith_for_template.checked == false)
                        actionSend.doSend_Apply(1)
                    actionSend.doSend_Manual(swith_for_polygon.checked ^ swith_for_template.checked )
                }

                checked: false
            }
        }

        Item
        {
            Text {
                id: text_for_slider_vertex
                x: 8
                y: 45
                width: 81
                height: 15
                text: "Vertex: 3"
                anchors.verticalCenterOffset: 84
                anchors.horizontalCenterOffset: 49
                font.pixelSize: 12
            }

            Slider {
                id: slider_vertex
                x: 8
                y: 65
                width: 110
                height: 22
                value: 3
                minimumValue: 1
                maximumValue: 50
                stepSize: 1

                onValueChanged :
                {
                    text_for_slider_vertex.text = "Vertex: " + value
                    actionSend.doSend_Vertex(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }
                }
            }
        }

        Item
        {
            Text {
                id: text_for_slider_eps
                x: 8
                y: 102
                width: 81
                height: 15
                text: "Eps: 0.01"
                anchors.verticalCenterOffset: 160
                anchors.horizontalCenterOffset: 49
                font.pixelSize: 12
            }

            Slider {
                id: slider_eps
                x: 8
                y: 122
                width: 110
                height: 22
                value: 0.001
                minimumValue: 0.0001
                maximumValue: 0.5
                stepSize: 0.001

                onValueChanged :
                {
                    text_for_slider_eps.text = "Eps: " + value.toExponential().toString()
                    actionSend.doSend_Eps(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }
                }
            }
        }

        Item
        {
            Text {
                id: text_for_slider_iteration
                x: 8
                y: 159
                width: 81
                height: 15
                text: "Iteration: 3"
                anchors.verticalCenterOffset: 226
                anchors.horizontalCenterOffset: 49
                font.pixelSize: 12
            }

            Slider {
                id: slider_iteration
                x: 8
                y: 179
                width: 110
                height: 22
                value: 5
                minimumValue: 0
                maximumValue: 10
                stepSize: 1

                onValueChanged :
                {
                    text_for_slider_iteration.text = "Iteration: " + value
                    actionSend.doSend_Iteration(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }
                }
            }
        }

        Item
        {
            Text {
                id: text_for_slider0
                x: 8
                y: 330
                width: 39
                height: 15
                text: "D: 1"
                anchors.verticalCenterOffset: 284
                anchors.horizontalCenterOffset: 28
                font.pixelSize: 12
            }

            Slider {
                id: slider0
                x: 8
                y: 236
                width: 110
                height: 22
                value: 1
                minimumValue: -1
                maximumValue: 1
                stepSize: 2

                onValueChanged :
                {
                    text_for_slider0.text = "D: " + value
                    actionSend.doSend_D(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }

                }
            }
        }

        Item
        {
            Text {
                id: text_for_slider1
                x: 8
                y: 273
                width: 45
                height: 15
                text: "Di: 0"
                anchors.verticalCenterOffset: 357
                anchors.horizontalCenterOffset: 31
                font.pixelSize: 12
            }

            Slider {
                id: slider1
                x: 8
                y: 293
                width: 110
                height: 22
                value: 0
                minimumValue: -1
                maximumValue: 1
                stepSize: 1

                onValueChanged :
                {

                    text_for_slider1.text = "Di: " + value
                    actionSend.doSend_Di(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }

                }
            }

        }

        Item
        {
            Text {
                id: text_for_slider2
                x: 8
                y: 216
                width: 48
                height: 15
                text: qsTr("D0: 0")
                anchors.verticalCenterOffset: 419
                anchors.horizontalCenterOffset: 32
                font.pixelSize: 12
            }

            Slider {
                id: slider2
                x: 8
                y: 350
                width: 110
                height: 22
                value: 0
                minimumValue: -1
                maximumValue: 1
                stepSize: 1

                onValueChanged :
                {
                    text_for_slider2.text = "D0: " + value
                    actionSend.doSend_D0(value)
                }

                style: SliderStyle {
                    groove: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "gray"
                        radius: 8
                    }

                }

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
        text: "Computer Graphics. Task 1.45 Naumov Andrew"
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
