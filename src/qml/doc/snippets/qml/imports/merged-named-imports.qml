// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [imports]
import QtQuick as Project
import QtMultimedia as Project

Project.Rectangle {
    width: 100; height: 50

    Project.Audio {
        source: "music.wav"
        autoPlay: true
    }
}
//! [imports]

