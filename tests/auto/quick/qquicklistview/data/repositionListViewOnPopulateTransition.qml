import QtQuick
import QtQuick.Controls.Basic

ListView {
    id: listView

    width: 100
    height: 100

    verticalLayoutDirection: ListView.BottomToTop
    interactive: true
    reuseItems: false

    model: ListModel {
        ListElement {index: 0; text: "Item0"}
    }

    delegate: Button {
        id: button

        required text

        property alias yScale: scaleTransform.yScale
        property bool inverted: ListView.view.verticalLayoutDirection === ListView.BottomToTop

        transform: Scale {
            id: scaleTransform
            origin.y: button.inverted ? button.implicitHeight : 0
        }
    }

    populate: Transition {
        id: populateTransition

        NumberAnimation {
            // in this case we do want to animate y to achieve a smooth
            // transition from y=0 to its assigned y location
            property: "y"
            duration: 500
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            property: "yScale"
            from: 0
            to: 1
            duration: 500
            easing.type: Easing.InOutQuad
        }
    }
}

