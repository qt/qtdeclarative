/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qml2cppdefaultpasses.h"
#include "qmltcpropertyutils.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qqueue.h>
#include <QtCore/qloggingcategory.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString const cppKeywords[] = {
    u"alignas"_s,
    u"alignof"_s,
    u"and"_s,
    u"and_eq"_s,
    u"asm"_s,
    u"atomic_cancel"_s,
    u"atomic_commit"_s,
    u"atomic_noexcept"_s,
    u"auto"_s,
    u"bitand"_s,
    u"bitor"_s,
    u"bool"_s,
    u"break"_s,
    u"case"_s,
    u"catch"_s,
    u"char"_s,
    u"char8_t"_s,
    u"char16_t"_s,
    u"char32_t"_s,
    u"class"_s,
    u"compl"_s,
    u"concept"_s,
    u"const"_s,
    u"consteval"_s,
    u"constexpr"_s,
    u"const_cast"_s,
    u"continue"_s,
    u"co_await"_s,
    u"co_return"_s,
    u"co_yield"_s,
    u"decltype"_s,
    u"default"_s,
    u"delete"_s,
    u"do"_s,
    u"double"_s,
    u"dynamic_cast"_s,
    u"else"_s,
    u"enum"_s,
    u"explicit"_s,
    u"export"_s,
    u"extern"_s,
    u"false"_s,
    u"float"_s,
    u"for"_s,
    u"friend"_s,
    u"goto"_s,
    u"if"_s,
    u"inline"_s,
    u"int"_s,
    u"long"_s,
    u"mutable"_s,
    u"namespace"_s,
    u"new"_s,
    u"noexcept"_s,
    u"not"_s,
    u"not_eq"_s,
    u"nullptr"_s,
    u"operator"_s,
    u"or"_s,
    u"or_eq"_s,
    u"private"_s,
    u"protected"_s,
    u"public"_s,
    u"reflexpr"_s,
    u"register"_s,
    u"reinterpret_cast"_s,
    u"requires"_s,
    u"return"_s,
    u"short"_s,
    u"signed"_s,
    u"sizeof"_s,
    u"static"_s,
    u"static_assert"_s,
    u"static_cast"_s,
    u"struct"_s,
    u"switch"_s,
    u"synchronized"_s,
    u"template"_s,
    u"this"_s,
    u"thread_local"_s,
    u"throw"_s,
    u"true"_s,
    u"try"_s,
    u"typedef"_s,
    u"typeid"_s,
    u"typename"_s,
    u"union"_s,
    u"unsigned"_s,
    u"using"_s,
    u"virtual"_s,
    u"void"_s,
    u"volatile"_s,
    u"wchar_t"_s,
    u"while"_s,
    u"xor"_s,
    u"xor_eq"_s,
};

static bool isReservedWord(QStringView word)
{
    if (word.startsWith(QChar(u'_')) && word.size() >= 2
        && (word[1].isUpper() || word[1] == QChar(u'_'))) {
        return true; // Indentifiers starting with underscore and uppercase are reserved in C++
    }
    return std::binary_search(std::begin(cppKeywords), std::end(cppKeywords), word);
}

Q_LOGGING_CATEGORY(lcDefaultPasses, "qml.qmltc.compilerpasses", QtWarningMsg);

// verify that object's strings are not reserved C++ words:
// * enumeration name (because enum name stays "as is") and keys
// * own property names
// * own method names and method parameter names
// * TODO: bindings are ignored
//
// additionally, verify that no redefinition happens (e.g. no two properties
// named the same way, etc.)
static void checkObjectStringsForCollisions(const Qml2CppContext &context,
                                            const QQmlJSScope::Ptr &type)
{
    const auto isValidName = [&](QStringView name, auto getLocation, const QString &errorPrefix,
                                 QSet<QStringView> &seenSymbols) {
        // check name (e.g. property name) for correctness
        const bool isReserved = isReservedWord(name);
        const bool isSeenBefore = seenSymbols.contains(name);
        // getLocation() might be slow, so only use it once if there is an error
        decltype(getLocation()) location {}; // stub value
        if (isReserved || isSeenBefore)
            location = getLocation();

        if (isReserved) {
            context.recordError(location,
                                errorPrefix + u" '" + name
                                        + u"' is a reserved C++ word, consider renaming");
        }
        if (isSeenBefore) {
            context.recordError(location, errorPrefix + u" with this name already exists");
        } else {
            seenSymbols.insert(name);
        }
    };

    QSet<QStringView> uniqueSymbols;

    const auto enums = type->ownEnumerations();
    for (auto it = enums.cbegin(); it != enums.cend(); ++it) {
        const QQmlJSMetaEnum e = it.value();
        QStringView name = e.name();
        const auto getEnumLocation = [&]() { return type->sourceLocation(); };
        isValidName(name, getEnumLocation, u"Enumeration"_s, uniqueSymbols);

        const auto enumKeys = e.keys();
        for (const auto &key : enumKeys) {
            const auto getEnumKeyLocation = [&]() { return type->sourceLocation(); };
            // no support for enum classes: each key is visible outside of enum
            isValidName(key, getEnumKeyLocation, u"Enumeration key"_s, uniqueSymbols);
        }
    }

    const auto properties = type->ownProperties();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        const QQmlJSMetaProperty &p = it.value();
        QStringView name = p.propertyName();
        const auto getPropertyLocation = [&]() { return p.type()->sourceLocation(); };
        isValidName(name, getPropertyLocation, u"Property"_s, uniqueSymbols);
    }

    const auto methods = type->ownMethods();
    for (auto it = methods.cbegin(); it != methods.cend(); ++it) {
        const QQmlJSMetaMethod &m = it.value();
        QStringView name = m.methodName();
        const auto getMethodLocation = [&]() { return m.returnType()->sourceLocation(); };
        isValidName(name, getMethodLocation, u"Method"_s, uniqueSymbols);

        const auto parameterNames = m.parameterNames();
        QSet<QStringView> uniqueParameters; // parameters can shadow
        for (qsizetype i = 0; i < parameterNames.size(); ++i) {
            QStringView paramName = parameterNames.at(i);
            const auto getParamLocation = [&]() {
                static auto paramTypes = m.parameterTypes();
                return paramTypes.at(i)->sourceLocation();
            };
            isValidName(paramName, getParamLocation, u"Parameter"_s, uniqueParameters);
        }
    }
}

void checkForNamingCollisionsWithCpp(const Qml2CppContext &context,
                                     QList<QQmlJSScope::Ptr> &objects)
{
    for (const auto &object : objects)
        checkObjectStringsForCollisions(context, object);
}

static void updateInternalName(QQmlJSScope::Ptr root, QString prefix,
                               QHash<QString, qsizetype> &typeCounts)
{
    if (!root)
        return;

    // gives unique C++ class name for a basic name based on type occurrence,
    // additionally updating the type occurrence
    const auto uniqueCppClassName = [&](QString basicName) -> QString {
        if (!typeCounts.contains(basicName)) {
            typeCounts.insert(basicName, 1);
            return basicName;
        }
        return basicName + u"_" + QString::number(typeCounts[basicName]++);
    };

    switch (root->scopeType()) {
    case QQmlJSScope::AttachedPropertyScope: {
        // special case
        Q_ASSERT(!root->baseTypeName().isEmpty());
        root->setInternalName(root->baseTypeName());
        break;
    }
    // case QQmlJSScope::JSFunctionScope:
    // case QQmlJSScope::JSLexicalScope:
    // case QQmlJSScope::GroupedPropertyScope:
    // case QQmlJSScope::QMLScope:
    // case QQmlJSScope::EnumScope:
    default: {
        root->setInternalName(prefix);
        break;
    }
    }

    const QList<QQmlJSScope::Ptr> children = root->childScopes();
    for (QQmlJSScope::Ptr child : children) {
        updateInternalName(child,
                           uniqueCppClassName(root->internalName() + u"_" + child->baseTypeName()),
                           typeCounts);
    }
}

void makeUniqueCppNames(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects)
{
    Q_UNUSED(objects);

    QHash<QString, qsizetype> typeCounts;
    for (const QString &str : cppKeywords)
        typeCounts.insert(str, 1);

    // root is special:
    QQmlJSScope::Ptr root = context.typeResolver->root();
    QFileInfo fi(context.documentUrl);
    auto cppName = fi.baseName();
    if (typeCounts.contains(cppName)) {
        context.recordError(root->sourceLocation(),
                            u"Root object name '" + cppName + u"' is reserved");
        return;
    }
    if (cppName.isEmpty()) {
        context.recordError(root->sourceLocation(), u"Root object's name is empty"_s);
        return;
    }
    typeCounts.insert(cppName, 1);

    updateInternalName(root, cppName, typeCounts);
}

static void setupQmlCppType(const Qml2CppContext &context, const QQmlJSScope::Ptr &type,
                            const QString &filePath)
{
    Q_ASSERT(type);
    if (filePath.isEmpty()) {
        context.recordError(type->sourceLocation(), u"QML type has unknown file path"_s);
        return;
    }
    if (type->filePath().endsWith(u".h")) // consider this one to be already set up
        return;
    if (!filePath.endsWith(u".qml"_s)) {
        context.recordError(type->sourceLocation(),
                            u"QML type has non-QML origin (internal error)"_s);
        return;
    }

    // TODO: this does not cover QT_QMLTC_FILE_BASENAME renaming
    if (filePath != context.documentUrl) {
        // this file name will be discovered during findCppIncludes
        type->setFilePath(QFileInfo(filePath).baseName().toLower() + u".h"_s);
    }

    const auto properties = type->ownProperties();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        QQmlJSMetaProperty p = it.value();
        Q_ASSERT(it.key() == p.propertyName());

        if (p.isAlias()) // we'll process aliases separately
            continue;

        QmltcPropertyData compiledData(p);
        if (p.read().isEmpty())
            p.setRead(compiledData.read);
        if (p.write().isEmpty() && p.isWritable() && !p.isList())
            p.setWrite(compiledData.write);
        if (p.bindable().isEmpty() && !p.isList())
            p.setBindable(compiledData.bindable);
        // TODO: p.setNotify(compiledData.notify); - ?
        type->addOwnProperty(p);
    }
}

void setupQmlCppTypes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects)
{
    // TODO: in general, the whole logic here is incomplete. it will suffice as
    // long as we only import QML types from our own module and C++ types from
    // any module. importing QML types (that are presumably compiled) from
    // external modules is not supported, so we can get away with it
    for (const auto &object : objects) {
        // 1. set up object itself
        setupQmlCppType(context, object, context.documentUrl);

        // 2. set up the base type if it is also QML originated
        if (auto base = object->baseType(); base->isComposite()) {
            auto pair = context.typeResolver->importedType(base);
            if (pair.first.isEmpty()) {
                context.recordError(object->sourceLocation(),
                                    u"QML base type has unknown origin. Do you miss an import?"_s);
                continue;
            }

            setupQmlCppType(context, pair.second, pair.first);
        }
    }
}

// TODO: this should really become a part of the visitor. otherwise, the
// to-be-compiled types from different documents do not get resolved aliases
// a.k.a. we have to use QObject::setProperty() instead.
static void resolveValidateOrSkipAlias(const Qml2CppContext &context,
                                       const QQmlJSScope::Ptr &aliasOwner, QQmlJSMetaProperty alias,
                                       QSet<QQmlJSMetaProperty> &unresolved)
{
    Q_ASSERT(alias.isAlias());
    Q_ASSERT(!alias.propertyName().isEmpty());
    // TODO: we might need some better location, but this is not
    // absolutely essential
    const auto loc = aliasOwner->sourceLocation();
    QStringList aliasExprBits = alias.aliasExpression().split(u'.');
    Q_ASSERT(aliasExprBits.size() > 1);

    bool canHaveWrite = true;

    QQmlJSMetaProperty p;
    QQmlJSScope::ConstPtr type = context.typeResolver->scopeForId(aliasExprBits[0], aliasOwner);
    Q_ASSERT(type);
    aliasExprBits.removeFirst();
    for (qsizetype i = 0; i < aliasExprBits.size(); ++i) {
        const QString &bit = qAsConst(aliasExprBits)[i];
        p = type->property(bit);
        Q_ASSERT(p.isValid() && p.type());

        const bool isYetToBeResolvedAlias = unresolved.contains(p);
        if (isYetToBeResolvedAlias) {
            // we can do nothing with `alias` at the moment, so just skip it for
            // now. the caller must then re-call this function, after `p` is
            // processed. we assume no cycles exist at this stage, so the
            // caller's code must terminate at some point.
            return;
        }

        struct Remover
        {
            QSet<QQmlJSMetaProperty> &unresolved;
            const QQmlJSMetaProperty &alias;
            bool needUpdate = true;
            ~Remover()
            {
                if (needUpdate)
                    unresolved.remove(alias);
            }
        } scopedRemover { unresolved, alias };

        // validate and resolve the alias:
        if (p.read().isEmpty()) { // we won't be able to read this property -- always an error
            context.recordError(loc,
                                u"Property '" + bit + u"' of '" + type->internalName()
                                        + u"' does not have READ method");
            return;
        }
        // NB: last iteration is special, don't check for WRITE as it might
        // not exist even for value property (in this case alias would only
        // have a READ method, which is fine)
        if (i == aliasExprBits.size() - 1)
            continue; // NB: this will trigger scopedRemover, which is good

        if (!p.isWritable()) {
            canHaveWrite = false;
            // check whether value type property has no WRITE method while it's
            // subproperties have WRITE - if so, this is likely at least a
            // warning/info
        }
        type = p.type();
        scopedRemover.needUpdate = false;
    }

    Q_ASSERT(!unresolved.contains(alias));

    // here, `p` is the actually aliased property, `type` is the owner of this
    // property

    QmltcPropertyData compiledData(alias);
    if (alias.read().isEmpty())
        alias.setRead(compiledData.read);
    // TODO: how to handle redefinition/overload?
    if (QString setName = p.write(); !setName.isEmpty()) {
        QList<QQmlJSMetaMethod> methods = type->methods(setName);
        if (methods.size() > 1) {
            context.recordError(loc,
                                u"WRITE method of aliased property '" + p.propertyName() + u"' of '"
                                        + type->internalName() + u"' is ambiguous");
        }
        if (canHaveWrite && alias.write().isEmpty())
            alias.setWrite(compiledData.write);
    }
    if (!p.bindable().isEmpty() && alias.bindable().isEmpty())
        alias.setBindable(compiledData.bindable);
    if (QString notifyName = p.notify(); !notifyName.isEmpty()) {
        QList<QQmlJSMetaMethod> methods = type->methods(notifyName);
        if (methods.size() > 1) {
            context.recordError(loc,
                                u"NOTIFY method of aliased property '" + p.propertyName()
                                        + u"' of '" + type->internalName() + u"' is ambiguous");
        }
        if (alias.notify().isEmpty())
            alias.setNotify(compiledData.notify);
    }
    aliasOwner->addOwnProperty(alias);
}

void deferredResolveValidateAliases(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects)
{
    QSet<QQmlJSMetaProperty> unresolved;
    for (const auto &type : objects) {
        Q_ASSERT(type); // assume verified

        const auto properties = type->ownProperties();
        for (QQmlJSMetaProperty p : properties) {
            if (!p.isAlias())
                continue;
            QStringList aliasExprBits = p.aliasExpression().split(u'.');
            Q_ASSERT(!aliasExprBits.isEmpty());
            if (aliasExprBits.size() == 1) { // special case
                Q_ASSERT(context.typeResolver->scopeForId(aliasExprBits.at(0), type));
                // since it points to an object, set only the essential stuff
                // and continue - it is already resolved after this
                QmltcPropertyData compiledData(p);
                if (p.read().isEmpty())
                    p.setRead(compiledData.read);
                // NB: id-pointing aliases are read-only
                type->addOwnProperty(p);
                continue;
            }
            unresolved.insert(p);
        }
    }

    // NB: assume no cycles at this stage - see
    // QQmlJSImportVisitor::resolveAliases()
    while (!unresolved.isEmpty()) {
        for (const auto &object : objects) {
            const auto properties = object->ownProperties();
            for (QQmlJSMetaProperty p : properties) {
                if (!unresolved.contains(p)) // only contains aliases, so p.isAlias() is redundant
                    continue;
                resolveValidateOrSkipAlias(context, object, p, unresolved);
            }
        }
    }
}

static void addFirstCppIncludeFromType(QSet<QString> &cppIncludes,
                                       const QQmlJSScope::ConstPtr &type)
{
    auto t = QQmlJSScope::nonCompositeBaseType(type);
    if (!t)
        return;
    if (QString includeFile = t->filePath(); includeFile.endsWith(u".h"))
        cppIncludes.insert(includeFile);
}

static void populateCppIncludes(QSet<QString> &cppIncludes, const QQmlJSScope::ConstPtr &type)
{
    const auto constructPrivateInclude = [](QStringView publicInclude) -> QString {
        if (publicInclude.isEmpty())
            return QString();
        Q_ASSERT(publicInclude.endsWith(u".h"_s) || publicInclude.endsWith(u".hpp"_s));
        const qsizetype dotLocation = publicInclude.endsWith(u".h"_s) ? publicInclude.size() - 2
                                                                       : publicInclude.size() - 4;
        QStringView extension = publicInclude.sliced(dotLocation);
        QStringView includeWithoutExtension = publicInclude.first(dotLocation);
        // check if the "public" include is actually private
        if (includeWithoutExtension.startsWith(u"private"))
            return includeWithoutExtension.toString() + u"_p" + extension.toString();
        return u"private/" + includeWithoutExtension.toString() + u"_p" + extension.toString();
    };

    // TODO: this pass is VERY slow - we have to do exhaustive search, however,
    // because some classes could do forward declarations

    // look in type itself
    // addFirstCppIncludeFromType(cppIncludes, type);

    // look in type hierarchy
    for (auto t = type; t; t = t->baseType()) {
        // NB: Composite types might have include files - this is custom qmltc
        // logic for local imports
        if (QString includeFile = t->filePath(); includeFile.endsWith(u".h"))
            cppIncludes.insert(includeFile);

        // look in property types
        const auto properties = t->ownProperties();
        for (const QQmlJSMetaProperty &p : properties) {
            addFirstCppIncludeFromType(cppIncludes, p.type());

            const auto baseType = QQmlJSScope::nonCompositeBaseType(t);

            if (p.isPrivate() && baseType->filePath().endsWith(u".h")) {
                const QString ownersInclude = baseType->filePath();
                QString privateInclude = constructPrivateInclude(ownersInclude);
                if (!privateInclude.isEmpty())
                    cppIncludes.insert(std::move(privateInclude));
            }
        }

        // look in method types
        const auto methods = t->ownMethods();
        for (const QQmlJSMetaMethod &m : methods) {
            addFirstCppIncludeFromType(cppIncludes, m.returnType());

            const auto parameters = m.parameterTypes();
            for (const auto &p : parameters)
                addFirstCppIncludeFromType(cppIncludes, p);
        }
    }
}

QSet<QString> findCppIncludes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects)
{
    Q_UNUSED(objects);
    QSet<QString> cppIncludes;

    QQueue<QQmlJSScope::ConstPtr> objectsQueue;
    objectsQueue.enqueue(context.typeResolver->root());

    while (!objectsQueue.isEmpty()) {
        QQmlJSScope::ConstPtr current = objectsQueue.dequeue();
        Q_ASSERT(current); // assume verified

        populateCppIncludes(cppIncludes, current);

        const auto children = current->childScopes();
        for (auto child : children)
            objectsQueue.enqueue(child);
    }

    return cppIncludes;
}

QT_END_NAMESPACE
