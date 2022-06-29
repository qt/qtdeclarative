// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml
import QtQuick

QtObject {
  property list<int> arrayOfInts: [1, 0, 42, -5]

  function incrementPlease() {
      arrayOfInts[2]++;
  }

    property list<font> arrayOfFonts: [
        Qt.font({ family: "Arial", pointSize: 20, weight: Font.DemiBold, italic: true }),
        Qt.font({ family: "Comic Sans", pointSize: 55, weight: Font.Bold, italic: false })
      ]

    property list<color> arrayOfColors: [
        "red",
        "green",
        "blue"
    ]
}
