import QtQuick

Item {
    Cycle1 {
        id: c1
        peer: c2
    }

    Cycle2 {
        id: c2
        peer: c3
    }

    Cycle3 {
        id: c3
        peer: c1
    }

    property int groupedBindingSum: c1.item.anchors.rightMargin
                                    + c2.item.anchors.rightMargin
                                    + c3.item.anchors.rightMargin
}
