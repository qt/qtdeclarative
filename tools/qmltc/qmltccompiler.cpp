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

#include "qmltccompiler.h"
#include "qmltcoutputir.h"
#include "qmltccodewriter.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQmltcCompiler, "qml.qmltc.compiler", QtWarningMsg);

QmltcCompiler::QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QmltcVisitor *visitor,
                             QQmlJSLogger *logger)
    : m_url(url), m_typeResolver(resolver), m_visitor(visitor), m_logger(logger)
{
    Q_UNUSED(m_typeResolver);
    Q_ASSERT(!hasErrors());
}

void QmltcCompiler::compile(const QmltcCompilerInfo &info)
{
    m_info = info;
    Q_ASSERT(!m_info.outputCppFile.isEmpty());
    Q_ASSERT(!m_info.outputHFile.isEmpty());
    Q_ASSERT(!m_info.resourcePath.isEmpty());

    const QList<QQmlJSScope::ConstPtr> types = m_visitor->qmlScopes();
    QList<QmltcType> compiledTypes;
    compiledTypes.reserve(types.size());

    for (const QQmlJSScope::ConstPtr &type : types) {
        compiledTypes.emplaceBack(); // creates empty type
        compileType(compiledTypes.back(), type);
        if (hasErrors())
            return;
    }

    QmltcProgram program;
    program.url = m_url;
    program.cppPath = m_info.outputCppFile;
    program.hPath = m_info.outputHFile;
    program.compiledTypes = compiledTypes;
    program.includes = m_visitor->cppIncludeFiles();

    QmltcOutput out;
    QmltcOutputWrapper code(out);
    QmltcCodeWriter::write(code, program);
}

void QmltcCompiler::compileType(QmltcType &current, const QQmlJSScope::ConstPtr &type)
{
    if (type->isSingleton()) {
        recordError(type->sourceLocation(), u"Singleton types are not supported"_qs);
        return;
    }

    Q_ASSERT(!type->internalName().isEmpty());
    current.cppType = type->internalName();
    Q_ASSERT(!type->baseType()->internalName().isEmpty());
    const QString baseClass = type->baseType()->internalName();

    const auto rootType = m_visitor->result();
    const bool documentRoot = (type == rootType);
    const bool baseTypeIsCompiledQml = false; // TODO: support this in QmltcTypeResolver
    const bool isAnonymous = type->internalName().at(0).isLower();

    current.baseClasses = { baseClass };
    if (!documentRoot) {
        // make document root a friend to allow it to access init and finalize
        current.otherCode << u"friend class %1;"_qs.arg(rootType->internalName());
    } else {
        // make QQmltcObjectCreationBase<DocumentRoot> a friend to allow it to
        // be created for the root object
        current.otherCode << u"friend class QQmltcObjectCreationBase<%1>;"_qs.arg(
                rootType->internalName());

        current.typeCount = QmltcVariable { u"uint"_qs, u"q_qmltc_typeCount"_qs, QString() };
        Q_ASSERT(m_visitor->qmlScopes().size() > 0);
        QList<QQmlJSScope::ConstPtr> typesWithBaseTypeCount = m_visitor->qmlScopesWithQmlBases();
        QStringList typeCountComponents;
        typeCountComponents.reserve(1 + typesWithBaseTypeCount.size());
        // add this document's type counts minus document root
        typeCountComponents << QString::number(m_visitor->qmlScopes().size() - 1);
        for (const QQmlJSScope::ConstPtr &t : qAsConst(typesWithBaseTypeCount)) {
            if (t == type) { // t is this document's root
                typeCountComponents << t->baseTypeName() + u"::" + current.typeCount->name;
            } else {
                typeCountComponents << t->internalName() + u"::" + current.typeCount->name;
            }
        }
        current.typeCount->defaultValue = typeCountComponents.join(u" + "_qs);
    }

    current.mocCode = {
        u"Q_OBJECT"_qs,
        // Note: isAnonymous holds for non-root types in the document as well
        isAnonymous ? u"QML_ANONYMOUS"_qs : u"QML_ELEMENT"_qs,
    };

    // add special member functions
    current.basicCtor.access = QQmlJSMetaMethod::Protected;
    current.init.access = QQmlJSMetaMethod::Protected;
    current.finalize.access = QQmlJSMetaMethod::Protected;
    current.fullCtor.access = QQmlJSMetaMethod::Public;

    current.basicCtor.name = current.cppType;
    current.fullCtor.name = current.cppType;
    current.init.name = u"qmltc_init"_qs;
    current.init.returnType = u"QQmlRefPointer<QQmlContextData>"_qs;
    current.finalize.name = u"qmltc_finalize"_qs;
    current.finalize.returnType = u"void"_qs;

    QmltcVariable creator(u"QQmltcObjectCreationHelper*"_qs, u"creator"_qs);
    QmltcVariable engine(u"QQmlEngine*"_qs, u"engine"_qs);
    QmltcVariable parent(u"QObject*"_qs, u"parent"_qs, u"nullptr"_qs);
    current.basicCtor.parameterList = { parent };
    QmltcVariable ctxtdata(u"const QQmlRefPointer<QQmlContextData>&"_qs, u"parentContext"_qs);
    QmltcVariable finalizeFlag(u"bool"_qs, u"canFinalize"_qs);
    if (documentRoot) {
        current.fullCtor.parameterList = { engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata, finalizeFlag };
        current.finalize.parameterList = { creator, engine, finalizeFlag };
    } else {
        current.fullCtor.parameterList = { creator, engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata };
        current.finalize.parameterList = { creator, engine };
    }

    current.fullCtor.initializerList = { current.basicCtor.name + u"(" + parent.name + u")" };
    if (baseTypeIsCompiledQml) {
        // call parent's (QML type's) basic ctor from this. that one will take
        // care about QObject::setParent()
        current.basicCtor.initializerList = { baseClass + u"(" + parent.name + u")" };
    } else {
        // default call to ctor is enough, but QQml_setParent_noEvent() is
        // needed (note: faster? version of QObject::setParent())
        current.basicCtor.body << u"QQml_setParent_noEvent(this, " + parent.name + u");";
    }

    // compilation stub:
    current.fullCtor.body << u"Q_UNUSED(engine);"_qs;
    current.init.body << u"Q_UNUSED(creator);"_qs;
    current.init.body << u"Q_UNUSED(engine);"_qs;
    current.init.body << u"Q_UNUSED(parentContext);"_qs;
    current.finalize.body << u"Q_UNUSED(engine);"_qs;
    current.finalize.body << u"Q_UNUSED(creator);"_qs;
    if (documentRoot) {
        current.fullCtor.body << u"// document root:"_qs;
        // if it's document root, we want to create our QQmltcObjectCreationBase
        // that would store all the created objects
        current.fullCtor.body << u"QQmltcObjectCreationBase<%1> objectHolder;"_qs.arg(
                type->internalName());
        current.fullCtor.body << u"QQmltcObjectCreationHelper creator = objectHolder.view();"_qs;
        // now call init
        current.fullCtor.body << current.init.name
                        + u"(&creator, engine, QQmlContextData::get(engine->rootContext()), /* "
                          u"finalize */ true);";

        current.init.body << u"Q_UNUSED(canFinalize);"_qs;
        current.finalize.body << u"Q_UNUSED(canFinalize);"_qs;
    } else {
        current.fullCtor.body << u"// not document root:"_qs;
        // just call init, we don't do any setup here otherwise
        current.fullCtor.body << current.init.name
                        + u"(creator, engine, QQmlData::get(parent)->outerContext);";
    }
    current.init.body << u"return nullptr;"_qs;
}

QT_END_NAMESPACE
