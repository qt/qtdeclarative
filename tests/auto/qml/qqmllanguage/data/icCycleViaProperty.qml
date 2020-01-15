import QtQuick 2.15

Item {
    component A : Item {
        property var test: B {}
    }
    component B: A {}
    A {}
}
