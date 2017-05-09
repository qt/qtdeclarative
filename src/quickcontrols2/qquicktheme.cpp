/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktheme_p.h"
#include "qquickstyle_p.h"

#include <QtCore/qmetaobject.h>
#include <QtCore/qsettings.h>

#include <functional>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(settings)
static void readValue(const QSharedPointer<QSettings> &settings, const QString &name, std::function<void(const QVariant &)> setValue)
{
    const QVariant var = settings->value(name);
    if (var.isValid())
        setValue(var);
}

template <typename Enum>
static Enum toEnumValue(const QVariant &var)
{
    // ### TODO: expose QFont enums to the meta object system using Q_ENUM
    //QMetaEnum enumeration = QMetaEnum::fromType<Enum>();
    //bool ok = false;
    //int value = enumeration.keyToValue(var.toByteArray(), &ok);
    //if (!ok)
    //    value = var.toInt();
    //return static_cast<Enum>(value);

    return static_cast<Enum>(var.toInt());
}

QFont *readFont(const QSharedPointer<QSettings> &settings)
{
    const QVariant var = settings->value(QStringLiteral("Font"));
    if (var.isValid())
        return new QFont(var.value<QFont>());

    QFont f;
    settings->beginGroup(QStringLiteral("Font"));
    readValue(settings, QStringLiteral("Family"), [&f](const QVariant &var) { f.setFamily(var.toString()); });
    readValue(settings, QStringLiteral("PointSize"), [&f](const QVariant &var) { f.setPointSizeF(var.toReal()); });
    readValue(settings, QStringLiteral("PixelSize"), [&f](const QVariant &var) { f.setPixelSize(var.toInt()); });
    readValue(settings, QStringLiteral("StyleHint"), [&f](const QVariant &var) { f.setStyleHint(toEnumValue<QFont::StyleHint>(var.toInt())); });
    readValue(settings, QStringLiteral("Weight"), [&f](const QVariant &var) { f.setWeight(toEnumValue<QFont::Weight>(var)); });
    readValue(settings, QStringLiteral("Style"), [&f](const QVariant &var) { f.setStyle(toEnumValue<QFont::Style>(var.toInt())); });
    settings->endGroup();
    return new QFont(f);
}
#endif // QT_CONFIG(settings)

QQuickTheme::QQuickTheme(const QString &style)
    : QQuickProxyTheme()
{
#if QT_CONFIG(settings)
    QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(style);
    if (settings)
        m_styleFont.reset(readFont(settings));
#endif
}

const QFont *QQuickTheme::font(Font type) const
{
    Q_UNUSED(type);
    return m_styleFont.data();
}

QFont QQuickTheme::resolveFont(const QFont &font) const
{
    if (!m_styleFont)
        return font;

    return m_styleFont->resolve(font);
}

QT_END_NAMESPACE
