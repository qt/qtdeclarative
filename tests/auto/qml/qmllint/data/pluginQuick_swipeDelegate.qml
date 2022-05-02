import QtQuick.Controls
import QtQuick

Item {
    SwipeDelegate {
        contentItem: Item { anchors.left: parent.left }
        background: Item { anchors.right: parent.right }

        swipe.left: Item {}
        swipe.behind: Item {}
    }
    SwipeDelegate {
        contentItem: Item { anchors.centerIn: parent }
        background: Item { anchors.fill: parent }

        swipe.right: Item {}
        swipe.behind: Item {}
    }
}
