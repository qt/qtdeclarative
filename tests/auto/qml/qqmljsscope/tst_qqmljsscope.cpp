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
#include <QtCore/qscopedpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtGui/qfont.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>

using namespace Qt::StringLiterals;

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

    QQmlJSScope::ConstPtr run(QString url, bool expectErrorsOrWarnings = false)
    {
        QmlIR::Document document(false);
        return run(url, &document, expectErrorsOrWarnings);
    }

    QQmlJSScope::ConstPtr run(QString url, QmlIR::Document *document,
                              bool expectErrorsOrWarnings = false)
    {
        url = testFile(url);
        const QString sourceCode = loadUrl(url);
        if (sourceCode.isEmpty())
            return QQmlJSScope::ConstPtr();

        // NB: JS unit generated here is ignored, so use noop function
        QQmlJSSaveFunction noop([](auto &&...) { return true; });
        QQmlJSCompileError error;
        [&]() {
            QVERIFY2(qCompileQmlFile(*document, url, noop, nullptr, &error),
                     qPrintable(error.message));
        }();
        if (!error.message.isEmpty())
            return QQmlJSScope::ConstPtr();


        QQmlJSLogger logger;
        logger.setFileName(url);
        logger.setCode(sourceCode);
        logger.setSilent(expectErrorsOrWarnings);
        QQmlJSScope::Ptr target = QQmlJSScope::create();
        QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
        QQmlJSTypeResolver typeResolver { &m_importer };
        typeResolver.init(&visitor, document->program);
        if (!expectErrorsOrWarnings) {
            [&]() {
                QVERIFY2(!logger.hasWarnings(), "Expected no warnings in this test");
                QVERIFY2(!logger.hasErrors(), "Expected no errors in this test");
            }();
        }
        if (QTest::currentTestFailed())
            return QQmlJSScope::ConstPtr();
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
    void labsQmlModelsSanity();
#endif
    void unknownCppBase();
    void groupedProperties();
    void descriptiveNameOfNull();
    void groupedPropertiesConsistency();
    void groupedPropertySyntax();
    void attachedProperties();
    void scriptIndices();
    void extensions();
    void emptyBlockBinding();
    void qualifiedName();

public:
    tst_qqmljsscope()
        : QQmlDataTest(QT_QMLTEST_DATADIR),
          m_importer(
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
        if (!url.endsWith(u".qml"_s)) // not interesting
            continue;
        const QFileInfo fi(url);
        QVERIFY(fi.exists());
        QFile f(fi.absoluteFilePath());
        QVERIFY(f.open(QIODevice::ReadOnly));
    }

    // test that we can import the module shipped with this test: if we have no
    // errors / warnings, it is a success
    QVERIFY(run(u"importOwnModule.qml"_s));
}

void tst_qqmljsscope::orderedBindings()
{
    QQmlJSScope::ConstPtr root = run(u"orderedBindings.qml"_s);
    QVERIFY(root);

    auto [pBindingsBegin, pBindingsEnd] = root->ownPropertyBindings(u"p"_s);
    QCOMPARE(std::distance(pBindingsBegin, pBindingsEnd), 2);

    // check that the bindings are properly ordered
    QCOMPARE(pBindingsBegin->bindingType(), QQmlJSMetaPropertyBinding::Object);
    QCOMPARE(std::next(pBindingsBegin)->bindingType(), QQmlJSMetaPropertyBinding::Interceptor);

    auto [itemsBindingsBegin, itemsBindingsEnd] = root->ownPropertyBindings(u"items"_s);
    QCOMPARE(std::distance(itemsBindingsBegin, itemsBindingsEnd), 2);

    QCOMPARE(itemsBindingsBegin->bindingType(), QQmlJSMetaPropertyBinding::Object);
    QCOMPARE(std::next(itemsBindingsBegin)->bindingType(), QQmlJSMetaPropertyBinding::Object);

    QCOMPARE(itemsBindingsBegin->objectType()->baseTypeName(), u"Item"_s);
    QCOMPARE(std::next(itemsBindingsBegin)->objectType()->baseTypeName(), u"Text"_s);
}

void tst_qqmljsscope::signalCreationDifferences()
{
    QQmlJSScope::ConstPtr root = run(u"signalCreationDifferences.qml"_s);
    QVERIFY(root);

    QVERIFY(root->hasOwnProperty(u"myProperty"_s));
    QVERIFY(root->hasOwnProperty(u"conflictingProperty"_s));
    QCOMPARE(root->ownMethods(u"mySignal"_s).size(), 1);

    const auto conflicting = root->ownMethods(u"conflictingPropertyChanged"_s);
    QCOMPARE(conflicting.size(), 2);
    QCOMPARE(conflicting[0].methodType(), QQmlJSMetaMethod::Signal);
    QCOMPARE(conflicting[1].methodType(), QQmlJSMetaMethod::Signal);

    const QQmlJSMetaMethod *explicitMethod = nullptr;
    if (conflicting[0].isImplicitQmlPropertyChangeSignal())
        explicitMethod = &conflicting[1];
    else
        explicitMethod = &conflicting[0];
    QCOMPARE(explicitMethod->parameterNames(), QStringList({ u"a"_s, u"c"_s }));
}

void tst_qqmljsscope::allTypesAvailable()
{
        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
            dataDirectory(),
        };

        QQmlJSImporter importer { importPaths, /* resource file mapper */ nullptr };
        const auto types = importer.importModule(u"QtQml"_s);
        QVERIFY(types.contains(u"$internal$.QObject"_s));
        QVERIFY(types.contains(u"QtObject"_s));
        QCOMPARE(types[u"$internal$.QObject"_s].scope, types[u"QtObject"_s].scope);
}

void tst_qqmljsscope::shadowing()
{
    QQmlJSScope::ConstPtr root = run(u"shadowing.qml"_s);
    QVERIFY(root);

    QVERIFY(root->baseType());

    // Check whether properties are properly shadowed
    const auto properties = root->properties();
    QVERIFY(properties.contains(u"property_not_shadowed"_s));
    QVERIFY(properties.contains(u"property_shadowed"_s));

    QCOMPARE(properties[u"property_not_shadowed"_s].typeName(), u"QString"_s);
    QCOMPARE(properties[u"property_shadowed"_s].typeName(), u"int"_s);

    // Check whether methods are properly shadowed
    const auto methods = root->methods();
    QCOMPARE(methods.count(u"method_not_shadowed"_s), 1);
    QCOMPARE(methods.count(u"method_shadowed"_s), 1);

    QCOMPARE(methods[u"method_not_shadowed"_s].parameterNames().size(), 1);
    QCOMPARE(methods[u"method_shadowed"_s].parameterNames().size(), 0);
}

#ifdef LABS_QML_MODELS_PRESENT
void tst_qqmljsscope::componentWrappedObjects()
{
    QQmlJSScope::ConstPtr root = run(u"componentWrappedObjects.qml"_s);
    QVERIFY(root);

    auto children = root->childScopes();
    QCOMPARE(children.size(), 4);

    const auto isGoodType = [](const QQmlJSScope::ConstPtr &type, const QString &propertyName,
                               bool isWrapped) {
        return type->hasOwnProperty(propertyName)
                && type->isWrappedInImplicitComponent() == isWrapped;
    };

    QVERIFY(isGoodType(children[0], u"nonWrapped1"_s, false));
    QVERIFY(isGoodType(children[1], u"nonWrapped2"_s, false));
    QVERIFY(isGoodType(children[2], u"nonWrapped3"_s, false));
    QVERIFY(isGoodType(children[3], u"wrapped"_s, true));
}

void tst_qqmljsscope::labsQmlModelsSanity()
{
    QQmlJSScope::ConstPtr root = run(u"labsQmlModelsSanity.qml"_s);
    QVERIFY(root);
    auto children = root->childScopes();
    QCOMPARE(children.size(), 1);

    // DelegateChooser: it inherits QQmlAbstractDelegateComponent (from
    // QmlModels) which inherits QQmlComponent. While
    // QQmlAbstractDelegateComponent has no properties, QQmlComponent does. If
    // the QmlModels dependency is lost, we don't "see" that DelegateChooser
    // inherits QQmlComponent - and so has no properties from it, hence, we can
    // test exactly that:
    QVERIFY(children[0]->hasProperty(u"progress"_s));
    QVERIFY(children[0]->hasProperty(u"status"_s));
    QVERIFY(children[0]->hasProperty(u"url"_s));
}
#endif

void tst_qqmljsscope::unknownCppBase()
{
    QQmlJSScope::ConstPtr root = run(u"unknownCppBaseAssigningToVar.qml"_s, true);
    QVERIFY(root);
    // we should not crash here, then it is a success
}

void tst_qqmljsscope::groupedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"groupProperties.qml"_s);
    QVERIFY(root);

    QVERIFY(root->hasProperty(u"anchors"_s));
    const auto anchorBindings = root->propertyBindings(u"anchors"_s);
    QVERIFY(!anchorBindings.isEmpty());
    QCOMPARE(anchorBindings.size(), 2); // from type itself and from the base type

    const auto getBindingsWithinGroup =
            [&](QMultiHash<QString, QQmlJSMetaPropertyBinding> *bindings, qsizetype index) -> void {
        const auto &binding = anchorBindings[index];
        QCOMPARE(binding.bindingType(), QQmlJSMetaPropertyBinding::GroupProperty);
        auto anchorScope = binding.groupType();
        QVERIFY(anchorScope);
        *bindings = anchorScope->ownPropertyBindings();
    };

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfType;
    getBindingsWithinGroup(&bindingsOfType, 0);
    QCOMPARE(bindingsOfType.size(), 2);
    QCOMPARE(value(bindingsOfType, u"left"_s).bindingType(), QQmlJSMetaPropertyBinding::Script);
    QCOMPARE(value(bindingsOfType, u"leftMargin"_s).bindingType(),
             QQmlJSMetaPropertyBinding::NumberLiteral);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinGroup(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"top"_s).bindingType(), QQmlJSMetaPropertyBinding::Script);
}

void tst_qqmljsscope::descriptiveNameOfNull()
{
    QQmlJSRegisterContent nullContent;
    QCOMPARE(nullContent.descriptiveName(), u"(invalid type)"_s);

    QQmlJSScope::Ptr stored = QQmlJSScope::create();
    stored->setInternalName(u"bar"_s);
    QQmlJSMetaProperty property;
    property.setPropertyName(u"foo"_s);
    property.setTypeName(u"baz"_s);
    QQmlJSRegisterContent unscoped = QQmlJSRegisterContent::create(
                stored, property, QQmlJSRegisterContent::ScopeProperty, QQmlJSScope::ConstPtr());
    QCOMPARE(unscoped.descriptiveName(), u"bar of (invalid type)::foo with type baz"_s);
}

void tst_qqmljsscope::groupedPropertiesConsistency()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"groupPropertiesConsistency.qml"_s));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QFont font = qvariant_cast<QFont>(root->property("font"));
        QCOMPARE(font.pixelSize(), 22);
    }

    {
        QQmlJSScope::ConstPtr root = run(u"groupPropertiesConsistency.qml"_s);
        QVERIFY(root);

        const auto fontBindings = root->propertyBindings(u"font"_s);
        QCOMPARE(fontBindings.size(), 2);

        // The binding order in QQmlJSScope case is "reversed": first come
        // bindings on the leaf type, followed by the bindings on the base type
        QCOMPARE(fontBindings[0].bindingType(), QQmlJSMetaPropertyBinding::GroupProperty);
        QCOMPARE(fontBindings[1].bindingType(), QQmlJSMetaPropertyBinding::Script);
    }
}

void tst_qqmljsscope::groupedPropertySyntax()
{
    QQmlJSScope::ConstPtr root = run(u"groupPropertySyntax.qml"_s);
    QVERIFY(root);

    const auto fontBindings = root->propertyBindings(u"font"_s);
    QCOMPARE(fontBindings.size(), 1);

    // The binding order in QQmlJSScope case is "reversed": first come
    // bindings on the leaf type, followed by the bindings on the base type
    QCOMPARE(fontBindings[0].bindingType(), QQmlJSMetaPropertyBinding::GroupProperty);
    auto fontScope = fontBindings[0].groupType();
    QVERIFY(fontScope);
    QCOMPARE(fontScope->accessSemantics(), QQmlJSScope::AccessSemantics::Value);
    auto subbindings = fontScope->ownPropertyBindings();
    QCOMPARE(subbindings.size(), 2);

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QCOMPARE(value(subbindings, u"pixelSize"_s).bindingType(),
             QQmlJSMetaPropertyBinding::NumberLiteral);
    QCOMPARE(value(subbindings, u"bold"_s).bindingType(), QQmlJSMetaPropertyBinding::BoolLiteral);
}

void tst_qqmljsscope::attachedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"attachedProperties.qml"_s);
    QVERIFY(root);

    const auto keysBindings = root->propertyBindings(u"Keys"_s);
    QVERIFY(!keysBindings.isEmpty());
    QCOMPARE(keysBindings.size(), 2); // from type itself and from the base type

    const auto getBindingsWithinAttached =
            [&](QMultiHash<QString, QQmlJSMetaPropertyBinding> *bindings, qsizetype index) -> void {
        const auto &binding = keysBindings[index];
        QCOMPARE(binding.bindingType(), QQmlJSMetaPropertyBinding::AttachedProperty);
        auto keysScope = binding.attachingType();
        QVERIFY(keysScope);
        QCOMPARE(keysScope->accessSemantics(), QQmlJSScope::AccessSemantics::Reference);
        *bindings = keysScope->ownPropertyBindings();
    };

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfType;
    getBindingsWithinAttached(&bindingsOfType, 0);
    QCOMPARE(bindingsOfType.size(), 2);
    QCOMPARE(value(bindingsOfType, u"enabled"_s).bindingType(),
             QQmlJSMetaPropertyBinding::BoolLiteral);
    QCOMPARE(value(bindingsOfType, u"forwardTo"_s).bindingType(),
             QQmlJSMetaPropertyBinding::Script);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinAttached(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"priority"_s).bindingType(),
             QQmlJSMetaPropertyBinding::Script);
}

inline QString getScopeName(const QQmlJSScope::ConstPtr &scope)
{
    Q_ASSERT(scope);
    QQmlJSScope::ScopeType type = scope->scopeType();
    if (type == QQmlJSScope::GroupedPropertyScope || type == QQmlJSScope::AttachedPropertyScope)
        return scope->internalName();
    return scope->baseTypeName();
}

void tst_qqmljsscope::scriptIndices()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"functionAndBindingIndices.qml"_s));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
    }

    QmlIR::Document document(false); // we need QmlIR information here
    QQmlJSScope::ConstPtr root = run(u"functionAndBindingIndices.qml"_s, &document);
    QVERIFY(root);
    QVERIFY(document.javaScriptCompilationUnit.unitData());

    using IndexedString = std::pair<QString, int>;
    // compare QQmlJSScope and QmlIR:

    // {property, function}Name and relative (per-object) function table index
    QList<IndexedString> orderedJSScopeExpressionsRelative;
    QList<IndexedString> orderedQmlIrExpressionsRelative;
    // {property, function}Name and absolute (per-document) function table index
    QList<IndexedString> orderedJSScopeExpressionsAbsolute;
    QList<IndexedString> orderedQmlIrExpressionsAbsolute;

    const auto populateQQmlJSScopeArrays =
            [&](const QQmlJSScope::ConstPtr &scope, const QString &name,
                QQmlJSMetaMethod::RelativeFunctionIndex relativeIndex) {
                orderedJSScopeExpressionsRelative.emplaceBack(name,
                                                              static_cast<int>(relativeIndex));
                auto absoluteIndex = scope->ownRuntimeFunctionIndex(relativeIndex);
                orderedJSScopeExpressionsAbsolute.emplaceBack(name,
                                                              static_cast<int>(absoluteIndex));
            };

    const auto populateQmlIRArrays = [&](const QmlIR::Object *irObject, const QString &name,
                                         int relative) {
        orderedQmlIrExpressionsRelative.emplaceBack(name, relative);
        auto absolute = irObject->runtimeFunctionIndices.at(relative);
        orderedQmlIrExpressionsAbsolute.emplaceBack(name, absolute);
    };

    const auto suitableScope = [](const QQmlJSScope::ConstPtr &scope) {
        const auto type = scope->scopeType();
        return type == QQmlJSScope::QMLScope || type == QQmlJSScope::GroupedPropertyScope
                || type == QQmlJSScope::AttachedPropertyScope;
    };

    QList<QQmlJSScope::ConstPtr> queue;
    queue.push_back(root);
    while (!queue.isEmpty()) {
        auto current = queue.front();
        queue.pop_front();

        if (suitableScope(current)) {

            const auto methods = current->ownMethods();
            for (const auto &method : methods) {
                if (method.methodType() == QQmlJSMetaMethod::Signal)
                    continue;
                QString name = method.methodName();
                auto relativeIndex = method.jsFunctionIndex();
                QVERIFY2(static_cast<int>(relativeIndex) >= 0,
                         qPrintable(QStringLiteral("Method %1 from %2 has no index")
                                            .arg(name, getScopeName(current))));

                populateQQmlJSScopeArrays(current, name, relativeIndex);
            }

            const auto bindings = current->ownPropertyBindings();
            for (const auto &binding : bindings) {
                if (binding.bindingType() != QQmlJSMetaPropertyBinding::Script)
                    continue;
                QString name = binding.propertyName();
                auto relativeIndex = binding.scriptIndex();
                QVERIFY2(static_cast<int>(relativeIndex) >= 0,
                         qPrintable(QStringLiteral("Binding on property %1 from %2 has no index")
                                            .arg(name, getScopeName(current))));

                populateQQmlJSScopeArrays(current, name, relativeIndex);
            }
        }

        const auto children = current->childScopes();
        for (const auto &c : children)
            queue.push_back(c);
    }

    for (const QmlIR::Object *irObject : qAsConst(document.objects)) {
        const QString objectName = document.stringAt(irObject->inheritedTypeNameIndex);
        for (auto it = irObject->functionsBegin(); it != irObject->functionsEnd(); ++it) {
            QString name = document.stringAt(it->nameIndex);
            populateQmlIRArrays(irObject, name, it->index);
        }
        for (auto it = irObject->bindingsBegin(); it != irObject->bindingsEnd(); ++it) {
            if (it->type() != QmlIR::Binding::Type_Script)
                continue;
            QString name = document.stringAt(it->propertyNameIndex);
            int index = it->value.compiledScriptIndex;
            populateQmlIRArrays(irObject, name, index);
        }
    }

    auto less = [](const IndexedString &x, const IndexedString &y) { return x.first < y.first; };
    std::sort(orderedJSScopeExpressionsRelative.begin(), orderedJSScopeExpressionsRelative.end(),
              less);
    std::sort(orderedQmlIrExpressionsRelative.begin(), orderedQmlIrExpressionsRelative.end(), less);

    std::sort(orderedJSScopeExpressionsAbsolute.begin(), orderedJSScopeExpressionsAbsolute.end(),
              less);
    std::sort(orderedQmlIrExpressionsAbsolute.begin(), orderedQmlIrExpressionsAbsolute.end(), less);

    QCOMPARE(orderedJSScopeExpressionsRelative, orderedQmlIrExpressionsRelative);
    QCOMPARE(orderedJSScopeExpressionsAbsolute, orderedQmlIrExpressionsAbsolute);
}

void tst_qqmljsscope::extensions()
{
    QQmlJSScope::ConstPtr root = run(u"extensions.qml"_s);
    QVERIFY(root);
    QVERIFY(root->isFullyResolved());

    const auto childScopes = root->childScopes();
    QCOMPARE(childScopes.size(), 5);

    QCOMPARE(childScopes[0]->baseTypeName(), u"Extended"_s);
    QCOMPARE(childScopes[1]->baseTypeName(), u"ExtendedIndirect"_s);
    QCOMPARE(childScopes[2]->baseTypeName(), u"ExtendedTwice"_s);
    QCOMPARE(childScopes[3]->baseTypeName(), u"NamespaceExtended"_s);
    QCOMPARE(childScopes[4]->baseTypeName(), u"NonNamespaceExtended"_s);
    QVERIFY(childScopes[0]->isFullyResolved());
    QVERIFY(childScopes[1]->isFullyResolved());
    QVERIFY(childScopes[2]->isFullyResolved());
    QVERIFY(childScopes[3]->isFullyResolved());
    QVERIFY(childScopes[4]->isFullyResolved());

    QCOMPARE(childScopes[0]->property(u"count"_s).typeName(), u"int"_s);
    QCOMPARE(childScopes[1]->property(u"count"_s).typeName(), u"double"_s);
    QCOMPARE(childScopes[2]->property(u"count"_s).typeName(), u"int"_s);
    QCOMPARE(childScopes[2]->property(u"str"_s).typeName(), u"QString"_s);

    QVERIFY(!childScopes[3]->hasProperty(u"count"_s));
    QVERIFY(!childScopes[3]->property(u"count"_s).isValid());
    QVERIFY(!childScopes[3]->hasProperty(u"p"_s));
    QVERIFY(!childScopes[3]->property(u"p"_s).isValid());
    QVERIFY(!childScopes[3]->hasMethod(u"someMethod"_s));
    QVERIFY(childScopes[3]->hasEnumeration(u"ExtensionEnum"_s));
    QVERIFY(childScopes[3]->hasEnumerationKey(u"Value1"_s));
    QVERIFY(childScopes[3]->enumeration(u"ExtensionEnum"_s).isValid());
    QCOMPARE(childScopes[3]->defaultPropertyName(), u"objectName"_s);
    QCOMPARE(childScopes[3]->parentPropertyName(), u"p"_s);
    QVERIFY(!childScopes[3]->hasInterface(u"QQmlParserStatus"_s));
    QCOMPARE(childScopes[3]->attachedTypeName(), QString());
    QVERIFY(!childScopes[3]->attachedType());

    QVERIFY(!childScopes[3]->extensionIsNamespace());
    QVERIFY(childScopes[3]->baseType()->extensionIsNamespace());

    QVERIFY(childScopes[4]->hasProperty(u"count"_s));
    QVERIFY(childScopes[4]->property(u"count"_s).isValid());
    QVERIFY(childScopes[4]->hasProperty(u"p"_s));
    QVERIFY(childScopes[4]->property(u"p"_s).isValid());
    QVERIFY(childScopes[4]->hasMethod(u"someMethod"_s));
    QVERIFY(childScopes[4]->hasEnumeration(u"ExtensionEnum"_s));
    QVERIFY(childScopes[4]->hasEnumerationKey(u"Value1"_s));
    QVERIFY(childScopes[4]->enumeration(u"ExtensionEnum"_s).isValid());
    QCOMPARE(childScopes[4]->defaultPropertyName(), u"objectName"_s);
    QCOMPARE(childScopes[4]->parentPropertyName(), u"p"_s);
    QVERIFY(!childScopes[4]->hasInterface(u"QQmlParserStatus"_s));
    QCOMPARE(childScopes[4]->attachedTypeName(), QString());
    QVERIFY(!childScopes[4]->attachedType());

    auto [owner, ownerKind] = QQmlJSScope::ownerOfProperty(childScopes[4], u"count"_s);
    QVERIFY(owner);
    QCOMPARE(ownerKind, QQmlJSScope::ExtensionType);
    QCOMPARE(owner, childScopes[4]->baseType()->extensionType().scope);
}

void tst_qqmljsscope::emptyBlockBinding()
{
    QQmlJSScope::ConstPtr root = run(u"emptyBlockBinding.qml"_s);
    QVERIFY(root);
    QVERIFY(root->hasOwnPropertyBindings(u"x"_s));
    QVERIFY(root->hasOwnPropertyBindings(u"y"_s));
}

void tst_qqmljsscope::qualifiedName()
{
    QQmlJSScope::ConstPtr root = run(u"qualifiedName.qml"_s);

    auto qualifiedNameOf = [](const QQmlJSScope::ConstPtr &ptr) {
        return ptr->baseType()->qualifiedName();
    };

    QQmlJSScope::ConstPtr item = root;

    // normal case
    QCOMPARE(qualifiedNameOf(item), "QtQuick/Item 2.0-6.3");

    QCOMPARE(item->childScopes().size(), 4);
    QQmlJSScope::ConstPtr textInItem = item->childScopes()[0];
    QQmlJSScope::ConstPtr nonQualifiedComponentInItem = item->childScopes()[1];
    QQmlJSScope::ConstPtr qualifiedComponentInItem = item->childScopes()[2];
    QQmlJSScope::ConstPtr componentInItem = item->childScopes()[3];

    // qualified case
    QCOMPARE(qualifiedNameOf(nonQualifiedComponentInItem), "QtQuick/TextEdit 2.0-6.3");

    // qualified case
    QCOMPARE(qualifiedNameOf(qualifiedComponentInItem), "QtQuick/TextEdit 2.0-6.3");

    // normal case
    QCOMPARE(qualifiedNameOf(textInItem), "QtQuick/Text 2.0-6.3");
    // qualified import of builtin variable
    QCOMPARE(qualifiedNameOf(componentInItem), "QML/Component 1.0");
    QCOMPARE(componentInItem->baseType()->moduleName(), "QML");

    QCOMPARE(componentInItem->childScopes().size(), 1);

    QQmlJSScope::ConstPtr itemInComponent = componentInItem->childScopes()[0];

    QCOMPARE(qualifiedNameOf(itemInComponent), "QtQuick/Item 2.0-6.3");

    QCOMPARE(itemInComponent->childScopes().size(), 5);
    QQmlJSScope::ConstPtr qualifiedImportTextInItemInComponent = itemInComponent->childScopes()[0];
    QQmlJSScope::ConstPtr textInItemInComponent = itemInComponent->childScopes()[1];
    QQmlJSScope::ConstPtr qualifiedImportTextInputInItemInComponent =
            itemInComponent->childScopes()[2];
    QQmlJSScope::ConstPtr indirectImportTimerInItemInComponent = itemInComponent->childScopes()[3];
    QQmlJSScope::ConstPtr qualifiedImportTimerInItemInComponent = itemInComponent->childScopes()[4];

    QCOMPARE(qualifiedNameOf(qualifiedImportTextInItemInComponent), "QtQuick/Text 2.0-6.3");
    QCOMPARE(qualifiedNameOf(textInItemInComponent), "QtQuick/Text 2.0-6.3");
    QCOMPARE(textInItemInComponent->baseType()->moduleName(), "QtQuick");
    QCOMPARE(qualifiedImportTextInItemInComponent->baseType()->moduleName(), "QtQuick");

    QCOMPARE(qualifiedNameOf(qualifiedImportTextInputInItemInComponent),
             "QtQuick/TextInput 2.0-6.3");

    QCOMPARE(qualifiedNameOf(indirectImportTimerInItemInComponent), "QtQml/Timer 2.0-6.0");
    QCOMPARE(qualifiedNameOf(qualifiedImportTimerInItemInComponent), "QtQml/Timer 2.0-6.0");
    QCOMPARE(indirectImportTimerInItemInComponent->baseType()->moduleName(), "QtQml");
    QCOMPARE(qualifiedImportTimerInItemInComponent->baseType()->moduleName(), "QtQml");
}

QTEST_MAIN(tst_qqmljsscope)
#include "tst_qqmljsscope.moc"
