/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qdeclarativesqldatabase_p.h"

#include "qdeclarativeengine.h"
#include "qdeclarativeengine_p.h"
#include <private/qdeclarativerefcount_p.h>

#include <QtCore/qobject.h>
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
#include <QtCore/qdebug.h>

#include <private/qv8engine_p.h>

QT_BEGIN_NAMESPACE

enum SqlException {
    UNKNOWN_ERR,
    DATABASE_ERR,
    VERSION_ERR,
    TOO_LARGE_ERR,
    QUOTA_ERR,
    SYNTAX_ERR,
    CONSTRAINT_ERR,
    TIMEOUT_ERR
};

static const char* sqlerror[] = {
    "UNKNOWN_ERR",
    "DATABASE_ERR",
    "VERSION_ERR",
    "TOO_LARGE_ERR",
    "QUOTA_ERR",
    "SYNTAX_ERR",
    "CONSTRAINT_ERR",
    "TIMEOUT_ERR",
    0
};

#define THROW_SQL(error, desc)

#define V8THROW_SQL(error, desc) \
{ \
    v8::Local<v8::Value> v = v8::Exception::Error(engine->toString(desc)); \
    v->ToObject()->Set(v8::String::New("code"), v8::Integer::New(error)); \
    v8::ThrowException(v); \
    return v8::Handle<v8::Value>(); \
}

#define V8THROW_REFERENCE(string) { \
    v8::ThrowException(v8::Exception::ReferenceError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}

#define V8THROW_REFERENCE_VOID(string) { \
    v8::ThrowException(v8::Exception::ReferenceError(v8::String::New(string))); \
    return; \
}

struct QDeclarativeSqlDatabaseData {
    QDeclarativeSqlDatabaseData(QV8Engine *engine);
    ~QDeclarativeSqlDatabaseData();

    QString offlineStoragePath;
    v8::Persistent<v8::Function> constructor;
    v8::Persistent<v8::Function> queryConstructor;
    v8::Persistent<v8::Function> rowsConstructor;

    static inline QDeclarativeSqlDatabaseData *data(QV8Engine *e) {
        return (QDeclarativeSqlDatabaseData *)e->sqlDatabaseData();
    }
    static inline QDeclarativeSqlDatabaseData *data(void *d) {
        return (QDeclarativeSqlDatabaseData *)d;
    }
};

class QV8SqlDatabaseResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(SQLDatabaseType)

public:
    enum Type { Database, Query, Rows };

    QV8SqlDatabaseResource(QV8Engine *e) 
    : QV8ObjectResource(e), type(Database), inTransaction(false), readonly(false), forwardOnly(false) {}

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

QDeclarativeSqlDatabaseData::~QDeclarativeSqlDatabaseData()
{
    qPersistentDispose(constructor);
    qPersistentDispose(queryConstructor);
    qPersistentDispose(rowsConstructor);
}

static QString qmlsqldatabase_databasesPath(QV8Engine *engine)
{
    return QDeclarativeSqlDatabaseData::data(engine)->offlineStoragePath +
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
    if (r->query.at() == index || r->query.seek(index)) {

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
        V8THROW_SQL(DATABASE_ERR,QDeclarativeEngine::tr("executeSql called outside transaction()"));

    QSqlDatabase db = r->database;

    QString sql = engine->toString(args[0]);

    if (r->readonly && !sql.startsWith(QLatin1String("SELECT"),Qt::CaseInsensitive)) {
        V8THROW_SQL(SYNTAX_ERR, QDeclarativeEngine::tr("Read-only Transaction"));
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
            v8::Handle<v8::Object> rows = QDeclarativeSqlDatabaseData::data(engine)->rowsConstructor->NewInstance();
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
        V8THROW_SQL(DATABASE_ERR,query.lastError().text());

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
        V8THROW_SQL(VERSION_ERR, QDeclarativeEngine::tr("Version mismatch: expected %1, found %2").arg(from_version).arg(r->version));

    v8::Local<v8::Object> instance = QDeclarativeSqlDatabaseData::data(engine)->queryConstructor->NewInstance();
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
            V8THROW_SQL(UNKNOWN_ERR,QDeclarativeEngine::tr("SQL transaction failed"));
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
        V8THROW_SQL(UNKNOWN_ERR,QDeclarativeEngine::tr("transaction: missing callback"));
    
    QSqlDatabase db = r->database;
    v8::Handle<v8::Function> callback = v8::Handle<v8::Function>::Cast(args[0]);

    v8::Local<v8::Object> instance = QDeclarativeSqlDatabaseData::data(engine)->queryConstructor->NewInstance();
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

/*
    Currently documented in doc/src/declarative/globalobject.qdoc
*/
static v8::Handle<v8::Value> qmlsqldatabase_open_sync(const v8::Arguments& args)
{
#ifndef QT_NO_SETTINGS
    QV8Engine *engine = V8ENGINE();
    qmlsqldatabase_initDatabasesPath(engine);

    QSqlDatabase database;

    QString dbname = engine->toString(args[0]);
    QString dbversion = engine->toString(args[1]);
    QString dbdescription = engine->toString(args[2]);
    int dbestimatedsize = args[3]->Int32Value();
    v8::Handle<v8::Value> dbcreationCallback = args[4];

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
                V8THROW_SQL(VERSION_ERR, QDeclarativeEngine::tr("SQL: database version mismatch"));
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
                    V8THROW_SQL(VERSION_ERR,QDeclarativeEngine::tr("SQL: database version mismatch"));
                }
                version = ini.value(QLatin1String("Version")).toString();
            }
            database.setDatabaseName(basename+QLatin1String(".sqlite"));
        }
        if (!database.isOpen())
            database.open();
    }

    v8::Local<v8::Object> instance = QDeclarativeSqlDatabaseData::data(engine)->constructor->NewInstance();
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
            return v8::Handle<v8::Value>();
        }
    }

    return instance;
#else
    return v8::Undefined();
#endif // QT_NO_SETTINGS
}

QDeclarativeSqlDatabaseData::QDeclarativeSqlDatabaseData(QV8Engine *engine)
{
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    offlineStoragePath = dataLocation.replace(QLatin1Char('/'), QDir::separator()) +
                         QDir::separator() + QLatin1String("QML") +
                         QDir::separator() + QLatin1String("OfflineStorage");

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

void *qt_add_qmlsqldatabase(QV8Engine *engine)
{
    v8::Local<v8::Function> openDatabase = V8FUNCTION(qmlsqldatabase_open_sync, engine);
    engine->global()->Set(v8::String::New("openDatabaseSync"), openDatabase);

    v8::PropertyAttribute attributes = (v8::PropertyAttribute)(v8::ReadOnly | v8::DontEnum | v8::DontDelete);
    v8::Local<v8::Object> sqlExceptionPrototype = v8::Object::New();
    for (int i=0; sqlerror[i]; ++i)
        sqlExceptionPrototype->Set(v8::String::New(sqlerror[i]), v8::Integer::New(i), attributes);
    engine->global()->Set(v8::String::New("SQLException"), sqlExceptionPrototype);

    return (void *)new QDeclarativeSqlDatabaseData(engine);
}

void qt_rem_qmlsqldatabase(QV8Engine * /* engine */, void *d)
{
    QDeclarativeSqlDatabaseData *data = (QDeclarativeSqlDatabaseData *)d;
    delete data;
}

void qt_qmlsqldatabase_setOfflineStoragePath(QV8Engine *engine, const QString &path)
{
    QDeclarativeSqlDatabaseData::data(engine)->offlineStoragePath = path;
}

QString qt_qmlsqldatabase_getOfflineStoragePath(const QV8Engine *engine)
{
    return QDeclarativeSqlDatabaseData::data(const_cast<QV8Engine *>(engine))->offlineStoragePath;
}

/*
HTML5 "spec" says "rs.rows[n]", but WebKit only impelments "rs.rows.item(n)". We do both (and property iterator).
We add a "forwardOnly" property that stops Qt caching results (code promises to only go forward
through the data.
*/

QT_END_NAMESPACE
