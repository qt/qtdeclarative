// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This JavaScript file is a single, large, imported script.
// It imports many other (non-nested) single, large, scripts,
// and also imports many other nested, large scripts.

.import "mldsi4.js" as Mldsi4
.import "mldsi9.js" as Mldsi9
.import "mlbsi1.js" as Mlbsi1
.import "mlbsi2.js" as Mlbsi2
.import "mlbsi3.js" as Mlbsi3
.import "mlbsi4.js" as Mlbsi4
.import "mlbsi5.js" as Mlbsi5
.import "mlbsi6.js" as Mlbsi6
.import "mlbsi7.js" as Mlbsi7
.import "mlbsi8.js" as Mlbsi8
.import "mlbsi9.js" as Mlbsi9
.import "mlbsi10.js" as Mlbsi10
.import "mlbsi11.js" as Mlbsi11
.import "mlbsi12.js" as Mlbsi12
.import "mlbsi13.js" as Mlbsi13
.import "mlbsi14.js" as Mlbsi14
.import "mlbsi15.js" as Mlbsi15

function testFunc(seedValue) {
    var firstFactor = calculateFirstFactor(seedValue);
    var secondFactor = calculateSecondFactor(seedValue);
    var modificationTerm = calculateModificationTerm(seedValue);

    // do some regexp matching
    var someString = "This is a random string which we'll perform regular expression matching on to reduce considerably.  This is meant to be part of a complex javascript expression whose evaluation takes considerably longer than the creation cost of QScriptValue.";
    var regexpPattern = new RegExp("is", "i");
    var regexpOutputLength = 0;
    var temp = regexpPattern.exec(someString);
    while (temp == "is") {
        regexpOutputLength += 4;
        regexpOutputLength *= 2;
        temp = regexpPattern.exec(someString);
        if (regexpOutputLength > (seedValue * 3)) {
            temp = "break";
        }
    }

    // spin in a for loop for a while
    var i = 0;
    var j = 0;
    var cumulativeTotal = 3;
    for (i = 20; i > 1; i--) {
        for (j = 31; j > 5; j--) {
            var branchVariable = i + j;
            if (branchVariable % 3 == 0) {
                cumulativeTotal -= secondFactor;
            } else {
                cumulativeTotal += firstFactor;
            }

            if (cumulativeTotal > (seedValue * 50)) {
                break;
            }
        }
    }
    var retn = cumulativeTotal;
    retn += Mlbsi1.testFunc(seedValue);
    retn += Mlbsi2.testFunc(seedValue);
    retn += Mlbsi3.testFunc(retn);
    retn += Mlbsi4.testFunc(seedValue);
    retn += Mlbsi5.testFunc(seedValue);
    retn += Mlbsi6.testFunc(seedValue);
    retn *= Mldsi9.testFunc(retn);
    retn += Mlbsi7.testFunc(seedValue);
    retn += Mlbsi8.testFunc(retn);
    retn += Mlbsi9.testFunc(seedValue);
    retn += Mlbsi10.testFunc(seedValue);
    retn += Mlbsi11.testFunc(seedValue);
    retn *= Mldsi4.testFunc(retn);
    retn += Mlbsi12.testFunc(seedValue);
    retn += Mlbsi13.testFunc(retn);
    retn += Mlbsi14.testFunc(seedValue);
    retn += Mlbsi15.testFunc(seedValue);
    return retn;
}

function calculateFirstFactor(seedValue) {
    var firstFactor = (0.45 * (9.3 / 3.1) - 0.90);
    firstFactor *= (1 + (0.00001 / seedValue));
    return firstFactor;
}

function calculateSecondFactor(seedValue) {
    var secondFactor = 0.78 * (6.3 / 2.1) - (0.39 * 4);
    secondFactor *= (1 + (0.00001 / seedValue));
    return secondFactor;
}

function calculateModificationTerm(seedValue) {
    var modificationTerm = (12 + (9*7) - 54 + 16 - ((calculateFirstFactor(seedValue) * seedValue) / 3) + (4*calculateSecondFactor(seedValue) * seedValue * 1.33)) + (calculateSecondFactor(seedValue) * seedValue);
    modificationTerm = modificationTerm + (33/2) + 19 - (9*2) - (61*3) + 177;
    return modificationTerm;
}
