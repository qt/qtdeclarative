/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
import "../shared" as Examples

/*!
    \title QtQuick Examples - Views
    \example quick/views
    \brief This is a collection of QML model and view examples
    \image qml-modelviews-example.png

    This is a collection of small QML examples relating to model and view functionality. They
    are focused on the views side, which is the visual representation of the data in the models.

    \section2 GridView and PathView demonstrate usage of these elements to display views.
    \snippet examples/quick/modelviews/gridview/gridview-example.qml 0

    \section2 Dynamic List demonstrates animation of runtime additions and removals to a ListView.

    The ListView.onAdd signal handler runs an animation when new items are added to the
    view, and the ListView.onRemove another when they are removed.
    \snippet examples/quick/modelviews/listview/dynamiclist.qml 0
    \snippet examples/quick/modelviews/listview/dynamiclist.qml 1

    \section2 Expanding Delegates demonstrates delegates that expand when activated.

    It has a complex delegate the size and appearance of which can change, displacing
    other items in the view.
    \snippet examples/quick/modelviews/listview/expandingdelegates.qml 0
    \snippet examples/quick/modelviews/listview/expandingdelegates.qml 1
    \snippet examples/quick/modelviews/listview/expandingdelegates.qml 2
    \snippet examples/quick/modelviews/listview/expandingdelegates.qml 3

    \section2 Highlight demonstrates adding a custom highlight to a ListView.
    \snippet examples/quick/modelviews/listview/highlight.qml 0

    \section2 Highlight Ranges shows the three different highlight range modes of ListView.
    \snippet examples/quick/modelviews/listview/highlightranges.qml 0
    \snippet examples/quick/modelviews/listview/highlightranges.qml 1
    \snippet examples/quick/modelviews/listview/highlightranges.qml 2

    \section2 Sections demonstrates the various section headers and footers available to ListView.
    \snippet examples/quick/modelviews/listview/sections.qml 0

    \section2 Packages demonstrates using Packages to transition delegates between two views.

    It has a Package which defines delegate items for each view and an item that can
    be transferred between delegates.

    \snippet examples/quick/modelviews/package/Delegate.qml 0

    A VisualDataModel allows the individual views to access their specific items from
    the shared package delegate.

    \snippet examples/quick/modelviews/package/view.qml 0

    \section2 VisualItemModel uses a VisualItemModel for the model instead of a ListModel.

    \snippet examples/quick/modelviews/visualitemmodel/visualitemmodel.qml 0
    */

    Item {
        height: 480
        width: 320
        Examples.LauncherList {
            id: ll
            anchors.fill: parent
            Component.onCompleted: {
                addExample("GridView", "A simple GridView", Qt.resolvedUrl("gridview/gridview-example.qml"))
                addExample("Dynamic List", "A dynamically alterable list", Qt.resolvedUrl("listview/dynamiclist.qml"))
                addExample("Expanding Delegates", "A ListView with delegates that expand", Qt.resolvedUrl("listview/expandingdelegates.qml"))
                addExample("Highlight", "A ListView with a custom highlight", Qt.resolvedUrl("listview/highlight.qml"))
                addExample("Highlight Ranges", "The three highlight ranges of ListView", Qt.resolvedUrl("listview/highlightranges.qml"))
                addExample("Sections", "ListView section headers and footers", Qt.resolvedUrl("listview/sections.qml"))
                addExample("Packages", "Transitions between a ListView and GridView", Qt.resolvedUrl("package/view.qml"))
                addExample("PathView", "A simple PathView", Qt.resolvedUrl("pathview/pathview-example.qml"))
                addExample("VisualItemModel", "Using a VisualItemModel", Qt.resolvedUrl("visualitemmodel/visualitemmodel.qml"))
            }
        }
}
