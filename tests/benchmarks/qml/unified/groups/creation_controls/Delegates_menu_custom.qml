import QmlBench
import QtQuick
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 2
    staticCount: 25

    Component {
        id: menuItemComponent

        MenuItem {
            contentItem: Text {
                text: parent.text
                color: "navajowhite"
            }
            background: Rectangle {
                color: "steelblue"
            }
        }
    }

    delegate: Item {
        Menu {
            id: menu
            title: "Root Menu"
            delegate: menuItemComponent
            visible: true

            Menu {
                title: "Sub-menu 1"
                delegate: menuItemComponent

                Menu {
                    title: "Sub-sub-menu"
                    delegate: menuItemComponent
                }
            }

            Menu { title: "Sub-menu 2" }
            Action { text: "Action 3" }
            Action { text: "Action 4" }
            Action { text: "Action 5" }
        }
    }
}
