/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <private/qqmlengine_p.h>
#include <QDebug>
#include <private/qv8engine_p.h>
#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlrecord.h>
#include <QtSql/qsqlfield.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qstack.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qsettings.h>
#include <QtCore/qdir.h>
#include <private/qv8sqlerrors_p.h>


#define V8THROW_SQL(error, desc) \
{ \
    v8::Local<v8::Value> v = v8::Exception::Error(engine->toString(desc)); \
    v->ToObject()->Set(v8::String::New("code"), v8::Integer::New(error)); \
    v8::ThrowException(v); \
    return v8::Handle<v8::Value>(); \
}

#define V8THROW_SQL_VOID(error, desc) \
{ \
    v8::Local<v8::Value> v = v8::Exception::Error(engine->toString(desc)); \
    v->ToObject()->Set(v8::String::New("code"), v8::Integer::New(error)); \
    v8::ThrowException(v); \
    return; \
}

#define V8THROW_REFERENCE(string) { \
    v8::ThrowException(v8::Exception::ReferenceError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}

#define V8THROW_REFERENCE_VOID(string) { \
    v8::ThrowException(v8::Exception::ReferenceError(v8::String::New(string))); \
    return; \
}

class QQmlSqlDatabaseData : public QV8Engine::Deletable
{
public:
    QQmlSqlDatabaseData(QV8Engine *engine);
    ~QQmlSqlDatabaseData();

    v8::Persistent<v8::Function> constructor;
    v8::Persistent<v8::Function> queryConstructor;
    v8::Persistent<v8::Function> rowsConstructor;
};

V8_DEFINE_EXTENSION(QQmlSqlDatabaseData, databaseData)

class QV8SqlDatabaseResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(SQLDatabaseType)

public:
    enum Type { Database, Query, Rows };
    QV8SqlDatabaseResource(QV8Engine *e)
    : QV8ObjectResource(e), type(Database), inTransaction(false), readonly(false), forwardOnly(false) {}

    ~QV8SqlDatabaseResource() {
    }

    Type type;
    QSqlDatabase database;

    QString version; // type == Database

    bool inTransaction; // type == Query
    bool readonly;   // type == Query

    QSqlQuery query; // type == Rows
    bool forwardOnly; // type == Rows
};

static v8::Handle<v8::Value> qmlsqldatabase_version(v8::Local<v8::String> /* property */, const v8::AccessorInfo& info)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(info.This());
    if (!r || r->type != QV8SqlDatabaseResource::Database)
        V8THROW_REFERENCE("Not a SQLDatabase object");

    return r->engine->toString(r->version);
}

static v8::Handle<v8::Value> qmlsqldatabase_rows_length(v8::Local<v8::String> /* property */, const v8::AccessorInfo& info)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(info.This());
    if (!r || r->type != QV8SqlDatabaseResource::Rows)
        V8THROW_REFERENCE("Not a SQLDatabase::Rows object");

    int s = r->query.size();
    if (s < 0) {
        // Inefficient
        if (r->query.last()) {
            s = r->query.at() + 1;
        } else {
            s = 0;
        }
    }
    return v8::Integer::New(s);
}

static v8::Handle<v8::Value> qmlsqldatabase_rows_forwardOnly(v8::Local<v8::String> /* property */,
                                                             const v8::AccessorInfo& info)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(info.This());
    if (!r || r->type != QV8SqlDatabaseResource::Rows)
        V8THROW_REFERENCE("Not a SQLDatabase::Rows object");
    return v8::Boolean::New(r->query.isForwardOnly());
}

static void qmlsqldatabase_rows_setForwardOnly(v8::Local<v8::String> /* property */,
                                               v8::Local<v8::Value> value,
                                               const v8::AccessorInfo& info)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(info.This());
    if (!r || r->type != QV8SqlDatabaseResource::Rows)
        V8THROW_REFERENCE_VOID("Not a SQLDatabase::Rows object");

    r->query.setForwardOnly(value->BooleanValue());
}

QQmlSqlDatabaseData::~QQmlSqlDatabaseData()
{
    qPersistentDispose(constructor);
    qPersistentDispose(queryConstructor);
    qPersistentDispose(rowsConstructor);
}

static QString qmlsqldatabase_databasesPath(QV8Engine *engine)
{
    return engine->engine()->offlineStoragePath() +
           QDir::separator() + QLatin1String("Databases");
}

static void qmlsqldatabase_initDatabasesPath(QV8Engine *engine)
{
    QDir().mkpath(qmlsqldatabase_databasesPath(engine));
}

static QString qmlsqldatabase_databaseFile(const QString& connectionName, QV8Engine *engine)
{
    return qmlsqldatabase_databasesPath(engine) + QDir::separator() + connectionName;
}

static v8::Handle<v8::Value> qmlsqldatabase_rows_index(QV8SqlDatabaseResource *r, uint32_t index)
{
    if (r->query.at() == (int)index || r->query.seek(index)) {

        QSqlRecord record = r->query.record();
        // XXX optimize
        v8::Local<v8::Object> row = v8::Object::New();
        for (int ii = 0; ii < record.count(); ++ii) {
            QVariant v = record.value(ii);
            if (v.isNull()) {
                row->Set(r->engine->toString(record.fieldName(ii)), v8::Null());
            } else {
                row->Set(r->engine->toString(record.fieldName(ii)),
                         r->engine->fromVariant(v));
            }
        }
        return row;
    } else {
        return v8::Undefined();
    }
}

static v8::Handle<v8::Value> qmlsqldatabase_rows_index(uint32_t index, const v8::AccessorInfo& info)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(info.This());
    if (!r || r->type != QV8SqlDatabaseResource::Rows)
        V8THROW_REFERENCE("Not a SQLDatabase::Rows object");

    return qmlsqldatabase_rows_index(r, index);
}

static v8::Handle<v8::Value> qmlsqldatabase_rows_item(const v8::Arguments& args)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(args.This());
    if (!r || r->type != QV8SqlDatabaseResource::Rows)
        V8THROW_REFERENCE("Not a SQLDatabase::Rows object");

    return qmlsqldatabase_rows_index(r, args.Length()?args[0]->Uint32Value():0);
}

static v8::Handle<v8::Value> qmlsqldatabase_executeSql(const v8::Arguments& args)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(args.This());
    if (!r || r->type != QV8SqlDatabaseResource::Query)
        V8THROW_REFERENCE("Not a SQLDatabase::Query object");

    QV8Engine *engine = r->engine;

    if (!r->inTransaction)
        V8THROW_SQL(SQLEXCEPTION_DATABASE_ERR,QQmlEngine::tr("executeSql called outside transaction()"));

    QSqlDatabase db = r->database;

    QString sql = engine->toString(args[0]);

    if (r->readonly && !sql.startsWith(QLatin1String("SELECT"),Qt::CaseInsensitive)) {
        V8THROW_SQL(SQLEXCEPTION_SYNTAX_ERR, QQmlEngine::tr("Read-only Transaction"));
    }

    QSqlQuery query(db);
    bool err = false;

    v8::Handle<v8::Value> result = v8::Undefined();

    if (query.prepare(sql)) {
        if (args.Length() > 1) {
            v8::Local<v8::Value> values = args[1];
            if (values->IsArray()) {
                v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(values);
                uint32_t size = array->Length();
                for (uint32_t ii = 0; ii < size; ++ii)
                    query.bindValue(ii, engine->toVariant(array->Get(ii), -1));
            } else if (values->IsObject() && !values->ToObject()->GetExternalResource()) {
                v8::Local<v8::Object> object = values->ToObject();
                v8::Local<v8::Array> names = object->GetPropertyNames();
                uint32_t size = names->Length();
                for (uint32_t ii = 0; ii < size; ++ii)
                    query.bindValue(engine->toString(names->Get(ii)),
                                    engine->toVariant(object->Get(names->Get(ii)), -1));
            } else {
                query.bindValue(0, engine->toVariant(values, -1));
            }
        }
        if (query.exec()) {
            v8::Handle<v8::Object> rows = databaseData(engine)->rowsConstructor->NewInstance();
            QV8SqlDatabaseResource *r = new QV8SqlDatabaseResource(engine);
            r->type = QV8SqlDatabaseResource::Rows;
            r->database = db;
            r->query = query;
            rows->SetExternalResource(r);

            v8::Local<v8::Object> resultObject = v8::Object::New();
            result = resultObject;
            // XXX optimize
            resultObject->Set(v8::String::New("rowsAffected"), v8::Integer::New(query.numRowsAffected()));
            resultObject->Set(v8::String::New("insertId"), engine->toString(query.lastInsertId().toString()));
            resultObject->Set(v8::String::New("rows"), rows);
        } else {
            err = true;
        }
    } else {
        err = true;
    }
    if (err)
        V8THROW_SQL(SQLEXCEPTION_DATABASE_ERR,query.lastError().text());

    return result;
}

static v8::Handle<v8::Value> qmlsqldatabase_changeVersion(const v8::Arguments& args)
{
    if (args.Length() < 2)
        return v8::Undefined();

    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(args.This());
    if (!r || r->type != QV8SqlDatabaseResource::Database)
        V8THROW_REFERENCE("Not a SQLDatabase object");

    QV8Engine *engine = r->engine;

    QSqlDatabase db = r->database;
    QString from_version = engine->toString(args[0]);
    QString to_version = engine->toString(args[1]);
    v8::Handle<v8::Value> callback = args[2];

    if (from_version != r->version)
        V8THROW_SQL(SQLEXCEPTION_VERSION_ERR, QQmlEngine::tr("Version mismatch: expected %1, found %2").arg(from_version).arg(r->version));

    v8::Local<v8::Object> instance = databaseData(engine)->queryConstructor->NewInstance();
    QV8SqlDatabaseResource *r2 = new QV8SqlDatabaseResource(engine);
    r2->type = QV8SqlDatabaseResource::Query;
    r2->database = db;
    r2->version = r->version;
    r2->inTransaction = true;
    instance->SetExternalResource(r2);

    bool ok = true;
    if (callback->IsFunction()) {
        ok = false;
        db.transaction();

        v8::TryCatch tc;
        v8::Handle<v8::Value> callbackArgs[] = { instance };
        v8::Handle<v8::Function>::Cast(callback)->Call(engine->global(), 1, callbackArgs);

        if (tc.HasCaught()) {
            db.rollback();
            tc.ReThrow();
            return v8::Handle<v8::Value>();
        } else if (!db.commit()) {
            db.rollback();
            V8THROW_SQL(SQLEXCEPTION_UNKNOWN_ERR,QQmlEngine::tr("SQL transaction failed"));
        } else {
            ok = true;
        }
    }

    r2->inTransaction = false;

    if (ok) {
        r2->version = to_version;
#ifndef QT_NO_SETTINGS
        QSettings ini(qmlsqldatabase_databaseFile(db.connectionName(),engine) + QLatin1String(".ini"), QSettings::IniFormat);
        ini.setValue(QLatin1String("Version"), to_version);
#endif
    }

    return v8::Undefined();
}

static v8::Handle<v8::Value> qmlsqldatabase_transaction_shared(const v8::Arguments& args, bool readOnly)
{
    QV8SqlDatabaseResource *r = v8_resource_cast<QV8SqlDatabaseResource>(args.This());
    if (!r || r->type != QV8SqlDatabaseResource::Database)
        V8THROW_REFERENCE("Not a SQLDatabase object");

    QV8Engine *engine = r->engine;

    if (args.Length() == 0 || !args[0]->IsFunction())
        V8THROW_SQL(SQLEXCEPTION_UNKNOWN_ERR,QQmlEngine::tr("transaction: missing callback"));

    QSqlDatabase db = r->database;
    v8::Handle<v8::Function> callback = v8::Handle<v8::Function>::Cast(args[0]);

    v8::Local<v8::Object> instance = databaseData(engine)->queryConstructor->NewInstance();
    QV8SqlDatabaseResource *q = new QV8SqlDatabaseResource(engine);
    q->type = QV8SqlDatabaseResource::Query;
    q->database = db;
    q->readonly = readOnly;
    q->inTransaction = true;
    instance->SetExternalResource(q);

    db.transaction();
    v8::TryCatch tc;
    v8::Handle<v8::Value> callbackArgs[] = { instance };
    callback->Call(engine->global(), 1, callbackArgs);

    q->inTransaction = false;

    if (tc.HasCaught()) {
        db.rollback();
        tc.ReThrow();
        return v8::Handle<v8::Value>();
    } else if (!db.commit()) {
        db.rollback();
    }

    return v8::Undefined();
}

static v8::Handle<v8::Value> qmlsqldatabase_transaction(const v8::Arguments& args)
{
    return qmlsqldatabase_transaction_shared(args, false);
}

static v8::Handle<v8::Value> qmlsqldatabase_read_transaction(const v8::Arguments& args)
{
    return qmlsqldatabase_transaction_shared(args, true);
}

QQmlSqlDatabaseData::QQmlSqlDatabaseData(QV8Engine *engine)
{
    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("transaction"),
                                 V8FUNCTION(qmlsqldatabase_transaction, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("readTransaction"),
                                 V8FUNCTION(qmlsqldatabase_read_transaction, engine));
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("version"), qmlsqldatabase_version);
    ft->PrototypeTemplate()->Set(v8::String::New("changeVersion"),
                                 V8FUNCTION(qmlsqldatabase_changeVersion, engine));
    constructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("executeSql"),
                                 V8FUNCTION(qmlsqldatabase_executeSql, engine));
    queryConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("item"), V8FUNCTION(qmlsqldatabase_rows_item, engine));
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("length"), qmlsqldatabase_rows_length);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("forwardOnly"), qmlsqldatabase_rows_forwardOnly,
                                        qmlsqldatabase_rows_setForwardOnly);
    ft->InstanceTemplate()->SetIndexedPropertyHandler(qmlsqldatabase_rows_index);
    rowsConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
}

/*
HTML5 "spec" says "rs.rows[n]", but WebKit only impelments "rs.rows.item(n)". We do both (and property iterator).
We add a "forwardOnly" property that stops Qt caching results (code promises to only go forward
through the data.
*/


/*!
    \qmlmodule QtQuick.LocalStorage 2
    \title Qt Quick Local Storage QML Types
    \ingroup qmlmodules
    \brief Provides a JavaScript object singleton type for accessing a local
    SQLite database

    This is a singleton type for reading and writing to SQLite databases.


    \section1 Methods

    \list
    \li object \b{\l{#openDatabaseSync}{openDatabaseSync}}(string name, string version, string description, int estimated_size, jsobject callback(db))
    \endlist


    \section1 Detailed Description

    To use the types in this module, import the module and call the
    relevant functions using the \c LocalStorage type:

    \code
    import QtQuick.LocalStorage 2.0
    import QtQuick 2.0

    Item {
        Component.onCompleted: {
            var db = LocalStorage.openDatabaseSync(...)
        }
    }
    \endcode


These databases are user-specific and QML-specific, but accessible to all QML applications.
They are stored in the \c Databases subdirectory
of QQmlEngine::offlineStoragePath(), currently as SQLite databases.

Database connections are automatically closed during Javascript garbage collection.

The API can be used from JavaScript functions in your QML:

\snippet localstorage/localstorage/hello.qml 0

The API conforms to the Synchronous API of the HTML5 Web Database API,
\link http://www.w3.org/TR/2009/WD-webdatabase-20091029/ W3C Working Draft 29 October 2009\endlink.

The \l{localstorage/localstorage}{SQL Local Storage example} demonstrates the basics of
using the Offline Storage API.

\section3 Open or create a databaseData
\code
import QtQuick.LocalStorage 2.0 as Sql

db = Sql.openDatabaseSync(identifier, version, description, estimated_size, callback(db))
\endcode
The above code returns the database identified by \e identifier. If the database does not already exist, it
is created, and the function \e callback is called with the database as a parameter. \e description
and \e estimated_size are written to the INI file (described below), but are otherwise currently
unused.

May throw exception with code property SQLException.DATABASE_ERR, or SQLException.VERSION_ERR.

When a database is first created, an INI file is also created specifying its characteristics:

\table
\header \li \b {Key} \li \b {Value}
\row \li Name \li The name of the database passed to \c openDatabase()
\row \li Version \li The version of the database passed to \c openDatabase()
\row \li Description \li The description of the database passed to \c openDatabase()
\row \li EstimatedSize \li The estimated size (in bytes) of the database passed to \c openDatabase()
\row \li Driver \li Currently "QSQLITE"
\endtable

This data can be used by application tools.

\section3 db.changeVersion(from, to, callback(tx))

This method allows you to perform a \e{Scheme Upgrade}.

If the current version of \e db is not \e from, then an exception is thrown.

Otherwise, a database transaction is created and passed to \e callback. In this function,
you can call \e executeSql on \e tx to upgrade the database.

May throw exception with code property SQLException.DATABASE_ERR or SQLException.UNKNOWN_ERR.

\section3 db.transaction(callback(tx))

This method creates a read/write transaction and passed to \e callback. In this function,
you can call \e executeSql on \e tx to read and modify the database.

If the callback throws exceptions, the transaction is rolled back.

\section3 db.readTransaction(callback(tx))

This method creates a read-only transaction and passed to \e callback. In this function,
you can call \e executeSql on \e tx to read the database (with SELECT statements).

\section3 results = tx.executeSql(statement, values)

This method executes a SQL \e statement, binding the list of \e values to SQL positional parameters ("?").

It returns a results object, with the following properties:

\table
\header \li \b {Type} \li \b {Property} \li \b {Value} \li \b {Applicability}
\row \li int \li rows.length \li The number of rows in the result \li SELECT
\row \li var \li rows.item(i) \li Function that returns row \e i of the result \li SELECT
\row \li int \li rowsAffected \li The number of rows affected by a modification \li UPDATE, DELETE
\row \li string \li insertId \li The id of the row inserted \li INSERT
\endtable

May throw exception with code property SQLException.DATABASE_ERR, SQLException.SYNTAX_ERR, or SQLException.UNKNOWN_ERR.


\section1 Method Documentation

\target openDatabaseSync
\code
object openDatabaseSync(string name, string version, string description, int estimated_size, jsobject callback(db))
\endcode

Opens or creates a local storage sql database by the given parameters.

\list
\li \c name is the database name
\li \c version is the database version
\li \c description is the database display name
\li \c estimated_size is the database's estimated size, in bytes
\li \c callback is an optional parameter, which is invoked if the database has not yet been created.
\endlist

Returns the created database object.

*/
class QQuickLocalStorage : public QObject
{
    Q_OBJECT
public:
    QQuickLocalStorage(QObject *parent=0) : QObject(parent)
    {
    }
    ~QQuickLocalStorage() {
    }

   Q_INVOKABLE void openDatabaseSync(QQmlV8Function* args);
};

void QQuickLocalStorage::openDatabaseSync(QQmlV8Function *args)
{
#ifndef QT_NO_SETTINGS
    QV8Engine *engine = args->engine();
    if (engine->engine()->offlineStoragePath().isEmpty())
        V8THROW_SQL_VOID(SQLEXCEPTION_DATABASE_ERR, QQmlEngine::tr("SQL: can't create database, offline storage is disabled."));

    qmlsqldatabase_initDatabasesPath(engine);

    QSqlDatabase database;

    QString dbname = engine->toString((*args)[0]);
    QString dbversion = engine->toString((*args)[1]);
    QString dbdescription = engine->toString((*args)[2]);
    int dbestimatedsize = (*args)[3]->Int32Value();
    v8::Handle<v8::Value> dbcreationCallback = (*args)[4];

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(dbname.toUtf8());
    QString dbid(QLatin1String(md5.result().toHex()));

    QString basename = qmlsqldatabase_databaseFile(dbid, engine);
    bool created = false;
    QString version = dbversion;

    {
        QSettings ini(basename+QLatin1String(".ini"),QSettings::IniFormat);

        if (QSqlDatabase::connectionNames().contains(dbid)) {
            database = QSqlDatabase::database(dbid);
            version = ini.value(QLatin1String("Version")).toString();
            if (version != dbversion && !dbversion.isEmpty() && !version.isEmpty())
                V8THROW_SQL_VOID(SQLEXCEPTION_VERSION_ERR, QQmlEngine::tr("SQL: database version mismatch"));
        } else {
            created = !QFile::exists(basename+QLatin1String(".sqlite"));
            database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), dbid);
            if (created) {
                ini.setValue(QLatin1String("Name"), dbname);
                if (dbcreationCallback->IsFunction())
                    version = QString();
                ini.setValue(QLatin1String("Version"), version);
                ini.setValue(QLatin1String("Description"), dbdescription);
                ini.setValue(QLatin1String("EstimatedSize"), dbestimatedsize);
                ini.setValue(QLatin1String("Driver"), QLatin1String("QSQLITE"));
            } else {
                if (!dbversion.isEmpty() && ini.value(QLatin1String("Version")) != dbversion) {
                    // Incompatible
                    V8THROW_SQL_VOID(SQLEXCEPTION_VERSION_ERR,QQmlEngine::tr("SQL: database version mismatch"));
                }
                version = ini.value(QLatin1String("Version")).toString();
            }
            database.setDatabaseName(basename+QLatin1String(".sqlite"));
        }
        if (!database.isOpen())
            database.open();
    }

    v8::Local<v8::Object> instance = databaseData(engine)->constructor->NewInstance();

    QV8SqlDatabaseResource *r = new QV8SqlDatabaseResource(engine);
    r->database = database;
    r->version = version;
    instance->SetExternalResource(r);

    if (created && dbcreationCallback->IsFunction()) {
        v8::TryCatch tc;
        v8::Handle<v8::Function> callback = v8::Handle<v8::Function>::Cast(dbcreationCallback);
        v8::Handle<v8::Value> args[] = { instance };
        callback->Call(engine->global(), 1, args);
        if (tc.HasCaught()) {
            tc.ReThrow();
            return;
        }
    }

    args->returnValue(instance);
#endif // QT_NO_SETTINGS
}

static QObject *module_api_factory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
   Q_UNUSED(engine)
   Q_UNUSED(scriptEngine)
   QQuickLocalStorage *api = new QQuickLocalStorage();

   return api;
}

class QQmlLocalStoragePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    QQmlLocalStoragePlugin()
    {
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == "QtQuick.LocalStorage");
        qmlRegisterSingletonType<QQuickLocalStorage>(uri, 2, 0, "LocalStorage", module_api_factory);
    }
};

#include "plugin.moc"
