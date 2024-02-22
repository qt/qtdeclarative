// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
  width: 200; height: 200

  Loader {
    // Explicitly set the size of the
    // Loader to the parent item's size
    anchors.fill: parent
    sourceComponent: rect
  }

  Component {
    id: rect
    Rectangle {
      width: 50
      height: 50
      color: "red"
      }
  }
}
//![0]
