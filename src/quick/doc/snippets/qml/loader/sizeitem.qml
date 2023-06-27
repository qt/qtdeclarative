// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//![0]
import QtQuick

Item {
  width: 200; height: 200

  Loader {
    // position the Loader in the center
    // of the parent
    anchors.centerIn: parent
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
