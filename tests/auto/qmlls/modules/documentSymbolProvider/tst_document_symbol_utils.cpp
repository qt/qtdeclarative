// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomelements_p.h>
#include <QtQmlLS/private/documentsymbolutils_p.h>
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

#include "tst_document_symbol_utils.h"

using namespace QQmlJS::Dom;
using QLspSpecification::DocumentSymbol;
using SymbolsList = QList<DocumentSymbol>;

QT_BEGIN_NAMESPACE
// TODO move out somewhere?
template <typename DomClass>
static inline DomItem domItem(DomClass &&c)
{
    DomItem domEnv(DomEnvironment::create({}));
    constexpr auto kind = std::decay<DomClass>::type::kindValue;
    if constexpr (domTypeIsDomElement(kind)) {
        c.updatePathFromOwner(
                Path::Current()); // necessary for querying fields, a.k.a. init DomElement
        // For the "DomElement"-s owners are required,
        // hence creating Env as an owner and then call copy on it
        return domEnv.copy(&c);
    }
    if constexpr (kind == DomType::QmlFile) {
        return domEnv.copy(&c);
    }
    // it's helpful to use wrap on DomEnv instead of just DomItem() to make
    // .canonicalPath() usable
    return domEnv.wrap(QQmlJS::Dom::PathEls::PathComponent(), c);
}
QT_END_NAMESPACE

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
 *     id: fakeId
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
    rootCmp.addId(Id("fakeId"));
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
    mainQmlCompSymbol.children = { // BEWARE
                                   // because Id is manually handled
                                   // outside of visiting logic, before it becomes a child of
                                   // Object, it's not present here
                                   std::move(rootQmlObjectSymbol)
    };

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

void tst_document_symbol_utils::symbolNameOf()
{
    { // Id
        // for the DomItem's representing Id-s, DocumentSymbol::name = ByteArray("id"),
        // the Dom::Id::name will be used as a DocumentSymbol::detail
        Id id;
        auto expectedName = QByteArray("id");
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(id));
        QCOMPARE(name, expectedName);
        id.name = "name";
        name = DocumentSymbolUtils::symbolNameOf(domItem(id));
        QCOMPARE(name, expectedName);
    }

    { // EnumItem
        const QString enumItemName("enumItem");
        EnumItem enumItem(enumItemName);
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(enumItem));
        QCOMPARE(name, enumItemName.toUtf8());
    }

    { // EnumDecl
        const QString enumDeclName("enumDecl");
        EnumDecl enumDecl(enumDeclName);
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(enumDecl));
        QCOMPARE(name, enumDeclName.toUtf8());
    }

    { // PropertyDefinition
        const QString propertyName("propName");
        PropertyDefinition propDef;
        propDef.name = propertyName;
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(propDef));
        QCOMPARE(name, propertyName.toUtf8());
    }

    { // Binding
        const QString bindingName("bindingName");
        Binding binding(bindingName);
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(binding));
        QCOMPARE(name, bindingName.toUtf8());
    }

    { // Method
        const QString methodName("methodName");
        MethodInfo mInfo;
        mInfo.name = methodName;
        auto name = DocumentSymbolUtils::symbolNameOf(domItem(mInfo));
        QCOMPARE(name, methodName.toUtf8());
    }

    { // QmlObject
        const QString objName("objName");
        QmlObject obj;
        obj.setName(objName);
        const auto name = DocumentSymbolUtils::symbolNameOf(domItem(obj));
        QCOMPARE(name, objName.toUtf8());
    }

    { // QmlComponent
        const QString cmpName("cmpName");
        QmlComponent cmp(cmpName);
        const auto name = DocumentSymbolUtils::symbolNameOf(domItem(cmp));
        QCOMPARE(name, cmpName.toUtf8());
    }
}

void tst_document_symbol_utils::symbolKindOf()
{
    { // Method && signal
        MethodInfo mInfo;
        auto kind = DocumentSymbolUtils::symbolKindOf(domItem(mInfo));
        QCOMPARE(kind, QLspSpecification::SymbolKind::Method);

        mInfo.methodType = MethodInfo::Signal;
        kind = DocumentSymbolUtils::symbolKindOf(domItem(mInfo));
        QCOMPARE(kind, QLspSpecification::SymbolKind::Event);
    }

    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(Binding())),
             QLspSpecification::SymbolKind::Variable);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(PropertyDefinition())),
             QLspSpecification::SymbolKind::Property);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(Id())), QLspSpecification::SymbolKind::Key);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(EnumDecl())),
             QLspSpecification::SymbolKind::Enum);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(EnumItem())),
             QLspSpecification::SymbolKind::EnumMember);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(QmlObject())),
             QLspSpecification::SymbolKind::Object);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(QmlComponent())),
             QLspSpecification::SymbolKind::Module);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(domItem(QmlFile())),
             QLspSpecification::SymbolKind::File);
    QCOMPARE(DocumentSymbolUtils::symbolKindOf(DomItem()), QLspSpecification::SymbolKind::Null);
}

void tst_document_symbol_utils::tryGetDetailOf()
{
    { // Id
        Id id;
        auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(id));
        QVERIFY(!detail.has_value());

        const QString name("name");
        id.name = name;
        const auto expectedDetail = name.toUtf8();
        detail = DocumentSymbolUtils::tryGetDetailOf(domItem(id));
        QVERIFY(detail.has_value());
        QCOMPARE(detail, expectedDetail);
    }
    { // EnumItem
        const int value = 4;
        const auto expectedDetail = QByteArray::number(value);
        EnumItem enumItem("a", value);
        const auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(enumItem));
        QCOMPARE(detail, expectedDetail);
    }
    { // Binding
        Binding b;
        auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
        QCOMPARE(detail, std::nullopt);

        b.setValue(std::make_unique<BindingValue>());
        detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
        QCOMPARE(detail, std::nullopt);

        b.setValue(std::make_unique<BindingValue>(QmlObject()));
        detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
        QCOMPARE(detail, std::nullopt);

        b.setValue(std::make_unique<BindingValue>(QList<QmlObject>()));
        detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
        QCOMPARE(detail, std::nullopt);

        {
            const QString bindingValue("4");
            const auto expectedDetail = bindingValue.toUtf8();
            const QString bindingExpr = bindingValue + ";";
            const auto exprPtr = std::make_shared<ScriptExpression>(
                    bindingExpr, ScriptExpression::ExpressionType::BindingExpression);
            b.setValue(std::make_unique<BindingValue>(exprPtr));
            detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
            QCOMPARE(detail, expectedDetail);
        }
        {
            const QString bindingExpr("12345678901234567890123456"); // 26 symbols
            const auto expectedDetail = QString("1234567890123456789012...").toUtf8();
            const auto exprPtr = std::make_shared<ScriptExpression>(
                    bindingExpr, ScriptExpression::ExpressionType::BindingExpression);
            b.setValue(std::make_unique<BindingValue>(exprPtr));
            detail = DocumentSymbolUtils::tryGetDetailOf(domItem(b));
            QCOMPARE(detail, expectedDetail);
        }
    }
    { // MethodInfo
        { // Method
            MethodParameter intA;
            intA.name = "a";
            intA.typeName = "int";
            MethodParameter stringB;
            stringB.name = "b";
            stringB.typeName = "string";
            MethodInfo method;
            method.parameters = { intA, stringB };
            method.typeName = "bool";

            const auto expectedDetail = QString("(a: int, b: string): bool").toUtf8();
            const auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(method));
            QVERIFY(detail.has_value());
            QCOMPARE(detail.value(), expectedDetail);
        }
        { // Signal
            MethodParameter intA;
            intA.name = "a";
            intA.typeName = "int";
            MethodInfo method;
            method.methodType = MethodInfo::MethodType::Signal;
            method.parameters = { intA };

            const auto expectedDetail = QString("(a: int)").toUtf8();
            const auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(method));
            QVERIFY(detail.has_value());
            QCOMPARE(detail.value(), expectedDetail);
        }
    }
    { // QmlObject
        QmlObject obj;
        auto detail = DocumentSymbolUtils::tryGetDetailOf(domItem(obj));
        QCOMPARE(detail, std::nullopt);

        const QString objId("objId");
        obj.setIdStr(objId);
        detail = DocumentSymbolUtils::tryGetDetailOf(domItem(obj));
        QCOMPARE(detail, objId.toUtf8());

        /*
         * Unfortunately because of the way DomItem::component() and filterUp() are working
         * (using full path from the root(env/top) and non-trivially trimming it)
         * I can't properly construct a fake hierarchy of components and objects
         * to verify the "root" case.
         */
    }
}

static std::pair<QList<QLspSpecification::DocumentSymbol>, QList<QLspSpecification::DocumentSymbol>>
reorganizeForOutlineViewTestData()
{
    /* The following fake QmlFile is used as testing data:
     * 0|12345678901234567890123456789012345678901234
     * 1|Item1 { Enum{}
     * 2|component c.1{Item2{}}
     * 3|Item3{Item4{ id: item4; component c.2{} } }
     *
     * inside DOM, SourceLocations, hence Ranges, of all objects/components, etc
     * are adhearing "structural" view, meaning that even though they are not
     * in "structural" hierarchy inside DOM, their Locations following "structural" hierarchy
     * order of children will be fixed/sorted by the client
     */

    using QLspSpecification::DocumentSymbol;
    using QLspSpecification::SymbolKind;
    // Construct Document Symbols, corresponding to the fakeQmlFile, that needs to be reorganized
    DocumentSymbol enumSymbol;
    enumSymbol.name = "Enum";
    enumSymbol.kind = DocumentSymbolUtils::symbolKindOf(domItem(EnumDecl()));
    enumSymbol.range = { { 1, 9 }, { 1, 14 } };

    DocumentSymbol item4Id;
    item4Id.name = "item4";
    item4Id.kind = DocumentSymbolUtils::symbolKindOf(domItem(Id()));
    item4Id.range = { { 3, 14 }, { 3, 23 } };

    DocumentSymbol item4;
    item4.range = { { 3, 7 }, { 3, 41 } };
    item4.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlObject()));
    item4.name = "Item4";
    item4.children.emplace({ item4Id });

    DocumentSymbol item3;
    item3.range = { { 3, 1 }, { 3, 43 } };
    item3.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlObject()));
    item3.name = "Item3";
    item3.children.emplace({ item4 });

    DocumentSymbol item1;
    item1.range = { { 1, 1 }, { 3, 43 } };
    item1.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlObject()));
    item1.name = "Item1";
    item1.children.emplace({ item3 });

    DocumentSymbol mainCmp;
    mainCmp.range = { { 1, 1 }, { 3, 43 } };
    mainCmp.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlComponent()));
    mainCmp.name = "mainCmp";
    // Create and add an fictional Id just to verify that it's not touched by the reorganisation
    auto fictionalId(item4Id);
    mainCmp.children.emplace({ enumSymbol, fictionalId, item1 });

    DocumentSymbol item2;
    item2.range = { { 2, 15 }, { 3, 21 } };
    item2.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlObject()));
    item2.name = "Item2";

    DocumentSymbol cmpC1;
    cmpC1.range = { { 2, 1 }, { 3, 22 } };
    cmpC1.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlComponent()));
    cmpC1.name = "c.1";
    cmpC1.children.emplace({ item2 });

    DocumentSymbol cmpC2;
    cmpC2.range = { { 3, 25 }, { 3, 39 } };
    cmpC2.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlComponent()));
    cmpC2.name = "c.2";

    DocumentSymbol qmlFile;
    qmlFile.range = { { 1, 1 }, { 3, 43 } };
    qmlFile.kind = DocumentSymbolUtils::symbolKindOf(domItem(QmlFile()));
    qmlFile.name = "fakeQmlFile";
    qmlFile.children.emplace({ mainCmp, cmpC1, cmpC2 });

    QList<DocumentSymbol> fakeQmlFileSymbols{ qmlFile };
    //-----------------------------------------------------------------------
    // during reorganization cmpC2 should be added as a child
    DocumentSymbol reorganizedItem4(item4);
    reorganizedItem4.children->push_back(cmpC2);

    // should contain reorganizedItem4
    DocumentSymbol reorganizedItem3(item3);
    reorganizedItem3.children = { reorganizedItem4 };

    // should contain reorganizedItem3, enum, cmpC1 as children
    DocumentSymbol reorganizedItem1(item1);
    reorganizedItem1.children = { reorganizedItem3, enumSymbol, cmpC1 };

    // verify fictionalId stays where it was added, not touched by the reorg
    QList<DocumentSymbol> reorganizedFakeQmlFileSymbols{ fictionalId, reorganizedItem1 };
    return std::make_pair(fakeQmlFileSymbols, reorganizedFakeQmlFileSymbols);
}

void tst_document_symbol_utils::reorganizeForOutlineView()
{
    auto [fakeQmlFileSymbols, expectedReorganizedFakeQmlFileSymbols] =
            reorganizeForOutlineViewTestData();
    DocumentSymbolUtils::reorganizeForOutlineView(fakeQmlFileSymbols);
    QVERIFY(compareDocumentSymbolsLists(fakeQmlFileSymbols, expectedReorganizedFakeQmlFileSymbols));
}

QTEST_MAIN(tst_document_symbol_utils)
