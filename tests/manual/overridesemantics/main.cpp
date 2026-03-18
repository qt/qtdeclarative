// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "cpptypes.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtemporarydir.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <algorithm>
#include <vector>

using namespace Qt::StringLiterals;

// ---------------------------------------------------------------------------
// Message capture
// ---------------------------------------------------------------------------

struct CapturedMessage
{
    QtMsgType type;
    QString category;
    QString text;
};

static std::vector<CapturedMessage> s_messages;
static QtMessageHandler s_previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    s_messages.push_back({ type, QString::fromUtf8(ctx.category), msg });
}

static void installHandler()
{
    s_messages.clear();
    s_previousHandler = qInstallMessageHandler(messageHandler);
}

static void restoreHandler()
{
    qInstallMessageHandler(s_previousHandler);
}

// ---------------------------------------------------------------------------
// Classify captured messages about property cache / override semantics
// ---------------------------------------------------------------------------

struct RuntimeResult
{
    QString level; // "clean", "debug", "warning", "error", "unknown"
    QString message; // full message text
};

static RuntimeResult classifyMessages()
{
    if (s_messages.empty())
        return { u"clean"_s, {} };

    RuntimeResult result{ u"unknown"_s, {} };

    for (const auto &m : s_messages) {
        if (!m.category.startsWith(u"qt.qml.propertyCache"_s))
            continue;

        QString level;
        if (m.type == QtDebugMsg)
            level = u"debug"_s;
        else if (m.type == QtWarningMsg || m.type == QtCriticalMsg)
            level = u"warning"_s;

        // Keep the most severe
        if (result.level == u"unknown"_s || (result.level == u"debug"_s && level == u"warning"_s)) {
            result.level = level;
            result.message = m.text;
        }
    }

    // If there were messages but none matched our category, flag as unexpected
    if (result.level == u"unknown"_s)
        result.message = u"unexpected: "_s + s_messages.front().text;

    return result;
}

// ---------------------------------------------------------------------------
// Test-case definition
// ---------------------------------------------------------------------------

struct TestCase
{
    const char *baseDescription; // e.g. "C++ plain property"
    const char *derivedDescription; // e.g. "QML override property"
    // For C++ derived types: the QML type name to instantiate
    // For QML derived types: empty (qmlSnippet is used instead)
    const char *cppTypeName;
    // QML body for the derived member (only for QML derived tests)
    // e.g. "override property int value: 2"
    const char *baseTypeName; // QML type name of the base
    const char *qmlMember; // QML code for the derived member
    bool derivedIsMethod; // true if derived declares a method, not a property
};

// clang-format off
static const TestCase s_testCases[] = {
    // ===== C++ over C++, grouped by base =====
    { "C++ plain property",    "C++ plain property",     "CppBare_CppPlain",      nullptr, nullptr, false },
    { "C++ plain property",    "C++ VIRTUAL property",   "CppVirtual_CppPlain",   nullptr, nullptr, false },
    { "C++ plain property",    "C++ OVERRIDE property",  "CppOverride_CppPlain",  nullptr, nullptr, false },
    { "C++ plain property",    "C++ FINAL property",     "CppFinal_CppPlain",     nullptr, nullptr, false },
    { "C++ plain property",    "C++ method",             "CppMethod_CppPlain",    nullptr, nullptr, true },

    { "C++ VIRTUAL property",  "C++ plain property",     "CppBare_CppVirtual",    nullptr, nullptr, false },
    { "C++ VIRTUAL property",  "C++ VIRTUAL property",   "CppVirtual_CppVirtual", nullptr, nullptr, false },
    { "C++ VIRTUAL property",  "C++ OVERRIDE property",  "CppOverride_CppVirtual",nullptr, nullptr, false },
    { "C++ VIRTUAL property",  "C++ FINAL property",     "CppFinal_CppVirtual",   nullptr, nullptr, false },
    { "C++ VIRTUAL property",  "C++ method",             "CppMethod_CppVirtual",  nullptr, nullptr, true },

    { "C++ FINAL property",    "C++ plain property",     "CppBare_CppFinal",      nullptr, nullptr, false },
    { "C++ FINAL property",    "C++ VIRTUAL property",   "CppVirtual_CppFinal",   nullptr, nullptr, false },
    { "C++ FINAL property",    "C++ OVERRIDE property",  "CppOverride_CppFinal",  nullptr, nullptr, false },
    { "C++ FINAL property",    "C++ FINAL property",     "CppFinal_CppFinal",     nullptr, nullptr, false },
    { "C++ FINAL property",    "C++ method",             "CppMethod_CppFinal",    nullptr, nullptr, true },

    { "C++ OVERRIDE property", "C++ plain property",     "CppBare_CppOverride",     nullptr, nullptr, false },
    { "C++ OVERRIDE property", "C++ VIRTUAL property",   "CppVirtual_CppOverride",  nullptr, nullptr, false },
    { "C++ OVERRIDE property", "C++ OVERRIDE property",  "CppOverride_CppOverride", nullptr, nullptr, false },
    { "C++ OVERRIDE property", "C++ FINAL property",     "CppFinal_CppOverride",    nullptr, nullptr, false },
    { "C++ OVERRIDE property", "C++ method",             "CppMethod_CppOverride",   nullptr, nullptr, true },

    { "C++ method",            "C++ plain property",     "CppBare_CppMethod",     nullptr, nullptr, false },
    { "C++ method",            "C++ VIRTUAL property",   "CppVirtual_CppMethod",  nullptr, nullptr, false },
    { "C++ method",            "C++ OVERRIDE property",  "CppOverride_CppMethod", nullptr, nullptr, false },
    { "C++ method",            "C++ FINAL property",     "CppFinal_CppMethod",    nullptr, nullptr, false },
    { "C++ method",            "C++ method",             "CppMethod_CppMethod",   nullptr, nullptr, true },

    // ===== QML over C++, grouped by base =====
    { "C++ plain property",    "QML plain property",     nullptr, "CppPlainBase",   "property int value: 2", false },
    { "C++ plain property",    "QML virtual property",   nullptr, "CppPlainBase",   "virtual property int value: 2", false },
    { "C++ plain property",    "QML override property",  nullptr, "CppPlainBase",   "override property int value: 2", false },
    { "C++ plain property",    "QML final property",     nullptr, "CppPlainBase",   "final property int value: 2", false },
    { "C++ plain property",    "QML method",             nullptr, "CppPlainBase",   "function value() : int { return 2; }", true },

    { "C++ VIRTUAL property",  "QML plain property",     nullptr, "CppVirtualBase", "property int value: 2", false },
    { "C++ VIRTUAL property",  "QML virtual property",   nullptr, "CppVirtualBase", "virtual property int value: 2", false },
    { "C++ VIRTUAL property",  "QML override property",  nullptr, "CppVirtualBase", "override property int value: 2", false },
    { "C++ VIRTUAL property",  "QML final property",     nullptr, "CppVirtualBase", "final property int value: 2", false },
    { "C++ VIRTUAL property",  "QML method",             nullptr, "CppVirtualBase", "function value() : int { return 2; }", true },

    { "C++ FINAL property",    "QML plain property",     nullptr, "CppFinalBase",   "property int value: 2", false },
    { "C++ FINAL property",    "QML virtual property",   nullptr, "CppFinalBase",   "virtual property int value: 2", false },
    { "C++ FINAL property",    "QML override property",  nullptr, "CppFinalBase",   "override property int value: 2", false },
    { "C++ FINAL property",    "QML final property",     nullptr, "CppFinalBase",   "final property int value: 2", false },
    { "C++ FINAL property",    "QML method",             nullptr, "CppFinalBase",   "function value() : int { return 2; }", true },

    { "C++ OVERRIDE property", "QML plain property",     nullptr, "CppOverrideBase","property int value: 2", false },
    { "C++ OVERRIDE property", "QML virtual property",   nullptr, "CppOverrideBase","virtual property int value: 2", false },
    { "C++ OVERRIDE property", "QML override property",  nullptr, "CppOverrideBase","override property int value: 2", false },
    { "C++ OVERRIDE property", "QML final property",     nullptr, "CppOverrideBase","final property int value: 2", false },
    { "C++ OVERRIDE property", "QML method",             nullptr, "CppOverrideBase","function value() : int { return 2; }", true },

    { "C++ method",            "QML plain property",     nullptr, "CppMethodBase",  "property int value: 2", false },
    { "C++ method",            "QML virtual property",   nullptr, "CppMethodBase",  "virtual property int value: 2", false },
    { "C++ method",            "QML override property",  nullptr, "CppMethodBase",  "override property int value: 2", false },
    { "C++ method",            "QML final property",     nullptr, "CppMethodBase",  "final property int value: 2", false },
    { "C++ method",            "QML method",             nullptr, "CppMethodBase",  "function value() : int { return 2; }", true },

    // ===== QML over QML, grouped by base =====
    { "QML plain property",    "QML plain property",     nullptr, "QmlPlainBase",   "property int value: 2", false },
    { "QML plain property",    "QML virtual property",   nullptr, "QmlPlainBase",   "virtual property int value: 2", false },
    { "QML plain property",    "QML override property",  nullptr, "QmlPlainBase",   "override property int value: 2", false },
    { "QML plain property",    "QML final property",     nullptr, "QmlPlainBase",   "final property int value: 2", false },
    { "QML plain property",    "QML method",             nullptr, "QmlPlainBase",   "function value() : int { return 2; }", true },

    { "QML virtual property",  "QML plain property",     nullptr, "QmlVirtualBase", "property int value: 2", false },
    { "QML virtual property",  "QML virtual property",   nullptr, "QmlVirtualBase", "virtual property int value: 2", false },
    { "QML virtual property",  "QML override property",  nullptr, "QmlVirtualBase", "override property int value: 2", false },
    { "QML virtual property",  "QML final property",     nullptr, "QmlVirtualBase", "final property int value: 2", false },
    { "QML virtual property",  "QML method",             nullptr, "QmlVirtualBase", "function value() : int { return 2; }", true },

    { "QML final property",    "QML plain property",     nullptr, "QmlFinalBase",   "property int value: 2", false },
    { "QML final property",    "QML virtual property",   nullptr, "QmlFinalBase",   "virtual property int value: 2", false },
    { "QML final property",    "QML override property",  nullptr, "QmlFinalBase",   "override property int value: 2", false },
    { "QML final property",    "QML final property",     nullptr, "QmlFinalBase",   "final property int value: 2", false },
    { "QML final property",    "QML method",             nullptr, "QmlFinalBase",   "function value() : int { return 2; }", true },

    { "QML override property", "QML plain property",     nullptr, "QmlOverrideBase","property int value: 2", false },
    { "QML override property", "QML virtual property",   nullptr, "QmlOverrideBase","virtual property int value: 2", false },
    { "QML override property", "QML override property",  nullptr, "QmlOverrideBase","override property int value: 2", false },
    { "QML override property", "QML final property",     nullptr, "QmlOverrideBase","final property int value: 2", false },
    { "QML override property", "QML method",             nullptr, "QmlOverrideBase","function value() : int { return 2; }", true },

    { "QML method",            "QML plain property",     nullptr, "QmlMethodBase",  "property int value: 2", false },
    { "QML method",            "QML virtual property",   nullptr, "QmlMethodBase",  "virtual property int value: 2", false },
    { "QML method",            "QML override property",  nullptr, "QmlMethodBase",  "override property int value: 2", false },
    { "QML method",            "QML final property",     nullptr, "QmlMethodBase",  "final property int value: 2", false },
    { "QML method",            "QML method",             nullptr, "QmlMethodBase",  "function value() : int { return 2; }", true },
};
// clang-format on

// ---------------------------------------------------------------------------
// QML snippet generation
// ---------------------------------------------------------------------------

static QByteArray makeQml(const TestCase &tc)
{
    // The testValue property reads "value" through QML, which uses the property
    // cache (unlike C++ QObject::property() which uses QMetaObject directly).
    // This correctly reflects override-dropped scenarios.
    const char *testValueBinding = tc.derivedIsMethod
            ? "    property var testValue: { var v = value; return typeof v === 'function' ? v() : "
              "v; }\n"
            : "    property var testValue: value\n";

    if (tc.cppTypeName) {
        // C++ derived type — instantiate it and add testValue binding
        return QByteArray("import QtQml\nimport OverrideSemantics\n") + tc.cppTypeName + " {\n"
                + testValueBinding + "}\n";
    }
    // QML derived type
    return QByteArray("import QtQml\nimport OverrideSemantics\n") + tc.baseTypeName + " {\n    "
            + tc.qmlMember + "\n" + testValueBinding + "}\n";
}

// QML for qmllint — without the testValue helper, so it only lints the actual override
static QByteArray makeLintQml(const TestCase &tc)
{
    if (tc.cppTypeName) {
        return QByteArray("import QtQml\nimport OverrideSemantics\n") + tc.cppTypeName + " {}\n";
    }
    return QByteArray("import QtQml\nimport OverrideSemantics\n") + tc.baseTypeName + " {\n    "
            + tc.qmlMember + "\n}\n";
}

// ---------------------------------------------------------------------------
// Runtime test
// ---------------------------------------------------------------------------

struct RuntimeTestResult
{
    QString level; // "clean", "debug", "warning", "error", "unknown"
    QString message; // full diagnostic message
    int cppPropValue = -1;    // value read via QObject::property("value")
    int cppMethodValue = -1;  // value read via QMetaObject::invokeMethod("value")
    int qmlValue = -1;        // value read via QML binding (testValue)
};

static RuntimeTestResult runRuntimeTest(QQmlEngine *engine, const TestCase &tc)
{
    RuntimeTestResult result;
    const QByteArray qml = makeQml(tc);

    installHandler();

    QQmlComponent component(engine);
    component.setData(qml, QUrl(u"test.qml"_s));

    if (component.isError()) {
        restoreHandler();
        auto rt = classifyMessages();
        result.level = u"error"_s;
        const auto errors = component.errors();
        if (!errors.isEmpty())
            result.message = errors.first().description();
        if (result.message.isEmpty())
            result.message = rt.message;
        return result;
    }

    QScopedPointer<QObject> obj(component.create());

    restoreHandler();

    if (!obj) {
        auto rt = classifyMessages();
        result.level = u"error"_s;
        result.message = rt.message.isEmpty() ? u"create() returned null"_s : rt.message;
        return result;
    }

    // Read value through both paths to show the difference
    // C++ property path: QObject::property() uses QMetaObject directly
    QVariant propVal = obj->property("value");
    if (propVal.isValid()) {
        bool ok = false;
        int v = propVal.toInt(&ok);
        result.cppPropValue = ok ? v : -1;
    }
    // C++ method path: QMetaObject::invokeMethod()
    // Only attempt if the meta-object actually has a "value()" method.
    if (obj->metaObject()->indexOfMethod("value()") != -1) {
        int methodResult = -1;
        if (QMetaObject::invokeMethod(obj.data(), "value", Q_RETURN_ARG(int, methodResult)))
            result.cppMethodValue = methodResult;
    }
    // QML path: testValue binding goes through the QML property cache
    QVariant qmlVal = obj->property("testValue");
    if (qmlVal.isValid()) {
        bool ok = false;
        int v = qmlVal.toInt(&ok);
        result.qmlValue = ok ? v : -1;
    }

    auto rt = classifyMessages();
    result.level = rt.level;
    result.message = rt.message;

    return result;
}

// ---------------------------------------------------------------------------
// qmllint test
// ---------------------------------------------------------------------------

static QString findQmllint()
{
    // Use QLibraryInfo to find qmllint in the Qt installation
    const QString binDir = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    const QString candidate = binDir + u"/qmllint"_s;
    if (QFile::exists(candidate))
        return candidate;

    // Try libexec too
    const QString libexecDir = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath);
    const QString candidate2 = libexecDir + u"/qmllint"_s;
    if (QFile::exists(candidate2))
        return candidate2;

    // Fallback: try PATH
    return u"qmllint"_s;
}

struct LintResult
{
    QString level; // "clean", "warning", "error", "unknown", "n/a"
    QString message; // full qmllint output
};

static LintResult runQmllint(const QString &qmllintPath, const QString &filePath,
                             const QStringList &importPaths)
{
    LintResult result;

    QProcess proc;
    QStringList args;
    for (const QString &path : importPaths) {
        args << u"-I"_s << path;
    }
    args << filePath;

    proc.start(qmllintPath, args);
    if (!proc.waitForFinished(10000)) {
        result.level = u"n/a"_s;
        result.message = u"timeout"_s;
        return result;
    }

    const QString output = (QString::fromUtf8(proc.readAllStandardError())
                            + QString::fromUtf8(proc.readAllStandardOutput())).trimmed();

    if (output.isEmpty()) {
        result.level = u"clean"_s;
    } else if (output.contains(u"Error:"_s) || output.contains(u"error:"_s)) {
        result.level = u"error"_s;
    } else if (output.contains(u"Warning:"_s) || output.contains(u"warning:"_s)) {
        result.level = u"warning"_s;
    } else if (output.contains(u"Info:"_s) || output.contains(u"info:"_s)) {
        result.level = u"info"_s;
    } else {
        result.level = u"unknown"_s; // unexpected output
    }

    result.message = output;

    return result;
}

// ---------------------------------------------------------------------------
// Table formatting
// ---------------------------------------------------------------------------

static QString padRight(const QString &s, int width)
{
    return s + QString(qMax(0, width - s.size()), u' ');
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    // Enable debug-level logging for the property cache
    QLoggingCategory::setFilterRules(u"qt.qml.propertyCache.append.debug=true"_s);

    QCoreApplication app(argc, argv);

    const bool verbose = app.arguments().contains(u"--verbose"_s);

    QQmlEngine engine;

    // Set up import paths: the module is in qrc, but we also need file-system
    // access for QML base types when using setData()
    const QString appDir = QCoreApplication::applicationDirPath();
    engine.addImportPath(appDir);

    // Find qmllint
    const QString qmllintPath = findQmllint();

    // Import paths for qmllint
    QStringList lintImportPaths;
    lintImportPaths << appDir;
    lintImportPaths << QLibraryInfo::path(QLibraryInfo::QmlImportsPath);

    // Temporary directory for generated QML files
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        qWarning("Failed to create temporary directory");
        return 1;
    }

    // Column widths
    const int colBase = 22;
    const int colDerived = 24;
    const int colRuntime = 7;
    const int colCppVal = 9;
    const int colQmlVal = 9;
    const int colLint = 7;

    auto mdRow = [&](const QString &base, const QString &derived, const QString &runtime,
                     const QString &cppVal, const QString &qmlVal, const QString &lint,
                     const QString &rtMsg = {}, const QString &lintMsg = {}) {
        fprintf(stdout, "| %s | %s | %s | %s | %s | %s",
                qPrintable(padRight(base, colBase)),
                qPrintable(padRight(derived, colDerived)),
                qPrintable(padRight(runtime, colRuntime)),
                qPrintable(padRight(cppVal, colCppVal)),
                qPrintable(padRight(qmlVal, colQmlVal)),
                qPrintable(padRight(lint, colLint)));
        if (verbose)
            fprintf(stdout, " | %s | %s", qPrintable(rtMsg), qPrintable(lintMsg));
        fprintf(stdout, " |\n");
    };

    auto mdSep = [&]() {
        fprintf(stdout, "|%s|%s|%s|%s|%s|%s",
                qPrintable(QString(colBase + 2, u'-')),
                qPrintable(QString(colDerived + 2, u'-')),
                qPrintable(QString(colRuntime + 2, u'-')),
                qPrintable(QString(colCppVal + 2, u'-')),
                qPrintable(QString(colQmlVal + 2, u'-')),
                qPrintable(QString(colLint + 2, u'-')));
        if (verbose)
            fprintf(stdout, "|---|---");
        fprintf(stdout, "|\n");
    };

    // Header
    mdRow(u"Base"_s, u"Derived"_s, u"Runtime"_s, u"C++ value"_s, u"QML value"_s, u"qmllint"_s,
          verbose ? u"Runtime message"_s : QString(),
          verbose ? u"qmllint message"_s : QString());
    mdSep();

    const size_t numTests = sizeof(s_testCases) / sizeof(s_testCases[0]);
    QString lastBaseDesc;

    for (size_t i = 0; i < numTests; ++i) {
        const TestCase &tc = s_testCases[i];

        // Print separator between different base groups
        const QString baseDesc = QString::fromUtf8(tc.baseDescription);
        if (!lastBaseDesc.isEmpty() && baseDesc != lastBaseDesc)
            mdSep();
        lastBaseDesc = baseDesc;

        // --- Runtime test ---
        auto rtResult = runRuntimeTest(&engine, tc);

        QString runtimeStr = rtResult.level;

        auto fmtValue = [](int v) -> QString {
            if (v == 2) return u"derived"_s;
            if (v == 1) return u"base"_s;
            return QString();
        };
        QString cppValStr;
        {
            QString propStr = fmtValue(rtResult.cppPropValue);
            QString methStr = fmtValue(rtResult.cppMethodValue);
            if (!propStr.isEmpty() && !methStr.isEmpty() && propStr != methStr)
                cppValStr = u"both"_s;
            else if (!propStr.isEmpty())
                cppValStr = propStr;
            else if (!methStr.isEmpty())
                cppValStr = methStr;
            else
                cppValStr = u"n/a"_s;
        }
        QString qmlValStr = fmtValue(rtResult.qmlValue);
        if (qmlValStr.isEmpty())
            qmlValStr = u"n/a"_s;

        // --- qmllint test ---
        LintResult lintResult;
        if (tc.cppTypeName && !tc.qmlMember) {
            lintResult.level = u"n/a"_s;
        } else {
            // Write QML to temp file and run qmllint
            const QByteArray qml = makeLintQml(tc);
            const QString tempFile = tempDir.filePath(
                    QString::fromUtf8(tc.derivedDescription).replace(u' ', u'_') + u"_"_s
                    + QString::fromUtf8(tc.baseDescription).replace(u' ', u'_') + u".qml"_s);
            QFile f(tempFile);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(qml);
                f.close();
            }

            lintResult = runQmllint(qmllintPath, tempFile, lintImportPaths);
        }

        mdRow(baseDesc, QString::fromUtf8(tc.derivedDescription), runtimeStr,
              cppValStr, qmlValStr, lintResult.level,
              rtResult.message, lintResult.message);
    }

    fprintf(stdout, "\nC++ value / QML value: base = base wins, derived = derived wins,"
                    " both = property and method coexist, n/a = type failed to load\n");
    fprintf(stdout, "Runtime: clean = silent, debug = off by default, warning = on by default\n");
    fflush(stdout);

    return 0;
}
