pragma Strict
import QtQml

QtObject {
    enum Parameters {
        Length = 32,
        Iterations = 32768,

        Category0 = 0xf0f,
        Category1 = 0xf0f0,
        Category2 = 0xf0f0f,
        Maximum   = 0xf0f0f0,
        Mask      = 0xabcdef
    }

    property list<double> nnn: {
        var result = [];
        result[0] = 10;
        return result;
    }

    function randomNumber() : int {
        return (Math.random() * Categorizer.Maximum);
    }

    property list<double> numbers: {
        var result = [];
        for (var i = 0; i < Categorizer.Length; ++i)
            result[i] = randomNumber();
        return result;
    }
}
