import qqmllanguage.Test
import QtQml

MyTypeObject {
    property bool datePropertyWasRead: false
    property bool timePropertyWasRead: false
    property bool dateTimePropertyWasRead: false

    Component.onCompleted: {
        let date;
        let time;
        let dateTime;

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setMonth(8), datePropertyWasRead = (date.getMonth() === dateProperty.getMonth());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setMonth(8), timePropertyWasRead = (time.getMonth() === timeProperty.getMonth());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setMonth(8), dateTimePropertyWasRead = (dateTime.getMonth() === dateTimeProperty.getMonth());
    }
}
