/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldirparser_p.h"
#include "qqmlerror.h"
#include "qqmlglobal_p.h"

#include <QtQml/qqmlfile.h>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QtDebug>

QT_BEGIN_NAMESPACE

QQmlDirParser::QQmlDirParser()
    : _isParsed(false)
{
}

QQmlDirParser::~QQmlDirParser()
{
}

QString QQmlDirParser::source() const
{
    return _source;
}

void QQmlDirParser::setSource(const QString &source)
{
    _isParsed = false;
    _source = source;
}

bool QQmlDirParser::isParsed() const
{
    return _isParsed;
}

/*!
\a url is used for generating errors.
*/
bool QQmlDirParser::parse()
{
    if (_isParsed)
        return true;

    _isParsed = true;
    _errors.clear();
    _plugins.clear();
    _components.clear();
    _scripts.clear();

    QTextStream stream(&_source);
    int lineNumber = 0;

    forever {
        ++lineNumber;

        const QString line = stream.readLine();
        if (line.isNull())
            break;

        QString sections[3];
        int sectionCount = 0;

        int index = 0;
        const int length = line.length();

        while (index != length) {
            const QChar ch = line.at(index);

            if (ch.isSpace()) {
                do { ++index; }
                while (index != length && line.at(index).isSpace());

            } else if (ch == QLatin1Char('#')) {
                // recognized a comment
                break;

            } else {
                const int start = index;

                do { ++index; }
                while (index != length && !line.at(index).isSpace());

                const QString lexeme = line.mid(start, index - start);

                if (sectionCount >= 3) {
                    reportError(lineNumber, start, QLatin1String("unexpected token"));

                } else {
                    sections[sectionCount++] = lexeme;
                }
            }
        }

        if (sectionCount == 0) {
            continue; // no sections, no party.

        } else if (sections[0] == QLatin1String("plugin")) {
            if (sectionCount < 2) {
                reportError(lineNumber, -1,
                            QString::fromUtf8("plugin directive requires one or two arguments, but %1 were provided").arg(sectionCount - 1));

                continue;
            }

            const Plugin entry(sections[1], sections[2]);

            _plugins.append(entry);

        } else if (sections[0] == QLatin1String("internal")) {
            if (sectionCount != 3) {
                reportError(lineNumber, -1,
                            QString::fromUtf8("internal types require 2 arguments, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            Component entry(sections[1], sections[2], -1, -1);
            entry.internal = true;
            _components.insertMulti(entry.typeName, entry);
        } else if (sections[0] == QLatin1String("typeinfo")) {
            if (sectionCount != 2) {
                reportError(lineNumber, -1,
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
                reportError(lineNumber, -1, QLatin1String("expected '.'"));
            } else if (version.indexOf(QLatin1Char('.'), dotIndex + 1) != -1) {
                reportError(lineNumber, -1, QLatin1String("unexpected '.'"));
            } else {
                bool validVersionNumber = false;
                const int majorVersion = version.left(dotIndex).toInt(&validVersionNumber);

                if (validVersionNumber) {
                    const int minorVersion = version.mid(dotIndex + 1).toInt(&validVersionNumber);

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
            reportError(lineNumber, -1, 
                        QString::fromUtf8("a component declaration requires two or three arguments, but %1 were provided").arg(sectionCount));
        }
    }

    return hasError();
}

void QQmlDirParser::reportError(int line, int column, const QString &description)
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
    QList<QQmlError> errors = _errors;
    for (int i = 0; i < errors.size(); ++i) {
        QQmlError &e = errors[i];
        QString description = e.description();
        description.replace(QLatin1String("$$URI$$"), uri);
        e.setDescription(description);
    }
    return errors;
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
