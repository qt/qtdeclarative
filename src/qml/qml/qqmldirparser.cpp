/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldirparser_p.h"
#include "qqmlerror.h"
#include "qqmlglobal_p.h"

#include <QtQml/qqmlfile.h>
#include <QtCore/QtDebug>

QT_BEGIN_NAMESPACE

static int parseInt(const QStringRef &str, bool *ok)
{
    int pos = 0;
    int number = 0;
    while (pos < str.length() && str.at(pos).isDigit()) {
        if (pos != 0)
            number *= 10;
        number += str.at(pos).unicode() - '0';
        ++pos;
    }
    if (pos != str.length())
        *ok = false;
    else
        *ok = true;
    return number;
}

QQmlDirParser::QQmlDirParser()
{
}

QQmlDirParser::~QQmlDirParser()
{
}

inline static void scanSpace(const QChar *&ch) {
    while (ch->isSpace() && !ch->isNull() && *ch != QLatin1Char('\n'))
        ++ch;
}

inline static void scanToEnd(const QChar *&ch) {
    while (*ch != QLatin1Char('\n') && !ch->isNull())
        ++ch;
}

inline static void scanWord(const QChar *&ch) {
    while (!ch->isSpace() && !ch->isNull())
        ++ch;
}

/*!
\a url is used for generating errors.
*/
bool QQmlDirParser::parse(const QString &source)
{
    _errors.clear();
    _plugins.clear();
    _components.clear();
    _scripts.clear();

    quint16 lineNumber = 0;
    bool firstLine = true;

    const QChar *ch = source.constData();
    while (!ch->isNull()) {
        ++lineNumber;

        bool invalidLine = false;
        const QChar *lineStart = ch;

        scanSpace(ch);
        if (*ch == QLatin1Char('\n')) {
            ++ch;
            continue;
        }
        if (ch->isNull())
            break;

        QString sections[3];
        int sectionCount = 0;

        do {
            if (*ch == QLatin1Char('#')) {
                scanToEnd(ch);
                break;
            }
            const QChar *start = ch;
            scanWord(ch);
            if (sectionCount < 3) {
                sections[sectionCount++] = source.mid(start-source.constData(), ch-start);
            } else {
                reportError(lineNumber, start-lineStart, QLatin1String("unexpected token"));
                scanToEnd(ch);
                invalidLine = true;
                break;
            }
            scanSpace(ch);
        } while (*ch != QLatin1Char('\n') && !ch->isNull());

        if (!ch->isNull())
            ++ch;

        if (invalidLine) {
            reportError(lineNumber, 0,
                        QString::fromUtf8("invalid qmldir directive contains too many tokens"));
            continue;
        } else if (sectionCount == 0) {
            continue; // no sections, no party.

        } else if (sections[0] == QLatin1String("module")) {
            if (sectionCount != 2) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("module identifier directive requires one argument, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            if (!_typeNamespace.isEmpty()) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("only one module identifier directive may be defined in a qmldir file"));
                continue;
            }
            if (!firstLine) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("module identifier directive must be the first command in a qmldir file"));
                continue;
            }

            _typeNamespace = sections[1];

        } else if (sections[0] == QLatin1String("plugin")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("plugin directive requires one or two arguments, but %1 were provided").arg(sectionCount - 1));

                continue;
            }

            const Plugin entry(sections[1], sections[2]);

            _plugins.append(entry);

        } else if (sections[0] == QLatin1String("internal")) {
            if (sectionCount != 3) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("internal types require 2 arguments, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            Component entry(sections[1], sections[2], -1, -1);
            entry.internal = true;
            _components.insertMulti(entry.typeName, entry);
        } else if (sections[0] == QLatin1String("typeinfo")) {
            if (sectionCount != 2) {
                reportError(lineNumber, 0,
                            QString::fromUtf8("typeinfo requires 1 argument, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
#ifdef QT_CREATOR
            TypeInfo typeInfo(sections[1]);
            _typeInfos.append(typeInfo);
#endif

        } else if (sectionCount == 2) {
            // No version specified (should only be used for relative qmldir files)
            const Component entry(sections[0], sections[1], -1, -1);
            _components.insertMulti(entry.typeName, entry);
        } else if (sectionCount == 3) {
            const QString &version = sections[1];
            const int dotIndex = version.indexOf(QLatin1Char('.'));

            if (dotIndex == -1) {
                reportError(lineNumber, 0, QLatin1String("expected '.'"));
            } else if (version.indexOf(QLatin1Char('.'), dotIndex + 1) != -1) {
                reportError(lineNumber, 0, QLatin1String("unexpected '.'"));
            } else {
                bool validVersionNumber = false;
                const int majorVersion = parseInt(QStringRef(&version, 0, dotIndex), &validVersionNumber);

                if (validVersionNumber) {
                    const int minorVersion = parseInt(QStringRef(&version, dotIndex+1, version.length()-dotIndex-1), &validVersionNumber);

                    if (validVersionNumber) {
                        const QString &fileName = sections[2];

                        if (fileName.endsWith(QLatin1String(".js"))) {
                            // A 'js' extension indicates a namespaced script import
                            const Script entry(sections[0], fileName, majorVersion, minorVersion);
                            _scripts.append(entry);
                        } else {
                            const Component entry(sections[0], fileName, majorVersion, minorVersion);
                            _components.insertMulti(entry.typeName, entry);
                        }
                    }
                }
            }
        } else {
            reportError(lineNumber, 0,
                        QString::fromUtf8("a component declaration requires two or three arguments, but %1 were provided").arg(sectionCount));
        }

        firstLine = false;
    }

    return hasError();
}

void QQmlDirParser::reportError(quint16 line, quint16 column, const QString &description)
{
    QQmlError error;
    error.setLine(line);
    error.setColumn(column);
    error.setDescription(description);
    _errors.append(error);
}

bool QQmlDirParser::hasError() const
{
    if (! _errors.isEmpty())
        return true;

    return false;
}

void QQmlDirParser::setError(const QQmlError &e)
{
    _errors.clear();
    _errors.append(e);
}

QList<QQmlError> QQmlDirParser::errors(const QString &uri) const
{
    QUrl url(uri);
    QList<QQmlError> errors = _errors;
    for (int i = 0; i < errors.size(); ++i) {
        QQmlError &e = errors[i];
        QString description = e.description();
        description.replace(QLatin1String("$$URI$$"), uri);
        e.setDescription(description);
        e.setUrl(url);
    }
    return errors;
}

QString QQmlDirParser::typeNamespace() const
{
    return _typeNamespace;
}

void QQmlDirParser::setTypeNamespace(const QString &s)
{
    _typeNamespace = s;
}

QList<QQmlDirParser::Plugin> QQmlDirParser::plugins() const
{
    return _plugins;
}

QHash<QHashedStringRef,QQmlDirParser::Component> QQmlDirParser::components() const
{
    return _components;
}

QList<QQmlDirParser::Script> QQmlDirParser::scripts() const
{
    return _scripts;
}

#ifdef QT_CREATOR
QList<QQmlDirParser::TypeInfo> QQmlDirParser::typeInfos() const
{
    return _typeInfos;
}
#endif

QDebug &operator<< (QDebug &debug, const QQmlDirParser::Component &component)
{
    const QString output = QString::fromLatin1("{%1 %2.%3}").
        arg(component.typeName).arg(component.majorVersion).arg(component.minorVersion);
    return debug << qPrintable(output);
}

QDebug &operator<< (QDebug &debug, const QQmlDirParser::Script &script)
{
    const QString output = QString::fromLatin1("{%1 %2.%3}").
        arg(script.nameSpace).arg(script.majorVersion).arg(script.minorVersion);
    return debug << qPrintable(output);
}

QT_END_NAMESPACE
