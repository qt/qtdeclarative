// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
  width: 400
  height: 400

  property bool forceNoFlicking: true
  property double heightRatioIs: flickable.visibleArea.heightRatio
  property double heightRatioShould: flickable.height / flickable.contentHeight
  property double widthRatioIs: flickable.visibleArea.widthRatio
  property double widthRatioShould: flickable.height / flickable.contentWidth

  Flickable {
    id: flickable
    flickableDirection: Flickable.AutoFlickDirection
    width: forceNoFlicking ? contentItem.width   /* so xflick() returns false */ : 20
    height: forceNoFlicking ? contentItem.height /* likewise */ : 20
    contentHeight: contentItem.height
    contentWidth: contentItem.width
    clip: true

    Rectangle {
      id: contentItem
      color: "red"
      width: 300
      height: 300
    }
  }
}

