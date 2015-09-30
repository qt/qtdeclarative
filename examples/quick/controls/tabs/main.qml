/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

import QtQuick 2.6
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0
import Qt.labs.templates 1.0 as T
import QtQuick.XmlListModel 2.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls - Tabs Example"

    header: TabBar {
        id: bar
        currentIndex: view.currentIndex
        TabButton {
            text: "Home"
        }
        TabButton {
            text: "Discover"
        }
        TabButton {
            text: "Activity"
        }
    }

    SwipeView {
        id: view

        spacing: 1
        anchors.fill: parent
        currentIndex: bar.currentIndex
        background: Rectangle { color: Theme.frameColor }

        Rectangle {
            Image {
                id: logo
                width: window.width / 2
                height: window.height / 2
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                source: "qrc:/images/qt-logo.png"
            }

            Label {
                text: "Things just got better"
                color: Theme.accentColor
                anchors.margins: 40
                anchors.top: logo.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                font.pointSize: 26
            }
        }

        Rectangle {
            ListView {
                anchors.fill: parent
                anchors.topMargin: -1
                model: XmlListModel {
                    id: feedModel
                    query: "/rss/channel/item"
                    source: "http://blog.qt.io/feed/"
                    namespaceDeclarations: "declare namespace dc='http://purl.org/dc/elements/1.1/';"
                    XmlRole { name: "title"; query: "title/string()" }
                    XmlRole { name: "link"; query: "link/string()" }
                    XmlRole { name: "pubDate"; query: "pubDate/string()" }
                    XmlRole { name: "creator"; query: "dc:creator/string()" }
                    XmlRole { name: "description"; query: "description/string()" }
                }

                delegate: Item {
                    width: parent.width
                    height: feedItem.height
                    Column {
                        id: feedItem
                        width: parent.width
                        spacing: 6
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.frameColor
                            visible: index == 0
                        }
                        Item { width: 1; height: 6 }
                        Label {
                            text: model.title
                            x: 6
                            width: parent.width - 12
                            elide: Text.ElideRight
                            color: Theme.accentColor
                            font.pointSize: 20
                            lineHeight: 0.75
                        }
                        Label {
                            text: model.description
                            textFormat: Text.StyledText
                            x: 6
                            width: parent.width - 12
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            x: 6
                            width: parent.width - 12
                            spacing: 6
                            Label {
                                id: discoverCreator
                                text: model.creator
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.focusColor
                                font.pointSize: 8
                            }
                            Label {
                                text: model.pubDate
                                height: parent.height
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.disabledColor
                                font.pointSize: 8
                                Layout.preferredWidth: parent.width - discoverCreator.width - discoverMore.width - 2 * parent.spacing
                                visible: Layout.preferredWidth > 0
                            }
                            Button {
                                id: discoverMore
                                text: "Read more..."
                                onClicked: Qt.openUrlExternally(model.link)
                            }
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.frameColor
                        }
                    }
                }

                T.ScrollIndicator.vertical: ScrollIndicator { }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: feedModel.status === XmlListModel.Loading
            }
        }

        Rectangle {
            ListView {
                anchors.fill: parent
                anchors.topMargin: -1
                model: XmlListModel {
                    id: commentModel
                    query: "/rss/channel/item"
                    source: "http://blog.qt.io/comments/feed/"
                    namespaceDeclarations: "declare namespace dc='http://purl.org/dc/elements/1.1/';"
                    XmlRole { name: "title"; query: "title/string()" }
                    XmlRole { name: "link"; query: "link/string()" }
                    XmlRole { name: "pubDate"; query: "pubDate/string()" }
                    XmlRole { name: "creator"; query: "dc:creator/string()" }
                    XmlRole { name: "description"; query: "description/string()" }
                }

                delegate: Rectangle {
                    width: parent.width
                    height: commentItem.height
                    Column {
                        id: commentItem
                        width: parent.width
                        spacing: 6
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.frameColor
                            visible: index == 0
                        }
                        Item { width: 1; height: 6 }
                        Label {
                            text: model.title
                            x: 6
                            width: parent.width - 12
                            elide: Text.ElideRight
                            color: Theme.accentColor
                            font.pointSize: 14
                            lineHeight: 0.75
                        }
                        Item { width: 1; height: 6 }
                        Label {
                            text: model.description
                            textFormat: Text.StyledText
                            x: 6
                            width: parent.width - 12
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            x: 6
                            width: parent.width - 12
                            spacing: 6
                            Label {
                                id: activityCreator
                                text: model.creator
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.focusColor
                                font.pointSize: 8
                            }
                            Label {
                                text: model.pubDate
                                height: parent.height
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                color: Theme.disabledColor
                                font.pointSize: 8
                                Layout.preferredWidth: parent.width - activityCreator.width - activityMore.width - 2 * parent.spacing
                                visible: Layout.preferredWidth > 0
                            }
                            Button {
                                id: activityMore
                                text: "Read more..."
                                onClicked: Qt.openUrlExternally(model.link)
                            }
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.frameColor
                        }
                    }
                }

                T.ScrollIndicator.vertical: ScrollIndicator { }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: feedModel.status === XmlListModel.Loading
            }
        }
    }

    PageIndicator {
        count: view.count
        currentIndex: view.currentIndex
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
