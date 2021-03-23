/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/

#include "qqmldomfieldfilter_p.h"
#include "QtCore/qglobal.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

QString FieldFilter::describeFieldsFilter() const
{
    QString fieldFilterStr;
    {
        auto it = m_fieldFilterRemove.begin();
        while (it != m_fieldFilterRemove.end()) {
            if (!fieldFilterStr.isEmpty())
                fieldFilterStr.append(u",");
            fieldFilterStr.append(QLatin1String("-%1:%2").arg(it.key(), it.value()));
            ++it;
        }
    }
    {
        auto it = m_fieldFilterAdd.begin();
        while (it != m_fieldFilterAdd.end()) {
            if (!fieldFilterStr.isEmpty())
                fieldFilterStr.append(u",");
            fieldFilterStr.append(QLatin1String("+%1:%2").arg(it.key(), it.value()));
            ++it;
        }
    }
    return fieldFilterStr;
}

bool FieldFilter::operator()(DomItem &obj, Path p, DomItem &i) const
{
    if (p)
        return this->operator()(obj, p.component(0), i);
    else
        return this->operator()(obj, PathEls::Empty(), i);
}

bool FieldFilter::operator()(DomItem &base, const PathEls::PathComponent &c, DomItem &obj) const
{
    DomType baseK = base.internalKind();
    if (c.kind() == Path::Kind::Field) {
        DomType objK = obj.internalKind();
        if (!m_filtredTypes.contains(baseK) && !m_filtredTypes.contains(objK)
            && !m_filtredFields.contains(qHash(c.stringView())))
            return m_filtredDefault;
        QString typeStr = domTypeToString(baseK);
        QList<QString> tVals = m_fieldFilterRemove.values(typeStr);
        QString name = c.name();
        if (tVals.contains(name))
            return false;
        if (tVals.contains(QString())
            || m_fieldFilterRemove.values(domTypeToString(objK)).contains(QString())
            || m_fieldFilterRemove.values(QString()).contains(name)) {
            return m_fieldFilterAdd.values(typeStr).contains(name);
        }
    } else if (m_filtredTypes.contains(baseK)) {
        QString typeStr = domTypeToString(baseK);
        QList<QString> tVals = m_fieldFilterRemove.values(typeStr);
        return !tVals.contains(QString());
    }
    return true;
}

bool FieldFilter::addFilter(QString fFields)
{
    for (QString fField : fFields.split(QLatin1Char(','))) {
        QRegularExpression fieldRe(QRegularExpression::anchoredPattern(QStringLiteral(
                uR"((?<op>[-+])?(?:(?<type>[a-zA-Z0-9_]*):)?(?<field>[a-zA-Z0-9_]+))")));
        QRegularExpressionMatch m = fieldRe.match(fField);
        if (m.hasMatch()) {
            if (m.captured(u"op") == u"+") {
                m_fieldFilterRemove.remove(m.captured(u"type"), m.captured(u"field"));
                m_fieldFilterAdd.insert(m.captured(u"type"), m.captured(u"field"));
            } else {
                m_fieldFilterRemove.insert(m.captured(u"type"), m.captured(u"field"));
                m_fieldFilterAdd.remove(m.captured(u"type"), m.captured(u"field"));
            }
        } else {
            qCWarning(domLog) << "could not extract filter from" << fField;
            return false;
        }
    }
    return true;
}

FieldFilter FieldFilter::defaultFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd { { QLatin1String("ScriptExpression"),
                                                   QLatin1String("code") } };
    QMultiMap<QString, QString> fieldFilterRemove { { QString(), QLatin1String("code") },
                                                    { QString(), QLatin1String("propertyInfos") },
                                                    { QLatin1String("AttachedInfo"),
                                                      QLatin1String("parent") } };

    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

QQmlJS::Dom::FieldFilter QQmlJS::Dom::FieldFilter::noLocationFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove {
        { QString(), QLatin1String("code") },
        { QString(), QLatin1String("propertyInfos") },
        { QString(), QLatin1String("fileLocationsTree") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("ScriptExpression"), QLatin1String("preCode") },
        { QLatin1String("ScriptExpression"), QLatin1String("postCode") },
        { QLatin1String("AttachedInfo"), QLatin1String("parent") },
        { QLatin1String("Reference"), QLatin1String("get") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

FieldFilter FieldFilter::compareFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove {
        { QString(), QLatin1String("propertyInfos") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("FileLocations"), QLatin1String("regions") },
        { QLatin1String("AttachedInfo"), QLatin1String("parent") },
        { QLatin1String("Reference"), QLatin1String("get") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

FieldFilter FieldFilter::compareNoCommentsFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove {
        { QString(), QLatin1String("propertyInfos") },
        { QLatin1String("FileLocations"), QLatin1String("regions") },
        { QLatin1String("Reference"), QLatin1String("get") },
        { QLatin1String(), QLatin1String("code") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("AttachedInfo"), QLatin1String("parent") },
        { QLatin1String(), QLatin1String("fileLocationsTree") },
        { QLatin1String(), QLatin1String("preCode") },
        { QLatin1String(), QLatin1String("postCode") },
        { QLatin1String(), QLatin1String("comments") },
        { QLatin1String(), QLatin1String("preCommentLocations") },
        { QLatin1String(), QLatin1String("postCommentLocations") },
        { QLatin1String(), QLatin1String("astComments") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

void FieldFilter::setFiltred()
{
    auto types = domTypeToStringMap();
    QSet<QString> filtredFieldStrs;
    QSet<QString> filtredTypeStrs;
    static QHash<QString, DomType> fieldToId = []() {
        QHash<QString, DomType> res;
        auto reverseMap = domTypeToStringMap();
        auto it = reverseMap.cbegin();
        auto end = reverseMap.cend();
        while (it != end) {
            res[it.value()] = it.key();
            ++it;
        }
        return res;
    }();
    auto addFilteredOfMap = [&](const QMultiMap<QString, QString> &map) {
        auto it = map.cbegin();
        auto end = map.cend();
        while (it != end) {
            filtredTypeStrs.insert(it.key());
            ++it;
        }
        for (auto f : map.values(QString()))
            filtredFieldStrs.insert(f);
    };
    addFilteredOfMap(m_fieldFilterAdd);
    addFilteredOfMap(m_fieldFilterRemove);
    m_filtredDefault = true;
    if (m_fieldFilterRemove.values(QString()).contains(QString()))
        m_filtredDefault = false;
    m_filtredFields.clear();
    for (auto s : filtredFieldStrs)
        if (!s.isEmpty())
            m_filtredFields.insert(qHash(QStringView(s)));
    m_filtredTypes.clear();
    for (auto s : filtredTypeStrs) {
        if (s.isEmpty())
            continue;
        if (fieldToId.contains(s)) {
            m_filtredTypes.insert(fieldToId.value(s));
        } else {
            qCWarning(domLog) << "Filter on unknonw type " << s << " will be ignored";
        }
    }
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
