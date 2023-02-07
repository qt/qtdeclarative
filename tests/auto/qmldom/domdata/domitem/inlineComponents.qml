import QtQuick

Item {
    id: mainComponent

    component IC1: Item { property string firstIC }
    component IC2: Item { property string secondIC }

    Item {
        Item {
            Item {
                component IC3: Item { property string thirdIC }
            }
        }
    }

}
