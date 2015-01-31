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
        onDrawSortType: canvas.qmlFunctionDraw(drawX, drawY, drawType)
        onInputChanged: {
            if (actionApp.input)
                canvas.qmlFunctionClear()
        }
    }

    Canvas {
        id: canvas

        width: parent.width
        height: parent.height
        property variant drawXArray: Array()
        property variant drawYArray: Array()
        property variant drawTypeArray: Array()

        function qmlFunctionDraw(drawX, drawY, drawType) {
            console.log("qmlFunctionDraw")
            drawXArray.push(drawX)
            drawYArray.push(drawY)
            drawTypeArray.push(drawType)
            canvas.requestPaint()
        }

        function qmlFunctionClear(drawX, drawY, drawType) {
            console.log("qmlFunctionClear")
            drawXArray = new Array()
            drawYArray = new Array()
            drawTypeArray = new Array()
            canvas.requestPaint()
        }

        onPaint: {
            console.log("onPaint", drawTypeArray.length)
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            for (var i = 0; i < drawTypeArray.length; ++i) {
                console.log(drawXArray[i], drawYArray[i], drawTypeArray[i]);
                ctx.moveTo(drawXArray[i], drawYArray[i])
                ctx.fillStyle = "gray";
                if (drawTypeArray[i] === 1)
                    ctx.fillStyle = "green";
                else if (drawTypeArray[i] === -1)
                    ctx.fillStyle = "red";
                ctx.beginPath();
                ctx.arc(drawXArray[i], drawYArray[i], 15, 0,Math.PI*2,true);
                ctx.fill();
                ctx.restore();
            }
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
