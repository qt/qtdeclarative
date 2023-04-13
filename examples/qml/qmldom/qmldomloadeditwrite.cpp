// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// common declarations
#include <QtQmlDom/private/qqmldomitem_p.h>
// comparisons of two DomItems
#include <QtQmlDom/private/qqmldomcompare_p.h>
// field filters to compare only selected fields (ignore for example location changes)
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>
// needed to edit and cast to concrete type (PropertyDefinition, ScriptExpression,...)
#include <QtQmlDom/private/qqmldomelements_p.h>
// cast of the top level items (DomEnvironments,...)
#include <QtQmlDom/private/qqmldomtop_p.h>

#include <QtTest/QtTest>
#include <QCborValue>
#include <QDebug>
#include <QLatin1String>
#include <QLatin1Char>
#include <QLibraryInfo>
#include <QDir>

#include <memory>

// everything is in the QQmlJS::Dom namespace
using namespace QQmlJS::Dom;

int main()
{
    QString baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/reformatter");
    QStringList qmltypeDirs =
            QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });

    qDebug() << "Creating an environment loading qml from the directories" << qmltypeDirs;
    qDebug() << "single threaded, no dependencies";
    DomItem env =
            DomEnvironment::create(qmltypeDirs,
                                   QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                                           | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    QString testFilePath = baseDir + QLatin1String("/file1.qml");
    DomItem tFile; // place where to store the loaded file
    // env.loadBuiltins();

    qDebug() << "loading the file" << testFilePath;
    env.loadFile(
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFilePath),
#else
            testFilePath, QString(),
#endif
            [&tFile](Path, const DomItem &, const DomItem &newIt) {
                tFile = newIt; // callback called when everything is loaded that receives the loaded
                               // external file pair (path, oldValue, newValue)
            },
            LoadOption::DefaultLoad);

    // trigger the load
    env.loadPendingDependencies();

    // # Read only API: DomItem is a generic pointer for read only access to Dom Itmes :)
    {
        // ## declarative json like API
        DomItem qmlFile = tFile.field(Fields::currentItem);
        DomItem imports = qmlFile.field(Fields::imports);
        DomItem qmlObj = qmlFile.field(Fields::components)
                                 .key(QString())
                                 .index(0)
                                 .field(Fields::objects)
                                 .index(0);

        // ### Dump
        // any DomItem can be dumped
        qDebug() << "writing to QDebug dumps that element:" << imports;
        // often the dump is too verbose, and one might want it to a separate file
        QString dumpFilePath =
                QDir(QDir::tempPath())
                        .filePath(QFileInfo(testFilePath).baseName() + QLatin1String(".dump.json"));
        qmlFile.dump(dumpFilePath, FieldFilter::defaultFilter());
        qDebug() << "dumped file to" << dumpFilePath;

        // ### Paths
        qDebug() << "To identify a DomItem a canonical path can be used:"
                 << imports.canonicalPath();
        // a path can be converted to/from strings
        QString pString = imports.canonicalPath().toString();
        Path importsPath = Path::fromString(pString);
        // and loaded again using the .path(somePath) method
        DomItem imports2 = env.path(importsPath);
        Q_ASSERT(imports == imports2);
        // the canonical path is absolute, but you can have relative paths
        Path first = Path::Index(0);
        DomItem firstImport = imports.path(first);
        // an existing path can also be extended
        Path firstImportPath = importsPath.index(0);
        Q_ASSERT(firstImportPath == firstImport.canonicalPath());
        // the normal elements of a path are index, key, field
        // Uppercase static method creates one, lowercase appends to an existing path.
        Path mainComponentPath = Path::Field(Fields::components).key("").index(0);
        DomItem mainComponent = qmlFile.path(mainComponentPath);
        // DomItems have the same methods to access their elements
        DomItem mainComponent2 = qmlFile.field(Fields::components).key("").index(0);
        // two other special ements are root (root element for absolute paths)
        Path topPath = Path::Root(PathRoot::Top);
        Q_ASSERT(topPath == importsPath[0]);
        // the current element performs an operation (tipically a lookup or iteration) at the
        // current path location (not handled here)
        Path lookupPath = Path::Current(PathCurrent::Lookup);

        // there are various visit methods to iterate/visit DomItems in particular visitTree
        // which is quite flexible.
        // They normally use callbacks that can return false to stop the iteration.
        // Still often the DomKind specific for loop presentated later are clearer and more
        // convenient
        {
            QDebug dbg = qDebug().noquote().nospace();
            imports.visitTree(
                    Path(),
                    [&dbg](Path p, const DomItem &el, bool adopted) {
                        dbg << QStringLiteral(u" ").repeated(p.length()) << "*" << p.last() << " "
                            << domKindToString(el.domKind()) << "(" << el.internalKindStr()
                            << ")\n";
                        // returning false here stops the whole iteration
                        return true;
                    },
                    VisitOption::Default, // we want a recursive visit visiting also the top and
                                          // adopted
                    [&dbg](Path p, const DomItem &, bool canonicalChild) {
                        // returning false here skips that branch
                        if (!canonicalChild) {
                            dbg << QStringLiteral(u" ").repeated(p.length()) << "+" << p.last()
                                << " (adopted, will not recurse)\n";
                        } else if (p && p.headIndex(0) % 2 == 1) {
                            dbg << QStringLiteral(u" ").repeated(p.length()) << "-" << p.last()
                                << " *recursive visit skipped*\n";
                            return false; // we skip odd entries in lists;
                        } else {
                            dbg << QStringLiteral(u" ").repeated(p.length()) << "+" << p.last()
                                << "\n";
                        }
                        return true;
                    },
                    [&dbg](Path p, const DomItem &, bool) {
                        dbg << QStringLiteral(u" ").repeated(p.length()) << "=" << p.last() << "\n";
                        return true;
                    });
        }

        // ### DomKind
        // any DomItem belongs to one of 5 fundamental types

        // 1. Object (a C++ object)
        Q_ASSERT(qmlFile.domKind() == DomKind::Object);
        // The underlying type of the c++ object can be found with .internalKind()
        Q_ASSERT(qmlFile.internalKind() == DomType::QmlFile);
        // .initernalKindStr() is a convenience string version of it
        Q_ASSERT(qmlFile.internalKindStr() == u"QmlFile");
        // the object attributes (fields) can be reached using .field(u"filedName")
        // normally one should not use a string, but the Fields:: constant
        DomItem qmlFile2 = tFile.field(Fields::currentItem);
        // all the available fields can be listed via fields()
        qDebug() << "The" << qmlObj.internalKindStr() << "at" << qmlObj.canonicalPath()
                 << "has the following fields:" << qmlObj.fields();
        // we can access the underlying C++ object with as<>
        if (const QmlFile *qmlFilePtr = qmlFile.as<QmlFile>())
            qDebug() << "The QmlFile lives at the address" << qmlFilePtr;
        // We can get the shared pointer of the owner type (which for the file is the QmlFile itself
        if (std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>())
            qDebug() << "QmlFile uses shared pointers as ownership method, the underlying address "
                        "is the same"
                     << qmlFilePtr.get();

        // 2. a (Cbor-) Value, i.e a string, number,...
        DomItem fPath = qmlFile.field(Fields::canonicalFilePath);
        Q_ASSERT(fPath.domKind() == DomKind::Value);
        // the Cbor representation of a value can be extracted with .value(), and in this case we
        // can then call toString
        qDebug() << "The filePath DomItem is " << fPath << " and it still 'knows' its path "
                 << fPath.canonicalPath() << " but can have it also as value:" << fPath.value()
                 << "or even better as string." << fPath.value().toString(QLatin1String("*none*"));
        // a DomItem might have a valid value() even if it is not of type DomKind::Value, indeed
        // CBor maps and lists are mapped to DomKind::Map and DomKind::List, and can be traversed
        // thought that but also have a valid value().

        // 3. a list
        Q_ASSERT(imports.domKind() == DomKind::List);
        // the number of elements can be sound with .indexes() and with .index(n) we access each
        // element
        qDebug() << "We have " << imports.indexes() << " imports, and the first is "
                 << imports.index(0);
        // If we want to just loop on the elements .values() is the most convenient way
        // technically values *always* works even for objects and maps, iterating on the values
        for (DomItem import : imports.values()) {
            if (const Import *importPtr = import.as<Import>()) {
                if (importPtr->implicit)
                    qDebug() << importPtr->uri.toString() << importPtr->version.stringValue();
            }
        }

        // 4. a map
        DomItem bindings = qmlObj.field(Fields::bindings);
        Q_ASSERT(bindings.domKind() == DomKind::Map);
        // The keys of the map can be reached either with .keys() or .sortedKeys(), each element
        // with .key(k)
        qDebug() << "bindings";
        for (QString k : bindings.sortedKeys()) {
            for (DomItem b : bindings.key(k).values()) {
                qDebug() << k << ":" << b;
            }
        }

        // 5 The empty element
        DomItem empty;
        Q_ASSERT(empty.domKind() == DomKind::Empty);
        // The empty element is the only DomItem that casted to bool returns false, so checking for
        // it can be just an implicit cast to bool
        Q_ASSERT(bindings && !empty);
        // the empty element supports all the previus operations so that one can traverse a non
        // existing path without checking at every element, but only check the result
        DomItem nonExisting = qmlFile.field(u"no-existing").key(u"a").index(0);
        Q_ASSERT(!nonExisting);

        // the index operator [] can be used instead of .index/.key/.field, it might be slightly
        // less efficient but works

        // find type
        // access type

        // ### write out
        // it is possible to write out a qmlFile (actually also parts of it), which will
        // automatically reformat it
        QString reformattedFilePath =
                QDir(QDir::tempPath())
                        .filePath(QFileInfo(testFilePath).baseName() + QLatin1String(".qml"));
        DomItem newFile = qmlFile.writeOut(reformattedFilePath);
        qDebug() << "reformatted written at " << reformattedFilePath;

        // ## Jumping around
        // ### Generic Methods
        // from a DomItem you do no have just deeper in the tree, you can also go up the hierarch
        // toward the root .container() just goes up one step in the canonicalPath of the object
        Q_ASSERT(imports == firstImport.container());
        // .containingObject() goes up to the containing DomKind::Object, skipping over all Maps and
        // Lists
        Q_ASSERT(qmlFile == firstImport.containingObject());
        // .owner() returns the shared pointer based "owner" object, qmlFile and ScriptExpression
        // are owningItems
        Q_ASSERT(qmlFile == bindings.owner());
        // .top() goes to the top of the tree, i.e the environment (or the universe)
        Q_ASSERT(env == bindings.top());
        // environment is normally the same as top, but making sure it is a actually a
        // DomEnvironment
        Q_ASSERT(env = bindings.environment());
        // the universe is a cache of loaded files which for each file keeps two versions: the
        // latest and the latest valid it can be reached with .universe(), from the universe you
        // cannot get back to the environment.
        Q_ASSERT(env.universe().internalKind() == DomType::DomUniverse);

        // ## QML Oriented Methods
        // The Dom model is not for generic json-like structures, so there are methods tailored for
        // Qml and its structure
        // The methods can succeed if there is a clearly defined unique result.
        // sometime there is an obivious, but not necessarily unique choice (tipically going up the
        // hierarchy), for example given a qml file the obvious choice for a component is the root
        // component, but the file might contain other inline components, and for an object with
        // different version exposed (C++ property versioning) the latest version is the natural
        // choice, but other might be available. In these case passing GoTo::MostLikely as argument
        // makes the method to this obivious choice (or possibly even only choice if no other
        // versions/components are actually defined), instead of refusing any potentially ambiguous
        // situation and returning the empty element.

        // .fileObject() goes to the object representing the whole file
        // (from either the external object returned by load or from inside the file)
        DomItem fileObject = tFile.fileObject();
        DomItem fileObject2 = imports.fileObject();
        Q_ASSERT(fileObject == fileObject2 && fileObject.internalKind() == DomType::QmlFile);
        // .component() goes to the component object.
        Q_ASSERT(qmlObj.component() == qmlFile.component(GoTo::MostLikely));
        // .pragmas gives access to the pragmas of the current component
        Q_ASSERT(qmlFile.pragmas() == qmlFile.field(Fields::pragmas));

        // QmlObject
        // QmlObject if the main to represent the type information (methods, bindings,
        // properties,...) of qml. Please note that QmlObject -> component operation is potentially
        // lossy, when multiple version are exposed, so we represent a type through its root object,
        // not through a component.

        // .qmlObject() goes to the current QmlObject
        Q_ASSERT(qmlObj == bindings.qmlObject());

        // Given the centrality of QmlObject several of its attributes have convenience methods
        // to access them:

        // .children() makes subObjects contained inside a QmlObject accessible
        // note that it is possible to add objects also by directly binding the children or data
        // attribute, those children are not listed here, this accesses only those listed inside
        // the QmlObject
        Q_ASSERT(qmlObj.children() == qmlObj.field(Fields::children));
        DomItem subObj0 = qmlObj.children().index(0);
        // .child(<i>) is a shortcut for .children.index(<i>)
        Q_ASSERT(subObj0 == qmlObj.child(0));
        // rootQmlObject goes to the root qmlObject (unless one reaches an empty element)
        Q_ASSERT(!subObj0 || subObj0.rootQmlObject() == qmlObj);
        // .bindings() returns the bindings defined in the current object
        Q_ASSERT(bindings == qmlObj.bindings());
        DomItem mCompObj = qmlObj.child(0)
                                   .child(0)
                                   .bindings()
                                   .key(u"delegate")
                                   .index(0)
                                   .field(Fields::value)
                                   .child(1);
        // .methods() gives methods definitions and signals
        DomItem methods = mCompObj.methods();
        qDebug() << "mCompObj methods:";
        for (QString methodName : methods.sortedKeys()) {
            for (DomItem method : methods.key(methodName).values()) {
                if (const MethodInfo *methodPtr = method.as<MethodInfo>()) {
                    Q_ASSERT(methodName == methodPtr->name);
                    qDebug() << " " << methodPtr->name << methodPtr->methodType;
                }
            }
        }
        qDebug() << "mCompObj propertyDefs:";
        // .propertyDefs() returns the properties defined in the current object
        DomItem pDefs = mCompObj.propertyDefs();
        for (QString pDefName : pDefs.sortedKeys()) {
            for (DomItem pDef : pDefs.key(pDefName).values()) {
                if (const PropertyDefinition *pDefPtr = pDef.as<PropertyDefinition>()) {
                    Q_ASSERT(pDefName == pDefPtr->name);
                    qDebug() << " " << pDefPtr->name << pDefPtr->typeName;
                }
            }
        }
        // binding and property definitions are about the ones defined in the current object
        // often one is interested also to the inherited properties.
        // Here PropertyInfo helps, it list all the definitions and bindings for a given property
        // in the inheritance order (local definitions, parent definitions, parent parent
        // definitions,...)
        // .propertyInfos() gives access in the usual way (through a DomItem)
        DomItem propertyInfos = mCompObj.propertyInfos();
        // .propertyInfoWithName(<name>) directly accesses one
        PropertyInfo pInfo = mCompObj.propertyInfoWithName(QStringLiteral(u"a"));
        qDebug() << "bindings" << pInfo.bindings;
        // .propertyInfoNames() gives the names  of the properties
        Q_ASSERT(propertyInfos.keys() == mCompObj.propertyInfoNames());

        // .globalScope() goes to the globa scope object
        Q_ASSERT(qmlObj.globalScope().internalKind() == DomType::GlobalScope);
        // and scope to the containing scope
        Q_ASSERT(bindings.scope() == qmlObj);
    }
    // mutate & edit
    {
        // DomItem handles read-only access, but if one wants to change something it cannot be used.
        // MutableDomItem can be initialized with a DomItem, and provides also the methods to modify
        // the item. It keeps the OwningItem and the path to the current item.
        // Mutability can invalidate pointers to non owning items (and thus DomItem).
        // For this reason one should not modify something that other code can have a DomItem
        // pointer to, the best practice is to make shared object immutable and never change them.
        // One should modify only a copy that is used only by a single thread, and
        // do not shared untils all modifications are done.
        // A MutableItem stays valid (or becomes Empty), but stays safe to use
        //
        // Assuming one guarantees that editing is ok, doing it in practice is just about using
        // MutableDomItem instead of DomItem
        // It is possible to simply initialize a mutable item with a DomItem
        DomItem origFile = tFile.fileObject();
        MutableDomItem myFile0(origFile);
        // Normally it is better to have a separate environment. Is possible to avoid re-reading
        // the files already read by sharing the Universe between two environments.
        // But normally it is better and just as safe to work on a copy, so that one can be sure
        // that no DomItem is kept by other code gets invalidated. The .makeCopy creates a deep
        // copy, and by default (DomItem::CopyOption::EnvConnected) creates an environment which to
        // takes all non local elements from the current environment (its parent environment) but
        // replaces the file object with the copy. When finished one can replace the file object of
        // the parent with the new one using .commitToBase().
        MutableDomItem myFile = origFile.makeCopy();
        Q_ASSERT(myFile.ownerAs<QmlFile>()
                 && myFile.ownerAs<QmlFile>() != myFile0.ownerAs<QmlFile>());
        Q_ASSERT(myFile.environment().ownerAs<DomEnvironment>()
                 && myFile.environment().ownerAs<DomEnvironment>()
                         != myFile0.environment().ownerAs<DomEnvironment>());
        // we can check that the two files are really identical (.item() give back the DomItem of
        // a MutableDomItem
        Q_ASSERT(domCompareStrList(origFile, myFile, FieldFilter::compareFilter()).isEmpty());
        // MutableDomItem has the same methods as DomItem
        MutableDomItem qmlObj = myFile.qmlObject(GoTo::MostLikely);
        MutableDomItem qmlObj2 = myFile.field(Fields::components)
                                         .key(QString())
                                         .index(0)
                                         .field(Fields::objects)
                                         .index(0);
        Q_ASSERT(qmlObj && qmlObj == qmlObj2);
        qDebug() << "mutable qmlObj has canonicalPath " << qmlObj.canonicalPath();
        // but it adds methods to add
        // * new PropertyDefinitions
        PropertyDefinition b;
        b.name = QLatin1String("xx");
        b.typeName = QLatin1String("int");
        // if we make t true we also have to give a value...
        MutableDomItem addedPDef = qmlObj.addPropertyDef(b);
        qDebug() << "added property definition at:" << addedPDef.pathFromOwner();
        // * new bindings
        MutableDomItem addedBinding0 = qmlObj.addBinding(
                Binding("height",
                        std::shared_ptr<ScriptExpression>(new ScriptExpression(
                                QStringLiteral(u"243"),
                                ScriptExpression::ExpressionType::BindingExpression))));
        // by default addBinding, addPropertyDef and addMethod have the AddOption::Override
        // to make it more difficult to create invalid documents, so that only the
        // following binding remains (where we use the convenience constructor that constucts
        // the ScriptExpression internally
        MutableDomItem addedBinding = qmlObj.addBinding(Binding("height", QStringLiteral(u"242")));
        qDebug() << "added binding at:" << addedBinding.pathFromOwner();
        // * new methods
        MethodInfo mInfo;
        mInfo.name = QLatin1String("foo2");
        MethodParameter param;
        param.name = QLatin1String("x");
        mInfo.parameters.append(param);
        mInfo.setCode(QLatin1String("return 4*10+2 - x"));
        // we can change the added binding
        addedBinding.setCode(QLatin1String("245"));
        MutableDomItem addedMethod = qmlObj.addMethod(mInfo);
        qDebug() << "added method at:" << addedMethod.pathFromOwner();
        // * new QmlObjects
        QmlObject subObj;
        subObj.setName(QLatin1String("Item"));
        MutableDomItem addedSubObj = qmlObj.addChild(subObj);
        qDebug() << "added subObject at:" << addedMethod.pathFromOwner();
        // It is possible to modify the content of objects, using the mutableAs method
        if (PropertyDefinition *addedPDefPtr = addedPDef.mutableAs<PropertyDefinition>()) {
            addedPDefPtr->isRequired = true;
        }
        MutableDomItem firstChild = qmlObj.child(0);
        qDebug() << "firstChild:" << firstChild;
        // It is possible remove objects
        if (QmlObject *qmlObjPtr = qmlObj.mutableAs<QmlObject>()) {
            QList<QmlObject> children = qmlObjPtr->children();
            children.removeAt(0);
            qmlObjPtr->setChildren(children);
        }
        // But as MutableDomItem does not keep the identity, just the same position, the addedSubObj
        // becomes invalid (and firstChild changes)
        qDebug() << "after removal firstChild:" << firstChild;
        qDebug() << "addedSubObj becomes invalid:" << addedSubObj;
        qDebug() << "But the last object is the added one:"
                 << qmlObj.child(qmlObj.children().indexes() - 1);

        // now origFile are different
        Q_ASSERT(!domCompareStrList(origFile, myFile, FieldFilter::compareFilter()).isEmpty());
        // and we can look at the places where they differ
        qDebug().noquote().nospace()
                << "Edits introduced the following diffs (ignoring file locations"
                << " and thus whitespace/reformatting changes):\n"
                << domCompareStrList(origFile, myFile, FieldFilter::noLocationFilter(),
                                     DomCompareStrList::AllDiffs)
                           .join(QString());

        QString reformattedFilePath =
                QDir(QDir::tempPath())
                        .filePath(QStringLiteral(u"edited") + QFileInfo(testFilePath).baseName()
                                  + QLatin1String(".qml"));
        MutableDomItem reformattedEditedFile = myFile.writeOut(reformattedFilePath);
        // the reformatted edited file might be different from the edited file
        // but the differences are just in file location/formatting
        Q_ASSERT(domCompareStrList(myFile, reformattedEditedFile, FieldFilter::noLocationFilter())
                         .isEmpty());

        qDebug() << "The edited file was written at " << reformattedFilePath;
        QString dumpFilePath =
                QDir(QDir::tempPath())
                        .filePath(QStringLiteral(u"edited0") + QFileInfo(testFilePath).baseName()
                                  + QLatin1String(".dump.json"));
        myFile.dump(dumpFilePath);
        qDebug() << "The non reformatted edited file was dumped at " << dumpFilePath;
        QString reformattedDumpFilePath =
                QDir(QDir::tempPath())
                        .filePath(QStringLiteral(u"edited") + QFileInfo(testFilePath).baseName()
                                  + QLatin1String(".dump.json"));
        reformattedEditedFile.dump(reformattedDumpFilePath);
        qDebug() << "The edited file was dumped at " << reformattedDumpFilePath;
        // The top environment still contains the original loaded file
        Q_ASSERT(origFile.ownerAs<QmlFile>() != reformattedEditedFile.ownerAs<QmlFile>());
        Q_ASSERT(tFile.fileObject().refreshed().ownerAs<QmlFile>());
        Q_ASSERT(tFile.fileObject().refreshed().ownerAs<QmlFile>() == origFile.ownerAs<QmlFile>());
        Q_ASSERT(tFile.fileObject().ownerAs<QmlFile>() == origFile.ownerAs<QmlFile>());
        Q_ASSERT(tFile.fileObject().refreshed().ownerAs<QmlFile>()
                 != reformattedEditedFile.ownerAs<QmlFile>());
        // we can commit the reformatted file
        if (!reformattedEditedFile.commitToBase()) {
            qWarning() << "No reformatted file to commit";
        }
        // myFile might not be the same (If and updated check is requested, not the case here)
        if (myFile.ownerAs<QmlFile>() != reformattedEditedFile.ownerAs<QmlFile>()
            && !myFile.commitToBase()) {
            qWarning() << "Could not commit edited file";
        }
        // but refreshing it (looking up its canonical path) we always find the updated file
        Q_ASSERT(myFile.refreshed().ownerAs<QmlFile>() == reformattedEditedFile.ownerAs<QmlFile>());
        Q_ASSERT(tFile.fileObject().refreshed().ownerAs<QmlFile>()
                 == reformattedEditedFile.ownerAs<QmlFile>());
    }
}
