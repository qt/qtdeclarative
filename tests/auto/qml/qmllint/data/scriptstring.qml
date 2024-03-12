import QtQuick

Window {
    id: root

    Rectangle {
        id: main

        MouseArea {
            id: mouse
            property int clickCount: 0
            onClicked: {
                clickCount++

                switch ( clickCount % 3 ) {
                case 1 :
                    main.state = "middleState"
                    break
                case 2 :
                    main.state = "rightState"
                    break
                default :
                    main.state = "leftState"
                }
            }
        }

        Rectangle {
            id: mover
            anchors {
                left: undefined
                right: undefined
                horizontalCenter: undefined
                top: main.top
                bottom: main.bottom
            }
        }

        states: [
            State {
                name: "leftState"
                AnchorChanges {
                    target: mover
                    anchors.left: main.left
                    anchors.right: undefined
                    anchors.horizontalCenter: undefined
                }
            },
            State {
                name: "middleState"
                AnchorChanges {
                    target: mover
                    anchors {
                        left: undefined
                        right: undefined
                        horizontalCenter: main.horizontalCenter
                    }
                }
            },
            State {
                name: "rightState"
                AnchorChanges {
                    target: mover
                    anchors {
                        left: undefined
                        right: main.right
                        horizontalCenter: undefined
                    }
                }
            }
        ]
    }
}
