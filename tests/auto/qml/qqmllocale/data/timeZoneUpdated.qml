import QtQuick 2.0

Item {
    property bool success: false

    property date localDate
    property date utcDate

    Component.onCompleted: {
        // Test date: 2012-06-01T02:15:30+10:00 (AEST timezone)
        localDate = new Date(2012, 6-1, 1, 2, 15, 30)
        utcDate = new Date(Date.UTC(2012, 6-1, 1, 2, 15, 30))

        if (localDate.getTimezoneOffset() != -600) {
            console.log("Wrong initial time-zone offset: " + localDate.getTimezoneOffset());
            return;
        }

        if (localDate.toLocaleString() != getLocalizedForm('2012-06-01T02:15:30')) {
            console.log("Wrong localized string for local date-time: "
                        + localDate.toLocaleString());
            return;
        }
        if (localDate.toISOString() != "2012-05-31T16:15:30.000Z") {
            console.log("Wrong ISO date string for local date-time: "
                        + localDate.toISOString());
            return;
        }

        if (utcDate.toISOString() != "2012-06-01T02:15:30.000Z") {
            console.log("Wrong ISO string for UTC date-time: "
                        + utcDate.toISOString());
            return;
        }
        if (utcDate.toLocaleString() != getLocalizedForm('2012-06-01T12:15:30')) {
            console.log("Wrong localized string for UTC date-time: "
                        + utcDate.toLocaleString());
            return;
        }

        success = true
    }

    function check() {
        success = false

        // We have changed to IST time zone - inform JS:
        Date.timeZoneUpdated()

        if (localDate.getTimezoneOffset() != -330) {
            console.log("Wrong revised time-zone offset: " + localDate.getTimezoneOffset());
            return;
        }

        if (localDate.toLocaleString() != getLocalizedForm('2012-06-01T02:15:30')) {
            console.log("Wrong localized string for old local date-time: "
                        + localDate.toLocaleString());
            return;
        }
        if (localDate.toISOString() != "2012-05-31T20:45:30.000Z") {
            console.log("Wrong ISO date string for old local date-time: "
                        + localDate.toISOString());
            return;
        }

        if (utcDate.toISOString() != "2012-06-01T06:45:30.000Z") {
            console.log("Wrong ISO string for old UTC date-time: "
                        + utcDate.toISOString());
            return;
        }
        if (utcDate.toLocaleString() != getLocalizedForm("2012-06-01T12:15:30")) {
            console.log("Wrong localized string for old UTC date-time: "
                        + utcDate.toLocaleString());
            return;
        }

        // Create new dates in this timezone
        localDate = new Date(2012, 6-1, 1, 2, 15, 30)
        utcDate = new Date(Date.UTC(2012, 6-1, 1, 2, 15, 30))

        if (localDate.toLocaleString() != getLocalizedForm("2012-06-01T02:15:30")) {
            console.log("Wrong localized string for fresh local date-time: "
                        + localDate.toLocaleString());
            return;
        }
        if (localDate.toISOString() != "2012-05-31T20:45:30.000Z") {
            console.log("Wrong ISO date string for fresh local date-time: "
                        + localDate.toISOString());
            return;
        }

        if (utcDate.toISOString() != "2012-06-01T02:15:30.000Z") {
            console.log("Wrong ISO string for fresh UTC date-time: "
                        + utcDate.toISOString());
            return;
        }
        if (utcDate.toLocaleString() != getLocalizedForm("2012-06-01T07:45:30")) {
            console.log("Wrong localized string for fresh UTC date-time: "
                        + utcDate.toLocaleString());
            return;
        }

        success = true
    }

    function resetTimeZone() {
        Date.timeZoneUpdated()
    }
}
