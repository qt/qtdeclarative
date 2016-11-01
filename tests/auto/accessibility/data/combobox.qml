import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.0

Window {
    visible: true

    ComboBox {
        id: button
        objectName: "combobox"
        model: ["ComboBox"]
    }
}
