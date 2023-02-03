// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldirparser_p.h"

#include <QtCore/QtDebug>

QT_BEGIN_NAMESPACE

static int parseInt(QStringView str, bool *ok)
{
    int pos = 0;
    int number = 0;
    while (pos < str.size() && str.at(pos).isDigit()) {
        if (pos != 0)
            number *= 10;
        number += str.at(pos).unicode() - '0';
        ++pos;
    }
    if (pos != str.size())
        *ok = false;
    else
        *ok = true;
    return number;
}

static QTypeRevision parseVersion(const QString &str)
{
    const int dotIndex = str.indexOf(QLatin1Char('.'));
    if (dotIndex != -1 && str.indexOf(QLatin1Char('.'), dotIndex + 1) == -1) {
        bool ok = false;
        const int major = parseInt(QStringView(str).left(dotIndex), &ok);
        if (!ok) return QTypeRevision();
        const int minor = parseInt(QStringView(str).mid(dotIndex + 1, str.size() - dotIndex - 1), &ok);
        return ok ? QTypeRevision::fromVersion(major, minor) : QTypeRevision();
    }
    return QTypeRevision();
}

void QQmlDirParser::clear()
{
    _errors.clear();
    _typeNamespace.clear();
    _components.clear();
    _dependencies.clear();
    _imports.clear();
    _scripts.clear();
    _plugins.clear();
    _designerSupported = false;
    _typeInfos.clear();
    _classNames.clear();
    _linkTarget.clear();
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
    quint16 lineNumber = 0;
    bool firstLine = true;

    auto readImport = [&](const QString *sections, int sectionCount, Import::Flags flags) {
        Import import;
        if (sectionCount == 2) {
            import = Import(sections[1], QTypeRevision(), flags);
        } else if (sectionCount == 3) {
            if (sections[2] == QLatin1String("auto")) {
                import = Import(sections[1], QTypeRevision(), flags | Import::Auto);
            } else {
                const auto version = parseVersion(sections[2]);
                if (version.isValid()) {
                    import = Import(sections[1], version, flags);
                } else {
                    reportError(lineNumber, 0,
                                QStringLiteral("invalid version %1, expected <major>.<minor>")
                                .arg(sections[2]));
                    return false;
                }
            }
        } else {
            reportError(lineNumber, 0,
                        QStringLiteral("%1 requires 1 or 2 arguments, but %2 were provided")
                        .arg(sections[0]).arg(sectionCount - 1));
            return false;
        }
        if (sections[0] == QStringLiteral("import"))
            _imports.append(import);
        else
            _dependencies.append(import);
        return true;
    };

    auto readPlugin = [&](const QString *sections, int sectionCount, bool isOptional) {
        if (sectionCount < 2 || sectionCount > 3) {
            reportError(lineNumber, 0, QStringLiteral("plugin directive requires one or two "
                                                      "arguments, but %1 were provided")
                        .arg(sectionCount - 1));
            return false;
        }

        const Plugin entry(sections[1], sections[2], isOptional);
        _plugins.append(entry);
        return true;
    };

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

        QString sections[4];
        int sectionCount = 0;

        do {
            if (*ch == QLatin1Char('#')) {
                scanToEnd(ch);
                break;
            }
            const QChar *start = ch;
            scanWord(ch);
            if (sectionCount < 4) {
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
                        QStringLiteral("invalid qmldir directive contains too many tokens"));
            continue;
        } else if (sectionCount == 0) {
            continue; // no sections, no party.

        } else if (sections[0] == QLatin1String("module")) {
            if (sectionCount != 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("module identifier directive requires one argument, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            if (!_typeNamespace.isEmpty()) {
                reportError(lineNumber, 0,
                            QStringLiteral("only one module identifier directive may be defined in a qmldir file"));
                continue;
            }
            if (!firstLine) {
                reportError(lineNumber, 0,
                            QStringLiteral("module identifier directive must be the first directive in a qmldir file"));
                continue;
            }

            _typeNamespace = sections[1];

        } else if (sections[0] == QLatin1String("plugin")) {
            if (!readPlugin(sections, sectionCount, false))
                continue;
        } else if (sections[0] == QLatin1String("optional")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0, QStringLiteral("optional directive requires further "
                                                          "arguments, but none were provided."));
                continue;
            }

            if (sections[1] == QStringLiteral("plugin")) {
                if (!readPlugin(sections + 1, sectionCount - 1, true))
                    continue;
            } else if (sections[1] == QLatin1String("import")) {
                if (!readImport(sections + 1, sectionCount - 1, Import::Optional))
                    continue;
            } else {
                reportError(lineNumber, 0, QStringLiteral("only import and plugin can be optional, "
                                                          "not %1.").arg(sections[1]));
                continue;
            }
        } else if (sections[0] == QLatin1String("default")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("default directive requires further "
                                           "arguments, but none were provided."));
                continue;
            }
            if (sections[1] == QLatin1String("import")) {
                if (!readImport(sections + 1, sectionCount - 1,
                                Import::Flags({ Import::Optional, Import::OptionalDefault })))
                    continue;
            } else {
                reportError(lineNumber, 0,
                            QStringLiteral("only optional imports can have a a defaultl, "
                                           "not %1.")
                                    .arg(sections[1]));
            }
        } else if (sections[0] == QLatin1String("classname")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("classname directive requires an argument, but %1 were provided").arg(sectionCount - 1));

                continue;
            }

            _classNames.append(sections[1]);

        } else if (sections[0] == QLatin1String("internal")) {
            if (sectionCount == 3) {
                Component entry(sections[1], sections[2], QTypeRevision());
                entry.internal = true;
                _components.insert(entry.typeName, entry);
            } else if (sectionCount == 4) {
                const QTypeRevision version = parseVersion(sections[2]);
                if (version.isValid()) {
                    Component entry(sections[1], sections[3], version);
                    entry.internal = true;
                    _components.insert(entry.typeName, entry);
                } else {
                    reportError(lineNumber, 0,
                                QStringLiteral("invalid version %1, expected <major>.<minor>")
                                    .arg(sections[2]));
                    continue;
                }
            } else {
                reportError(lineNumber, 0,
                            QStringLiteral("internal types require 2 or 3 arguments, "
                                           "but %1 were provided").arg(sectionCount - 1));
                continue;
            }
        } else if (sections[0] == QLatin1String("singleton")) {
            if (sectionCount < 3 || sectionCount > 4) {
                reportError(lineNumber, 0,
                            QStringLiteral("singleton types require 2 or 3 arguments, but %1 were provided").arg(sectionCount - 1));
                continue;
            } else if (sectionCount == 3) {
                // handle qmldir directory listing case where singleton is defined in the following pattern:
                // singleton TestSingletonType TestSingletonType.qml
                Component entry(sections[1], sections[2], QTypeRevision());
                entry.singleton = true;
                _components.insert(entry.typeName, entry);
            } else {
                // handle qmldir module listing case where singleton is defined in the following pattern:
                // singleton TestSingletonType 2.0 TestSingletonType20.qml
                const QTypeRevision version = parseVersion(sections[2]);
                if (version.isValid()) {
                    const QString &fileName = sections[3];
                    Component entry(sections[1], fileName, version);
                    entry.singleton = true;
                    _components.insert(entry.typeName, entry);
                } else {
                    reportError(lineNumber, 0, QStringLiteral("invalid version %1, expected <major>.<minor>").arg(sections[2]));
                }
            }
        } else if (sections[0] == QLatin1String("typeinfo")) {
            if (sectionCount != 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("typeinfo requires 1 argument, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            _typeInfos.append(sections[1]);
        } else if (sections[0] == QLatin1String("designersupported")) {
            if (sectionCount != 1)
                reportError(lineNumber, 0, QStringLiteral("designersupported does not expect any argument"));
            else
                _designerSupported = true;
        } else if (sections[0] == QLatin1String("static")) {
            if (sectionCount != 1)
                reportError(lineNumber, 0, QStringLiteral("static does not expect any argument"));
            else
                _isStaticModule = true;
        } else if (sections[0] == QLatin1String("system")) {
            if (sectionCount != 1)
                reportError(lineNumber, 0, QStringLiteral("system does not expect any argument"));
            else
                _isSystemModule = true;
        } else if (sections[0] == QLatin1String("import")
                   || sections[0] == QLatin1String("depends")) {
            if (!readImport(sections, sectionCount, Import::Default))
                continue;
        } else if (sections[0] == QLatin1String("prefer")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("prefer directive requires one argument, "
                                           "but %1 were provided").arg(sectionCount - 1));
                continue;
            }

            if (!_preferredPath.isEmpty()) {
                reportError(lineNumber, 0, QStringLiteral(
                                "only one prefer directive may be defined in a qmldir file"));
                continue;
            }

            if (!sections[1].endsWith(u'/')) {
                // Yes. People should realize it's a directory.
                reportError(lineNumber, 0, QStringLiteral(
                                "the preferred directory has to end with a '/'"));
                continue;
            }

            _preferredPath = sections[1];
        } else if (sections[0] == QLatin1String("linktarget")) {
            if (sectionCount < 2) {
                reportError(lineNumber, 0,
                            QStringLiteral("linktarget directive requires an argument, "
                                           "but %1 were provided")
                                    .arg(sectionCount - 1));
                continue;
            }

            if (!_linkTarget.isEmpty()) {
                reportError(
                        lineNumber, 0,
                        QStringLiteral(
                                "only one linktarget directive may be defined in a qmldir file"));
                continue;
            }

            _linkTarget = sections[1];
        } else if (sectionCount == 2) {
            // No version specified (should only be used for relative qmldir files)
            const Component entry(sections[0], sections[1], QTypeRevision());
            _components.insert(entry.typeName, entry);
        } else if (sectionCount == 3) {
            const QTypeRevision version = parseVersion(sections[1]);
            if (version.isValid()) {
                const QString &fileName = sections[2];

                if (fileName.endsWith(QLatin1String(".js")) || fileName.endsWith(QLatin1String(".mjs"))) {
                    // A 'js' extension indicates a namespaced script import
                    const Script entry(sections[0], fileName, version);
                    _scripts.append(entry);
                } else {
                    const Component entry(sections[0], fileName, version);
                    _components.insert(entry.typeName, entry);
                }
            } else {
                reportError(lineNumber, 0, QStringLiteral("invalid version %1, expected <major>.<minor>").arg(sections[1]));
            }
        } else {
            reportError(lineNumber, 0,
                        QStringLiteral("a component declaration requires two or three arguments, but %1 were provided").arg(sectionCount));
        }

        firstLine = false;
    }

    return hasError();
}

void QQmlDirParser::reportError(quint16 line, quint16 column, const QString &description)
{
    QQmlJS::DiagnosticMessage error;
    error.loc.startLine = line;
    error.loc.startColumn = column;
    error.message = description;
    _errors.append(error);
}

void QQmlDirParser::setError(const QQmlJS::DiagnosticMessage &e)
{
    _errors.clear();
    reportError(e.loc.startLine, e.loc.startColumn, e.message);
}

QList<QQmlJS::DiagnosticMessage> QQmlDirParser::errors(const QString &uri) const
{
    QList<QQmlJS::DiagnosticMessage> errors;
    const int numErrors = _errors.size();
    errors.reserve(numErrors);
    for (int i = 0; i < numErrors; ++i) {
        QQmlJS::DiagnosticMessage e = _errors.at(i);
        e.message.replace(QLatin1String("$$URI$$"), uri);
        errors << e;
    }
    return errors;
}

QDebug &operator<< (QDebug &debug, const QQmlDirParser::Component &component)
{
    const QString output = QStringLiteral("{%1 %2.%3}")
            .arg(component.typeName).arg(component.version.majorVersion())
            .arg(component.version.minorVersion());
    return debug << qPrintable(output);
}

QDebug &operator<< (QDebug &debug, const QQmlDirParser::Script &script)
{
    const QString output = QStringLiteral("{%1 %2.%3}")
            .arg(script.nameSpace).arg(script.version.majorVersion())
            .arg(script.version.minorVersion());
    return debug << qPrintable(output);
}

QT_END_NAMESPACE
