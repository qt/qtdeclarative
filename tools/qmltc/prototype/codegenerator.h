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

#include "prototype/typeresolver.h"
#include "prototype/qmlcompiler.h"
#include "prototype/generatedcodeprimitives.h"
#include "prototype/qml2cppcontext.h"

#include <QtCore/qlist.h>
#include <QtCore/qqueue.h>
#include <QtCore/qhash.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljstyperesolver_p.h>

#include <variant>
#include <utility>

class CodeGenerator
{
public:
    CodeGenerator(const QString &url, QQmlJSLogger *logger, QmlIR::Document *doc,
                  const Qmltc::TypeResolver *localResolver);

    // main function: given compilation options, generates C++ code (implicitly)
    void generate(const Options &options);

    // TODO: this should really be just QQmlJSScope::ConstPtr (and maybe C++
    // class name), but bindings are currently not represented in QQmlJSScope,
    // so have to keep track of QmlIR::Object and consequently some extra stuff
    using CodeGenObject = Qml2CppObject;

private:
    QString m_url; // document url
    QQmlJSLogger *m_logger = nullptr;
    QmlIR::Document *m_doc = nullptr;
    const Qmltc::TypeResolver *m_localTypeResolver = nullptr;
    QStringList m_qmlSource; // QML source code split to lines

    Options m_options = {}; // compilation options

    // convenient object abstraction, laid out as QmlIR::Document.objects
    QList<CodeGenObject> m_objects;
    // mapping from type to m_objects index
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_typeToObjectIndex; // TODO: remove this
    // parents for each type that will (also) create the type
    QHash<QQmlJSScope::ConstPtr, QQmlJSScope::ConstPtr> m_immediateParents;
    // mapping from component-wrapped object to component index (real or not)
    QHash<int, int> m_componentIndices;
    // types ignored by the code generator
    QSet<QQmlJSScope::ConstPtr> m_ignoredTypes;

    QQmlJSAotMethod m_urlMethod;

    // helper struct used for unique string generation
    struct UniqueStringId
    {
        QString combined;
        UniqueStringId(const QQmlJSAotObject &compiled, const QString &value)
            : combined(compiled.cppType + u"_" + value)
        {
            Q_ASSERT(!compiled.cppType.isEmpty());
        }
        operator QString() { return combined; }

        friend bool operator==(const UniqueStringId &x, const UniqueStringId &y)
        {
            return x.combined == y.combined;
        }
        friend bool operator!=(const UniqueStringId &x, const UniqueStringId &y)
        {
            return !(x == y);
        }
        friend bool operator<(const UniqueStringId &x, const UniqueStringId &y)
        {
            return x.combined < y.combined;
        }
        friend size_t qHash(const UniqueStringId &x) { return qHash(x.combined); }
    };

    // QML attached types that are already added as member variables (faster lookup)
    QSet<UniqueStringId> m_attachedTypesAlreadyRegistered;

    // machinery for unique names generation
    QHash<QString, qsizetype> m_typeCounts;
    QString makeGensym(const QString &base);

    // crutch to remember QQmlListReference names, the unique naming convention
    // is used for these so there's never a conflict
    QSet<UniqueStringId> m_listReferencesCreated;

    // native QML base type names of the to-be-compiled objects which happen to
    // also be generated (but separately)
    QSet<QString> m_qmlCompiledBaseTypes;

    // a set of objects that participate in "on" assignments
    QSet<UniqueStringId> m_onAssignmentObjectsCreated;

    // a vector of names of children that need to be end-initialized within type
    QList<QString> m_localChildrenToEndInit;
    // a vector of children that need to be finalized within type
    QList<QQmlJSScope::ConstPtr> m_localChildrenToFinalize;

    // a crutch (?) to enforce special generated code for aliases to ids, for
    // example: `property alias p: root`
    QSet<QQmlJSMetaProperty> m_aliasesToIds;

    // init function that constructs m_objects
    void constructObjects(QSet<QString> &requiredCppIncludes);

    bool m_isAnonymous = false; // crutch to distinguish QML_ELEMENT from QML_ANONYMOUS

    // code compilation functions that produce "compiled" entities
    void
    compileObject(QQmlJSAotObject &current, const CodeGenObject &object,
                  std::function<void(QQmlJSAotObject &, const CodeGenObject &)> compileElements);
    void compileObjectElements(QQmlJSAotObject &current, const CodeGenObject &object);
    void compileQQmlComponentElements(QQmlJSAotObject &current, const CodeGenObject &object);

    void compileEnum(QQmlJSAotObject &current, const QQmlJSMetaEnum &e);
    void compileProperty(QQmlJSAotObject &current, const QQmlJSMetaProperty &p,
                         const QQmlJSScope::ConstPtr &owner);
    void compileAlias(QQmlJSAotObject &current, const QQmlJSMetaProperty &alias,
                      const QQmlJSScope::ConstPtr &owner);
    void compileMethod(QQmlJSAotObject &current, const QQmlJSMetaMethod &m,
                       const QmlIR::Function *f, const CodeGenObject &object);
    void compileUrlMethod(); // special case

    // helper structure that holds the information necessary for most bindings,
    // such as accessor name, which is used to reference the properties like:
    // (accessor.name)->(propertyName): this->myProperty. it is also used in
    // more advanced scenarios by attached and grouped properties
    struct AccessorData
    {
        QQmlJSScope::ConstPtr scope; // usually object.type
        QString name; // usually "this"
        QString propertyName; // usually empty
        bool isValueType = false; // usually false
    };
    void compileBinding(QQmlJSAotObject &current, const QmlIR::Binding &binding,
                        const CodeGenObject &object, const AccessorData &accessor);
    // special case (for simplicity)
    void compileScriptBinding(QQmlJSAotObject &current, const QmlIR::Binding &binding,
                              const QString &bindingSymbolName, const CodeGenObject &object,
                              const QString &propertyName,
                              const QQmlJSScope::ConstPtr &propertyType,
                              const AccessorData &accessor);

    // TODO: remove this special case
    void compileScriptBindingOfComponent(QQmlJSAotObject &current, const QmlIR::Object *object,
                                         const QQmlJSScope::ConstPtr objectType,
                                         const QmlIR::Binding &binding,
                                         const QString &propertyName);

    // helper methods:
    void recordError(const QQmlJS::SourceLocation &location, const QString &message);
    void recordError(const QV4::CompiledData::Location &location, const QString &message);
};

#endif // CODEGENERATOR_H
