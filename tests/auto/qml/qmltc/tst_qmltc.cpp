/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "tst_qmltc.h"

// Generated headers:
#include "ResolvedNameConflict.h"
#include "helloworld.h"
#include "simpleqtquicktypes.h"
#include "typewithenums.h"
#include "methods.h"

// Qt:
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qurl.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

// on top of testing different cache configurations, we can also test namespace
// generation for the test classes using the same macro
#ifdef QMLTC_TESTS_DISABLE_CACHE
#    if QMLTC_TESTS_DISABLE_CACHE
#        define PREPEND_NAMESPACE(name) QT_PREPEND_NAMESPACE(name)
#    else
#        define PREPEND_NAMESPACE(name)                                                            \
            ::QmltcTest::name // silent contract that the namespace is QmltcTest
#    endif
#else
#    error "QMLTC_TESTS_DISABLE_CACHE is supposed to be defined and be equal to either 0 or 1"
#endif

tst_qmltc::tst_qmltc()
{
#if defined(QMLTC_TESTS_DISABLE_CACHE) && QMLTC_TESTS_DISABLE_CACHE
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#else
    qputenv("QML_DISABLE_DISK_CACHE", "0");
#endif
}

void tst_qmltc::initTestCase()
{
    const auto status = isCacheDisabled() ? u"DISABLED" : u"ENABLED";
    qInfo() << u"Disk cache is" << status;

    // Note: just check whether the QML code is valid. QQmlComponent is good for
    // it. also, we can use qrc to make sure the file is in the resource system.
    QUrl urls[] = {
        QUrl("qrc:/QmltcTests/data/NameConflict.qml"),
        QUrl("qrc:/QmltcTests/data/HelloWorld.qml"),
        QUrl("qrc:/QmltcTests/data/simpleQtQuickTypes.qml"),
        QUrl("qrc:/QmltcTests/data/typeWithEnums.qml"),
    };

    QQmlEngine e;
    QQmlComponent component(&e);
    for (const auto &url : urls) {
        component.loadUrl(url);
        QVERIFY2(!component.isError(), qPrintable(u"Bad QML file. "_qs + component.errorString()));
    }
}

void tst_qmltc::qmlNameConflictResolution()
{
    // we can include user-renamed files
    QQmlEngine e;
    // Note: the C++ class name is derived from the source qml file path, not
    // the output .h/.cpp, so: NameConflict class name for NameConflict.qml
    PREPEND_NAMESPACE(NameConflict) created(&e); // note: declared in ResolvedNameConflict.h
}

void tst_qmltc::helloWorld()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(HelloWorld) created(&e);
    QSKIP("Nothing is supported yet.");
}

void tst_qmltc::qtQuickIncludes()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(simpleQtQuickTypes) created(&e); // it should just compile as well
    // since the file name is lower-case, let's also test that it's marked as
    // QML_ANONYMOUS
    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);
    QCOMPARE(mo->classInfo(mo->indexOfClassInfo("QML.Element")).value(), "anonymous");
}

void tst_qmltc::enumerations()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(typeWithEnums) created(&e);

    // sanity
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::A, 0);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::B, 1);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::C, 2);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::D, 3);

    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::A_, 1);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B_, 2);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B2_, 3);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::C_, 41);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::D_, 42);

    const QMetaObject *mo = created.metaObject();
    const QMetaEnum enumerator1 = mo->enumerator(mo->indexOfEnumerator("NoValuesSpecified"));
    QCOMPARE(enumerator1.enumName(), "NoValuesSpecified");
    QCOMPARE(enumerator1.keyCount(), 4);
    QCOMPARE(enumerator1.key(2), "C");
    QCOMPARE(enumerator1.value(2), PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::C);

    const QMetaEnum enumerator2 = mo->enumerator(mo->indexOfEnumerator("ValuesSpecified"));
    QCOMPARE(enumerator2.enumName(), "ValuesSpecified");
    QCOMPARE(enumerator2.keyCount(), 5);
    QCOMPARE(enumerator2.key(2), "B2_");
    QCOMPARE(enumerator2.value(2), PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B2_);
}

void tst_qmltc::methods()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(methods) created(&e);

    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);

    QMetaMethod metaJustSignal = mo->method(mo->indexOfSignal("justSignal()"));
    QMetaMethod metaTypedSignal = mo->method(mo->indexOfSignal(
            QMetaObject::normalizedSignature("typedSignal(QString,QObject *,double)")));
    QMetaMethod metaJustMethod = mo->method(mo->indexOfMethod("justMethod()"));
    QMetaMethod metaUntypedMethod = mo->method(mo->indexOfMethod(
            QMetaObject::normalizedSignature("untypedMethod(QVariant,QVariant)")));
    QMetaMethod metaTypedMethod = mo->method(
            mo->indexOfMethod(QMetaObject::normalizedSignature("typedMethod(double,int)")));

    QVERIFY(metaJustSignal.isValid());
    QVERIFY(metaTypedSignal.isValid());
    QVERIFY(metaJustMethod.isValid());
    QVERIFY(metaUntypedMethod.isValid());
    QVERIFY(metaTypedMethod.isValid());

    QCOMPARE(metaJustSignal.methodType(), QMetaMethod::Signal);
    QCOMPARE(metaTypedSignal.methodType(), QMetaMethod::Signal);
    QCOMPARE(metaJustMethod.methodType(), QMetaMethod::Method);
    QCOMPARE(metaUntypedMethod.methodType(), QMetaMethod::Method);
    QCOMPARE(metaTypedMethod.methodType(), QMetaMethod::Method);

    QCOMPARE(metaTypedSignal.parameterMetaType(0), QMetaType::fromType<QString>());
    QCOMPARE(metaTypedSignal.parameterMetaType(1), QMetaType::fromType<QObject *>());
    QCOMPARE(metaTypedSignal.parameterMetaType(2), QMetaType::fromType<double>());
    QCOMPARE(metaTypedSignal.parameterNames(), QList<QByteArray>({ "a", "b", "c" }));

    QCOMPARE(metaUntypedMethod.parameterMetaType(0), QMetaType::fromType<QVariant>());
    QCOMPARE(metaUntypedMethod.parameterMetaType(1), QMetaType::fromType<QVariant>());
    QCOMPARE(metaUntypedMethod.parameterNames(), QList<QByteArray>({ "d", "c" }));

    QCOMPARE(metaTypedMethod.parameterMetaType(0), QMetaType::fromType<double>());
    QCOMPARE(metaTypedMethod.parameterMetaType(1), QMetaType::fromType<int>());
    QCOMPARE(metaTypedMethod.returnMetaType(), QMetaType::fromType<QString>());
    QCOMPARE(metaTypedMethod.parameterNames(), QList<QByteArray>({ "a", "b" }));
}

QTEST_MAIN(tst_qmltc)
