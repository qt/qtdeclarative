import QtQuick 2.0

Item {
    enum Status {
        On, Off
    }

    property int status: EnumAccess1.Status.Off
}
