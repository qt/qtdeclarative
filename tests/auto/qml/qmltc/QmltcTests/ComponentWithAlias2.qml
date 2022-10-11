import QtQuick

Item {
    property alias setMe: firstComponent.setMe
    ComponentWithAlias3 {
        id: firstComponent
    }
}
