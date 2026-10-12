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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setMinutes(47), datePropertyWasRead = (date.getMinutes() === dateProperty.getMinutes());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setMinutes(47), timePropertyWasRead = (time.getMinutes() === timeProperty.getMinutes());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setMinutes(47), dateTimePropertyWasRead = (dateTime.getMinutes() === dateTimeProperty.getMinutes());
    }
}
