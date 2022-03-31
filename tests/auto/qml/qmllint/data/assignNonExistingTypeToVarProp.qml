import QtQuick

ListView {
    // model is a QVariant, so we can theoretically assign anything to it
    model: NonExistingType {}
}
