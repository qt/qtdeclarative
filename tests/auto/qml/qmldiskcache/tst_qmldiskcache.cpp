/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qtest.h>

#include <private/qv4compileddata_p.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QThread>

class tst_qmldiskcache: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void regenerateAfterChange();
    void registerImportForImplicitComponent();
};

struct TestCompiler
{
    TestCompiler(QQmlEngine *engine)
        : engine(engine)
        , tempDir()
        , testFilePath(tempDir.path() + QStringLiteral("/test.qml"))
        , cacheFilePath(tempDir.path() + QStringLiteral("/test.qmlc"))
        , mappedFile(cacheFilePath)
        , currentMapping(nullptr)
    {
    }

    bool compile(const QByteArray &contents)
    {
        if (currentMapping) {
            mappedFile.unmap(currentMapping);
            currentMapping = nullptr;
        }
        mappedFile.close();

        // Qt API limits the precision of QFileInfo::modificationTime() to seconds, so to ensure that
        // the newly written file has a modification date newer than an existing cache file, we must
        // wait.
        QThread::sleep(1);
        {
            QFile f(testFilePath);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                lastErrorString = f.errorString();
                return false;
            }
            if (f.write(contents) != contents.size()) {
                lastErrorString = f.errorString();
                return false;
            }
        }

        QQmlComponent component(engine, testFilePath);
        if (!component.isReady()) {
            lastErrorString = component.errorString();
            return false;
        }

        return true;
    }

    const QV4::CompiledData::Unit *mapUnit()
    {
        if (!mappedFile.open(QIODevice::ReadOnly)) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }

        currentMapping = mappedFile.map(/*offset*/0, mappedFile.size());
        if (!currentMapping) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }
        QV4::CompiledData::Unit *unitPtr;
        memcpy(&unitPtr, &currentMapping, sizeof(unitPtr));
        return unitPtr;
    }

    QQmlEngine *engine;
    const QTemporaryDir tempDir;
    const QString testFilePath;
    const QString cacheFilePath;
    QString lastErrorString;
    QFile mappedFile;
    uchar *currentMapping;
};

void tst_qmldiskcache::initTestCase()
{
    qputenv("QML_DISK_CACHE", "1");
}

void tst_qmldiskcache::regenerateAfterChange()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

#ifdef V4_ENABLE_JIT
    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        QCOMPARE(quint32(testUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = testUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Script));
        QCOMPARE(quint32(obj->bindingTable()->value.compiledScriptIndex), quint32(1));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(2));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(1);
        QVERIFY(bindingFunction->codeOffset > testUnit->unitSize);
    }
#else
    QVERIFY(!testCompiler.mapUnit());
    return;
#endif

    engine.clearComponentCache();

    {
        const QByteArray newContents = QByteArrayLiteral("import QtQml 2.0\n"
                                                         "QtObject {\n"
                                                         "    property string blah: Qt.platform;\n"
                                                         "    property int secondProperty: 42;\n"
                                                         "}");

        QVERIFY2(testCompiler.compile(newContents), qPrintable(testCompiler.lastErrorString));
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        QCOMPARE(quint32(testUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = testUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(2));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Number));
        QCOMPARE(obj->bindingTable()->valueAsNumber(), double(42));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(2));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(1);
        QVERIFY(bindingFunction->codeOffset > testUnit->unitSize);
    }
}

void tst_qmldiskcache::registerImportForImplicitComponent()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQuick 2.0\n"
                                                  "Loader {\n"
                                                   "    sourceComponent: Item {}\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
#ifdef V4_ENABLE_JIT
    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        QCOMPARE(quint32(testUnit->nImports), quint32(2));
        QCOMPARE(testUnit->stringAt(testUnit->importAt(0)->uriIndex), QStringLiteral("QtQuick"));

        QQmlType *componentType = QQmlMetaType::qmlType(&QQmlComponent::staticMetaObject);

        QCOMPARE(testUnit->stringAt(testUnit->importAt(1)->uriIndex), QString(componentType->module()));
        QCOMPARE(testUnit->stringAt(testUnit->importAt(1)->qualifierIndex), QStringLiteral("QmlInternals"));

        QCOMPARE(quint32(testUnit->nObjects), quint32(3));

        const QV4::CompiledData::Object *obj = testUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Object));

        const QV4::CompiledData::Object *implicitComponent = testUnit->objectAt(obj->bindingTable()->value.objectIndex);
        QCOMPARE(testUnit->stringAt(implicitComponent->inheritedTypeNameIndex), QStringLiteral("QmlInternals.") + componentType->elementName());
    }
#else
    QVERIFY(!testCompiler.mapUnit());
    return;
#endif
}

QTEST_MAIN(tst_qmldiskcache)

#include "tst_qmldiskcache.moc"
