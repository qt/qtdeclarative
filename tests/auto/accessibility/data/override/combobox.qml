import QtQuick 2.12
import QtQuick.Controls 2.12

ComboBox {
    model: ["ComboBox"]
    Accessible.name: model[0] + "Override"
}
