// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

BaseConstraint {
    id: self

    property Variable myInput
    property Variable myOutput

    property Variable scale
    property Variable offset

    function addToGraph() {
        let ihn = myInput;
        if (ihn)
            ihn.addConstraint(self);
        let out = myOutput;
        if (out)
            out.addConstraint(self);
        let s = scale;
        if (s)
            s.addConstraint(self);
        let o = offset;
        if (o)
            o.addConstraint(self);
        satisfaction = Satisfaction.NONE;
    }

    function removeFromGraph() {
        if (myInput)
            myInput.removeConstraint(self);
        if (myOutput)
            myOutput.removeConstraint(self);
        if (scale)
            scale.removeConstraint(self);
        if (offset)
            offset.removeConstraint(self);
        satisfaction = Satisfaction.NONE;
    }

    property Variable input: (satisfaction === Satisfaction.BACKWARD)
        ? myOutput
        : myInput

    property Variable output: (satisfaction === Satisfaction.BACKWARD)
        ? myInput
        : myOutput

    function recalculate() {
        let ihn = input;
        let out = output;

        if (!ihn) {
            out.walkStrength = strength;
            out.stay = !isInput;
            return;
        }

        out.walkStrength = Strength.weakestOf(strength, ihn.walkStrength);
        let stay = ihn.stay;

        // Optimize for number of lookups. We lookup scale and offset only once and we keep
        // stay in a local as long as we can.

        let s = scale
        if (s)
            stay = stay && s.stay;

        let o = offset
        if (o)
            stay = stay && o.stay;

        out.stay = stay;

        if (stay)
            out.value = evaluate();
    }

    function chooseMethod(mark: int) {
        let ihn = myInput;
        let out = myOutput;
        let outStrength = out.walkStrength;

        if (!ihn) {
            satisfaction = (out.mark !== mark && Strength.stronger(strength, outStrength))
                ? Satisfaction.FORWARD
                : Satisfaction.NONE;
            return;
        }

        let ihnStrength = ihn.walkStrength;

        if (Strength.weaker(ihnStrength, outStrength)) {
            satisfaction = Strength.stronger(strength, ihnStrength)
                ? Satisfaction.BACKWARD
                : Satisfaction.NONE;
            return;
        }

        satisfaction = Strength.stronger(strength, outStrength)
            ? Satisfaction.FORWARD
            : Satisfaction.BACKWARD
    }

    function markInputs(mark: int) {
        let i = input;
        if (i)
            i.mark = mark;
        let s = scale;
        if (s)
            s.mark = mark;
        let o = offset;
        if (o)
            o.mark = mark;
    }

    function inputsKnown(mark: int) : bool {
        let ihn = input;
        return !ihn || ihn.mark === mark || ihn.stay || ihn.determinedBy === null;
    }

    function evaluate() : int {
        let result = input.value;

        // This is a rather hot code path. It pays off to do the lookups for offset and scale
        // only once
        let o = offset;
        let s = scale;

        if (satisfaction === Satisfaction.BACKWARD) {
            if (o)
                result = result - o.value;
            if (s)
                result = result / s.value;
        } else {
            if (s)
                result = result * s.value;
            if (o)
                result = result + o.value; // TODO: += and -= miscompile!
        }

        return result;
    }
}
