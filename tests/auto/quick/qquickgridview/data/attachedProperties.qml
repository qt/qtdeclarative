// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

GridView {
  width: 100
  height: 100

  Component.onCompleted: currentIndex = 0

  model: ListModel {
    ListElement {
      name: "A"
    }
    ListElement {
      name: "B"
    }
  }

  delegate: Text {
    width: GridView.view.width
    text: model.name
  }

  header: Rectangle {
    width: GridView.view.width
  }

  footer: Rectangle {
    width: GridView.view.width
  }

  highlight: Rectangle {
    width: GridView.view.width
  }
}
