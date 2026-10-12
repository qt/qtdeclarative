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

        date = dateProperty, dateProperty.setTime(3829292), datePropertyWasRead = (date.getUTCDay() === dateProperty.getUTCDay());
        time = timeProperty, timeProperty.setTime(3489245), timePropertyWasRead = (time.getUTCDay() === timeProperty.getUTCDay());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimePropertyWasRead = (dateTime.getUTCDay() === dateTimeProperty.getUTCDay());
    }
}
