/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QByteArray>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <QtEndian>
#include <QStack>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>

/*!
 * \internal
 * Mangles \a str to be a unique C++ identifier. Characters that are invalid for C++ identifiers
 * are replaced by the pattern \c _0x<hex>_ where <hex> is the hexadecimal unicode
 * representation of the character. As identifiers with leading underscores followed by either
 * another underscore or a capital letter are reserved in C++, we also escape those, by escaping
 * the first underscore, using the above method.
 *
 * \note
 * Although C++11 allows for non-ascii (unicode) characters to be used in identifiers,
 * many compilers forgot to read the spec and do not implement this. Some also do not
 * implement C99 identifiers, because that is \e {at the implementation's discretion}. So,
 * we are stuck with plain old boring identifiers.
 */
QString mangledIdentifier(const QString &str)
{
    Q_ASSERT(!str.isEmpty());

    QString mangled;
    mangled.reserve(str.size());

    int i = 0;
    if (str.startsWith(QLatin1Char('_')) && str.size() > 1) {
        QChar ch = str.at(1);
        if (ch == QLatin1Char('_')
                || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))) {
            mangled += QLatin1String("_0x5f_");
            ++i;
        }
    }

    for (int ei = str.length(); i != ei; ++i) {
        auto c = str.at(i).unicode();
        if ((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
            || (c >= QLatin1Char('a') && c <= QLatin1Char('z'))
            || (c >= QLatin1Char('A') && c <= QLatin1Char('Z'))
            || c == QLatin1Char('_')) {
            mangled += QChar(c);
        } else {
            mangled += QLatin1String("_0x") + QString::number(c, 16) + QLatin1Char('_');
        }
    }

    return mangled;
}

QString symbolNamespaceForPath(const QString &relativePath)
{
    QFileInfo fi(relativePath);
    QString symbol = fi.path();
    if (symbol.length() == 1 && symbol.startsWith(QLatin1Char('.'))) {
        symbol.clear();
    } else {
        symbol.replace(QLatin1Char('/'), QLatin1Char('_'));
        symbol += QLatin1Char('_');
    }
    symbol += fi.baseName();
    symbol += QLatin1Char('_');
    symbol += fi.completeSuffix();
    return mangledIdentifier(symbol);
}

static QString qtResourceNameForFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString name = fi.completeBaseName();
    if (name.isEmpty())
        name = fi.fileName();
    name.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
    return name;
}

bool generateLoader(const QStringList &compiledFiles, const QString &outputFileName,
                    const QStringList &resourceFileMappings, QString *errorString)
{
    QByteArray generatedLoaderCode;

    {
        QTextStream stream(&generatedLoaderCode);
        stream << "#include <QtQml/qqmlprivate.h>\n";
        stream << "#include <QtCore/qdir.h>\n";
        stream << "#include <QtCore/qurl.h>\n";
        stream << "\n";

        stream << "namespace QmlCacheGeneratedCode {\n";
        for (int i = 0; i < compiledFiles.count(); ++i) {
            const QString compiledFile = compiledFiles.at(i);
            const QString ns = symbolNamespaceForPath(compiledFile);
            stream << "namespace " << ns << " { \n";
            stream << "    extern const unsigned char qmlData[];\n";
            stream << "    const QQmlPrivate::CachedQmlUnit unit = {\n";
            stream << "        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), nullptr, nullptr\n";
            stream << "    };\n";
            stream << "}\n";
        }

        stream << "\n}\n";
        stream << "namespace {\n";

        stream << "struct Registry {\n";
        stream << "    Registry();\n";
        stream << "    ~Registry();\n";
        stream << "    QHash<QString, const QQmlPrivate::CachedQmlUnit*> resourcePathToCachedUnit;\n";
        stream << "    static const QQmlPrivate::CachedQmlUnit *lookupCachedUnit(const QUrl &url);\n";
        stream << "};\n\n";
        stream << "Q_GLOBAL_STATIC(Registry, unitRegistry)\n";
        stream << "\n\n";

        stream << "Registry::Registry() {\n";

        for (int i = 0; i < compiledFiles.count(); ++i) {
            const QString qrcFile = compiledFiles.at(i);
            const QString ns = symbolNamespaceForPath(qrcFile);
            stream << "        resourcePathToCachedUnit.insert(QStringLiteral(\"" << qrcFile << "\"), &QmlCacheGeneratedCode::" << ns << "::unit);\n";
        }

        stream << "    QQmlPrivate::RegisterQmlUnitCacheHook registration;\n";
        stream << "    registration.version = 0;\n";
        stream << "    registration.lookupCachedQmlUnit = &lookupCachedUnit;\n";
        stream << "    QQmlPrivate::qmlregister(QQmlPrivate::QmlUnitCacheHookRegistration, &registration);\n";

        stream << "}\n\n";
        stream << "Registry::~Registry() {\n";
        stream << "    QQmlPrivate::qmlunregister(QQmlPrivate::QmlUnitCacheHookRegistration, quintptr(&lookupCachedUnit));\n";
        stream << "}\n\n";

        stream << "const QQmlPrivate::CachedQmlUnit *Registry::lookupCachedUnit(const QUrl &url) {\n";
        stream << "    if (url.scheme() != QLatin1String(\"qrc\"))\n";
        stream << "        return nullptr;\n";
        stream << "    QString resourcePath = QDir::cleanPath(url.path());\n";
        stream << "    if (resourcePath.isEmpty())\n";
        stream << "        return nullptr;\n";
        stream << "    if (!resourcePath.startsWith(QLatin1Char('/')))\n";
        stream << "        resourcePath.prepend(QLatin1Char('/'));\n";
        stream << "    return unitRegistry()->resourcePathToCachedUnit.value(resourcePath, nullptr);\n";
        stream << "}\n";
        stream << "}\n";

        for (const QString &mapping: resourceFileMappings) {
            QString originalResourceFile = mapping;
            QString newResourceFile;
            const int mappingSplit = originalResourceFile.indexOf(QLatin1Char('='));
            if (mappingSplit != -1) {
                newResourceFile = originalResourceFile.mid(mappingSplit + 1);
                originalResourceFile.truncate(mappingSplit);
            }

            const QString suffix = qtResourceNameForFile(originalResourceFile);
            const QString initFunction = QLatin1String("qInitResources_") + suffix;

            stream << QStringLiteral("int QT_MANGLE_NAMESPACE(%1)() {\n").arg(initFunction);
            stream << "    ::unitRegistry();\n";
            if (!newResourceFile.isEmpty())
                stream << "    Q_INIT_RESOURCE(" << qtResourceNameForFile(newResourceFile) << ");\n";
            stream << "    return 1;\n";
            stream << "}\n";
            stream << "Q_CONSTRUCTOR_FUNCTION(QT_MANGLE_NAMESPACE(" << initFunction << "))\n";

            const QString cleanupFunction = QLatin1String("qCleanupResources_") + suffix;
            stream << QStringLiteral("int QT_MANGLE_NAMESPACE(%1)() {\n").arg(cleanupFunction);
            if (!newResourceFile.isEmpty())
                stream << "    Q_CLEANUP_RESOURCE(" << qtResourceNameForFile(newResourceFile) << ");\n";
            stream << "    return 1;\n";
            stream << "}\n";
        }
    }

#if QT_CONFIG(temporaryfile)
    QSaveFile f(outputFileName);
#else
    QFile f(outputFileName);
#endif
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *errorString = f.errorString();
        return false;
    }

    if (f.write(generatedLoaderCode) != generatedLoaderCode.size()) {
        *errorString = f.errorString();
        return false;
    }

#if QT_CONFIG(temporaryfile)
    if (!f.commit()) {
        *errorString = f.errorString();
        return false;
    }
#endif

    return true;
}
