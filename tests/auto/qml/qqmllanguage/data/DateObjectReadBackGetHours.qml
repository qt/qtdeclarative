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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setHours(7), datePropertyWasRead = (date.getHours() === dateProperty.getHours());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setHours(7), timePropertyWasRead = (time.getHours() === timeProperty.getHours());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setHours(7), dateTimePropertyWasRead = (dateTime.getHours() === dateTimeProperty.getHours());
    }
}
