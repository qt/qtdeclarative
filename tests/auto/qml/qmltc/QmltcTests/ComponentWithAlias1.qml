import QtQuick

Item {
    property alias setMe: firstComponent.setMe
    ComponentWithAlias2 {
        id: firstComponent
    }
}
