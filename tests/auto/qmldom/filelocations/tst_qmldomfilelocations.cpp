// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmldomfilelocations.h"
#include <QtQmlDom/private/qqmldomfilelocations_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

using namespace QQmlJS::Dom;
using namespace QQmlJS::Dom::FileLocations;

void TestFileLocations::InfoDOMAPI()
{
    const auto fullRegionLoc = QQmlJS::SourceLocation(1, 1, 1, 1);
    const auto tokenRegion(FileLocationRegion::AsTokenRegion);
    const auto tokenRegionLoc = QQmlJS::SourceLocation(2, 2, 2, 2);

    Info regionsInfo;
    regionsInfo.fullRegion = fullRegionLoc;
    regionsInfo.regions[tokenRegion] = tokenRegionLoc;

    // Info is a simple object wrap
    auto infoItem = DomItem().wrap({}, regionsInfo);

    const QList<QString> expectedFields = { fieldNameToQString(Fields::fullRegion), fieldNameToQString(Fields::regions) };
    QCOMPARE(infoItem.fields(), expectedFields);
    QCOMPARE(infoItem.field(Fields::fullRegion).value(), sourceLocationToQCborValue(fullRegionLoc));
    QCOMPARE(infoItem.field(Fields::regions).key(fileLocationRegionName(tokenRegion)).value(),
             sourceLocationToQCborValue(tokenRegionLoc));
}

void TestFileLocations::NodeAPI()
{
    const auto parentNode = Node::instantiate();
    QVERIFY(parentNode->subItems().empty());
    const auto childNode = parentNode->insertOrReturnChildAt(Path::fromKey(u"child"));
    QVERIFY(childNode);
    QCOMPARE(childNode->parent(), parentNode);
    QCOMPARE(parentNode->subItems().size(), 1);
}

void TestFileLocations::NodeDOMAPI()
{
    const auto rootNodePtr = Node::instantiate();
    // Wrap with the help of environment to make subsequent canonicalPath calls valid
    auto rootNodeItem = DomItem(DomEnvironment::create({})).wrap({}, rootNodePtr);

    const QList<QString> expectedRootNodeFields = {
        // root has no parent => not visited
        fieldNameToQString(Fields::path), fieldNameToQString(Fields::subItems), fieldNameToQString(Fields::infoItem)
    };
    QCOMPARE(rootNodeItem.fields(), expectedRootNodeFields);

    // add child Node
    const auto childPath = Path::fromKey(u"child");
    rootNodePtr->insertOrReturnChildAt(childPath);
    auto childNodeItem = rootNodeItem.field(Fields::subItems).key(childPath.toString());

    const QList<QString> expectedChildNodeFields = { fieldNameToQString(Fields::parent), fieldNameToQString(Fields::path),
                                                     fieldNameToQString(Fields::subItems),
                                                     fieldNameToQString(Fields::infoItem) };
    QCOMPARE(childNodeItem.fields(), expectedChildNodeFields);
    QCOMPARE(childNodeItem.field(Fields::parent).ownerAs<Node>(), rootNodePtr);
}

void TestFileLocations::createEnsureFind()
{
    const auto childCanonicalPath = Path::fromString(u"$top.children[0]");
    auto rootNodePtr = FileLocations::createTree(childCanonicalPath.head());

    // drop front because paths in the tree are relative
    const auto childNodePath = childCanonicalPath.dropFront();
    auto childNodePtr = FileLocations::ensure(rootNodePtr, childNodePath);
    QVERIFY(childNodePtr);

    auto foundChildNodePtr = FileLocations::find(rootNodePtr, childNodePath);
    QVERIFY(foundChildNodePtr);
    QCOMPARE(foundChildNodePtr, childNodePtr);
}

void TestFileLocations::visitTree()
{
    const auto child1CanonicalPath = Path::fromString(u"$top.children[0]");
    const auto child2CanonicalPath = Path::fromString(u"$top.children[1]");
    const auto grandChild1CanonicalPath = Path::fromString(u"$top.children[1].child[0]");
    const auto grandChild2CanonicalPath = Path::fromString(u"$top.children[1].child[1]");

    auto rootNodePtr = FileLocations::createTree(child1CanonicalPath.head());
    FileLocations::ensure(rootNodePtr, child1CanonicalPath.dropFront());
    FileLocations::ensure(rootNodePtr, child2CanonicalPath.dropFront());
    FileLocations::ensure(rootNodePtr, grandChild1CanonicalPath.dropFront());
    auto grandChild2Ptr = FileLocations::ensure(rootNodePtr, grandChild2CanonicalPath.dropFront());

    QCOMPARE(FileLocations::canonicalPathForTesting(grandChild2Ptr),
             grandChild2CanonicalPath.toString());

    QList<QString> visitedNodePaths;
    FileLocations::visitTree(rootNodePtr, [&visitedNodePaths](const Path &p, const Tree &) {
        visitedNodePaths.append(p.toString());
        return true;
    });

    // Atm FileLocations tree is far from perfect and it creates unnecessary nodes when it comes to
    // arrays or maps...
    // partially due to the nature of Path structure and DOM wrappers... But we still want
    // to test that all of those are correctly visited....
    QList<QString> expectedVisitedNodePaths = { "$top",
                                                "$top.children",
                                                "$top.children[0]",
                                                "$top.children[1]",
                                                "$top.children[1][\"child\"]",
                                                "$top.children[1][\"child\"][0]",
                                                "$top.children[1][\"child\"][1]" };

    QCOMPARE(visitedNodePaths, expectedVisitedNodePaths);
}

void TestFileLocations::addRegion()
{
    { // Main region update on a node
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));
        QVERIFY(!rootNodePtr->info().fullRegion.isValid());
        QVERIFY(rootNodePtr->info().regions.empty());

        const auto rootMainLoc = QQmlJS::SourceLocation(0, 1);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, rootMainLoc);

        QCOMPARE(rootNodePtr->info().regions.value(FileLocationRegion::MainRegion), rootMainLoc);
        QCOMPARE(rootNodePtr->info().fullRegion, rootMainLoc);
    }

    { // Update on a node if loc.begin < fullRegion.begin()
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));

        const auto rootMainLoc = QQmlJS::SourceLocation(2, 4);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, rootMainLoc);

        const auto rootLBraceLoc = QQmlJS::SourceLocation(0, 1);
        const auto rootMainRegionAfterAdd = combine(rootNodePtr->info().fullRegion, rootLBraceLoc);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::LeftBraceRegion, rootLBraceLoc);

        QCOMPARE(rootNodePtr->info().regions.value(FileLocationRegion::LeftBraceRegion),
                 rootLBraceLoc);
        QCOMPARE(rootNodePtr->info().fullRegion, rootMainRegionAfterAdd);
    }

    { // Update on a node if loc.end > fullRegion.end()
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));

        const auto rootMainLoc = QQmlJS::SourceLocation(0, 4);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, rootMainLoc);

        const auto rootLBraceLoc = QQmlJS::SourceLocation(4, 1);
        const auto rootMainRegionAfterAdd = combine(rootNodePtr->info().fullRegion, rootLBraceLoc);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::LeftBraceRegion, rootLBraceLoc);

        QCOMPARE(rootNodePtr->info().regions.value(FileLocationRegion::LeftBraceRegion),
                 rootLBraceLoc);
        QCOMPARE(rootNodePtr->info().fullRegion, rootMainRegionAfterAdd);
    }

    { // Update all parents
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));
        auto child = FileLocations::ensure(rootNodePtr, Path::fromString(u".child"));
        auto grandChild = FileLocations::ensure(rootNodePtr, Path::fromString(u".child.child"));

        const auto grandChildMainLoc = QQmlJS::SourceLocation(0, 3);
        const auto rootMainRegionAfterAdd =
                combine(rootNodePtr->info().fullRegion, grandChildMainLoc);
        const auto childMainRegionAfterAdd = combine(child->info().fullRegion, grandChildMainLoc);

        FileLocations::addRegion(grandChild, FileLocationRegion::MainRegion, grandChildMainLoc);

        QCOMPARE(grandChild->info().fullRegion, grandChildMainLoc);
        QCOMPARE(child->info().fullRegion, childMainRegionAfterAdd);
        QCOMPARE(rootNodePtr->info().fullRegion, rootMainRegionAfterAdd);
    }

    { // Update only one parent
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));
        const auto rootMainLoc = QQmlJS::SourceLocation(0, 4);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, rootMainLoc);

        auto child = FileLocations::ensure(rootNodePtr, Path::fromString(u".child"));
        auto grandChild = FileLocations::ensure(rootNodePtr, Path::fromString(u".child.child"));

        const auto grandChildMainLoc = QQmlJS::SourceLocation(0, 3);
        const auto childMainRegionAfterAdd = combine(child->info().fullRegion, grandChildMainLoc);

        FileLocations::addRegion(grandChild, FileLocationRegion::MainRegion, grandChildMainLoc);

        QCOMPARE(grandChild->info().fullRegion, grandChildMainLoc);
        QCOMPARE(child->info().fullRegion, childMainRegionAfterAdd);
        QCOMPARE(rootNodePtr->info().fullRegion, rootMainLoc);
    }

    { // FullRegion is not updated if adding MainRegion, which is smaller than current
        auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));

        const auto rootMainRegionLoc = QQmlJS::SourceLocation(0, 7);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, rootMainRegionLoc);

        const auto newRootMainRegionLoc = QQmlJS::SourceLocation(0, 3);
        FileLocations::addRegion(rootNodePtr, FileLocationRegion::MainRegion, newRootMainRegionLoc);

        QCOMPARE(rootNodePtr->info().regions.value(FileLocationRegion::MainRegion),
                 newRootMainRegionLoc);
        QEXPECT_FAIL(0, "shrinking of regions is currently not supported (QTBUG-131288)", Continue);
        QCOMPARE(rootNodePtr->info().fullRegion, newRootMainRegionLoc);
    }
}

void TestFileLocations::treeOf()
{
    // prepare mock File locations for the "rootObject"
    auto rootNodePtr = FileLocations::createTree(Path::fromRoot(PathRoot::Top));
    auto objNodePtr =
            FileLocations::ensure(rootNodePtr, Path::fromString(u".components[\"\"][0].objects[0]"));
    const auto objLocation = QQmlJS::SourceLocation(1, 1);
    objNodePtr->info().fullRegion = objLocation;

    // set up hierarchy of Dom Objects and fLoc to QmlFile
    QmlObject obj;
    QmlComponent c;
    c.addObject(obj);
    auto qmlFilePtr = std::make_shared<QmlFile>();
    qmlFilePtr->addComponent(c);
    qmlFilePtr->setFileLocationsTree(rootNodePtr);
    auto qmlFileItem = DomItem(DomEnvironment::create({})).wrap({}, qmlFilePtr);

    // query and verify
    auto treeOfObj = FileLocations::treeOf(qmlFileItem.qmlObject(GoTo::MostLikely));
    QCOMPARE(treeOfObj, objNodePtr);
    QCOMPARE(treeOfObj->info().fullRegion, objLocation);
}

#ifndef NO_QTEST_MAIN
QTEST_MAIN(TestFileLocations)
#endif
