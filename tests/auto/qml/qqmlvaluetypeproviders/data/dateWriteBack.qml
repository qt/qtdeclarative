import QtQml
import Test

MyTypeObject {
    aDateTime: new Date()
    aDate: new Date()
    aTime: new Date()
    aVariant: new Date()

    Component.onCompleted: {
        aDateTime.setDate(14);
        aDate.setDate(10);
        aDate.setMonth(8);
        aTime.setHours(5);
        aVariant.setMinutes(44);
    }
}
