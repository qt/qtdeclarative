import QtQuick

Item {
    enum Status {
        Unknown,
        Loading,
        Ready
    }
    property int ownStatus: EnumType.Status.Ready
}
