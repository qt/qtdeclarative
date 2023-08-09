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
                            QStringLiteral("only optional imports can have a default, "
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

/* removes all file selector occurrences in path
   firstPlus is the position of the initial '+' in the path
   which we always have as we check for '+' to decide whether
   we need to do some work at all
*/
static QString pathWithoutFileSelectors(QString path, // we want a copy of path
                                        qsizetype firstPlus)
{
    do {
        Q_ASSERT(path.at(firstPlus) == u'+');
        const auto eos = path.size();
        qsizetype terminatingSlashPos = firstPlus + 1;
        while (terminatingSlashPos != eos && path.at(terminatingSlashPos) != u'/')
            ++terminatingSlashPos;
        path.remove(firstPlus, terminatingSlashPos - firstPlus + 1);
        firstPlus = path.indexOf(u'+', firstPlus);
    } while (firstPlus != -1);
    return path;
}

static bool canDisambiguate(
        const QString &fileName1, const QString &fileName2, QString *correctedFileName)
{
    // If the entries are exactly the same we can delete one without losing anything.
    if (fileName1 == fileName2)
        return true;

    // If we detect conflicting paths, we check if they agree when we remove anything
    // looking like a file selector.

    // ugly heuristic to deal with file selectors
    const qsizetype file2PotentialFileSelectorPos = fileName2.indexOf(u'+');
    const bool file2MightHaveFileSelector = file2PotentialFileSelectorPos != -1;

    if (const qsizetype fileSelectorPos1 = fileName1.indexOf(u'+'); fileSelectorPos1 != -1) {
        // existing entry was file selector entry, fix it up
        // it could also be the case that _both_ are using file selectors
        const QString baseName = file2MightHaveFileSelector
                ? pathWithoutFileSelectors(fileName2, file2PotentialFileSelectorPos)
                : fileName2;

        if (pathWithoutFileSelectors(fileName1, fileSelectorPos1) != baseName)
            return false;

        *correctedFileName = baseName;
        return true;
    }

    // new entry contains file selector (and we know that fileName1 did not)
    if (file2MightHaveFileSelector
            && pathWithoutFileSelectors(fileName2, file2PotentialFileSelectorPos) == fileName1) {
        *correctedFileName = fileName1;
        return true;
    }

    return false;
}

static void disambiguateFileSelectedComponents(QQmlDirComponents *components)
{
    using ConstIterator = QQmlDirComponents::const_iterator;

    // end iterator may get invalidated by the erasing below.
    // Therefore, refetch it on each iteration.
    for (ConstIterator cit = components->constBegin(); cit != components->constEnd();) {

        // We can erase and re-assign cit if we immediately forget cit2.
        // But we cannot erase cit2 without potentially invalidating cit.

        bool doErase = false;
        const ConstIterator cend = components->constEnd();
        for (ConstIterator cit2 = ++ConstIterator(cit); cit2 != cend; ++cit2) {
            if (cit2.key() != cit.key())
                break;

            Q_ASSERT(cit2->typeName == cit->typeName);

            if (cit2->version != cit->version
                || cit2->internal != cit->internal
                || cit2->singleton != cit->singleton) {
                continue;
            }

            // The two components may differ only by fileName now.

            if (canDisambiguate(cit->fileName, cit2->fileName, &(cit2->fileName))) {
                doErase = true;
                break;
            }
        }

        if (doErase)
            cit = components->erase(cit);
        else
            ++cit;
    }
}

static void disambiguateFileSelectedScripts(QQmlDirScripts *scripts)
{
    using Iterator = QQmlDirScripts::iterator;

    Iterator send = scripts->end();

    for (Iterator sit = scripts->begin(); sit != send; ++sit) {
        send = std::remove_if(++Iterator(sit), send, [sit](const QQmlDirParser::Script &script2) {
            if (sit->nameSpace != script2.nameSpace || sit->version != script2.version)
                return false;

            // The two scripts may differ only by fileName now.
            return canDisambiguate(sit->fileName, script2.fileName, &(sit->fileName));
        });
    }

    scripts->erase(send, scripts->end());
}

void QQmlDirParser::disambiguateFileSelectors()
{
    disambiguateFileSelectedComponents(&_components);
    disambiguateFileSelectedScripts(&_scripts);
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
