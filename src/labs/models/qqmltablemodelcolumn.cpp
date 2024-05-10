// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltablemodelcolumn_p.h"

#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TableModelColumn
//!  \instantiates QQmlTableModelColumn
    \inqmlmodule Qt.labs.qmlmodels
    \brief Represents a column in a model.
    \since 5.14

    \section1 Supported Roles

    TableModelColumn supports all of \l {Qt::ItemDataRole}{Qt's roles},
    with the exception of \c Qt::InitialSortOrderRole.
    Roles can be accessed by as listed below, e.g.
    \code
    text: display

    required property string display
    \endcode

    \table
    \row \li Qt::DisplayRole \li display
    \row \li Qt::DecorationRole \li decoration
    \row \li Qt::EditRole \li edit
    \row \li Qt::ToolTipRole \li toolTip
    \row \li Qt::StatusTipRole \li statusTip
    \row \li Qt::WhatsThisRole \li whatsThis
    \row \li Qt::FontRole \li font
    \row \li Qt::TextAlignmentRole \li textAlignment
    \row \li Qt::BackgroundRole \li background
    \row \li Qt::ForegroundRole \li foreground
    \row \li Qt::CheckStateRole \li checkState
    \row \li Qt::AccessibleTextRole \li accessibleText
    \row \li Qt::AccessibleDescriptionRole \li accessibleDescription
    \row \li Qt::SizeHintRole \li sizeHintRoleNam
    \endtable

    \sa TableModel, TableView
*/

static constexpr QLatin1StringView displayRoleName("display");
static constexpr QLatin1StringView decorationRoleName("decoration");
static constexpr QLatin1StringView editRoleName("edit");
static constexpr QLatin1StringView toolTipRoleName("toolTip");
static constexpr QLatin1StringView statusTipRoleName("statusTip");
static constexpr QLatin1StringView whatsThisRoleName("whatsThis");

static constexpr QLatin1StringView fontRoleName("font");
static constexpr QLatin1StringView textAlignmentRoleName("textAlignment");
static constexpr QLatin1StringView backgroundRoleName("background");
static constexpr QLatin1StringView foregroundRoleName("foreground");
static constexpr QLatin1StringView checkStateRoleName("checkState");

static constexpr QLatin1StringView accessibleTextRoleName("accessibleText");
static constexpr QLatin1StringView accessibleDescriptionRoleName("accessibleDescription");

static constexpr QLatin1StringView sizeHintRoleName("sizeHint");


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
    static const QHash<int, QString> names {
        {Qt::DisplayRole, displayRoleName},
        {Qt::DecorationRole, decorationRoleName},
        {Qt::EditRole, editRoleName},
        {Qt::ToolTipRole, toolTipRoleName},
        {Qt::StatusTipRole, statusTipRoleName},
        {Qt::WhatsThisRole, whatsThisRoleName},
        {Qt::FontRole, fontRoleName},
        {Qt::TextAlignmentRole, textAlignmentRoleName},
        {Qt::BackgroundRole, backgroundRoleName},
        {Qt::ForegroundRole, foregroundRoleName},
        {Qt::CheckStateRole, checkStateRoleName},
        {Qt::AccessibleTextRole, accessibleTextRoleName},
        {Qt::AccessibleDescriptionRole, accessibleDescriptionRoleName},
        {Qt::SizeHintRole, sizeHintRoleName}
    };
    return names;
}

QT_END_NAMESPACE

#include "moc_qqmltablemodelcolumn_p.cpp"
