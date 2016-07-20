//
//  UsernameCollisionBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"

Item {
    id: usernameCollisionBody
    clip: true
    width: pane.width
    height: pane.height

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int maxWidth: 1280
        readonly property int minHeight: 120
        readonly property int maxHeight: 720

        function resize() {
            var targetWidth = Math.max(titleWidth, mainTextContainer.visible ? mainTextContainer.contentWidth : 0)
            var targetHeight = (mainTextContainer.visible ? mainTextContainer.height : 0) +
                               4 * hifi.dimensions.contentSpacing.y + form.height +
                               4 * hifi.dimensions.contentSpacing.y + buttons.height

            root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    MenuItem {
        id: mainTextContainer
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: qsTr("Choose your High Fidelity user name:")
        wrapMode: Text.WordWrap
        color: hifi.colors.baseGrayHighlight
        lineHeight: 1
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter
    }


    Column {
        id: form
        anchors {
            top: mainTextContainer.bottom
            left: parent.left
            margins: 0
            topMargin: 2 * hifi.dimensions.contentSpacing.y
        }
        spacing: 2 * hifi.dimensions.contentSpacing.y

        Row {
            spacing: hifi.dimensions.contentSpacing.x

            TextField {
                id: usernameField
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                width: 350

                label: "User Name or Email"
            }

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: "Need help?"

                color: hifi.colors.blueAccent
                font.underline: true

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
        Row {
            spacing: hifi.dimensions.contentSpacing.x

            TextField {
                id: passwordField
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                width: 350

                label: "Password"
                echoMode: TextInput.Password
            }

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: "Need help?"

                color: hifi.colors.blueAccent
                font.underline: true

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

    }

    Row {
        id: buttons
        anchors {
            top: form.bottom
            right: parent.right
            margins: 0
            topMargin: 3 * hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: 200

            text: qsTr("Create your profile")
            color: hifi.buttons.blue
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Cancel")

            onClicked: root.destroy()
        }
    }

    Component.onCompleted: {
        root.title = qsTr("Complete Your Profile")
        root.iconText = "<"
        d.resize();
    }
}
