import QtQuick

Window {
    component WithId: Item {
        Item { id: colliding }

        property int colliding
        function colliding() {}

        function f() {
            let colliding = 3;
            return colliding; // should be the local JS variable
        }
        function g() {
            return colliding; // should be the id
        }
    }
    component WithoutId: Item {
        property int colliding
        function colliding() {}

        function h() {
            return colliding; // should be the property
        }
    }
    component WithoutId2: Item {
        function colliding() {}
        property int colliding

        function h() {
            return colliding; // should be the property
        }
    }
    component WithoutIdAndWithoutProperty: Item {
        function colliding() {}

        function h() {
            return colliding; // should be the method
        }
    }
}
