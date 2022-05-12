/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "prototype/codegenerator.h"
#include "prototype/qml2cppdefaultpasses.h"
#include "qmltcpropertyutils.h"
#include "qmltccodewriter.h"
#include "qmltccompilerpieces.h"

#include "qmltccompiler.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qregularexpression.h>

#include <QtCore/qloggingcategory.h>

#include <private/qqmljsutils_p.h>

#include <optional>
#include <utility>
#include <numeric>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcCodeGenerator, "qml.qmltc.compiler", QtWarningMsg);

CodeGenerator::CodeGenerator(const QString &url, QQmlJSLogger *logger, QmlIR::Document *doc,
                             const QmltcTypeResolver *localResolver, const QmltcVisitor *visitor,
                             const QmltcCompilerInfo *info)
    : m_url(url),
      m_logger(logger),
      m_doc(doc),
      m_localTypeResolver(localResolver),
      m_visitor(visitor),
      m_info(info)
{
    Q_ASSERT(m_info);
    Q_ASSERT(!m_info->outputHFile.isEmpty());
    Q_ASSERT(!m_info->outputCppFile.isEmpty());
}

void CodeGenerator::constructObjects(QSet<QString> &requiredCppIncludes)
{
    const auto &objects = m_doc->objects;
    m_objects.reserve(objects.size());

    m_typeToObjectIndex.reserve(objects.size());

    for (qsizetype objectIndex = 0; objectIndex != objects.size(); ++objectIndex) {
        QmlIR::Object *irObject = objects[objectIndex];
        if (!irObject) {
            recordError(QQmlJS::SourceLocation {},
                        u"Internal compiler error: IR object is null"_s);
            return;
        }
        QQmlJSScope::Ptr object = m_localTypeResolver->scopeForLocation(irObject->location);
        if (!object) {
            recordError(irObject->location, u"Object of unknown type"_s);
            return;
        }
        m_typeToObjectIndex.insert(object, objectIndex);
        m_objects.emplaceBack(CodeGenObject { irObject, object });
    }

    // objects are constructed, now we can run compiler passes to make sure the
    // objects are in good state
    Qml2CppCompilerPassExecutor executor(m_doc, m_localTypeResolver, m_url, m_objects,
                                         m_typeToObjectIndex);
    executor.addPass(&verifyTypes);
    executor.addPass(&checkForNamingCollisionsWithCpp);
    executor.addPass(&makeUniqueCppNames);
    const auto setupQmlBaseTypes = [&](const Qml2CppContext &context,
                                       QList<Qml2CppObject> &objects) {
        m_qmlCompiledBaseTypes = setupQmlCppTypes(context, objects);
    };
    executor.addPass(setupQmlBaseTypes);
    executor.addPass(&deferredResolveValidateAliases);
    const auto populateCppIncludes = [&](const Qml2CppContext &context,
                                         QList<Qml2CppObject> &objects) {
        requiredCppIncludes = findCppIncludes(context, objects);
    };
    executor.addPass(populateCppIncludes);
    const auto resolveExplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveExplicitComponents(context, objects));
    };
    const auto resolveImplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveImplicitComponents(context, objects));
    };
    executor.addPass(resolveExplicitComponents);
    executor.addPass(resolveImplicitComponents);
    executor.addPass(&setObjectIds); // NB: must be after Component resolution
    const auto setIgnoredTypes = [&](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_ignoredTypes = collectIgnoredTypes(context, objects);
    };
    executor.addPass(setIgnoredTypes);

    // run all passes:
    executor.run(m_logger);
}

void CodeGenerator::prepare(QSet<QString> *cppIncludes)
{
    constructObjects(*cppIncludes); // this populates all the codegen objects
}

bool CodeGenerator::ignoreObject(const CodeGenObject &object) const
{
    if (m_ignoredTypes.contains(object.type)) {
        // e.g. object.type is a view delegate
        qCDebug(lcCodeGenerator) << u"Scope '" + object.type->internalName()
                        + u"' is a QQmlComponent sub-component. It won't be compiled to "
                          u"C++.";
        return true;
    }
    return false;
}

void CodeGenerator::recordError(const QQmlJS::SourceLocation &location, const QString &message)
{
    m_logger->log(message, Log_Compiler, location);
}

void CodeGenerator::recordError(const QV4::CompiledData::Location &location, const QString &message)
{
    recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message);
}

QT_END_NAMESPACE
