/**
 * This file is part of SaucyBacon.
 *
 * Copyright 2013 (C) Giulio Collura <random.cpp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

import QtQuick 2.0
import Ubuntu.Components 0.1
import "../components"

Page {
    id: page

    signal imageCaptured(string image)

    tools: ToolbarItems {
        ToolbarButton {
            id: snapButton
            text: i18n.tr("Snaps")
            iconSource: "/usr/share/icons/ubuntu-mobile/apps/symbolic/camera-symbolic.svg"

            onTriggered: {
                camera.captureImage()
            }
        }
    }

    states: [
        State {
            when: visible
            PropertyChanges {
                target: tools
                opened: true
                locked: true
            }
        }
    ]

    Camera {
        id: camera
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
            fill: parent
        }
        onImageCaptured: page.imageCaptured(image);
        onVisibleChanged: setActiveState(visible)
    }
}