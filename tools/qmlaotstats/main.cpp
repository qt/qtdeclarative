// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <private/qqmljscompilerstats_p.h>
#include <private/qqmljscompilerstatsreporter_p.h>

using namespace Qt::Literals::StringLiterals;

bool saveFormattedStats(const QString &stats, const QString &outputPath)
{
    QString directory = QFileInfo(outputPath).dir().path();
    if (!QDir().mkpath(directory)) {
        qDebug() << "Could not ensure the existence of" << directory;
        return false;
    }

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::Text | QIODevice::WriteOnly)) {
        qDebug() << "Could not open file" << outputPath;
        return false;
    }

    if (outputFile.write(stats.toLatin1()) == -1) {
        qDebug() << "Could not write formatted AOT stats to" << outputPath;
        return false;
    } else {
        qDebug() << "Formatted AOT stats saved to" << outputPath;
    }

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.setApplicationDescription("Internal development tool.");
    parser.addPositionalArgument("mode", "Choose whether to aggregate or display aotstats files",
                                 "[aggregate|format]");
    parser.addPositionalArgument("input", "Aggregate mode: the aotstatslist file to aggregate. "
                                          "Format mode: the aotstats file to display.");
    parser.addPositionalArgument("output", "Aggregate mode: the path where to store the "
                                           "aggregated aotstats. Format mode: the the path where "
                                           "the formatted output will be saved.");
    QCommandLineOption emptyModulesOption("empty-modules", QCoreApplication::translate("main", "Format mode: File containing a list of modules with no QML files."), "file");
    parser.addOption(emptyModulesOption);
    QCommandLineOption onlyBytecodeModulesOption("only-bytecode-modules", QCoreApplication::translate("main", "Format mode: File containing a list of modules for which only the bytecode is generated."), "file");
    parser.addOption(onlyBytecodeModulesOption);
    parser.process(app);

    const auto &positionalArgs = parser.positionalArguments();
    if (positionalArgs.size() != 3) {
        qDebug().noquote() << parser.helpText();
        return EXIT_FAILURE;
    }

    const auto &mode = positionalArgs.first();
    if (mode == u"aggregate"_s) {
        const auto aggregated = QQmlJS::AotStats::aggregateAotstatsList(positionalArgs[1]);
        if (!aggregated.has_value())
            return EXIT_FAILURE;
        if (!aggregated->saveToDisk(positionalArgs[2]))
            return EXIT_FAILURE;

    } else if (mode == u"format"_s) {
        const auto aotstats = QQmlJS::AotStats::parseAotstatsFile(positionalArgs[1]);
        if (!aotstats.has_value())
            return EXIT_FAILURE;

        const auto emptyModules = parser.isSet(emptyModulesOption)
                ? QQmlJS::AotStats::readAllLines(parser.value(emptyModulesOption))
                : QStringList();
        const auto onlyBytecodeModules = parser.isSet(onlyBytecodeModulesOption)
                ? QQmlJS::AotStats::readAllLines(parser.value(onlyBytecodeModulesOption))
                : QStringList();
        if (!emptyModules || !onlyBytecodeModules)
            return EXIT_FAILURE;

        const QQmlJS::AotStatsReporter reporter(aotstats.value(), emptyModules.value(),
                                                onlyBytecodeModules.value());
        if (!saveFormattedStats(reporter.format(), positionalArgs[2]))
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
