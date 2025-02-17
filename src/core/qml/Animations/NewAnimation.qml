// Copyright (C) 2023 justforlxz <justforlxz@gmail.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Treeland
import Waylib.Server
import QtQuick.Effects

Item {
    id: root

    enum Direction {
        Show = 1,
        Hide
    }

    signal finished

    required property SurfaceWrapper target
    required property var direction
    property int duration: 400 * Helper.animationSpeed
    property var enableBlur: false

    x: target.x
    y: target.y
    width: target.width
    height: target.height
    transform: [
        Rotation {
            id: rotation
            origin.x: width / 2
            origin.y: height / 2
            axis {
                x: 1
                y: 0
                z: 0
            }
            angle: 0
        }
    ]

    function start() {
        animation.start();
    }

    Loader {
        active: root.enableBlur
        anchors.fill: parent
        sourceComponent: Blur {
            anchors.fill: parent
            radius: root.target.radius
        }
    }

    ShaderEffectSource {
        id: effect
        // 50 > shadow width
        x: -50
        y: -50
        width: root.target.width + 100
        height: root.target.height + 100
        anchors.centerIn: parent
        live: direction === NewAnimation.Direction.Show
        hideSource: true
        sourceItem: root.target
        sourceRect: Qt.rect(effect.x, effect.y, effect.width, effect.height)
    }

    ParallelAnimation {
        id: animation

        onFinished: {
            root.finished();
        }

        PropertyAnimation {
            target: rotation
            property: "angle"
            duration: root.duration
            from: root.direction === NewAnimation.Direction.Show ? 75 : 0
            to: root.direction !== NewAnimation.Direction.Show ? 75 : 0
            easing.type: Easing.OutExpo
        }
        PropertyAnimation {
            target: root
            property: "scale"
            duration: root.duration
            from: root.direction === NewAnimation.Direction.Show ? 0.3 : 1
            to: root.direction !== NewAnimation.Direction.Show ? 0.3 : 1
            easing.type: Easing.OutExpo
        }
        PropertyAnimation {
            target: root
            property: "opacity"
            duration: root.duration
            from: root.direction === NewAnimation.Direction.Show ? 0 : 1
            to: root.direction !== NewAnimation.Direction.Show ? 0 : 1
            easing.type: Easing.OutExpo
        }
    }
}
