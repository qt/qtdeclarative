/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickthemedata_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQuickThemeData::QQuickThemeData(const QString &filePath) : d(new Data)
{
    if (!filePath.isEmpty())
        load(filePath);
}

static QColor readColorValue(const QJsonValue &value, const QColor &defaultValue)
{
    if (value.isString())
        return QColor(value.toString());
    return QColor::fromRgba(value.toInt(defaultValue.rgba()));
}

static double readNumberValue(const QJsonValue &value, double defaultValue)
{
    return value.toDouble(defaultValue);
}

bool QQuickThemeData::load(const QString &filePath)
{
    QJsonDocument doc;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.error();
        qWarning() << "QQuickTheme: failed to open ':/qtquickcontrols/theme.json': " << qPrintable(file.errorString());
        return false;
    } else {
        QJsonParseError error;
        doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "QQuickTheme: failed to parse ':/qtquickcontrols/theme.json': " << qPrintable(error.errorString());
            return false;
        }
    }

    QJsonObject theme = doc.object();
    d->accentColor = readColorValue(theme.value(QStringLiteral("accentColor")), QColor("#7bc258"));
    d->backgroundColor = readColorValue(theme.value(QStringLiteral("backgroundColor")), QColor("#ffffff"));
    d->baseColor = readColorValue(theme.value(QStringLiteral("baseColor")), QColor("#eeeeee"));
    d->focusColor = readColorValue(theme.value(QStringLiteral("focusColor")), QColor("#45a7d7"));
    d->frameColor = readColorValue(theme.value(QStringLiteral("frameColor")), QColor("#bdbebf"));
    d->pressColor = readColorValue(theme.value(QStringLiteral("pressColor")), QColor("#33333333"));
    d->selectedTextColor = readColorValue(theme.value(QStringLiteral("selectedTextColor")), QColor("#ffffff"));
    d->selectionColor = readColorValue(theme.value(QStringLiteral("selectionColor")), QColor("#45a7d7"));
    d->shadowColor = readColorValue(theme.value(QStringLiteral("shadowColor")), QColor("#28282a"));
    d->textColor = readColorValue(theme.value(QStringLiteral("textColor")), QColor("#26282a"));
    d->padding = readNumberValue(theme.value(QStringLiteral("padding")), 6);
    d->roundness = readNumberValue(theme.value(QStringLiteral("roundness")), 3);
    d->spacing = readNumberValue(theme.value(QStringLiteral("spacing")), 6);
    d->disabledOpacity = readNumberValue(theme.value(QStringLiteral("disabledOpacity")), 0.3);
    return true;
}

QT_END_NAMESPACE
