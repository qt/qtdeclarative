import QtQuick 2.0
import Qt.labs.controls 1.0

Item {
    width: 200
    height: 320

    //! [1]
    SwipeView {
        id: view

        currentIndex: 1
        anchors.fill: parent

        Item {
            id: firstPage
        }
        Item {
            id: secondPage
        }
        Item {
            id: thirdPage
        }
    }

    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: view.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    //! [1]
}
