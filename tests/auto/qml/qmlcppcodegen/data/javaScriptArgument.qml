pragma Strict
import QtQml

QtObject {
    function absMinusOne(amount: real) : real {
        // Access it before the condition below, to make sure we still get the original
        var minusOne = amount !== 0 ? -1 : 0;

        // The condition causes the original arguemnt to be overwritten rather than a new
        // register to be allocated
        if (amount < 0)
            amount = -amount;

        return amount + minusOne;
    }

    property real a: absMinusOne(-5)
    property real b: absMinusOne(10)

    function stringMinusOne(amount: real) : string {
        // Access it before the condition below, to make sure we still get the original
        var minusOne = amount !== 0 ? -1 : 0;

        // The condition causes the original arguemnt to be overwritten rather than a new
        // register to be allocated
        if (amount < 0)
            amount = -amount + "t";

        return amount + minusOne;
    }

    property string c: stringMinusOne(-5)
    property string d: stringMinusOne(10)

    function printAmount(amount: real) : string {
        var sign;
        if (amount < 0) {
            sign = "-";
            amount = -amount;
        } else {
            sign = "";
        }
        return sign + amount;
    }

    property string e: printAmount(10);
    property string f: printAmount(-10);

    readonly property string units: " kMGT"

    function roundTo3Digits(number: real) : real  {
        var factor;

        if (number < 10)
            factor = 100;
        else if (number < 100)
            factor = 10;
        else
            factor = 1;

        return Math.round(number * factor) / factor;
    }

    function prettyPrintScale(amount: real) : string {
        var sign;
        if (amount < 0) {
            sign = "-";
            amount = -amount;
        } else {
            sign = "";
        }
        var unitOffset = 0;
        var unitAmount = 1;
        for (unitOffset = 0; amount > unitAmount * 1024; ++unitOffset, unitAmount *= 1024) {}
        var result = amount / unitAmount;
        return sign + roundTo3Digits(result) + units[unitOffset];
    }

    property list<string> scales: [
        prettyPrintScale(0),

        prettyPrintScale(1),
        prettyPrintScale(10),
        prettyPrintScale(100),
        prettyPrintScale(1000),
        prettyPrintScale(10000),
        prettyPrintScale(100000),
        prettyPrintScale(1000000),
        prettyPrintScale(10000000),
        prettyPrintScale(100000000),
        prettyPrintScale(1000000000),
        prettyPrintScale(10000000000),
        prettyPrintScale(100000000000),
        prettyPrintScale(1000000000000),
        prettyPrintScale(10000000000000),

        prettyPrintScale(-1),
        prettyPrintScale(-10),
        prettyPrintScale(-100),
        prettyPrintScale(-1000),
        prettyPrintScale(-10000),
        prettyPrintScale(-100000),
        prettyPrintScale(-1000000),
        prettyPrintScale(-10000000),
        prettyPrintScale(-100000000),
        prettyPrintScale(-1000000000),
        prettyPrintScale(-10000000000),
        prettyPrintScale(-100000000000),
        prettyPrintScale(-1000000000000),
        prettyPrintScale(-10000000000000),
    ]

    function forwardArg(a: real) : string {
        return prettyPrintScale(a);
    }
}
