import QtQuick 2.15

Item {
    id: outer

    property real innerWidth: 0

    Item {
        id: inner
        width: style.width
        onWidthChanged: outer.innerWidth = width
    }

    width: inner.width

    onWidthChanged: {
        if (width !== inner.width)
            inner.width = width // overwrite binding
    }

    QtObject {
        id: style
        property int width: 50
    }
}
