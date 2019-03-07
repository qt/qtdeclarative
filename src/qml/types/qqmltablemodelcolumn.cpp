/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
**
****************************************************************************/

#include "qqmltablemodelcolumn_p.h"

#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TableModelColumn
    \instantiates QQmlTableModelColumn
    \inqmlmodule Qt.labs.qmlmodels
    \brief Represents a column in a model.
    \since 5.14

    \section1 Supported Roles

    TableModelColumn supports all of \l {Qt::ItemDataRole}{Qt's roles},
    with the exception of \c Qt::InitialSortOrderRole.

    \sa TableModel, TableView
*/

static const QString displayRoleName = QStringLiteral("display");
static const QString decorationRoleName = QStringLiteral("decoration");
static const QString editRoleName = QStringLiteral("edit");
static const QString toolTipRoleName = QStringLiteral("toolTip");
static const QString statusTipRoleName = QStringLiteral("statusTip");
static const QString whatsThisRoleName = QStringLiteral("whatsThis");

static const QString fontRoleName = QStringLiteral("font");
static const QString textAlignmentRoleName = QStringLiteral("textAlignment");
static const QString backgroundRoleName = QStringLiteral("background");
static const QString foregroundRoleName = QStringLiteral("foreground");
static const QString checkStateRoleName = QStringLiteral("checkState");

static const QString accessibleTextRoleName = QStringLiteral("accessibleText");
static const QString accessibleDescriptionRoleName = QStringLiteral("accessibleDescription");

static const QString sizeHintRoleName = QStringLiteral("sizeHint");


QQmlTableModelColumn::QQmlTableModelColumn(QObject *parent)
    : QObject(parent)
{
}

QQmlTableModelColumn::~QQmlTableModelColumn()
{
}

#define DEFINE_ROLE_PROPERTIES(getterGetterName, getterSetterName, getterSignal, setterGetterName, setterSetterName, setterSignal, roleName) \
QJSValue QQmlTableModelColumn::getterGetterName() const \
{ \
    return mGetters.value(roleName); \
} \
\
void QQmlTableModelColumn::getterSetterName(const QJSValue &stringOrFunction) \
{ \
    if (!stringOrFunction.isString() && !stringOrFunction.isCallable()) { \
        qmlWarning(this).quote() << "getter for " << roleName << " must be a function"; \
        return; \
    } \
    if (stringOrFunction.strictlyEquals(decoration())) \
        return; \
\
    mGetters[roleName] = stringOrFunction; \
    emit decorationChanged(); \
} \
\
QJSValue QQmlTableModelColumn::setterGetterName() const \
{ \
    return mSetters.value(roleName); \
} \
\
void QQmlTableModelColumn::setterSetterName(const QJSValue &function) \
{ \
    if (!function.isCallable()) { \
        qmlWarning(this).quote() << "setter for " << roleName << " must be a function"; \
        return; \
    } \
\
    if (function.strictlyEquals(getSetDisplay())) \
        return; \
\
    mSetters[roleName] = function; \
    emit setDisplayChanged(); \
}

DEFINE_ROLE_PROPERTIES(display, setDisplay, displayChanged,
    getSetDisplay, setSetDisplay, setDisplayChanged, displayRoleName)
DEFINE_ROLE_PROPERTIES(decoration, setDecoration, decorationChanged,
    getSetDecoration, setSetDecoration, setDecorationChanged, decorationRoleName)
DEFINE_ROLE_PROPERTIES(edit, setEdit, editChanged,
    getSetEdit, setSetEdit, setEditChanged, editRoleName)
DEFINE_ROLE_PROPERTIES(toolTip, setToolTip, toolTipChanged,
    getSetToolTip, setSetToolTip, setToolTipChanged, toolTipRoleName)
DEFINE_ROLE_PROPERTIES(statusTip, setStatusTip, statusTipChanged,
    getSetStatusTip, setSetStatusTip, setStatusTipChanged, statusTipRoleName)
DEFINE_ROLE_PROPERTIES(whatsThis, setWhatsThis, whatsThisChanged,
    getSetWhatsThis, setSetWhatsThis, setWhatsThisChanged, whatsThisRoleName)

DEFINE_ROLE_PROPERTIES(font, setFont, fontChanged,
    getSetFont, setSetFont, setFontChanged, fontRoleName)
DEFINE_ROLE_PROPERTIES(textAlignment, setTextAlignment, textAlignmentChanged,
    getSetTextAlignment, setSetTextAlignment, setTextAlignmentChanged, textAlignmentRoleName)
DEFINE_ROLE_PROPERTIES(background, setBackground, backgroundChanged,
    getSetBackground, setSetBackground, setBackgroundChanged, backgroundRoleName)
DEFINE_ROLE_PROPERTIES(foreground, setForeground, foregroundChanged,
    getSetForeground, setSetForeground, setForegroundChanged, foregroundRoleName)
DEFINE_ROLE_PROPERTIES(checkState, setCheckState, checkStateChanged,
    getSetCheckState, setSetCheckState, setCheckStateChanged, checkStateRoleName)

DEFINE_ROLE_PROPERTIES(accessibleText, setAccessibleText, accessibleTextChanged,
    getSetAccessibleText, setSetAccessibleText, setAccessibleTextChanged, accessibleTextRoleName)
DEFINE_ROLE_PROPERTIES(accessibleDescription, setAccessibleDescription, accessibleDescriptionChanged,
    getSetAccessibleDescription, setSetAccessibleDescription, setAccessibleDescriptionChanged, accessibleDescriptionRoleName)

DEFINE_ROLE_PROPERTIES(sizeHint, setSizeHint, sizeHintChanged,
    getSetSizeHint, setSetSizeHint, setSizeHintChanged, sizeHintRoleName)

QJSValue QQmlTableModelColumn::getterAtRole(const QString &roleName)
{
    auto it = mGetters.find(roleName);
    if (it == mGetters.end())
        return QJSValue();
    return *it;
}

QJSValue QQmlTableModelColumn::setterAtRole(const QString &roleName)
{
    auto it = mSetters.find(roleName);
    if (it == mSetters.end())
        return QJSValue();
    return *it;
}

const QHash<QString, QJSValue> QQmlTableModelColumn::getters() const
{
    return mGetters;
}

const QHash<int, QString> QQmlTableModelColumn::supportedRoleNames()
{
    QHash<int, QString> names;
    names[Qt::DisplayRole] = QLatin1String("display");
    names[Qt::DecorationRole] = QLatin1String("decoration");
    names[Qt::EditRole] = QLatin1String("edit");
    names[Qt::ToolTipRole] = QLatin1String("toolTip");
    names[Qt::StatusTipRole] = QLatin1String("statusTip");
    names[Qt::WhatsThisRole] = QLatin1String("whatsThis");
    names[Qt::FontRole] = QLatin1String("font");
    names[Qt::TextAlignmentRole] = QLatin1String("textAlignment");
    names[Qt::BackgroundRole] = QLatin1String("background");
    names[Qt::ForegroundRole] = QLatin1String("foreground");
    names[Qt::CheckStateRole] = QLatin1String("checkState");
    names[Qt::AccessibleTextRole] = QLatin1String("accessibleText");
    names[Qt::AccessibleDescriptionRole] = QLatin1String("accessibleDescription");
    names[Qt::SizeHintRole] = QLatin1String("sizeHint");
    return names;
}

QT_END_NAMESPACE
