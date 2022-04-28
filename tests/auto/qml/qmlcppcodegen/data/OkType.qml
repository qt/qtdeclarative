import QtQuick
import App

Item {
    property alias useListDelegate: inner.useListDelegate

    MyCppType {
        id: inner
    }
}
