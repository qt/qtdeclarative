// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomlinewriterfactory_p.h>

#if QT_CONFIG(commandlineparser)
#    include <QCommandLineParser>
#endif

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlFormat/private/qqmlformatsettings_p.h>
#include <QtQmlFormat/private/qqmlformatoptions_p.h>

using namespace QQmlJS::Dom;

static void logParsingErrors(const DomItem &fileItem, const QString &filename)
{
    fileItem.iterateErrors(
            [](const DomItem &, const ErrorMessage &msg) {
                errorToQDebug(msg);
                return true;
            },
            true);
    qWarning().noquote() << "Failed to parse" << filename;
}

// TODO
// refactor this workaround. ExternalOWningItem is not recognized as an owning type
// in ownerAs.
static std::shared_ptr<ExternalOwningItem> getFileItemOwner(const DomItem &fileItem)
{
    std::shared_ptr<ExternalOwningItem> filePtr = nullptr;
    switch (fileItem.internalKind()) {
    case DomType::JsFile:
        filePtr = fileItem.ownerAs<JsFile>();
        break;
    default:
        filePtr = fileItem.ownerAs<QmlFile>();
        break;
    }
    return filePtr;
}

// TODO refactor
// Introduce better encapsulation and separation of concerns and move to DOM API
// returns a DomItem corresponding to the loaded file and bool indicating the validity of the file
static std::pair<DomItem, bool> parse(const QString &filename)
{
    auto envPtr =
            DomEnvironment::create(QStringList(),
                                   QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                                           | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
    // placeholder for a node
    // containing metadata (ExternalItemInfo) about the loaded file
    DomItem fMetadataItem;
    envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, filename),
                     // callback called when everything is loaded that receives the
                     // loaded external file pair (path, oldValue, newValue)
                     [&fMetadataItem](Path, const DomItem &, const DomItem &extItemInfo) {
                         fMetadataItem = extItemInfo;
                     });
    auto fItem = fMetadataItem.fileObject();
    auto filePtr = getFileItemOwner(fItem);
    return { fItem, filePtr && filePtr->isValid() };
}

static bool parseFile(const QString &filename, const QQmlFormatOptions &options)
{
    const auto [fileItem, validFile] = parse(filename);
    if (!validFile) {
        logParsingErrors(fileItem, filename);
        return false;
    }

    // Turn AST back into source code
    if (options.isVerbose())
        qWarning().noquote() << "Dumping" << filename;

    const auto &code = getFileItemOwner(fileItem)->code();
    auto lwOptions = options.optionsForCode(code);
    WriteOutChecks checks = WriteOutCheck::Default;
    //Disable writeOutChecks for some usecases
    if (options.sortImports() || options.forceEnabled() || options.isMaxColumnWidthSet() || code.size() > 32000
        || fileItem.internalKind() == DomType::JsFile) {
        checks = WriteOutCheck::None;
    }

    bool res = false;
    if (options.isInplace()) {
        if (options.isVerbose())
            qWarning().noquote() << "Writing to file" << filename;
        FileWriter fw;
        const unsigned numberOfBackupFiles = 0;
        res = fileItem.writeOut(filename, numberOfBackupFiles, lwOptions, &fw, checks);
    } else {
        QFile out;
        if (out.open(stdout, QIODevice::WriteOnly)) {
            auto lw = createLineWriter([&out](QStringView s) { out.write(s.toUtf8()); }, filename,
                                       lwOptions);
            OutWriter ow(*lw);
            res = fileItem.writeOutForFile(ow, checks);
            ow.flush();
        } else {
            res = false;
        }
    }
    return res;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qmlformat");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR "-preserveProps");

    QQmlFormatSettings settings(QLatin1String("qmlformat"));
    const auto &options = QQmlFormatOptions::buildCommandLineOptions(app.arguments());
    if (!options.isValid()) {
        for (const auto &error : options.errors()) {
            qWarning().noquote() << error;
        }

        return -1;
    }

    if (options.writeDefaultSettingsEnabled())
        return settings.writeDefaults() ? 0 : -1;

    bool success = true;
    if (!options.files().isEmpty()) {
        if (!options.arguments().isEmpty())
            qWarning() << "Warning: Positional arguments are ignored when -F is used";

        for (const QString &file : options.files()) {
            Q_ASSERT(!file.isEmpty());

            if (!parseFile(file, options.optionsForFile(file, &settings)))
                success = false;
        }
    } else {
        for (const QString &file : options.arguments()) {
            if (!parseFile(file, options.optionsForFile(file, &settings)))
                success = false;
        }
    }

    return success ? 0 : 1;
}
