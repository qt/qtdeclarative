// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

ListView {
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
    width: ListView.view.width
    text: model.name
  }

  header: Rectangle {
    width: ListView.view.width
  }

  footer: Rectangle {
    width: ListView.view.width
  }

  highlight: Rectangle {
    width: ListView.view.width
  }

  section.property: "name"
  section.criteria: ViewSection.FirstCharacter
  section.delegate: Rectangle {
    objectName: "sectionItem"
    width: ListView.view.width
  }

}
