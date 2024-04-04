import QtQuick

Item {
    component IC1: Item { property int inIc1: 123 }
    component IC2: Item { property IC1 inIc2 }

    IC1 {
        id: firstUsage
        property int inFirstUsage
    }
    IC2 {
        id: secondUsage
        property int inSecondUsage
    }
    Item {
        Item {
            IC1 {
                id: thirdUsage
                property int inThirdUsage
            }
        }
    }
}
