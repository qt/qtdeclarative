import QtQuick

Item {
    width: 400
    height: 400

    component ZeroWidthItem: Item {
        implicitWidth: 0
        implicitHeight: 20
    }

    component ZeroHeightItem: Item {
        implicitWidth: 20
        implicitHeight: 0
    }

    Row {
        objectName: "row"

        ZeroWidthItem {}
        ZeroHeightItem {}
    }
    Column {
        objectName: "column"

        ZeroWidthItem {}
        ZeroHeightItem {}
    }
    Grid {
        objectName: "grid"

        ZeroWidthItem {}
        ZeroHeightItem {}
    }
    Flow {
        objectName: "flow"

        ZeroWidthItem {}
        ZeroHeightItem {}
    }
}
