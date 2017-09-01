import Qt 4.7

Rectangle {
    id: mainBox
    width: 400
    height: 400
    color: "black"

    Connections { 
        target: easyData
        onCountChanged: { countText.text=new_count }
        onTimeChanged: { if (new_time >= 10) {timeText.text=":"+new_time} else {timeText.text=":0"+new_time} }
    }

    Image {
        id: backBtn
        source: "images/easy0.png"
        anchors.centerIn: parent
    }

    Image {
        id: frontBtn
        source: "images/easy1.png"
        anchors.centerIn: parent
        opacity: 0.0
        // optional - may prefer without
        Behavior on opacity {
            PropertyAnimation {
                duration: 100
            }
        }
    }

    // build the "active" mouse area
    Image {
        id: activeOverlay
        source: "images/easy2.png"
        anchors.horizontalCenter : backBtn.horizontalCenter
        anchors.verticalCenter : backBtn.verticalCenter
        anchors.horizontalCenterOffset : 0
        anchors.verticalCenterOffset : -48
        opacity: 0.0
    }

    MouseArea {
        id: mouse; anchors.fill: activeOverlay;

        onPressed:  frontBtn.opacity=1.0
        onReleased: {frontBtn.opacity=0.0; easyData.incrCount();}
    }

    // text field for count of button presses
    Text {
            id: countText
            anchors.centerIn: parent
            anchors.horizontalCenterOffset : 150
            anchors.verticalCenterOffset : 170
            font.family: "Arial"
            font.pointSize: 24
            smooth: true
            color: "gray"
            style: Text.Outline
            styleColor: "black"
            text: " "
    }

    // text field for current seconds count
    Text {
            id: timeText
            anchors.centerIn: parent
            anchors.horizontalCenterOffset : -160
            anchors.verticalCenterOffset : -170
            font.family: "Arial"
            font.pointSize: 24
            smooth: true
            color: "gray"
            style: Text.Outline
            styleColor: "black"
            text: " "
    }
}
