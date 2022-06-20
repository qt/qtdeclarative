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
}
