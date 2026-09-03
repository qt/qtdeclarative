pragma Strict
import QtQml

QtObject {
    id: root

    enum Parameters {
        Length = 32,
        Iterations = 2,

        Category0 = 0xf0f,
        Category1 = 0xf0f0,
        Category2 = 0xf0f0f,
        Maximum   = 0xf0f0f0,
        Mask      = 0xabcdef
    }

    function randomNumber() : int {
        return (Math.random() * Categorizer.Maximum);
    }

    property list<double> numbers: {
        // Can't StoreElement on non-list-properties in 6.5.
        // To keep the pragma Strict, slightly adjust the original code here.
        return [
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
            randomNumber(), randomNumber(), randomNumber(), randomNumber(),
        ];
    }

    function sum() : list<double> {
        var numbers = root.numbers;

        var cat1Sum = 0;
        var cat2Sum = 0;
        var cat3Sum = 0;
        var huge = 0;
        for (var i = 0; i < Categorizer.Iterations; ++i) {
            for (var j = 0; j < Categorizer.Length; ++j) {
                var num = numbers[j] & Categorizer.Mask;
                if (num < Categorizer.Category0)
                    cat1Sum += num;
                else if (num < Categorizer.Category1)
                    cat2Sum += num;
                else if (num < Categorizer.Category2)
                    cat3Sum += num;
                else
                    huge += num;
            }
        }

        return [cat1Sum, cat2Sum, cat3Sum, huge];
    }
}
