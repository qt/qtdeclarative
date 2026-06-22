// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Singleton
import QtQml

QtObject {
    property Component newPlanner: Planner {}
    property Component newVariable: Variable {}
    property Component newConstraint: Constraint {
        id: constraint
        Component.onCompleted: planner.incrementalAdd(constraint)
    }
    property Component newPlan: Plan {}

    // Global variable holding the current planner.
    property Planner planner: null

    property Variable prev
    property Variable first
    property Variable last
    function chainTest(n: int) {
        planner = newPlanner.createObject(this) as Planner;
        prev = null;
        first = null;
        last = null;

        // Build chain of n equality constraints
        for (var i = 0; i <= n; i++) {
            var v = newVariable.createObject(this, {objectName: "v" + i}) as Variable;
            if (prev !== null) {
                newConstraint.createObject(this, {
                    myInput: prev,
                    myOutput: v,
                    strength: Strength.REQUIRED
                });
            }
            if (i === 0)
                first = v;
            if (i === n)
                last = v;
            prev = v;
        }

        newConstraint.createObject(this, {myOutput: last, strength: Strength.STRONG_DEFAULT});
        let chainConstraint =
            newConstraint.createObject(this, {
                isInput: true,
                myOutput: first,
                strength: Strength.PREFERRED
            }) as Constraint;

        let plan = newPlan.createObject(this);
        planner.populatePlanFromConstraint(chainConstraint, plan);
        for (let i = 0; i < 100; i++) {
            first.value = i;
            plan.execute();
            if (last.value != i)
                console.error("Chain test failed.");
        }
    }

    property Variable scale
    property Variable offset
    property Variable src
    property Variable dst
    property list<Variable> dests
    function projectionTest(n: int) {
        planner = newPlanner.createObject(this) as Planner;
        scale = newVariable.createObject(this, {objectName: "scale", value: 10}) as Variable;
        offset = newVariable.createObject(this, {objectName: "offset", value: 1000}) as Variable;
        src = null;
        dst = null;

        dests = [];
        for (let i = 0; i < n; i++) {
            src = newVariable.createObject(this, {objectName: "src" + i, value: i}) as Variable;
            dst = newVariable.createObject(this, {objectName: "dst" + i, value: i}) as Variable;
            dests.push(dst);
            newConstraint.createObject(this, {myOutput: src, strength: Strength.NORMAL});
            newConstraint.createObject(this, {
                myInput: src,
                myOutput: dst,
                scale: scale,
                offset: offset,
                strength: Strength.REQUIRED
            });
        }

        change(src, 17);
        if (dst.value !== 1170)
            console.error("Projection 1 failed");
        change(dst, 1050);
        if (src.value !== 5)
            console.error("Projection 2 failed");
        change(scale, 5);
        for (let i = 0; i < n - 1; i++) {
            if (dests[i].value !== i * 5 + 1000)
                console.error("Projection 3 failed");
        }
        change(offset, 2000);
        for (let i = 0; i < n - 1; i++) {
            if (dests[i].value !== i * 5 + 2000)
                console.error("Projection 4 failed");
        }
    }

    property Constraint edit
    function change(v: Variable, newValue: int) {
        edit = newConstraint.createObject(this, {
            isInput: true,
            myOutput: v,
            strength: Strength.PREFERRED
        }) as Constraint
        let plan = newPlan.createObject(this);
        planner.populatePlanFromConstraint(edit, plan);
        for (let i = 0; i < 10; i++) {
            v.value = newValue;
            plan.execute();
        }

        if (edit.satisfaction !== Satisfaction.NONE)
            planner.incrementalRemove(edit);
        else
            edit.removeFromGraph();
    }

    function deltaBlue() {
        chainTest(100);
        projectionTest(100);
    }
}
