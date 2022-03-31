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
        QmlIR::Document document(false);
        return run(url, &document);
    }

    QQmlJSScope::ConstPtr run(QString url, QmlIR::Document *document)
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
        logger.setSilent(true);
        QQmlJSScope::Ptr target = QQmlJSScope::create();
        QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
        QQmlJSTypeResolver typeResolver { &m_importer };
        typeResolver.init(&visitor, document->program);
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
    void relativeScriptIndices();

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

void tst_qqmljsscope::labsQmlModelsSanity()
{
    QQmlJSScope::ConstPtr root = run(u"labsQmlModelsSanity.qml"_qs);
    QVERIFY(root);
    auto children = root->childScopes();
    QCOMPARE(children.size(), 1);

    // DelegateChooser: it inherits QQmlAbstractDelegateComponent (from
    // QmlModels) which inherits QQmlComponent. While
    // QQmlAbstractDelegateComponent has no properties, QQmlComponent does. If
    // the QmlModels dependency is lost, we don't "see" that DelegateChooser
    // inherits QQmlComponent - and so has no properties from it, hence, we can
    // test exactly that:
    QVERIFY(children[0]->hasProperty(u"progress"_qs));
    QVERIFY(children[0]->hasProperty(u"status"_qs));
    QVERIFY(children[0]->hasProperty(u"url"_qs));
}
#endif

void tst_qqmljsscope::unknownCppBase()
{
    QQmlJSScope::ConstPtr root = run(u"unknownCppBaseAssigningToVar.qml"_qs);
    QVERIFY(root);
    // we should not crash here, then it is a success
}

void tst_qqmljsscope::groupedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"groupProperties.qml"_qs);
    QVERIFY(root);

    QVERIFY(root->hasProperty(u"anchors"_qs));
    const auto anchorBindings = root->propertyBindings(u"anchors"_qs);
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
    QCOMPARE(value(bindingsOfType, u"left"_qs).bindingType(), QQmlJSMetaPropertyBinding::Script);
    QCOMPARE(value(bindingsOfType, u"leftMargin"_qs).bindingType(),
             QQmlJSMetaPropertyBinding::NumberLiteral);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinGroup(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"top"_qs).bindingType(), QQmlJSMetaPropertyBinding::Script);
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

void tst_qqmljsscope::groupedPropertiesConsistency()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"groupPropertiesConsistency.qml"_qs));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QFont font = qvariant_cast<QFont>(root->property("font"));
        QCOMPARE(font.pixelSize(), 22);
    }

    {
        QQmlJSScope::ConstPtr root = run(u"groupPropertiesConsistency.qml"_qs);
        QVERIFY(root);

        const auto fontBindings = root->propertyBindings(u"font"_qs);
        QCOMPARE(fontBindings.size(), 2);

        // The binding order in QQmlJSScope case is "reversed": first come
        // bindings on the leaf type, followed by the bindings on the base type
        QCOMPARE(fontBindings[0].bindingType(), QQmlJSMetaPropertyBinding::GroupProperty);
        QCOMPARE(fontBindings[1].bindingType(), QQmlJSMetaPropertyBinding::Script);
    }
}

void tst_qqmljsscope::groupedPropertySyntax()
{
    QQmlJSScope::ConstPtr root = run(u"groupPropertySyntax.qml"_qs);
    QVERIFY(root);

    const auto fontBindings = root->propertyBindings(u"font"_qs);
    QCOMPARE(fontBindings.size(), 1);

    // The binding order in QQmlJSScope case is "reversed": first come
    // bindings on the leaf type, followed by the bindings on the base type
    QCOMPARE(fontBindings[0].bindingType(), QQmlJSMetaPropertyBinding::GroupProperty);
    auto fontScope = fontBindings[0].groupType();
    QVERIFY(fontScope);
    auto subbindings = fontScope->ownPropertyBindings();
    QCOMPARE(subbindings.size(), 2);

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QCOMPARE(value(subbindings, u"pixelSize"_qs).bindingType(),
             QQmlJSMetaPropertyBinding::NumberLiteral);
    QCOMPARE(value(subbindings, u"bold"_qs).bindingType(), QQmlJSMetaPropertyBinding::BoolLiteral);
}

void tst_qqmljsscope::attachedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"attachedProperties.qml"_qs);
    QVERIFY(root);

    const auto keysBindings = root->propertyBindings(u"Keys"_qs);
    QVERIFY(!keysBindings.isEmpty());
    QCOMPARE(keysBindings.size(), 2); // from type itself and from the base type

    const auto getBindingsWithinAttached =
            [&](QMultiHash<QString, QQmlJSMetaPropertyBinding> *bindings, qsizetype index) -> void {
        const auto &binding = keysBindings[index];
        QCOMPARE(binding.bindingType(), QQmlJSMetaPropertyBinding::AttachedProperty);
        auto keysScope = binding.attachingType();
        QVERIFY(keysScope);
        *bindings = keysScope->ownPropertyBindings();
    };

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfType;
    getBindingsWithinAttached(&bindingsOfType, 0);
    QCOMPARE(bindingsOfType.size(), 2);
    QCOMPARE(value(bindingsOfType, u"enabled"_qs).bindingType(),
             QQmlJSMetaPropertyBinding::BoolLiteral);
    QCOMPARE(value(bindingsOfType, u"forwardTo"_qs).bindingType(),
             QQmlJSMetaPropertyBinding::Script);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinAttached(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"priority"_qs).bindingType(),
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

void tst_qqmljsscope::relativeScriptIndices()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"functionAndBindingIndices.qml"_qs));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
    }

    QmlIR::Document document(false); // we need QmlIR information here
    QQmlJSScope::ConstPtr root = run(u"functionAndBindingIndices.qml"_qs, &document);
    QVERIFY(root);

    using IndexedString = std::pair<QString, int>;
    // compare {property, function}Name and relative function table index
    // between QQmlJSScope and QmlIR:
    QList<IndexedString> orderedJSScopeExpressions;
    QList<IndexedString> orderedQmlIrExpressions;

    QList<QQmlJSScope::ConstPtr> queue;
    queue.push_back(root);
    while (!queue.isEmpty()) {
        auto current = queue.front();
        queue.pop_front();

        const auto methods = current->ownMethods();
        for (const auto &method : methods) {
            if (method.methodType() == QQmlJSMetaMethod::Signal)
                continue;
            QString name = method.methodName();
            int index = static_cast<int>(method.jsFunctionIndex());
            QVERIFY2(index >= 0,
                     qPrintable(QStringLiteral("Method %1 from %2 has no index")
                                        .arg(name, getScopeName(current))));
            orderedJSScopeExpressions.emplaceBack(name, index);
        }

        const auto bindings = current->ownPropertyBindings();
        for (const auto &binding : bindings) {
            if (binding.bindingType() != QQmlJSMetaPropertyBinding::Script)
                continue;
            QString name = binding.propertyName();
            int index = static_cast<int>(binding.scriptIndex());
            QVERIFY2(index >= 0,
                     qPrintable(QStringLiteral("Binding on property %1 from %2 has no index")
                                        .arg(name, getScopeName(current))));
            orderedJSScopeExpressions.emplaceBack(name, index);
        }

        const auto children = current->childScopes();
        for (const auto &c : children)
            queue.push_back(c);
    }

    for (const QmlIR::Object *irObject : qAsConst(document.objects)) {
        const QString objectName = document.stringAt(irObject->inheritedTypeNameIndex);
        for (auto it = irObject->functionsBegin(); it != irObject->functionsEnd(); ++it) {
            QString name = document.stringAt(it->nameIndex);
            QVERIFY2(it->index >= 0,
                     qPrintable(QStringLiteral("(qmlir) Method %1 from %2 has no index")
                                        .arg(name, objectName)));
            orderedQmlIrExpressions.emplaceBack(name, it->index);
        }
        for (auto it = irObject->bindingsBegin(); it != irObject->bindingsEnd(); ++it) {
            if (it->type != QmlIR::Binding::Type_Script)
                continue;
            QString name = document.stringAt(it->propertyNameIndex);
            int index = it->value.compiledScriptIndex;
            QVERIFY2(
                    index >= 0,
                    qPrintable(QStringLiteral("(qmlir) Binding on property %1 from %2 has no index")
                                       .arg(name, objectName)));
            orderedQmlIrExpressions.emplaceBack(name, index);
        }
    }

    auto less = [](const IndexedString &x, const IndexedString &y) { return x.first < y.first; };
    std::sort(orderedJSScopeExpressions.begin(), orderedJSScopeExpressions.end(), less);
    std::sort(orderedQmlIrExpressions.begin(), orderedQmlIrExpressions.end(), less);

    QCOMPARE(orderedJSScopeExpressions, orderedQmlIrExpressions);
}

QTEST_MAIN(tst_qqmljsscope)
#include "tst_qqmljsscope.moc"
