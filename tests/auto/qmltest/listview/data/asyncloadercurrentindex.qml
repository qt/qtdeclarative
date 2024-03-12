// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    color: "black"

    Rectangle {
      color: "red"

      height: 150
      width: 150

      anchors {
          horizontalCenter: parent.horizontalCenter
          verticalCenter: parent.verticalCenter
      }

      NumberAnimation on rotation {
          from: 0
          to: 360
          duration: 5000
          loops: Animation.Infinite
      }
    }
}
