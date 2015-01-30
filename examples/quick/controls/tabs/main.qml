/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0
import QtQuick.XmlListModel 2.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls - Tabs Example"

    TabView {
        id: tabView

        spacing: 1
        anchors.fill: parent
        background: Rectangle { color: style.frameColor }

        Rectangle {
            Tab.title: "Home"
            anchors.fill: parent

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
                color: window.style.accentColor
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
            Tab.title: "Discover"
            anchors.fill: parent

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
                        spacing: window.style.spacing
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: window.style.frameColor
                            visible: index == 0
                        }
                        Item { width: 1; height: window.style.spacing }
                        Label {
                            text: model.title
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            elide: Text.ElideRight
                            color: window.style.accentColor
                            font.pointSize: 20
                            lineHeight: 0.75
                        }
                        Label {
                            text: model.description
                            textFormat: Qt.RichText
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            spacing: window.style.spacing
                            Label {
                                text: model.creator
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: window.style.focusColor
                                font.pointSize: 8
                            }
                            Label {
                                text: model.pubDate
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                opacity: window.style.disabledOpacity
                                font.pointSize: 8
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Read more..."
                                onClicked: Qt.openUrlExternally(model.link)
                            }
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: window.style.frameColor
                        }
                    }
                }

                AbstractScrollIndicator.vertical: ScrollIndicator { }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: feedModel.status === XmlListModel.Loading
            }
        }

        Rectangle {
            Tab.title: "Activity"
            anchors.fill: parent

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
                        spacing: window.style.spacing
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: window.style.frameColor
                            visible: index == 0
                        }
                        Item { width: 1; height: window.style.spacing }
                        Label {
                            text: model.title
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            elide: Text.ElideRight
                            color: window.style.accentColor
                            font.pointSize: 14
                            lineHeight: 0.75
                        }
                        Item { width: 1; height: window.style.spacing }
                        Label {
                            text: model.description
                            textFormat: Qt.RichText
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            x: window.style.padding
                            width: parent.width - 2 * window.style.padding
                            spacing: window.style.spacing
                            Label {
                                text: model.creator
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                color: window.style.focusColor
                                font.pointSize: 8
                            }
                            Label {
                                text: model.pubDate
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                opacity: window.style.disabledOpacity
                                font.pointSize: 8
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Read more..."
                                onClicked: Qt.openUrlExternally(model.link)
                            }
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: window.style.frameColor
                        }
                    }
                }

                AbstractScrollIndicator.vertical: ScrollIndicator { }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: feedModel.status === XmlListModel.Loading
            }
        }
    }

    PageIndicator {
        count: tabView.count
        currentIndex: tabView.currentIndex
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
