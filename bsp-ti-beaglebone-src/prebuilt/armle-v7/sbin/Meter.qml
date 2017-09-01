/****************************************************************************
 *                                                                          *
 * File: Demo.qml                                                           *
 * Desc: Meter - simple QML widget                                          *
 * (c) 2013, CBD BC, Evgeny Gorelov, http://www.kpda.ru                     *
 *                                                                          *
 ****************************************************************************/

import Qt 4.7

Item {
    width: 100
    height: 62

    property int min: -58
    property int max: 58

    property real rAngle: 50

    Timer{
        property int cr: 1

        interval: 100; running: true; repeat: true
        onTriggered:{

            rAngle += cr

            if( rAngle == min || rAngle == max )
            {
                cr = -cr
            }
        }
    }

    Image{
        id: meterTop
        source: "images/meter_top.png"
        anchors.centerIn: parent

        Text{
            text: "V"
            font.pointSize: 14
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
        }

        Image{
            id: arrow
            source: "images/arrow.png"

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            smooth: true

            transform: Rotation{
                id: arrowRotation
                axis.x: 0
                axis.y: 0
                axis.z: 1

                origin.x: arrow.width/2
                origin.y: arrow.height

                angle: rAngle

                Behavior on angle{ PropertyAnimation{ duration: 200 } }
            }
        }

        Image{
            id: arrowBase
            source: "images/arrow_base.png"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }
    }

    Image{
        id: meterBottom
        source: "images/meter_bottom.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: meterTop.bottom
    }
}
