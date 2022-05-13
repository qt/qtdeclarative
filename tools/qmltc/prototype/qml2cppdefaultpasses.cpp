// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qml2cppdefaultpasses.h"
#include "qmltcpropertyutils.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qqueue.h>
#include <QtCore/qloggingcategory.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcDefaultPasses, "qml.qmltc.compilerpasses", QtWarningMsg);

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
