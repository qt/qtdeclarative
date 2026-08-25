import Test 1.0

MyContainer {
    id: root

    property alias background: backgroundItem
    property alias navigation: mainItem.navigation

    MyContainer {
        id: mainItem
        property alias navigation: navCtrl

        MyContainer { id: navCtrl; objectName: "theNav" }
        MyContainer { id: backgroundItem; objectName: "theBg" }
    }
}
