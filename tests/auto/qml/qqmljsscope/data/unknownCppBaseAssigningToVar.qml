import QtQuick

ListView {
    // model is a QVariant, so we can theoretically assign anything to it, even
    // the non existing type!
    model: NonExistingType {}
}
