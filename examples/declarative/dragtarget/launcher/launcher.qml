import QtQuick 2.0

Rectangle {
    id: root
    width:  480; height: 640

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#FEFEF2" }
        GradientStop { position: 1.0; color: "#FEF0C9" }
    }

    GridView {
        id: applicationsView
        anchors {
            left: parent.left; top: parent.top; right: parent.right; bottom: favoritesFrame.top
            margins: 32
        }

        interactive: false

        add: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad }
        }
        move: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad }
        }

        model: VisualItemModel {
            id: applicationsVisualModel
            VisualDataModel {
                delegate: IconDelegate { drag.keys: [ "applications" ] }
                model: ListModel {
                    id: applicationsModel

                    ListElement { icon: "images/AudioPlayer_48.png" }
                    ListElement { icon: "images/EMail_48.png" }
                    ListElement { icon: "images/Camera_48.png" }
                    ListElement { icon: "images/VideoPlayer_48.png" }
                    ListElement { icon: "images/AddressBook_48.png" }
                    ListElement { icon: "images/DateBook_48.png" }
                    ListElement { icon: "images/TodoList_48.png" }
                    ListElement { icon: "images/AudioPlayer_48.png" }
                    ListElement { icon: "images/EMail_48.png" }
                    ListElement { icon: "images/Camera_48.png" }
                    ListElement { icon: "images/VideoPlayer_48.png" }
                    ListElement { icon: "images/AddressBook_48.png" }
                    ListElement { icon: "images/DateBook_48.png" }
                    ListElement { icon: "images/TodoList_48.png" }
                }
            }
        }
    }

    DragTarget {
        property int sourceIndex
        property int destinationIndex

        anchors.fill: applicationsView
        keys: [ "applications" ]

        onEntered: {
            sourceIndex = applicationsView.indexAt(drag.x, drag.y)
            destinationIndex = sourceIndex
            if (destinationIndex == -1)
                drag.accepted = false
        }
        onPositionChanged: {
            var index = applicationsView.indexAt(drag.x, drag.y)
            if (index != -1) {
                applicationsVisualModel.move(destinationIndex, index, 1)
                destinationIndex = index
            }
        }
//        onDropped: applicationsModel.move(sourceIndex, destinationIndex, 1)
//        onExited: applicationsModel.move(destinationIndex, sourceIndex, 1)
    }

    DragTarget {
        property int sourceIndex
        property int destinationIndex

        anchors.fill: applicationsView
        keys: [ "favorites" ]

        onEntered: {
            sourceIndex = drag.data
            destinationIndex = applicationsView.indexAt(drag.x, drag.y)
            if (destinationIndex == -1) {
                drag.accepted = false
            } else {
                applicationsVisualModel.insert(destinationIndex, favoritesVisualModel, sourceIndex, 1)
            }
        }
        onPositionChanged: {
            var index = applicationsView.indexAt(drag.x, drag.y)
            if (index != -1) {
                applicationsVisualModel.move(destinationIndex, index, 1)
                destinationIndex = index
            }
        }
//        onDropped: applicationsModel.move(sourceIndex, destinationIndex, 1)
//        onExited: applicationsModel.move(destinationIndex, sourceIndex, 1)
    }

    Rectangle {
        id: favoritesFrame
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 24 }
        height: 96

        radius: 13

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#D2691E" }
            GradientStop { position: 1.0; color: "#FF7F24" }
        }

        Rectangle {
            radius: 12
            anchors { fill: parent; leftMargin: 4; topMargin: 4; rightMargin: 1; bottomMargin: 1 }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FEFEF2" }
                GradientStop { position: 1.0; color: "#FEF0C9" }
            }
        }

        ListView {
            id: favoritesView

            anchors { fill: parent; leftMargin: 32; topMargin: 24; rightMargin: 32; bottomMargin: 24 }

            interactive: false
            orientation: ListView.Horizontal
            spacing: 32

            add: Transition {
                NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad }
            }

            model: VisualItemModel {
                id: favoritesVisualModel
                VisualDataModel {
                    delegate: IconDelegate { drag.keys: [ "favorites" ] }
                    model: ListModel {
                        id: favoritesModel
                        ListElement { icon: "images/AudioPlayer_48.png" }
                        ListElement { icon: "images/EMail_48.png" }
                        ListElement { icon: "images/Camera_48.png" }
                        ListElement { icon: "images/VideoPlayer_48.png" }
                    }
                }
            }
        }

        DragTarget {
            property int sourceIndex
            property int destinationIndex

            anchors.fill: favoritesView
            keys: [ "favorites" ]

            onEntered: {
                sourceIndex = favoritesView.indexAt(drag.x, drag.y)
                destinationIndex = sourceIndex
                if (destinationIndex == -1)
                    drag.accepted = false
            }
            onPositionChanged: {
                var index = favoritesView.indexAt(drag.x, drag.y)
                if (index != -1) {
                    favoritesVisualModel.move(destinationIndex, index, 1)
                    destinationIndex = index
                }
            }
//            onDropped: favoritesModel.move(sourceIndex, destinationIndex, 1)
//            onExited:  favoritesVisualModel.move(destinationIndex, sourceIndex, 1)
        }

        DragTarget {
            property int sourceIndex
            property int destinationIndex

            enabled: favoritesVisualModel.count < 4

            anchors.fill: favoritesView
            keys: [ "applications" ]

            onEntered: {
                sourceIndex = drag.data
                destinationIndex = favoritesView.indexAt(drag.x, drag.y)
                if (destinationIndex == -1) {
                    drag.accepted = false
                } else {
                    favoritesVisualModel.insert(destinationIndex, applicationsVisualModel, sourceIndex, 1)
                }
            }
            onPositionChanged: {
                var index = favoritesView.indexAt(drag.x, drag.y)
                if (index != -1) {
                    favoritesVisualModel.move(destinationIndex, index, 1)
                    destinationIndex = index
                }
            }
//            onDropped: favoritesModel.move(sourceIndex, destinationIndex, 1)
//            onExited: favoritesModel.move(destinationIndex, sourceIndex, 1)
        }
    }
}
