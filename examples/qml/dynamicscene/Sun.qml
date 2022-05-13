// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Image {
    id: sun

    property bool created: false
    property string image: "images/sun.png"

    source: image
    onCreatedChanged: {
        if (created) {
            sun.z = 1;    // above the sky but below the ground layer
            window.activeSuns++;
            // once item is created, start moving offscreen
            dropYAnim.duration = (window.height + window.centerOffset - sun.y) * 16;
            dropAnim.running = true;
        } else {
            window.activeSuns--;
        }
    }

    SequentialAnimation on y{
        id: dropAnim
        running: false
        NumberAnimation {
            id: dropYAnim
            to: (window.height / 2) + window.centerOffset
        }
        ScriptAction {
            script: { sun.created = false; sun.destroy() }
        }
    }
}
