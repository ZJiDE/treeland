// Copyright (C) 2024 lbwtw <xiaoyaobing@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick


Rectangle {
    property ListView sourceView: ListView.view
    readonly property int aniDuration: 400

    x: sourceView.currentItem.x + sourceView.borderMargin
    y: sourceView.currentItem.y + (sourceView.vMargin / 2 - 2 * sourceView.borderMargin)
    height: sourceView.currentItem.height - 2 * (sourceView.vMargin / 2 - 2 * sourceView.borderMargin)
    width: sourceView.currentItem.width - 2 * sourceView.vSpacing + 2 * sourceView.borderMargin
    color: "transparent"
    radius: sourceView.radius
    border {
        width: sourceView.borderWidth
        color: "#0081FF"
    }

    Behavior on x {
        enabled: sourceView.enableDelegateAnimation
        NumberAnimation {
            duration: aniDuration
            easing.type: Easing.OutExpo
        }
    }

    Behavior on width {
        enabled: sourceView.enableDelegateAnimation
        NumberAnimation {
            duration: aniDuration
            easing.type: Easing.OutExpo
        }
    }
}
