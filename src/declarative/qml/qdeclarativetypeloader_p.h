/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVETYPELOADER_P_H
#define QDECLARATIVETYPELOADER_P_H

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
#include <QtDeclarative/qdeclarativeerror.h>
#include <QtDeclarative/qdeclarativeengine.h>

#include <private/qv8_p.h>
#include <private/qhashedstring_p.h>
#include <private/qdeclarativescript_p.h>
#include <private/qdeclarativeimport_p.h>
#include <private/qdeclarativecleanup_p.h>
#include <private/qdeclarativedirparser_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeScriptData;
class QDeclarativeScriptBlob;
class QDeclarativeQmldirData;
class QDeclarativeTypeLoader;
class QDeclarativeCompiledData;
class QDeclarativeComponentPrivate;
class QDeclarativeTypeData;
class QDeclarativeDataLoader;
class QDeclarativeExtensionInterface;

// Exported for QtQuick1
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeDataBlob : public QDeclarativeRefCount
{
public:
    enum Status {
        Null,                    // Prior to QDeclarativeDataLoader::load()
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

    QDeclarativeDataBlob(const QUrl &, Type);
    virtual ~QDeclarativeDataBlob();

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

    QList<QDeclarativeError> errors() const;

protected:
    // Can be called from within callbacks
    void setError(const QDeclarativeError &);
    void setError(const QList<QDeclarativeError> &errors);
    void addDependency(QDeclarativeDataBlob *);

    // Callbacks made in load thread
    virtual void dataReceived(const QByteArray &) = 0;
    virtual void done();
    virtual void networkError(QNetworkReply::NetworkError);
    virtual void dependencyError(QDeclarativeDataBlob *);
    virtual void dependencyComplete(QDeclarativeDataBlob *);
    virtual void allDependenciesDone();

    // Callbacks made in main thread
    virtual void downloadProgressChanged(qreal);
    virtual void completed();
private:
    friend class QDeclarativeDataLoader;
    friend class QDeclarativeDataLoaderThread;

    void tryDone();
    void cancelAllWaitingFor();
    void notifyAllWaitingOnMe();
    void notifyComplete(QDeclarativeDataBlob *);

    struct ThreadData {
        inline ThreadData();
        inline QDeclarativeDataBlob::Status status() const;
        inline void setStatus(QDeclarativeDataBlob::Status);
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
    QList<QDeclarativeError> m_errors;

    Type m_type;

    QUrl m_url;
    QUrl m_finalUrl;

    // List of QDeclarativeDataBlob's that are waiting for me to complete.
    QList<QDeclarativeDataBlob *> m_waitingOnMe;

    // List of QDeclarativeDataBlob's that I am waiting for to complete.
    QList<QDeclarativeDataBlob *> m_waitingFor;

    // Manager that is currently fetching data for me
    QDeclarativeDataLoader *m_manager;
    int m_redirectCount:30;
    bool m_inCallback:1;
    bool m_isDone:1;
};

class QDeclarativeDataLoaderThread;
// Exported for QtQuick1
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeDataLoader 
{
public:
    QDeclarativeDataLoader(QDeclarativeEngine *);
    ~QDeclarativeDataLoader();

    void lock();
    void unlock();

    bool isConcurrent() const { return true; }

    enum Mode { PreferSynchronous, Asynchronous };

    void load(QDeclarativeDataBlob *, Mode = PreferSynchronous);
    void loadWithStaticData(QDeclarativeDataBlob *, const QByteArray &, Mode = PreferSynchronous);

    QDeclarativeEngine *engine() const;
    void initializeEngine(QDeclarativeExtensionInterface *, const char *);

private:
    friend class QDeclarativeDataBlob;
    friend class QDeclarativeDataLoaderThread;
    friend class QDeclarativeDataLoaderNetworkReplyProxy;

    void loadThread(QDeclarativeDataBlob *);
    void loadWithStaticDataThread(QDeclarativeDataBlob *, const QByteArray &);
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);
    
    typedef QHash<QNetworkReply *, QDeclarativeDataBlob *> NetworkReplies;

    void setData(QDeclarativeDataBlob *, const QByteArray &);

    QDeclarativeEngine *m_engine;
    QDeclarativeDataLoaderThread *m_thread;
    NetworkReplies m_networkReplies;
};

// Exported for QtQuick1
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeTypeLoader : public QDeclarativeDataLoader
{
    Q_DECLARE_TR_FUNCTIONS(QDeclarativeTypeLoader)
public:
    QDeclarativeTypeLoader(QDeclarativeEngine *);
    ~QDeclarativeTypeLoader();

    enum Option {
        None,
        PreserveParser
    };
    Q_DECLARE_FLAGS(Options, Option)

    QDeclarativeTypeData *get(const QUrl &url);
    QDeclarativeTypeData *get(const QByteArray &, const QUrl &url, Options = None);
    void clearCache();

    QDeclarativeScriptBlob *getScript(const QUrl &);
    QDeclarativeQmldirData *getQmldir(const QUrl &);

    QString absoluteFilePath(const QString &path);
    bool directoryExists(const QString &path);
    const QDeclarativeDirParser *qmlDirParser(const QString &absoluteFilePath);
private:
    typedef QHash<QUrl, QDeclarativeTypeData *> TypeCache;
    typedef QHash<QUrl, QDeclarativeScriptBlob *> ScriptCache;
    typedef QHash<QUrl, QDeclarativeQmldirData *> QmldirCache;
    typedef QStringHash<bool> StringSet;
    typedef QStringHash<StringSet*> ImportDirCache;
    typedef QStringHash<QDeclarativeDirParser*> ImportQmlDirCache;

    TypeCache m_typeCache;
    ScriptCache m_scriptCache;
    QmldirCache m_qmldirCache;
    ImportDirCache m_importDirCache;
    ImportQmlDirCache m_importQmlDirCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativeTypeLoader::Options)

class Q_AUTOTEST_EXPORT QDeclarativeTypeData : public QDeclarativeDataBlob
{
public:
    struct TypeReference
    {
        TypeReference() : type(0), majorVersion(0), minorVersion(0), typeData(0) {}

        QDeclarativeScript::Location location;
        QDeclarativeType *type;
        int majorVersion;
        int minorVersion;
        QDeclarativeTypeData *typeData;
    };

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QDeclarativeScript::Location location;
        QString qualifier;
        QDeclarativeScriptBlob *script;
    };

    QDeclarativeTypeData(const QUrl &, QDeclarativeTypeLoader::Options, QDeclarativeTypeLoader *);
    ~QDeclarativeTypeData();

    QDeclarativeTypeLoader *typeLoader() const;

    const QDeclarativeImports &imports() const;
    const QDeclarativeScript::Parser &parser() const;

    const QList<TypeReference> &resolvedTypes() const;
    const QList<ScriptReference> &resolvedScripts() const;

    QDeclarativeCompiledData *compiledData() const;

    // Used by QDeclarativeComponent to get notifications
    struct TypeDataCallback {
        ~TypeDataCallback() {}
        virtual void typeDataProgress(QDeclarativeTypeData *, qreal) {}
        virtual void typeDataReady(QDeclarativeTypeData *) {}
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

    QDeclarativeTypeLoader::Options m_options;

    QDeclarativeQmldirData *qmldirForUrl(const QUrl &);

    QDeclarativeScript::Parser scriptParser;
    QDeclarativeImports m_imports;

    QList<ScriptReference> m_scripts;
    QList<QDeclarativeQmldirData *> m_qmldirs;

    QList<TypeReference> m_types;
    bool m_typesResolved:1;

    QDeclarativeCompiledData *m_compiledData;

    QList<TypeDataCallback *> m_callbacks;
   
    QDeclarativeTypeLoader *m_typeLoader;
};

// QDeclarativeScriptData instances are created, uninitialized, by the loader in the 
// load thread.  The first time they are used by the VME, they are initialized which
// creates their v8 objects and they are referenced and added to the  engine's cleanup
// list.  During QDeclarativeCleanup::clear() all v8 resources are destroyed, and the 
// reference that was created is released but final deletion only occurs once all the
// references as released.  This is all intended to ensure that the v8 resources are
// only created and destroyed in the main thread :)
class Q_AUTOTEST_EXPORT QDeclarativeScriptData : public QDeclarativeCleanup, 
                                                 public QDeclarativeRefCount
{
public:
    QDeclarativeScriptData();
    ~QDeclarativeScriptData();

    QUrl url;
    QDeclarativeTypeNameCache *importCache;
    QList<QDeclarativeScriptBlob *> scripts;
    QDeclarativeScript::Object::ScriptBlock::Pragmas pragmas;

    bool isInitialized() const { return hasEngine(); }
    void initialize(QDeclarativeEngine *);

protected:
    virtual void clear(); // From QDeclarativeCleanup

private:
    friend class QDeclarativeVME;
    friend class QDeclarativeScriptBlob;

    bool m_loaded;
    QString m_programSource;
    v8::Persistent<v8::Script> m_program;
    v8::Persistent<v8::Object> m_value;
};

class Q_AUTOTEST_EXPORT QDeclarativeScriptBlob : public QDeclarativeDataBlob
{
public:
    QDeclarativeScriptBlob(const QUrl &, QDeclarativeTypeLoader *);
    ~QDeclarativeScriptBlob();

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QDeclarativeScript::Location location;
        QString qualifier;
        QDeclarativeScriptBlob *script;
    };

    QDeclarativeScript::Object::ScriptBlock::Pragmas pragmas() const;
    QString scriptSource() const;

    QDeclarativeTypeLoader *typeLoader() const;
    const QDeclarativeImports &imports() const;

    QDeclarativeScriptData *scriptData() const;

protected:
    virtual void dataReceived(const QByteArray &);
    virtual void done();

private:
    QDeclarativeScript::Object::ScriptBlock::Pragmas m_pragmas;
    QString m_source;

    QDeclarativeImports m_imports;
    QList<ScriptReference> m_scripts;
    QDeclarativeScriptData *m_scriptData;

    QDeclarativeTypeLoader *m_typeLoader;
};

class Q_AUTOTEST_EXPORT QDeclarativeQmldirData : public QDeclarativeDataBlob
{
public:
    QDeclarativeQmldirData(const QUrl &);

    const QDeclarativeDirComponents &dirComponents() const;

protected:
    virtual void dataReceived(const QByteArray &);

private:
    QDeclarativeDirComponents m_components;

};

QT_END_NAMESPACE

#endif // QDECLARATIVETYPELOADER_P_H
