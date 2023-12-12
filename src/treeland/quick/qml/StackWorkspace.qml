// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server
import TreeLand
import TreeLand.Utils

Item {
    id: root
    required property Item activeFocusItem
    function getSurfaceItemFromWaylandSurface(surface) {
        let finder = function(props) {
            if (!props.waylandSurface)
                return false
            // surface is WToplevelSurface or WSurfce
            if (props.waylandSurface === surface || props.waylandSurface.surface === surface)
                return true
        }

        let toplevel = QmlHelper.xdgSurfaceManager.getIf(toplevelComponent, finder)
        if (toplevel) {
            return {
                shell: toplevel,
                item: toplevel
            }
        }

        let popup = QmlHelper.xdgSurfaceManager.getIf(popupComponent, finder)
        if (popup) {
            return {
                shell: popup,
                item: popup.xdgSurface
            }
        }

        let layer = QmlHelper.layerSurfaceManager.getIf(layerComponent, finder)
        if (layer) {
            return {
                shell: layer,
                item: layer.surfaceItem
            }
        }

        let xwayland = QmlHelper.xwaylandSurfaceManager.getIf(xwaylandComponent, finder)
        if (xwayland) {
            return {
                shell: xwayland,
                item: xwayland
            }
        }

        return null
    }

    MiniDock {
        id: dock
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            margins: 8
        }
        width: 250
    }

    DynamicCreatorComponent {
        id: toplevelComponent
        creator: QmlHelper.xdgSurfaceManager
        chooserRole: "type"
        chooserRoleValue: "toplevel"
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy()
        }

        XdgSurface {
            id: surface

            property var doDestroy: helper.doDestroy
            property var cancelMinimize: helper.cancelMinimize
            property var surfaceDecorationMapper: surface.waylandSurface.XdgDecorationManager
            property var personalizationMapper: surface.waylandSurface.PersonalizationManager

            topPadding: decoration.enable ? decoration.topMargin : 0
            bottomPadding: decoration.enable ? decoration.bottomMargin : 0
            leftPadding: decoration.enable ? decoration.leftMargin : 0
            rightPadding: decoration.enable ? decoration.rightMargin : 0

            WindowDecoration {
                property var enable: surfaceDecorationMapper.serverDecorationEnabled

                id: decoration
                anchors.fill: parent
                z: surface.contentItem.z - 1
                visible: enable
            }

            StackToplevelHelper {
                id: helper
                surface: surface
                waylandSurface: surface.waylandSurface
                dockModel: dock.model
                switcherModel: switcher.model
                creator: toplevelComponent
                decoration: decoration
            }

            Image {
                id: background
                z: surface.contentItem.z - 1
                visible: personalizationMapper.backgroundWallpaper
                source: "file:///usr/share/wallpapers/deepin/desktop.jpg"
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                anchors.fill: parent
            }
        }
    }

    DynamicCreatorComponent {
        id: popupComponent
        creator: QmlHelper.xdgSurfaceManager
        chooserRole: "type"
        chooserRoleValue: "popup"

        Popup {
            id: popup

            required property WaylandXdgSurface waylandSurface
            property string type

            property alias xdgSurface: surface
            property var parentItem: root.getSurfaceItemFromWaylandSurface(waylandSurface.parentSurface)

            parent: parentItem ? parentItem.shell : root
            visible: parentItem && parentItem.item.effectiveVisible
                    && waylandSurface.surface.mapped && waylandSurface.WaylandSocket.rootSocket.enabled
            x: {
                if (!parentItem)
                    return surface.implicitPosition.x
                return surface.implicitPosition.x / parentItem.item.surfaceSizeRatio + parentItem.item.contentItem.x
            }
            y: {
                if (!parentItem)
                    return surface.implicitPosition.y
                return surface.implicitPosition.y / parentItem.item.surfaceSizeRatio + parentItem.item.contentItem.y
            }
            padding: 0
            background: null
            closePolicy: Popup.CloseOnPressOutside

            XdgSurface {
                id: surface
                waylandSurface: popup.waylandSurface
            }

            onClosed: {
                if (waylandSurface)
                    waylandSurface.surface.unmap()
            }
        }
    }

    DynamicCreatorComponent {
        id: layerComponent
        creator: QmlHelper.layerSurfaceManager
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy()
        }

        LayerSurface {
            id: layerSurface
            creator: layerComponent
        }
    }

    DynamicCreatorComponent {
        id: xwaylandComponent
        creator: QmlHelper.xwaylandSurfaceManager
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy()
        }

        XWaylandSurfaceItem {
            id: surface

            required property XWaylandSurface waylandSurface
            property var doDestroy: helper.doDestroy
            property var cancelMinimize: helper.cancelMinimize
            property var surfaceParent: root.getSurfaceItemFromWaylandSurface(waylandSurface.parentXWaylandSurface)

            surface: waylandSurface
            parentSurfaceItem: surfaceParent ? surfaceParent.item : null
            z: waylandSurface.bypassManager ? 1 : 0 // TODO: make to enum type
            positionMode: {
                if (!surface.effectiveVisible)
                    return XWaylandSurfaceItem.ManualPosition

                return (TreeLandHelper.movingItem === surface || resizeMode === SurfaceItem.SizeToSurface)
                        ? XWaylandSurfaceItem.PositionToSurface
                        : XWaylandSurfaceItem.PositionFromSurface
            }

            topPadding: decoration.enable ? decoration.topMargin : 0
            bottomPadding: decoration.enable ? decoration.bottomMargin : 0
            leftPadding: decoration.enable ? decoration.leftMargin : 0
            rightPadding: decoration.enable ? decoration.rightMargin : 0

            surfaceSizeRatio: {
                const po = waylandSurface.surface.primaryOutput
                if (!po)
                    return 1.0
                if (bufferScale >= po.scale)
                    return 1.0
                return po.scale / bufferScale
            }

            onEffectiveVisibleChanged: {
                if (surface.effectiveVisible)
                    surface.move(XWaylandSurfaceItem.PositionToSurface)
            }

            // TODO: ensure the event to WindowDecoration before WSurfaceItem::eventItem on surface's edges
            // maybe can use the SinglePointHandler?
            WindowDecoration {
                id: decoration

                property bool enable: !waylandSurface.bypassManager
                                      && waylandSurface.decorationsType !== XWaylandSurface.DecorationsNoBorder

                anchors.fill: parent
                z: surface.contentItem.z - 1
                visible: enable
            }

            StackToplevelHelper {
                id: helper
                surface: surface
                waylandSurface: surface.waylandSurface
                dockModel: dock.model
                creator: xwaylandComponent
                decoration: decoration
            }

            OutputLayoutItem {
                anchors.fill: parent
                layout: QmlHelper.layout

                onEnterOutput: function(output) {
                    if (surface.waylandSurface.surface)
                        surface.waylandSurface.surface.enterOutput(output);
                    TreeLandHelper.onSurfaceEnterOutput(waylandSurface, surface, output)
                    surfaceItem.x = TreeLandHelper.getLeftExclusiveMargin(waylandSurface) + 10
                    surfaceItem.y = TreeLandHelper.getTopExclusiveMargin(waylandSurface) + 10
                }
                onLeaveOutput: function(output) {
                    if (surface.waylandSurface.surface)
                        surface.waylandSurface.surface.leaveOutput(output);
                    TreeLandHelper.onSurfaceLeaveOutput(waylandSurface, surface, output)
                }
            }
        }
    }

    DynamicCreatorComponent {
        id: inputPopupComponent
        creator: QmlHelper.inputPopupSurfaceManager

        InputPopupSurface {
            required property InputMethodHelper inputMethodHelper
            required property WaylandInputPopupSurface popupSurface

            id: inputPopupSurface
            surface: popupSurface
            helper: inputMethodHelper
        }
    }

    WindowsSwitcher {
        id: switcher
        z: 1
        anchors.fill: parent
        visible: false
        onSurfaceActivated: (surface) => {
            surface.cancelMinimize()
            TreeLandHelper.activatedSurface = surface.waylandSurface
        }
    }

    Connections {
        target: TreeLandHelper
        function onSwitcherChanged(mode) {
            switch (mode) {
            case (TreeLandHelper.Show):
                switcher.visible = true
                break
            case (TreeLandHelper.Hide):
                switcher.visible = false
                break
            case (TreeLandHelper.Next):
                switcher.next()
                break
            case (TreeLandHelper.Previous):
                switcher.previous()
                break
            }
        }
    }
}
