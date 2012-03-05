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
import "../shared"

Item {
    height: 480
    width: 480
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Dynamic List", "A ListView harboring dynamic data",  Qt.resolvedUrl("listview/dynamiclist.qml"));
            addExample("Expanding Delegates", "Delegates that expand to fill the list when clicked", Qt.resolvedUrl("listview/expandingdelegates.qml"));
            addExample("Highlight", "Adding a highlight to the current item", Qt.resolvedUrl("listview/highlight.qml"));
            addExample("Sections", "A ListView with section headers", Qt.resolvedUrl("listview/sections.qml"));
            addExample("GridView", "A view laid out in a grid", Qt.resolvedUrl("gridview/gridview-example.qml"));
            addExample("PathView", "A view laid out along a path", Qt.resolvedUrl("pathview/pathview-example.qml"));
            addExample("Package", "Using a package to transition items between views", Qt.resolvedUrl("package/view.qml"));
            addExample("Parallax", "Adds a background and a parallax effect to a ListView", Qt.resolvedUrl("parallax/parallax.qml"));
            addExample("Slideshow", "A model demonstrating delayed image loading", Qt.resolvedUrl("visualdatamodel/slideshow.qml"));
            addExample("Sorted Model", "Two views on a model, one of which is sorted", Qt.resolvedUrl("visualdatamodel/sortedmodel.qml"));
            addExample("VisualItemModel", "A model that consists of the actual Items", Qt.resolvedUrl("visualitemmodel/visualitemmodel.qml"));
        }
    }
}
