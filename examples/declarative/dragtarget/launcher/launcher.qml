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

        model: VisualItemModel {
            id: applicationsVisualModel
            VisualDataModel {
                delegate: IconDelegate { drag.keys: [ "applications" ] }
                model: ListModel {
                    id: applicationsModel
                    ListElement { icon: "images/AddressBook_48.png" }
                    ListElement { icon: "images/DateBook_48.png" }
                    ListElement { icon: "images/TodoList_48.png" }
                }
            }
        }
    }

    DragTarget {
        property int initialIndex
        property int previousIndex

        anchors.fill: applicationsView
        keys: [ "applications" ]

        onEntered: {
            initialIndex = applicationsView.indexAt(drag.x, drag.y)
            previousIndex = initialIndex
            if (previousIndex == -1)
                drag.accepted = false
        }
        onPositionChanged: {
            var index = applicationsView.indexAt(drag.x, drag.y)
            if (index != -1) {
                applicationsVisualModel.move(previousIndex, index, 1)
                previousIndex = index
            }
        }
        onDropped: applicationsModel.move(initialIndex, previousIndex, 1)
        onExited: applicationsModel.move(previousIndex, initialIndex, 1)
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
            property int initialIndex
            property int previousIndex

            anchors.fill: favoritesView
            keys: [ "favorites" ]

            onEntered: {
                initialIndex = favoritesView.indexAt(drag.x, drag.y)
                previousIndex = initialIndex
                if (previousIndex == -1)
                    drag.accepted = false
            }
            onPositionChanged: {
                var index = favoritesView.indexAt(drag.x, drag.y)
                if (index != -1) {
                    favoritesVisualModel.move(previousIndex, index, 1)
                    previousIndex = index
                }
            }
            onDropped: favoritesModel.move(initialIndex, previousIndex, 1)
            onExited:  favoritesVisualModel.move(previousIndex, initialIndex, 1)
        }
    }
}
