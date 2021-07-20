import QtQuick
import QtQuick.Controls

ComboBox {
    model: ["ComboBox"]
    Accessible.name: model[0] + "Override"
}
