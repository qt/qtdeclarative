// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_STYLEGENERATOR_CALLBACK
#define QT_STYLEGENERATOR_CALLBACK

#include <QQmlApplicationEngine>

class StyleGenerator;

class Bridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString targetDirectory MEMBER m_targetDirectory NOTIFY targetDirectoryChanged)
    Q_PROPERTY(QString figmaUrlOrId MEMBER m_figmaUrlOrId NOTIFY figmaUrlOrIdChanged)
    Q_PROPERTY(QString figmaToken MEMBER m_figmaToken NOTIFY figmaTokenChanged)
    Q_PROPERTY(bool verbose MEMBER m_verbose NOTIFY verboseChanged)
    Q_PROPERTY(bool sanity MEMBER m_sanity NOTIFY sanityChanged)
    Q_PROPERTY(bool overwriteQml MEMBER m_overwriteQml NOTIFY overwriteQmlChanged)

public:
    Bridge();
    ~Bridge();

    Q_INVOKABLE void generate();
    Q_INVOKABLE void stop();

signals:
    void targetDirectoryChanged();
    void figmaUrlOrIdChanged();
    void figmaTokenChanged();
    void verboseChanged();
    void sanityChanged();
    void generatingChanged();
    void overwriteQmlChanged();

    void progressToChanged(int to) const;
    void progressLabelChanged(const QString &label) const;
    void progress() const;

    void error(const QString &msg) const;
    void warning(const QString &msg) const;
    void debug(const QString &msg) const;

    void started() const;
    void finished() const;

    void figmaFileNameChanged(const QString &name) const;

public:
    std::unique_ptr<QThread> m_generatorThread;
    std::unique_ptr<StyleGenerator> m_generator;

    QString m_targetDirectory;
    QString m_figmaUrlOrId;
    QString m_fileId;
    QString m_figmaToken;
    QString m_controlToGenerate;

    bool m_sanity = false;
    bool m_verbose = false;
    bool m_overwriteQml = false;

private:
    QString m_progressLabel;
    int m_progress = 0;
};

#endif // QT_STYLEGENERATOR_CALLBACK
