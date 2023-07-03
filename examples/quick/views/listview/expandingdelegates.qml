// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "content"

// This example illustrates expanding a list item to show a more detailed view.

Rectangle {
    id: page
    width: 400
    height: 240
    color: "black"

    // Delegate for the recipes.  This delegate has two modes:
    // 1. List mode (default), which just shows the picture and title of the recipe.
    // 2. Details mode, which also shows the ingredients and method.
    Component {
        id: recipeDelegate
//! [0]
        Item {
            id: recipe

            required property string title
            required property string picture
            required property string ingredients
            required property string method

            // Create a property to contain the visibility of the details.
            // We can bind multiple element's opacity to this one property,
            // rather than having a "PropertyChanges" line for each element we
            // want to fade.
            property real detailsOpacity : 0
//! [0]
            width: ListView.view.width
            height: 70

            // A simple rounded rectangle for the background
            Rectangle {
                id: background
                x: 2
                y: 2
                width: parent.width - x * 2
                height: parent.height - y * 2
                color: "ivory"
                border.color: "orange"
                radius: 5
            }

            // This mouse region covers the entire delegate.
            // When clicked it changes mode to 'Details'.  If we are already
            // in Details mode, then no change will happen.
//! [1]
            MouseArea {
                anchors.fill: parent
                onClicked: recipe.state = 'Details';
            }

            // Lay out the page: picture, title and ingredients at the top, and method at the
            // bottom.  Note that elements that should not be visible in the list
            // mode have their opacity set to recipe.detailsOpacity.

            Row {
                id: topLayout
                x: 10
                y: 10
                height: recipeImage.height
                width: parent.width
                spacing: 10

                Image {
                    id: recipeImage
                    width: 50
                    height: 50
                    source: recipe.picture
                }
//! [1]
                Column {
                    width: background.width - recipeImage.width - 20
                    height: recipeImage.height
                    spacing: 5

                    Text {
                        text: recipe.title
                        font.bold: true
                        font.pointSize: 16
                    }

                    SmallText {
                        text: qsTr("Ingredients")
                        font.bold: true
                        opacity: recipe.detailsOpacity
                    }

                    SmallText {
                        text: recipe.ingredients
                        wrapMode: Text.WordWrap
                        width: parent.width
                        opacity: recipe.detailsOpacity
                    }
                }
            }

//! [2]
            Item {
                id: details
                x: 10
                width: parent.width - 20

                anchors {
                    top: topLayout.bottom
                    topMargin: 10
                    bottom: parent.bottom
                    bottomMargin: 10
                }
                opacity: recipe.detailsOpacity
//! [2]
                SmallText {
                    id: methodTitle
                    anchors.top: parent.top
                    text: qsTr("Method")
                    font.pointSize: 12
                    font.bold: true
                }

                Flickable {
                    id: flick
                    width: parent.width
                    anchors {
                        top: methodTitle.bottom
                        bottom: parent.bottom
                    }
                    contentHeight: methodText.height
                    clip: true

                    Text {
                        id: methodText
                        text: recipe.method
                        wrapMode: Text.WordWrap
                        width: details.width
                    }
                }

                Image {
                    anchors {
                        right: flick.right
                        top: flick.top
                    }
                    source: "content/pics/moreUp.png"
                    opacity: flick.atYBeginning ? 0 : 1
                }

                Image {
                    anchors {
                        right: flick.right
                        bottom: flick.bottom
                    }
                    source: "content/pics/moreDown.png"
                    opacity: flick.atYEnd ? 0 : 1
                }
//! [3]
            }

            // A button to close the detailed view, i.e. set the state back to default ('').
            TextButton {
                y: 10
                anchors {
                    right: background.right
                    rightMargin: 10
                }
                opacity: recipe.detailsOpacity
                text: qsTr("Close")

                onClicked: recipe.state = '';
            }

            states: State {
                name: "Details"

                PropertyChanges {
                    background.color: "white"
                    recipeImage {
                         // Make picture bigger
                        width: 130
                        height: 130
                    }
                    recipe {
                        // Make details visible
                        detailsOpacity: 1
                        x: 0

                        // Fill the entire list area with the detailed view
                        height: listView.height
                    }
                }

                // Move the list so that this item is at the top.
                PropertyChanges {
                    recipe.ListView.view.contentY: recipe.y
                    explicit: true;
                }

                // Disallow flicking while we're in detailed view
                PropertyChanges {
                    recipe.ListView.view.interactive: false
                }
            }

            transitions: Transition {
                // Make the state changes smooth
                ParallelAnimation {
                    ColorAnimation {
                        property: "color"
                        duration: 500
                    }
                    NumberAnimation {
                        duration: 300
                        properties: "detailsOpacity,x,contentY,height,width"
                    }
                }
            }
        }
//! [3]
    }

    // The actual list
    ListView {
        id: listView
        anchors.fill: parent
        model: RecipesModel { }
        delegate: recipeDelegate
    }
}
