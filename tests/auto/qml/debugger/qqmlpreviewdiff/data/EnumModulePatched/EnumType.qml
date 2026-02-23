import QtQuick

Item {
    enum Status {
        Unknown,
        Loading,
        Processing,
        Ready
    }
    property int ownStatus: EnumType.Status.Ready
}
