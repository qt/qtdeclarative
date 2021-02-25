/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TST_QJSMANAGEDVALUE_H
#define TST_QJSMANAGEDVALUE_H

#include <QtCore/qobject.h>
#include <QtQml/qjsengine.h>

class tst_QJSManagedValue : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ctor_invalid();
    void ctor_undefinedWithEngine();
    void ctor_nullWithEngine();
    void ctor_boolWithEngine();
    void ctor_intWithEngine();
    void ctor_uintWithEngine();
    void ctor_floatWithEngine();
    void ctor_stringWithEngine();
    void ctor_copyAndAssignWithEngine();

    void toString();
    void toNumber();
    void toBoolean();
    void toVariant();

    void equals();
    void strictlyEquals();

    void hasProperty_basic();
    void hasProperty_globalObject();
    void hasProperty_changePrototype();
    void hasProperty_QTBUG56830_data();
    void hasProperty_QTBUG56830();

    void deleteProperty_basic();
    void deleteProperty_globalObject();
    void deleteProperty_inPrototype();

    void getSetPrototype_cyclicPrototype();
    void getSetPrototype_evalCyclicPrototype();
    void getSetPrototype_eval();
    void getSetPrototype_invalidPrototype();
    void getSetPrototype_twoEngines();
    void getSetPrototype_null();
    void getSetPrototype_notObjectOrNull();
    void getSetPrototype();
    void getSetProperty_propertyRemoval();
    void getSetProperty_resolveMode();
    void getSetProperty_twoEngines();
    void getSetProperty_gettersAndSettersThrowErrorJS();
    void getSetProperty_array();
    void getSetProperty();

    void call_function();
    void call_object();
    void call_newObjects();
    void call_this();
    void call_arguments();
    void call();
    void call_twoEngines();
    void call_nonFunction_data();
    void call_nonFunction();
    void construct_nonFunction_data();
    void construct_nonFunction();
    void construct_simple();
    void construct_newObjectJS();
    void construct_arg();
    void construct_proto();
    void construct_returnInt();
    void construct_throw();
    void construct_twoEngines();
    void construct_constructorThrowsPrimitive();
    void castToPointer();
    void engineDeleted();
    void valueOfWithClosure();

    void jsvalueArrayToSequenceType();

    void stringAndUrl();
    void jsFunctionInVariant();
    void stringByIndex();
    void jsMetaTypes();

    void exceptionsOnNullAccess();

private:
    void newEngine() { engine.reset(new QJSEngine()); }
    QScopedPointer<QJSEngine> engine;
};

#endif // TST_QJSMANAGEDVALUE
