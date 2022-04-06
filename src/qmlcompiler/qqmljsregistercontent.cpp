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

#include "qqmljsregistercontent_p.h"
#include "qqmljstyperesolver_p.h"

QT_BEGIN_NAMESPACE

QString QQmlJSRegisterContent::descriptiveName() const
{
    if (m_storedType.isNull())
        return u"(invalid type)"_qs;

    QString result = m_storedType->internalName() + u" of "_qs;
    const auto scope = [this]() -> QString {
        if (m_scope.isNull())
            return u"(invalid type)::"_qs;
        return (m_scope->internalName().isEmpty()
                        ? (m_scope->fileName().isEmpty()
                                   ? u"??"_qs
                                   : (u"(component in "_qs + m_scope->fileName() + u")"_qs))
                        : m_scope->internalName())
                + u"::"_qs;
    };

    switch (m_content.index()) {
    case Type:
        return result + std::get<QQmlJSScope::ConstPtr>(m_content)->internalName();
    case Property: {
        const QQmlJSMetaProperty prop = std::get<QQmlJSMetaProperty>(m_content);
        return result + scope() + prop.propertyName() + u" with type "_qs + prop.typeName();
    }
    case Method: {
        const auto methods = std::get<QList<QQmlJSMetaMethod>>(m_content);
        if (methods.isEmpty())
            return result + scope() + u"(unknown method)"_qs;
        else
            return result + scope() + methods[0].methodName() + u"(...)"_qs;
    }
    case Enum: {
        const auto e = std::get<std::pair<QQmlJSMetaEnum, QString>>(m_content);
        if (e.second.isEmpty())
            return result + scope() + e.first.name();
        else
            return result + scope() + e.first.name() + u"::"_qs + e.second;
    }
    case ImportNamespace: {
        return u"import namespace %1"_qs.arg(std::get<uint>(m_content));
    }
    }
    Q_UNREACHABLE();
    return result + u"wat?"_qs;
}

bool QQmlJSRegisterContent::isList() const
{
    switch (m_content.index()) {
    case Type:
        return std::get<QQmlJSScope::ConstPtr>(m_content)->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    case Property: {
        const auto prop = std::get<QQmlJSMetaProperty>(m_content);
        return prop.isList()
                || prop.type()->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;
    }
    default:
        return false;
    }
}

bool QQmlJSRegisterContent::isWritable() const
{
    switch (m_content.index()) {
    case Property:
        return std::get<QQmlJSMetaProperty>(m_content).isWritable();

    // TODO: What can we actually write?
    default:
        break;
    }

    return true;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSScope::ConstPtr &type,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = type;
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSMetaProperty &property,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = property;
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSMetaEnum &enumeration,
                                                    const QString &enumMember,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = std::make_pair(enumeration, enumMember);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QList<QQmlJSMetaMethod> &methods,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = methods;
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    uint importNamespaceStringId,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = importNamespaceStringId;
    return result;
}

bool operator==(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
{
    return a.m_storedType == b.m_storedType && a.m_variant == b.m_variant && a.m_scope == b.m_scope
            && a.m_content == b.m_content;
}

QT_END_NAMESPACE
