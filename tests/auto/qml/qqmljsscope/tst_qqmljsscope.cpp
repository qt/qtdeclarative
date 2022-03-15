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

#include <QtTest/QtTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qurl.h>
#include <QtCore/qlibraryinfo.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>

class tst_qqmljsscope : public QQmlDataTest
{
    Q_OBJECT

    QString loadUrl(const QString &url)
    {
        const QFileInfo fi(url);
        QFile f(fi.absoluteFilePath());
        f.open(QIODevice::ReadOnly);
        QByteArray data(fi.size(), Qt::Uninitialized);
        f.read(data.data(), data.length());
        return QString::fromUtf8(data);
    }

    QQmlJSScope::ConstPtr run(QString url)
    {
        url = testFile(url);
        const QString sourceCode = loadUrl(url);
        if (sourceCode.isEmpty())
            return QQmlJSScope::ConstPtr();

        QmlIR::Document document(false);
        // NB: JS unit generated here is ignored, so use noop function
        QQmlJSSaveFunction noop([](auto &&...) { return true; });
        QQmlJSCompileError error;
        [&]() {
            QVERIFY2(qCompileQmlFile(document, url, noop, nullptr, &error),
                     qPrintable(error.message));
        }();
        if (!error.message.isEmpty())
            return QQmlJSScope::ConstPtr();


        QQmlJSLogger logger;
        logger.setFileName(url);
        logger.setCode(sourceCode);
        logger.setSilent(true);
        QQmlJSImportVisitor visitor(&m_importer, &logger, dataDirectory());
        QQmlJSTypeResolver typeResolver { &m_importer };
        typeResolver.init(&visitor, document.program);
        return visitor.result();
    }

private Q_SLOTS:
    void initTestCase() override;

    void orderedBindings();
    void signalCreationDifferences();
    void allTypesAvailable();
    void shadowing();
#ifdef LABS_QML_MODELS_PRESENT
    void componentWrappedObjects();
#endif
    void unknownCppBase();

public:
    tst_qqmljsscope()
        : QQmlDataTest(QT_QMLTEST_DATADIR),
          m_importer(
                  {
                          QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
                          dataDirectory(),
                  },
                  nullptr)
    {
    }

private:
    QQmlJSImporter m_importer;
};

void tst_qqmljsscope::initTestCase()
{
    QQmlDataTest::initTestCase();

    QDirIterator it(dataDirectory(), QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString url = it.next();
        if (!url.endsWith(u".qml"_qs)) // not interesting
            continue;
        const QFileInfo fi(url);
        QVERIFY(fi.exists());
        QFile f(fi.absoluteFilePath());
        QVERIFY(f.open(QIODevice::ReadOnly));
    }
}

void tst_qqmljsscope::orderedBindings()
{
    QQmlJSScope::ConstPtr root = run(u"orderedBindings.qml"_qs);
    QVERIFY(root);

    auto [pBindingsBegin, pBindingsEnd] = root->ownPropertyBindings(u"p"_qs);
    QVERIFY(std::distance(pBindingsBegin, pBindingsEnd) == 2);

    // check that the bindings are properly ordered
    QCOMPARE(pBindingsBegin->bindingType(), QQmlJSMetaPropertyBinding::Object);
    QCOMPARE(std::next(pBindingsBegin)->bindingType(), QQmlJSMetaPropertyBinding::Interceptor);

    auto [itemsBindingsBegin, itemsBindingsEnd] = root->ownPropertyBindings(u"items"_qs);
    QVERIFY(std::distance(itemsBindingsBegin, itemsBindingsEnd) == 2);

    QCOMPARE(itemsBindingsBegin->bindingType(), QQmlJSMetaPropertyBinding::Object);
    QCOMPARE(std::next(itemsBindingsBegin)->bindingType(), QQmlJSMetaPropertyBinding::Object);

    QCOMPARE(itemsBindingsBegin->objectType()->baseTypeName(), u"Item"_qs);
    QCOMPARE(std::next(itemsBindingsBegin)->objectType()->baseTypeName(), u"Text"_qs);
}

void tst_qqmljsscope::signalCreationDifferences()
{
    QQmlJSScope::ConstPtr root = run(u"signalCreationDifferences.qml"_qs);
    QVERIFY(root);

    QVERIFY(root->hasOwnProperty(u"myProperty"_qs));
    QVERIFY(root->hasOwnProperty(u"conflictingProperty"_qs));
    QCOMPARE(root->ownMethods(u"mySignal"_qs).size(), 1);

    const auto conflicting = root->ownMethods(u"conflictingPropertyChanged"_qs);
    QCOMPARE(conflicting.size(), 2);
    QCOMPARE(conflicting[0].methodType(), QQmlJSMetaMethod::Signal);
    QCOMPARE(conflicting[1].methodType(), QQmlJSMetaMethod::Signal);

    const QQmlJSMetaMethod *explicitMethod = nullptr;
    if (conflicting[0].isImplicitQmlPropertyChangeSignal())
        explicitMethod = &conflicting[1];
    else
        explicitMethod = &conflicting[0];
    QCOMPARE(explicitMethod->parameterNames(), QStringList({ u"a"_qs, u"c"_qs }));
}

void tst_qqmljsscope::allTypesAvailable()
{
        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
            dataDirectory(),
        };

        QQmlJSImporter importer { importPaths, /* resource file mapper */ nullptr };
        const auto types = importer.importModule(u"QtQml"_qs);
        QVERIFY(types.contains(u"$internal$.QObject"_qs));
        QVERIFY(types.contains(u"QtObject"_qs));
        QCOMPARE(types[u"$internal$.QObject"_qs].scope, types[u"QtObject"_qs].scope);
}

void tst_qqmljsscope::shadowing()
{
    QQmlJSScope::ConstPtr root = run(u"shadowing.qml"_qs);
    QVERIFY(root);

    QVERIFY(root->baseType());

    // Check whether properties are properly shadowed
    const auto properties = root->properties();
    QVERIFY(properties.contains(u"property_not_shadowed"_qs));
    QVERIFY(properties.contains(u"property_shadowed"_qs));

    QCOMPARE(properties[u"property_not_shadowed"_qs].typeName(), u"QString"_qs);
    QCOMPARE(properties[u"property_shadowed"_qs].typeName(), u"int"_qs);

    // Check whether methods are properly shadowed
    const auto methods = root->methods();
    QCOMPARE(methods.count(u"method_not_shadowed"_qs), 1);
    QCOMPARE(methods.count(u"method_shadowed"_qs), 1);

    QCOMPARE(methods[u"method_not_shadowed"_qs].parameterNames().size(), 1);
    QCOMPARE(methods[u"method_shadowed"_qs].parameterNames().size(), 0);
}

#ifdef LABS_QML_MODELS_PRESENT
void tst_qqmljsscope::componentWrappedObjects()
{
    QQmlJSScope::ConstPtr root = run(u"componentWrappedObjects.qml"_qs);
    QVERIFY(root);

    auto children = root->childScopes();
    QCOMPARE(children.size(), 4);

    const auto isGoodType = [](const QQmlJSScope::ConstPtr &type, const QString &propertyName,
                               bool isWrapped) {
        return type->hasOwnProperty(propertyName)
                && type->isWrappedInImplicitComponent() == isWrapped;
    };

    QVERIFY(isGoodType(children[0], u"nonWrapped1"_qs, false));
    QVERIFY(isGoodType(children[1], u"nonWrapped2"_qs, false));
    QVERIFY(isGoodType(children[2], u"nonWrapped3"_qs, false));
    QVERIFY(isGoodType(children[3], u"wrapped"_qs, true));
}
#endif

void tst_qqmljsscope::unknownCppBase()
{
    QQmlJSScope::ConstPtr root = run(u"unknownCppBaseAssigningToVar.qml"_qs);
    QVERIFY(root);
    // we should not crash here, then it is a success
}

QTEST_MAIN(tst_qqmljsscope)
#include "tst_qqmljsscope.moc"
