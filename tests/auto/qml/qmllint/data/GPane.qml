import QtQuick
import QtQuick.Layouts

ColumnLayout {
    default property alias paneData: content.data

    Column {
        id: content
    }
}
