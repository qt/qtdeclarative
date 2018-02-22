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

#include "qquickstyleplugin_p.h"
#include "qquickstyle.h"
#include "qquickstyle_p.h"

#include <QtCore/qmetaobject.h>
#include <QtCore/qsettings.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

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

static const QFont *readFont(const QSharedPointer<QSettings> &settings)
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

static void readColorGroup(const QSharedPointer<QSettings> &settings, QPalette::ColorGroup group, QPalette *palette)
{
    const QStringList keys = settings->childKeys();
    if (keys.isEmpty())
        return;

    static const int index = QPalette::staticMetaObject.indexOfEnumerator("ColorRole");
    Q_ASSERT(index != -1);
    QMetaEnum metaEnum = QPalette::staticMetaObject.enumerator(index);

    for (const QString &key : keys) {
        bool ok = false;
        int role = metaEnum.keyToValue(key.toUtf8(), &ok);
        if (ok)
            palette->setColor(group, static_cast<QPalette::ColorRole>(role), settings->value(key).value<QColor>());
    }
}

static const QPalette *readPalette(const QSharedPointer<QSettings> &settings)
{
    QPalette p;
    settings->beginGroup(QStringLiteral("Palette"));
    readColorGroup(settings, QPalette::All, &p);

    settings->beginGroup(QStringLiteral("Normal"));
    readColorGroup(settings, QPalette::Normal, &p);
    settings->endGroup();

    settings->beginGroup(QStringLiteral("Disabled"));
    readColorGroup(settings, QPalette::Disabled, &p);
    settings->endGroup();
    return new QPalette(p);
}

#endif // QT_CONFIG(settings)

QQuickStylePlugin::QQuickStylePlugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
}

QQuickStylePlugin::~QQuickStylePlugin()
{
    if (QQuickTheme::current() == m_theme)
        QQuickTheme::setCurrent(nullptr);
}

void QQuickStylePlugin::registerTypes(const char *uri)
{
    Q_UNUSED(uri);
}

void QQuickStylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);

    // make sure not to re-create the theme if initializeEngine()
    // is called multiple times, like in case of qml2puppet (QTBUG-54995)
    if (m_theme)
        return;

    if (isCurrent()) {
        m_theme = createTheme();
        if (m_theme) {
            const QFont *font = nullptr;
            const QPalette *palette = nullptr;
#if QT_CONFIG(settings)
            QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(name());
            if (settings) {
                font = readFont(settings);
                palette = readPalette(settings);
            }
#endif
            m_theme->setDefaultFont(font);
            m_theme->setDefaultPalette(palette);
            QQuickTheme::setCurrent(m_theme);
        }
    }
}

bool QQuickStylePlugin::isCurrent() const
{
    QString style = QQuickStyle::name();
    if (style.isEmpty())
        style = QStringLiteral("Default");

    const QString theme = name();
    return theme.compare(style, Qt::CaseInsensitive) == 0;
}

QString QQuickStylePlugin::name() const
{
    return QString();
}

QQuickTheme *QQuickStylePlugin::createTheme() const
{
    return nullptr;
}

/*
    Returns either a file system path if Qt was built as shared libraries,
    or a QRC path if Qt was built statically.
*/
QUrl QQuickStylePlugin::typeUrl(const QString &name) const
{
#ifdef QT_STATIC
    QString url = QLatin1String("qrc") + baseUrl().path();
#else
    QString url = baseUrl().toString();
#endif
    if (!name.isEmpty())
        url += QLatin1Char('/') + name;
    return QUrl(url);
}

QT_END_NAMESPACE
