// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound
pragma Strict
import QtQml

QtObject {
    id: projectionTest
    property int length: 100

    property Planner planner: Planner {
        id: planner
    }

    property Variable scale: Variable {
        id: scaleVariable
        objectName: "scale"
        value: 10
    }

    property Variable offset: Variable {
        id: offsetVariable
        objectName: "offset"
        value: 1000
    }

    property Component variable: Variable {}

    property list<Variable> sources: new Array(length)
    property Variable src: sources[length - 1]
    function setSource(i: int, v: Variable) { sources[i] = v; }

    property list<Variable> destinations: new Array(length)
    property Variable dst: destinations[length - 1]
    function setDestination(i: int, v: Variable) { destinations[i] = v; }

    property Component constraint: Constraint {}

    Component.onCompleted: {
        let p = planner;
        for (let i = 0, end = length; i < end; ++i) {
            let source = variable.createObject(this, {
                objectName: "src" + i,
                value: i
            }) as Variable;
            setSource(i, source);

            let destination = variable.createObject(this, {
                objectName: "dst" + i,
                value: i
            }) as Variable;
            setDestination(i, destination);

            p.incrementalAdd(constraint.createObject(this, {
                myOutput: source,
                strength: Strength.NORMAL
            }) as Constraint);

            p.incrementalAdd(constraint.createObject(this, {
                myInput: source,
                myOutput: destination,
                scale: scaleVariable,
                offset: offsetVariable,
                strength: Strength.REQUIRED
            }) as Constraint);
        }
    }

    component Changer : Constraint {
        id: editConstraint
        isInput: true

        strength: Strength.PREFERRED

        property Plan plan: Plan {}

        function run(v: Variable, newValue: int) {
            myOutput = v;
            planner.incrementalAdd(editConstraint);
            planner.populatePlanFromConstraint(editConstraint, plan);
            for (let i = 0; i < 10; ++i) {
                v.value = newValue;
                plan.execute();
            }
            planner.incrementalRemove(editConstraint);
        }
    }

    property Changer changer: Changer { id: changer }

    function run() {
        changer.run(src, 17);
        if (dst.value !== 1170)
            console.error("Projection 1 failed", dst.value, offsetVariable.value);
        changer.run(dst, 1050);
        if (src.value !== 5)
            console.error("Projection 2 failed", src.value, offsetVariable.value);
        changer.run(scaleVariable, 5);

        let d = destinations;
        for (let i = 0, end = length - 1; i < end; ++i) {
            if (d[i].value !== i * 5 + 1000)
                console.error("Projection 3 failed");
        }
        changer.run(offsetVariable, 2000);
        for (let i = 0, end = length - 1; i < end; ++i) {
            if (d[i].value !== i * 5 + 2000)
                console.error("Projection 4 failed");
        }
    }
}
