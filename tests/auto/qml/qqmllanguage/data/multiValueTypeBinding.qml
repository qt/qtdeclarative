import QtQml

QtObject {
    component Wrapper: QtObject {
        id: parent
        property alias width: holder.rect.width
        property alias height: holder.rect.height
        property int defaultWidth: 12
        property int defaultHeight: 400
        property QtObject holder: QtObject {
            id: holder;
            property rect rect
            rect.width: parent.defaultWidth;
            rect.height: parent.defaultHeight
        }
    }

    property Wrapper case1: Wrapper { width: 30; height: 700 }
    property Wrapper case2: Wrapper { height: 700; width: 30 }
}
