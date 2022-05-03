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
        QQmlJSScope::Ptr target = QQmlJSScope::create();
        QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
        QQmlJSTypeResolver typeResolver { &m_importer };
        typeResolver.init(&visitor, document.program);
        return visitor.result();
    }

private Q_SLOTS:
    void initTestCase() override;

    void orderedBindings();
    void signalCreationDifferences();
    void descriptiveNameOfNull();
    void extensions();

public:
    tst_qqmljsscope()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
        , m_importer(
                {
                        QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
                        dataDirectory(),
                        // Note: to be able to import the QQmlJSScopeTests
                        // correctly, we need an additional import path. Use
                        // this application's binary directory as done by
                        // QQmlImportDatabase
                        QCoreApplication::applicationDirPath(),
                },
                nullptr)
    {}

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

void tst_qqmljsscope::descriptiveNameOfNull()
{
    QQmlJSRegisterContent nullContent;
    QCOMPARE(nullContent.descriptiveName(), u"(invalid type)"_qs);

    QQmlJSScope::Ptr stored = QQmlJSScope::create();
    stored->setInternalName(u"bar"_qs);
    QQmlJSMetaProperty property;
    property.setPropertyName(u"foo"_qs);
    property.setTypeName(u"baz"_qs);
    QQmlJSRegisterContent unscoped = QQmlJSRegisterContent::create(
                stored, property, QQmlJSRegisterContent::ScopeProperty, QQmlJSScope::ConstPtr());
    QCOMPARE(unscoped.descriptiveName(), u"bar of (invalid type)::foo with type baz"_qs);
}

void tst_qqmljsscope::extensions()
{
    QQmlJSScope::ConstPtr root = run(u"extensions.qml"_qs);
    QVERIFY(root);
    QVERIFY(root->isFullyResolved());

    const auto childScopes = root->childScopes();
    QCOMPARE(childScopes.size(), 3);

    QCOMPARE(childScopes[0]->baseTypeName(), u"Extended"_qs);
    QCOMPARE(childScopes[1]->baseTypeName(), u"ExtendedIndirect"_qs);
    QCOMPARE(childScopes[2]->baseTypeName(), u"ExtendedTwice"_qs);
    QVERIFY(childScopes[0]->isFullyResolved());
    QVERIFY(childScopes[1]->isFullyResolved());
    QVERIFY(childScopes[2]->isFullyResolved());

    QCOMPARE(childScopes[0]->property(u"count"_qs).typeName(), u"int"_qs);
    QCOMPARE(childScopes[1]->property(u"count"_qs).typeName(), u"double"_qs);
    QCOMPARE(childScopes[2]->property(u"count"_qs).typeName(), u"int"_qs);
    QCOMPARE(childScopes[2]->property(u"str"_qs).typeName(), u"QString"_qs);
}

QTEST_MAIN(tst_qqmljsscope)
#include "tst_qqmljsscope.moc"
