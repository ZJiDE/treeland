// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Waylib.Server
import TreeLand
import TreeLand.Greeter

Item {
    id :root

    WaylandServer {
        id: server

        WaylandBackend {
            id: backend

            onOutputAdded: function(output) {
                output.forceSoftwareCursor = true // Test

                if (QmlHelper.outputManager.count > 0)
                    output.scale = 2

                Helper.allowNonDrmOutputAutoChangeMode(output)
                QmlHelper.outputManager.add({waylandOutput: output})
            }
            onOutputRemoved: function(output) {
                QmlHelper.outputManager.removeIf(function(prop) {
                    return prop.waylandOutput === output
                })
            }
            onInputAdded: function(inputDevice) {
                seat0.addDevice(inputDevice)
            }
            onInputRemoved: function(inputDevice) {
                seat0.removeDevice(inputDevice)
            }
        }

        WaylandCompositor {
            id: compositor

            backend: backend
        }

        XdgShell {
            id: shell

            onSurfaceAdded: function(surface) {
                let type = surface.isPopup ? "popup" : "toplevel"
                QmlHelper.xdgSurfaceManager.add({type: type, waylandSurface: surface})
            }
            onSurfaceRemoved: function(surface) {
                QmlHelper.xdgSurfaceManager.removeIf(function(prop) {
                    return prop.waylandSurface === surface
                })
            }
        }

        Seat {
            id: seat0
            name: "seat0"
            cursor: Cursor {
                id: cursor1

                layout: QmlHelper.layout
            }

            eventFilter: Helper
            keyboardFocus: Helper.getFocusSurfaceFrom(renderWindow.activeFocusItem)
        }

        WaylandSocket {
            id: masterSocket

            freezeClientWhenDisable: false

            Component.onCompleted: {
                console.info("Listing on:", socketFile)
                Helper.socketFile = socketFile
            }
        }

        // TODO: add attached property for XdgSurface
        XdgDecorationManager {
            id: decorationManager
        }

        XWayland {
            compositor: compositor.compositor
            seat: seat0.seat
            lazy: false

            onReady: masterSocket.addClient(client())

            onSurfaceAdded: function(surface) {
                QmlHelper.xwaylandSurfaceManager.add({waylandSurface: surface})
            }
            onSurfaceRemoved: function(surface) {
                QmlHelper.xwaylandSurfaceManager.removeIf(function(prop) {
                    return prop.waylandSurface === surface
                })
            }
        }
    }

    OutputRenderWindow {
        id: renderWindow

        compositor: compositor
        width: outputRowLayout.implicitWidth + outputRowLayout.x
        height: outputRowLayout.implicitHeight + outputRowLayout.y

        EventJunkman {
            anchors.fill: parent
        }

        Row {
            id: outputRowLayout

            DynamicCreatorComponent {
                creator: QmlHelper.outputManager

                OutputDelegate {
                    property real topMargin: topbar.height
                }
            }
        }

        ColumnLayout {
            id: workspaceLoader
            anchors.fill: parent
            visible: false

            Row {
                Layout.fillWidth: true

                Rectangle {
                    color: 'white'
                    anchors.fill: parent
                }

                Button {
                    id: optionsBtn
                    text: qsTr("Options")
                    onClicked: optionsMenu.open()

                    Menu {
                        id: optionsMenu
                        y: optionsBtn.height

                        Menu {
                            title: qsTr("Switch Windows Layout")
                            MenuItem {
                                text: "Stack Layout"
                                onClicked: {
                                    decorationManager.mode = XdgDecorationManager.PreferClientSide
                                    stackLayout.visible = true
                                }
                            }
                            MenuItem {
                                text: "Tiled Layout"
                                onClicked: {
                                    decorationManager.mode = XdgDecorationManager.PreferServerSide
                                    stackLayout.visible = false
                                }
                            }
                        }

                        MenuSeparator { }

                        Menu {
                            title: "Session"
                            MenuItem {
                                text: "Lock"
                                onClicked: {
                                    greeter.visible = true
                                }
                            }
                        }

                        MenuSeparator { }

                        MenuItem {
                            text: qsTr("Back to normal mode")
                            onClicked: {
                                Helper.backToNormal()
                            }
                        }

                        MenuItem {
                            text: qsTr("Reboot")
                            onClicked: {
                                Helper.reboot()
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackWorkspace {
                    id: stackLayout
                    visible: true
                    anchors.fill: parent
                }

                TiledWorkspace {
                    visible: !stackLayout.visible
                    anchors.fill: parent
                }
            }
        }

        Greeter {
            id: greeter
            anchors.fill: parent
            onVisibleChanged: {
                workspaceLoader.visible = !greeter.visible
            }
        }
    }
}
