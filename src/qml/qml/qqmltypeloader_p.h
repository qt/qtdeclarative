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

#ifndef QQMLTYPELOADER_P_H
#define QQMLTYPELOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qatomic.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlengine.h>

#include <private/qv8_p.h>
#include <private/qhashedstring_p.h>
#include <private/qqmlscript_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlcleanup_p.h>
#include <private/qqmldirparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class QQmlScriptBlob;
class QQmlQmldirData;
class QQmlTypeLoader;
class QQmlCompiledData;
class QQmlComponentPrivate;
class QQmlTypeData;
class QQmlDataLoader;
class QQmlExtensionInterface;

// Exported for QtQuick1
class Q_QML_PRIVATE_EXPORT QQmlDataBlob : public QQmlRefCount
{
public:
    enum Status {
        Null,                    // Prior to QQmlDataLoader::load()
        Loading,                 // Prior to data being received and dataReceived() being called
        WaitingForDependencies,  // While there are outstanding addDependency()s
        Complete,                // Finished
        Error                    // Error
    };

    enum Type {
        QmlFile,
        JavaScriptFile,
        QmldirFile
    };

    QQmlDataBlob(const QUrl &, Type);
    virtual ~QQmlDataBlob();

    Type type() const;

    Status status() const;
    bool isNull() const;
    bool isLoading() const;
    bool isWaiting() const;
    bool isComplete() const;
    bool isError() const;
    bool isCompleteOrError() const;

    qreal progress() const;

    QUrl url() const;
    QUrl finalUrl() const;
    QString finalUrlString() const;

    QList<QQmlError> errors() const;

protected:
    // Can be called from within callbacks
    void setError(const QQmlError &);
    void setError(const QList<QQmlError> &errors);
    void addDependency(QQmlDataBlob *);

    // Callbacks made in load thread
    virtual void dataReceived(const QByteArray &) = 0;
    virtual void done();
    virtual void networkError(QNetworkReply::NetworkError);
    virtual void dependencyError(QQmlDataBlob *);
    virtual void dependencyComplete(QQmlDataBlob *);
    virtual void allDependenciesDone();

    // Callbacks made in main thread
    virtual void downloadProgressChanged(qreal);
    virtual void completed();
private:
    friend class QQmlDataLoader;
    friend class QQmlDataLoaderThread;

    void tryDone();
    void cancelAllWaitingFor();
    void notifyAllWaitingOnMe();
    void notifyComplete(QQmlDataBlob *);

    struct ThreadData {
        inline ThreadData();
        inline QQmlDataBlob::Status status() const;
        inline void setStatus(QQmlDataBlob::Status);
        inline bool isAsync() const;
        inline void setIsAsync(bool);
        inline quint8 progress() const;
        inline void setProgress(quint8);

    private:
        QAtomicInt _p;
    };
    ThreadData m_data;

    // m_errors should *always* be written before the status is set to Error.
    // We use the status change as a memory fence around m_errors so that locking
    // isn't required.  Once the status is set to Error (or Complete), m_errors 
    // cannot be changed.
    QList<QQmlError> m_errors;

    Type m_type;

    QUrl m_url;
    QUrl m_finalUrl;
    mutable QString m_finalUrlString;

    // List of QQmlDataBlob's that are waiting for me to complete.
    QList<QQmlDataBlob *> m_waitingOnMe;

    // List of QQmlDataBlob's that I am waiting for to complete.
    QList<QQmlDataBlob *> m_waitingFor;

    // Manager that is currently fetching data for me
    QQmlDataLoader *m_manager;
    int m_redirectCount:30;
    bool m_inCallback:1;
    bool m_isDone:1;
};

class QQmlDataLoaderThread;
// Exported for QtQuick1
class Q_QML_PRIVATE_EXPORT QQmlDataLoader 
{
public:
    QQmlDataLoader(QQmlEngine *);
    ~QQmlDataLoader();

    void lock();
    void unlock();

    bool isConcurrent() const { return true; }

    enum Mode { PreferSynchronous, Asynchronous };

    void load(QQmlDataBlob *, Mode = PreferSynchronous);
    void loadWithStaticData(QQmlDataBlob *, const QByteArray &, Mode = PreferSynchronous);

    QQmlEngine *engine() const;
    void initializeEngine(QQmlExtensionInterface *, const char *);

private:
    friend class QQmlDataBlob;
    friend class QQmlDataLoaderThread;
    friend class QQmlDataLoaderNetworkReplyProxy;

    void loadThread(QQmlDataBlob *);
    void loadWithStaticDataThread(QQmlDataBlob *, const QByteArray &);
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);
    
    typedef QHash<QNetworkReply *, QQmlDataBlob *> NetworkReplies;

    void setData(QQmlDataBlob *, const QByteArray &);

    QQmlEngine *m_engine;
    QQmlDataLoaderThread *m_thread;
    NetworkReplies m_networkReplies;
};

// Exported for QtQuick1
class Q_QML_PRIVATE_EXPORT QQmlTypeLoader : public QQmlDataLoader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeLoader)
public:
    QQmlTypeLoader(QQmlEngine *);
    ~QQmlTypeLoader();

    enum Option {
        None,
        PreserveParser
    };
    Q_DECLARE_FLAGS(Options, Option)

    QQmlTypeData *get(const QUrl &url, Mode mode = PreferSynchronous);
    QQmlTypeData *get(const QByteArray &, const QUrl &url, Options = None);
    void clearCache();

    QQmlScriptBlob *getScript(const QUrl &);
    QQmlQmldirData *getQmldir(const QUrl &);

    QString absoluteFilePath(const QString &path);
    bool directoryExists(const QString &path);
    const QQmlDirParser *qmlDirParser(const QString &absoluteFilePath);
private:
    typedef QHash<QUrl, QQmlTypeData *> TypeCache;
    typedef QHash<QUrl, QQmlScriptBlob *> ScriptCache;
    typedef QHash<QUrl, QQmlQmldirData *> QmldirCache;
    typedef QStringHash<bool> StringSet;
    typedef QStringHash<StringSet*> ImportDirCache;
    typedef QStringHash<QQmlDirParser*> ImportQmlDirCache;

    TypeCache m_typeCache;
    ScriptCache m_scriptCache;
    QmldirCache m_qmldirCache;
    ImportDirCache m_importDirCache;
    ImportQmlDirCache m_importQmlDirCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlTypeLoader::Options)

class Q_AUTOTEST_EXPORT QQmlTypeData : public QQmlDataBlob
{
public:
    struct TypeReference
    {
        TypeReference() : type(0), majorVersion(0), minorVersion(0), typeData(0) {}

        QQmlScript::Location location;
        QQmlType *type;
        int majorVersion;
        int minorVersion;
        QQmlTypeData *typeData;
    };

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QQmlScript::Location location;
        QString qualifier;
        QQmlScriptBlob *script;
    };

    QQmlTypeData(const QUrl &, QQmlTypeLoader::Options, QQmlTypeLoader *);
    ~QQmlTypeData();

    QQmlTypeLoader *typeLoader() const;

    const QQmlImports &imports() const;
    const QQmlScript::Parser &parser() const;

    const QList<TypeReference> &resolvedTypes() const;
    const QList<ScriptReference> &resolvedScripts() const;
    const QSet<QString> &namespaces() const;

    QQmlCompiledData *compiledData() const;

    // Used by QQmlComponent to get notifications
    struct TypeDataCallback {
        ~TypeDataCallback() {}
        virtual void typeDataProgress(QQmlTypeData *, qreal) {}
        virtual void typeDataReady(QQmlTypeData *) {}
    };
    void registerCallback(TypeDataCallback *);
    void unregisterCallback(TypeDataCallback *);

protected:
    virtual void done();
    virtual void completed();
    virtual void dataReceived(const QByteArray &);
    virtual void allDependenciesDone();
    virtual void downloadProgressChanged(qreal);

private:
    void resolveTypes();
    void compile();

    QQmlTypeLoader::Options m_options;

    QQmlQmldirData *qmldirForUrl(const QUrl &);

    QQmlScript::Parser scriptParser;
    QQmlImports m_imports;

    QList<ScriptReference> m_scripts;
    QList<QQmlQmldirData *> m_qmldirs;

    QSet<QString> m_namespaces;

    QList<TypeReference> m_types;
    bool m_typesResolved:1;

    QQmlCompiledData *m_compiledData;

    QList<TypeDataCallback *> m_callbacks;
   
    QQmlTypeLoader *m_typeLoader;
};

// QQmlScriptData instances are created, uninitialized, by the loader in the 
// load thread.  The first time they are used by the VME, they are initialized which
// creates their v8 objects and they are referenced and added to the  engine's cleanup
// list.  During QQmlCleanup::clear() all v8 resources are destroyed, and the 
// reference that was created is released but final deletion only occurs once all the
// references as released.  This is all intended to ensure that the v8 resources are
// only created and destroyed in the main thread :)
class Q_AUTOTEST_EXPORT QQmlScriptData : public QQmlCleanup, 
                                                 public QQmlRefCount
{
public:
    QQmlScriptData();
    ~QQmlScriptData();

    QUrl url;
    QString urlString;
    QQmlTypeNameCache *importCache;
    QList<QQmlScriptBlob *> scripts;
    QQmlScript::Object::ScriptBlock::Pragmas pragmas;

    bool isInitialized() const { return hasEngine(); }
    void initialize(QQmlEngine *);

protected:
    virtual void clear(); // From QQmlCleanup

private:
    friend class QQmlVME;
    friend class QQmlScriptBlob;

    bool m_loaded;
    QByteArray m_programSource;
    v8::Persistent<v8::Script> m_program;
    v8::Persistent<v8::Object> m_value;
};

class Q_AUTOTEST_EXPORT QQmlScriptBlob : public QQmlDataBlob
{
public:
    QQmlScriptBlob(const QUrl &, QQmlTypeLoader *);
    ~QQmlScriptBlob();

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QQmlScript::Location location;
        QString qualifier;
        QQmlScriptBlob *script;
    };

    QQmlScript::Object::ScriptBlock::Pragmas pragmas() const;

    QQmlTypeLoader *typeLoader() const;
    const QQmlImports &imports() const;

    QQmlScriptData *scriptData() const;

protected:
    virtual void dataReceived(const QByteArray &);
    virtual void done();

private:
    QQmlScript::Object::ScriptBlock::Pragmas m_pragmas;
    QString m_source;

    QQmlImports m_imports;
    QList<ScriptReference> m_scripts;
    QQmlScriptData *m_scriptData;

    QQmlTypeLoader *m_typeLoader;
};

class Q_AUTOTEST_EXPORT QQmlQmldirData : public QQmlDataBlob
{
public:
    QQmlQmldirData(const QUrl &);

    const QQmlDirComponents &dirComponents() const;

protected:
    virtual void dataReceived(const QByteArray &);

private:
    QQmlDirComponents m_components;

};

QT_END_NAMESPACE

#endif // QQMLTYPELOADER_P_H
