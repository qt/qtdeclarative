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

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "qmltctyperesolver.h"
#include "qmltcoutputir.h"
#include "prototype/qml2cppcontext.h"

#include <QtCore/qlist.h>
#include <QtCore/qqueue.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>

#include <variant>
#include <utility>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo;
class CodeGenerator
{
public:
    CodeGenerator(const QString &url, QQmlJSLogger *logger, QmlIR::Document *doc,
                  const QmltcTypeResolver *localResolver, const QmltcVisitor *visitor,
                  const QmltcCompilerInfo *info);

    // TODO: this should really be just QQmlJSScope::ConstPtr (and maybe C++
    // class name), but bindings are currently not represented in QQmlJSScope,
    // so have to keep track of QmlIR::Object and consequently some extra stuff
    using CodeGenObject = Qml2CppObject;

    // initializes code generator
    void prepare(QSet<QString> *cppIncludes);

    const QList<CodeGenObject> &objects() const { return m_objects; }
    bool ignoreObject(const CodeGenObject &object) const;

    qsizetype codegenObjectIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(m_typeToObjectIndex.contains(type));
        return m_typeToObjectIndex[type];
    }

    const CodeGenObject &objectFromType(const QQmlJSScope::ConstPtr &type) const
    {
        return m_objects[codegenObjectIndex(type)];
    }

    QString stringAt(int index) const { return m_doc->stringAt(index); }

    // QmlIR::Binding-specific sort function
    static QList<typename QmlIR::PoolList<QmlIR::Binding>::Iterator>
    toOrderedSequence(typename QmlIR::PoolList<QmlIR::Binding>::Iterator first,
                      typename QmlIR::PoolList<QmlIR::Binding>::Iterator last, qsizetype n);

    bool hasQmlCompiledBaseType(const QQmlJSScope::ConstPtr &type) const
    {
        return m_qmlCompiledBaseTypes.contains(type->baseTypeName());
    }

private:
    QString m_url; // document url
    QQmlJSLogger *m_logger = nullptr;
    QmlIR::Document *m_doc = nullptr;
    const QmltcTypeResolver *m_localTypeResolver = nullptr;
    const QmltcVisitor *m_visitor = nullptr;

    const QmltcCompilerInfo *m_info = nullptr;

    // convenient object abstraction, laid out as QmlIR::Document.objects
    QList<CodeGenObject> m_objects;
    // mapping from type to m_objects index
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_typeToObjectIndex; // TODO: remove this
    // mapping from component-wrapped object to component index (real or not)
    QHash<int, int> m_componentIndices;
    // types ignored by the code generator
    QSet<QQmlJSScope::ConstPtr> m_ignoredTypes;

    // native QML base type names of the to-be-compiled objects which happen to
    // also be generated (but separately)
    QSet<QString> m_qmlCompiledBaseTypes;

    // init function that constructs m_objects
    void constructObjects(QSet<QString> &requiredCppIncludes);

private:
    // helper methods:
    void recordError(const QQmlJS::SourceLocation &location, const QString &message);
    void recordError(const QV4::CompiledData::Location &location, const QString &message);
};

QT_END_NAMESPACE

#endif // CODEGENERATOR_H
