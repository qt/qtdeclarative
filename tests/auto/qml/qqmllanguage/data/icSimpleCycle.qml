import QtQuick 2.15

Item {
    component A : B {}
    component B: A {}
}
