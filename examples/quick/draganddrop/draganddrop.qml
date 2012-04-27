/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "../../shared" as Examples

/*!
    \title QtQuick Examples - Drag and Drop
    \example quick/draganddrop
    \brief This is a collection of QML drag and drop examples
    \image qml-draganddrop-example.png

    This is a collection of small QML examples relating to drag and drop functionality.

    \section2 Tiles adds drag and drop to simple rectangles, which you can drag into a specific grid.

    It has a DragTile component which uses a MouseArea to move an item when dragged:

    \snippet examples/quick/draganddrop/tiles/DragTile.qml 0
    \snippet examples/quick/draganddrop/tiles/DragTile.qml 1

    And a DropTile component which the dragged tiles can be dropped onto:

    \snippet examples/quick/draganddrop/tiles/DropTile.qml 0

    The keys property of the DropArea will only allow an item with matching key in
    it's Drag.keys property to be dropped on it.

    \section2 GridView adds drag and drop to a GridView, allowing you to reorder the list.

    It uses a VisualDataModel to move a delegate item to the position of another item
    it is dragged over.

    \snippet examples/quick/draganddrop/views/gridview.qml 0
    \snippet examples/quick/draganddrop/views/gridview.qml 1
*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Tiles", "Press and drag tiles to move them into the matching colored boxes",  Qt.resolvedUrl("tiles/tiles.qml"));
            addExample("GridView", "Press and drag to re-order items in the grid", Qt.resolvedUrl("views/gridview.qml"));
        }
    }
}
