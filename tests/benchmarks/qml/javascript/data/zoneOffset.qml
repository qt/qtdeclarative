// Benchmarks determination of local time-zone offset.
// This can be sensitive to changes to QTimeZone or QDateTime (depends on platform).

import QtQuick 2.0

QtObject {
    function runtest() {
        var tick = -864e8; // 1000 days before the epoch
        for (var ii = 0; ii < 5000000; ++ii) {
            new Date(tick).getTimezoneOffset();
            tick += 5e5;
        }
        // 25e11 milliseconds is about 79 years; so we span from
        // the start of 1967-04-07 to 2046-6-26 04:18:20.
    }
}
