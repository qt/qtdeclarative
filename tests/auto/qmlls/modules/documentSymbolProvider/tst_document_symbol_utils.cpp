// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlLS/private/documentsymbolutils_p.h>

#include "tst_document_symbol_utils.h"

using namespace QQmlJS::Dom;
using QLspSpecification::DocumentSymbol;
using SymbolsList = QList<DocumentSymbol>;

static bool compareDocumentSymbols(const DocumentSymbol &actual, const DocumentSymbol &expected)
{
    if (actual.name != expected.name) {
        return false;
    }
    // Compare children
    if (actual.children.has_value() != expected.children.has_value()) {
        return false;
    }
    if (!actual.children.has_value()) {
        return true;
    }
    return std::equal(actual.children->cbegin(), actual.children->cend(),
                      expected.children->cbegin(), expected.children->cend(),
                      compareDocumentSymbols);
}

static bool compareDocumentSymbolsLists(const QList<DocumentSymbol> &actual,
                                        const QList<DocumentSymbol> &expected)
{
    return std::equal(actual.cbegin(), actual.cend(), expected.cbegin(), expected.cend(),
                      compareDocumentSymbols);
}

/*
 * fakeQmlFileItem returns a DomItem representing a pseudo QmlFile like the following
 *
 * {
 *     property var
 *
 *     // bindings
 *     b :
 *     obj : {}
 *     objects: [{}, {}]
 *     //binding with ScriptExpression is not relevant to DocumentSymbolVisitor
 *
 *     // 2 default methodInfo objects
 *     (){}
 *     (){}
 *
 *     // 2 default QmlObject children
 *     {}
 *     {}
 *
 *     component inlCmp.withEnum {
 *         enum { , ,}
 *     }
 * }
 *
 * Ofc it's invalid QmlFile, however this is fine to make sure that DocumentSymbolVisitor
 * visits all relevant fields/entities and final QList<DocumentSymbol> has the same hierarchy
 * as this QmlFile item.
 */
QQmlJS::Dom::DomItem tst_document_symbol_utils::fakeQmlFileItem()
{
    auto qmlFile = std::make_shared<QmlFile>();
    auto rootQmlObject = QmlObject();

    // add explicit propertyDef to rootQmlObject
    auto propDef = PropertyDefinition();
    rootQmlObject.addPropertyDef(propDef, AddOption::KeepExisting);

    // add bindings to rootQmlObject
    auto binding = Binding("b");
    rootQmlObject.addBinding(binding, AddOption::KeepExisting);

    auto bindingWithObject = Binding("obj", QmlObject());
    rootQmlObject.addBinding(bindingWithObject, AddOption::KeepExisting);

    auto bindingWithArrayOfObjects = Binding("objects", QList<QmlObject>(2));
    rootQmlObject.addBinding(bindingWithArrayOfObjects, AddOption::KeepExisting);

    // add Methods to rootQmlObject
    rootQmlObject.setMethods(
            { std::pair<QString, MethodInfo>(), std::pair<QString, MethodInfo>() });

    // add children to rootQmlObject
    rootQmlObject.setChildren(QList<QmlObject>(2));

    // add root QmlObject to root component
    auto rootCmp = QmlComponent();
    rootCmp.addObject(rootQmlObject);
    qmlFile->addComponent(rootCmp);

    // add inline component with Enum
    auto inlCmp = QmlComponent("inlCmp.withEnum");
    auto enumDecl = EnumDecl();
    enumDecl.setValues(QList<EnumItem>(2));
    inlCmp.addEnumeration(enumDecl);
    qmlFile->addComponent(inlCmp);

    // create env and return qmlFileItem
    DomItem envItem = DomEnvironment::create({});
    return envItem.copy(qmlFile);
}

SymbolsList tst_document_symbol_utils::expectedSymbolsOfFakeQmlFile()
{
    // Main QmlComponent

    DocumentSymbol qmlObjectSymbol = fakeSymbol(DomType::QmlObject);

    DocumentSymbol bindingSymbol = fakeSymbol(DomType::Binding);

    DocumentSymbol bindingWithObjectSymbol(bindingSymbol);
    bindingWithObjectSymbol.children = { qmlObjectSymbol };

    DocumentSymbol bindingWithArrayOfObjectsSymbol(bindingSymbol);
    bindingWithArrayOfObjectsSymbol.children = { qmlObjectSymbol, qmlObjectSymbol };

    // Root QmlObject
    DocumentSymbol rootQmlObjectSymbol(qmlObjectSymbol);
    rootQmlObjectSymbol.children = {
        fakeSymbol(DomType::PropertyDefinition), // property var
        std::move(bindingSymbol), // b :
        std::move(bindingWithObjectSymbol), // obj : {}
        std::move(bindingWithArrayOfObjectsSymbol), // objects: [{}, {}]
        fakeSymbol(DomType::MethodInfo), // (){}
        fakeSymbol(DomType::MethodInfo), // (){}
        qmlObjectSymbol, // {}
        qmlObjectSymbol // {}
    };

    DocumentSymbol mainQmlCompSymbol = fakeSymbol(DomType::QmlComponent);
    mainQmlCompSymbol.children = { std::move(rootQmlObjectSymbol) };

    // Inline Component

    DocumentSymbol enumDeclSymbol = fakeSymbol(DomType::EnumDecl);
    enumDeclSymbol.children = { fakeSymbol(DomType::EnumItem), fakeSymbol(DomType::EnumItem) };

    DocumentSymbol inlQmlCompSymbol = fakeSymbol(DomType::QmlComponent);
    inlQmlCompSymbol.children = { std::move(enumDeclSymbol) };

    // QmlFile symbol
    DocumentSymbol qmlFileSymbol = fakeSymbol(DomType::QmlFile);
    qmlFileSymbol.children = { std::move(mainQmlCompSymbol), std::move(inlQmlCompSymbol) };
    return { std::move(qmlFileSymbol) };
}

void tst_document_symbol_utils::assembleSymbolsForQmlFile()
{
    const auto qmlFileItem = fakeQmlFileItem();
    // This assembling function simply transforms Item to a FakeSymbol with children
    const auto af = [](const DomItem &item, SymbolsList &&children) -> SymbolsList {
        if (domTypeIsContainer(item.internalKind())
            || item.internalKind() == QQmlJS::Dom::DomType::Empty) {
            // skip these wrappers to simplify manual construction of symbols a bit
            return std::move(children);
        }
        return SymbolsList{ fakeSymbol(item.internalKind(), std::move(children)) };
    };
    const auto expectedSymbols = expectedSymbolsOfFakeQmlFile();

    const auto symbols = DocumentSymbolUtils::assembleSymbolsForQmlFile(qmlFileItem, af);

    QVERIFY(compareDocumentSymbolsLists(symbols, expectedSymbols));
}

QTEST_MAIN(tst_document_symbol_utils)
