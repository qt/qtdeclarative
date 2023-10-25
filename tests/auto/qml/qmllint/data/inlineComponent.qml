import QtQuick 2.0

Item {
    component MyIC: IC {}
    component IC : QtObject {}
    QtObject {
        component IC2: QtObject {}

        property IC ic: IC {}
        property IC2 ic2: IC2 {}
    }

    property IC ic : IC {}
    property IC2 ic2: IC2 {}
}
