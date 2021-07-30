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

#ifndef QQMLJSTYPERESOLVER_P_H
#define QQMLJSTYPERESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsregistercontent_p.h"

#include <private/qqmljsscope_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSTypeResolver
{
public:
    enum BaseOrExtension { Base, Extension };
    enum Semantics { Static, Dynamic };
    enum TypeStorage { Direct, Indirect };

    struct GlobalProperty
    {
        QQmlJSScope::ConstPtr type;
        QString getter;
        QString setter;
    };

    QQmlJSTypeResolver(QQmlJSImporter *importer, const QmlIR::Document *document,
                       const QString &implicitImportDirectory, const QStringList &qmltypesFiles,
                       TypeStorage storage, Semantics semantics, QQmlJSLogger *logger);

    QQmlJSScope::ConstPtr voidType() const { return m_voidType; }
    QQmlJSScope::ConstPtr numberType() const { return m_numberType; }
    QQmlJSScope::ConstPtr realType() const { return m_realType; }
    QQmlJSScope::ConstPtr intType() const { return m_intType; }
    QQmlJSScope::ConstPtr boolType() const { return m_boolType; }
    QQmlJSScope::ConstPtr stringType() const { return m_stringType; }
    QQmlJSScope::ConstPtr urlType() const { return m_urlType; }
    QQmlJSScope::ConstPtr dateTimeType() const { return m_dateTimeType; }
    QQmlJSScope::ConstPtr variantListType() const { return m_variantListType; }
    QQmlJSScope::ConstPtr varType() const { return m_varType; }
    QQmlJSScope::ConstPtr jsValueType() const { return m_jsValueType; }
    QQmlJSScope::ConstPtr jsPrimitiveType() const { return m_jsPrimitiveType; }
    QQmlJSScope::ConstPtr listPropertyType() const { return m_listPropertyType; }

    QQmlJSScope::ConstPtr knownGlobalType(const QString &name) const
    {
        return m_knownGlobalTypes.value(name);
    }

    void addKnownGlobalType(const QString &name, const QQmlJSScope::ConstPtr &type)
    {
        m_knownGlobalTypes[name] = type;
    }

    QQmlJSImporter *importer() const { return m_importer; }

    QQmlJSScope::ConstPtr scopeForLocation(const QV4::CompiledData::Location &location) const;
    QQmlJSScope::ConstPtr scopeForId(const QString &id) const;

    bool isPrefix(const QString &name) { return m_imports.contains(name) && !m_imports[name]; }

    QQmlJSScope::ConstPtr typeForName(const QString &name) const { return m_imports[name]; }
    QQmlJSScope::ConstPtr typeFromAST(QQmlJS::AST::Type *type) const;
    QQmlJSScope::ConstPtr typeForConst(QV4::ReturnedValue rv) const;
    QQmlJSRegisterContent typeForBinaryOperation(QSOperator::Op oper,
                                                 const QQmlJSRegisterContent &left,
                                                 const QQmlJSRegisterContent &right) const;

    enum class UnaryOperator { Plus, Minus, Increment, Decrement };

    QQmlJSRegisterContent typeForUnaryOperation(UnaryOperator oper,
                                                const QQmlJSRegisterContent &operand) const;

    bool isPrimitive(const QQmlJSRegisterContent &type) const;
    bool isNumeric(const QQmlJSRegisterContent &type) const;

    bool canConvertFromTo(const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const;
    bool canConvertFromTo(const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to) const;
    QQmlJSRegisterContent merge(const QQmlJSRegisterContent &a,
                                const QQmlJSRegisterContent &b) const;

    enum class ComponentIsGeneric { No, Yes };
    QQmlJSScope::ConstPtr
    genericType(const QQmlJSScope::ConstPtr &type,
                ComponentIsGeneric allowComponent = ComponentIsGeneric::No) const;

    QQmlJSRegisterContent globalType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent scopedType(const QQmlJSScope::ConstPtr &scope, const QString &name) const;
    QQmlJSRegisterContent memberType(const QQmlJSRegisterContent &type, const QString &name) const;
    QQmlJSRegisterContent valueType(const QQmlJSRegisterContent &listType) const;

    bool registerContains(const QQmlJSRegisterContent &reg,
                          const QQmlJSScope::ConstPtr &type) const;
    QQmlJSScope::ConstPtr containedType(const QQmlJSRegisterContent &container) const;
    QString containedTypeName(const QQmlJSRegisterContent &container) const;

    TypeStorage typeStorage() const { return m_typeStorage; }
    Semantics semantics() const { return m_semantics; }

    QQmlJSScope::ConstPtr
    storedType(const QQmlJSScope::ConstPtr &type,
               ComponentIsGeneric allowComponent = ComponentIsGeneric::No) const;

    const QHash<QString, QQmlJSScope::ConstPtr> &objectsById() { return m_objectsById; }
    const QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> &signalHandlers()
    {
        return m_signalHandlers;
    }

private:
    QQmlJSScope::ConstPtr merge(const QQmlJSScope::ConstPtr &a,
                                const QQmlJSScope::ConstPtr &b) const;

    QQmlJSRegisterContent memberType(const QQmlJSScope::ConstPtr &type, const QString &name) const;
    QQmlJSRegisterContent memberEnumType(const QQmlJSScope::ConstPtr &type,
                                         const QString &name) const;
    bool isPrimitive(const QQmlJSScope::ConstPtr &type) const;
    bool isNumeric(const QQmlJSScope::ConstPtr &type) const;
    bool checkEnums(const QQmlJSScope::ConstPtr &scope, const QString &name,
                    QQmlJSRegisterContent *result, BaseOrExtension mode) const;

    QQmlJSScope::ConstPtr m_voidType;
    QQmlJSScope::ConstPtr m_numberType;
    QQmlJSScope::ConstPtr m_realType;
    QQmlJSScope::ConstPtr m_intType;
    QQmlJSScope::ConstPtr m_boolType;
    QQmlJSScope::ConstPtr m_stringType;
    QQmlJSScope::ConstPtr m_urlType;
    QQmlJSScope::ConstPtr m_dateTimeType;
    QQmlJSScope::ConstPtr m_variantListType;
    QQmlJSScope::ConstPtr m_varType;
    QQmlJSScope::ConstPtr m_jsValueType;
    QQmlJSScope::ConstPtr m_jsPrimitiveType;
    QQmlJSScope::ConstPtr m_listPropertyType;

    QHash<QString, QQmlJSScope::ConstPtr> m_knownGlobalTypes;
    QStringList m_jsGlobalProperties;

    QQmlJSImporter *m_importer = nullptr;
    const QmlIR::Document *m_document = nullptr;

    QHash<QString, QQmlJSScope::ConstPtr> m_objectsById;
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> m_objectsByLocation;
    QHash<QString, QQmlJSScope::ConstPtr> m_imports;
    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> m_signalHandlers;

    TypeStorage m_typeStorage = Direct;
    Semantics m_semantics = Dynamic;
    QQmlJSLogger *m_logger;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPERESOLVER_P_H
