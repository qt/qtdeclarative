// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQmlXmlListModel/private/qqmlxmllistmodel_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtTest/qsignalspy.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlnetworkaccessmanagerfactory.h>

#include <QtCore/qset.h>
#include <QtCore/qsortfilterproxymodel.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qtimer.h>

using namespace Qt::StringLiterals;

typedef QList<QVariantList> QQmlXmlModelData;

Q_DECLARE_METATYPE(QQmlXmlModelData)
Q_DECLARE_METATYPE(QQmlXmlListModel::Status)

class tst_QQmlXmlListModel : public QQmlDataTest

{
    Q_OBJECT
public:
    tst_QQmlXmlListModel() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

private slots:
    void initTestCase() override
    {
        QQmlDataTest::initTestCase();
        qRegisterMetaType<QQmlXmlListModel::Status>();
    }

    void buildModel();
    void cdata();
    void attributes();
    void roles();
    void elementErrors();
    void uniqueRoleNames();
    void headers();
    void source();
    void source_data();
    void data();
    void reload();
    void threading();
    void threading_data();
    void propertyChanges();
    void nestedElements();
    void malformedData();
    void malformedData_data();

    void roleCrash();
    void proxyCrash();

private:
    QString errorString(QAbstractItemModel *model)
    {
        QString ret;
        QMetaObject::invokeMethod(model, "errorString", Q_RETURN_ARG(QString, ret));
        return ret;
    }

    QString makeItemXmlAndData(const QString &data, QQmlXmlModelData *modelData = 0) const
    {
        if (modelData)
            modelData->clear();
        QString xml;

        if (!data.isEmpty()) {
            const QStringList items = data.split(QLatin1Char(';'));
            for (const QString &item : items) {
                if (item.isEmpty())
                    continue;
                QVariantList variants;
                xml += QLatin1String("<item>");
                const QStringList fields = item.split(QLatin1Char(','));
                for (const QString &field : fields) {
                    QStringList values = field.split(QLatin1Char('='));
                    if (values.size() != 2) {
                        qWarning() << "makeItemXmlAndData: invalid field:" << field;
                        continue;
                    }
                    xml += QString("<%1>%2</%1>").arg(values[0], values[1]);
                    if (!modelData)
                        continue;
                    bool isNum = false;
                    int number = values[1].toInt(&isNum);
                    if (isNum)
                        variants << number;
                    else
                        variants << values[1];
                }
                xml += QLatin1String("</item>");
                if (modelData)
                    modelData->append(variants);
            }
        }

        QString decl = "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>";
        return decl + QLatin1String("<data>") + xml + QLatin1String("</data>");
    }

    QQmlEngine engine;
};

class ScopedFile
{
public:
    ScopedFile(const QString &fileName, const QByteArray &data) : m_fileName(fileName)
    {
        m_file.setFileName(fileName);
        m_created = m_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if (m_created) {
            const auto written = m_file.write(data);
            m_created = written == data.size();
            m_file.close();
        }
    }
    ~ScopedFile() { QFile::remove(m_file.fileName()); }

    bool isCreated() const { return m_created; }
    QString fileName() const { return m_fileName; }

private:
    QFile m_file;
    const QString m_fileName;
    bool m_created = false;
};

class CustomNetworkAccessManagerFactory : public QObject, public QQmlNetworkAccessManagerFactory
{
    Q_OBJECT
public:
    QVariantMap lastSentHeaders;

protected:
    QNetworkAccessManager *create(QObject *parent) override;
};

class CustomNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    CustomNetworkAccessManager(CustomNetworkAccessManagerFactory *factory, QObject *parent)
        : QNetworkAccessManager(parent), m_factory(factory)
    {
    }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req,
                                 QIODevice *outgoingData = 0) override
    {
        if (m_factory) {
            QVariantMap map;
            const auto rawHeaderList = req.rawHeaderList();
            for (const QByteArray &header : rawHeaderList)
                map[header] = req.rawHeader(header);
            m_factory->lastSentHeaders = map;
        }
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
    }

    QPointer<CustomNetworkAccessManagerFactory> m_factory;
};

QNetworkAccessManager *CustomNetworkAccessManagerFactory::create(QObject *parent)
{
    return new CustomNetworkAccessManager(this, parent);
}

void tst_QQmlXmlListModel::buildModel()
{
    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QModelIndex index = model->index(3, 0);
    QCOMPARE(model->data(index, Qt::UserRole).toString(), QLatin1String("Spot"));
    QCOMPARE(model->data(index, Qt::UserRole + 1).toString(), QLatin1String("Dog"));
    QCOMPARE(model->data(index, Qt::UserRole + 2).toInt(), 9);
    QCOMPARE(model->data(index, Qt::UserRole + 3).toString(), QLatin1String("Medium"));
}

void tst_QQmlXmlListModel::cdata()
{
    QQmlComponent component(&engine, testFileUrl("recipes.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 5);

    QVERIFY(model->data(model->index(2, 0), Qt::UserRole + 2)
                    .toString()
                    .startsWith(QLatin1String("<html>")));
}

void tst_QQmlXmlListModel::attributes()
{
    QQmlComponent component(&engine, testFileUrl("attributes.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 4);

    const QList<QVariantList> desiredResults = { QVariantList { "Polly", "Parrot", 12, "Small" },
                                                 QVariantList { "Penny", "Turtle", 4, "Small" },
                                                 QVariantList { "Spot", "Dog", 9, "Medium" },
                                                 QVariantList { "Tiny", "Elephant", 15, "Large" } };

    QVERIFY(model->rowCount() == desiredResults.size());

    for (qsizetype idx = 0; idx < model->rowCount(); ++idx) {
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole).toString(),
                 desiredResults.at(idx).at(0).toString());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 1).toString(),
                 desiredResults.at(idx).at(1).toString());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 2).toInt(),
                 desiredResults.at(idx).at(2).toInt());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 3).toString(),
                 desiredResults.at(idx).at(3).toString());
    }
}

void tst_QQmlXmlListModel::roles()
{
    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 4);
    QVERIFY(roleNames.key("name", -1) >= 0);
    QVERIFY(roleNames.key("type", -1) >= 0);
    QVERIFY(roleNames.key("age", -1) >= 0);
    QVERIFY(roleNames.key("size", -1) >= 0);

    QSet<int> roles;
    roles.insert(roleNames.key("name"));
    roles.insert(roleNames.key("type"));
    roles.insert(roleNames.key("age"));
    roles.insert(roleNames.key("size"));
    QCOMPARE(roles.size(), 4);
}

void tst_QQmlXmlListModel::elementErrors()
{
    QQmlComponent component(&engine, testFileUrl("elementErrors.qml"));
    QTest::ignoreMessage(QtWarningMsg,
                         (testFileUrl("elementErrors.qml").toString()
                          + ":6:5: QML XmlListModelRole: An XML element must not start with '/'")
                                 .toUtf8()
                                 .constData());
    QTest::ignoreMessage(QtWarningMsg,
                         (testFileUrl("elementErrors.qml").toString()
                          + ":7:5: QML XmlListModelRole: An XML element must not end with '/'")
                                 .toUtf8()
                                 .constData());
    QTest::ignoreMessage(QtWarningMsg,
                         (testFileUrl("elementErrors.qml").toString()
                          + ":8:5: QML XmlListModelRole: An XML element must not contain \"//\"")
                                 .toUtf8()
                                 .constData());

    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QModelIndex index = model->index(3, 0);
    QCOMPARE(model->data(index, Qt::UserRole).toString(), QString());
    QCOMPARE(model->data(index, Qt::UserRole + 1).toString(), QString());
    QCOMPARE(model->data(index, Qt::UserRole + 2).toString(), QString());
}

void tst_QQmlXmlListModel::uniqueRoleNames()
{
    QQmlComponent component(&engine, testFileUrl("unique.qml"));
    QTest::ignoreMessage(QtWarningMsg,
                         (testFileUrl("unique.qml").toString()
                          + ":7:5: QML XmlListModelRole: \"name\" duplicates a previous role name "
                            "and will be disabled.")
                                 .toUtf8()
                                 .constData());
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);
}

void tst_QQmlXmlListModel::headers()
{
    // ensure the QNetworkAccessManagers created for this test are immediately deleted
    QQmlEngine qmlEng;

    CustomNetworkAccessManagerFactory factory;
    qmlEng.setNetworkAccessManagerFactory(&factory);

    QQmlComponent component(&qmlEng, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
                 QQmlXmlListModel::Ready);

    // It doesn't do a network request for a local file
    QCOMPARE(factory.lastSentHeaders.size(), 0);

    model->setProperty("source", QUrl("http://localhost/filethatdoesnotexist.xml"));
    QTRY_COMPARE_WITH_TIMEOUT(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
                              QQmlXmlListModel::Error, 10000);

    QVariantMap expectedHeaders;
    expectedHeaders["Accept"] = "application/xml,*/*";

    QCOMPARE(factory.lastSentHeaders.size(), expectedHeaders.size());
    for (auto it = expectedHeaders.cbegin(), end = expectedHeaders.cend(); it != end; ++it) {
        QVERIFY(factory.lastSentHeaders.contains(it.key()));
        QCOMPARE(factory.lastSentHeaders[it.key()].toString(), it.value().toString());
    }
}

void tst_QQmlXmlListModel::source()
{
    QFETCH(QUrl, source);
    QFETCH(int, count);
    QFETCH(QQmlXmlListModel::Status, status);

    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QSignalSpy spy(model.get(), SIGNAL(statusChanged(QQmlXmlListModel::Status)));

    QVERIFY(errorString(model.get()).isEmpty());
    QCOMPARE(model->property("progress").toDouble(), qreal(1.0));
    QCOMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
             QQmlXmlListModel::Loading);
    QTRY_COMPARE(spy.size(), 1);
    spy.clear();
    QCOMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
             QQmlXmlListModel::Ready);
    QVERIFY(errorString(model.get()).isEmpty());
    QCOMPARE(model->property("progress").toDouble(), qreal(1.0));
    QCOMPARE(model->rowCount(), 9);

    model->setProperty("source", source);
    if (model->property("source").toString().isEmpty())
        QCOMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
                 QQmlXmlListModel::Null);
    qreal expectedProgress = (source.isLocalFile() || (source.scheme() == "qrc"_L1)) ? 1.0 : 0.0;
    QCOMPARE(model->property("progress").toDouble(), expectedProgress);
    QTRY_COMPARE(spy.size(), 1);
    spy.clear();
    QCOMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")),
             QQmlXmlListModel::Loading);
    QVERIFY(errorString(model.get()).isEmpty());

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(model.get(), SIGNAL(statusChanged(QQmlXmlListModel::Status)), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(20000);
    loop.exec();

    if (spy.size() == 0 && status != QQmlXmlListModel::Ready) {
        qWarning("QQmlXmlListModel invalid source test timed out");
    } else {
        QCOMPARE(spy.size(), 1);
        spy.clear();
    }

    QCOMPARE(qvariant_cast<QQmlXmlListModel::Status>(model->property("status")), status);
    QCOMPARE(model->rowCount(), count);

    if (status == QQmlXmlListModel::Ready)
        QCOMPARE(model->property("progress").toDouble(), qreal(1.0));

    QCOMPARE(errorString(model.get()).isEmpty(), status == QQmlXmlListModel::Ready);
}

void tst_QQmlXmlListModel::source_data()
{
    QTest::addColumn<QUrl>("source");
    QTest::addColumn<int>("count");
    QTest::addColumn<QQmlXmlListModel::Status>("status");

    QTest::newRow("valid") << testFileUrl("model2.xml") << 2 << QQmlXmlListModel::Ready;
    QTest::newRow("invalid") << QUrl("http://blah.blah/blah.xml") << 0 << QQmlXmlListModel::Error;

    // empty file
    QTemporaryFile *temp = new QTemporaryFile(this);
    if (temp->open())
        QTest::newRow("empty file")
                << QUrl::fromLocalFile(temp->fileName()) << 0 << QQmlXmlListModel::Ready;
    temp->close();
}

void tst_QQmlXmlListModel::data()
{
    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);

    for (int i = 0; i < 9; i++) {
        QModelIndex index = model->index(i, 0);
        for (int j = 0; j < model->roleNames().size(); j++) {
            QCOMPARE(model->data(index, j), QVariant());
        }
    }
    QTRY_COMPARE(model->rowCount(), 9);
}

void tst_QQmlXmlListModel::reload()
{
    // If no keys are used, the model should be rebuilt from scratch when
    // reload() is called.

    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QSignalSpy spyInsert(model.get(), SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy spyRemove(model.get(), SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy spyCount(model.get(), SIGNAL(countChanged()));
    // reload multiple times to test the xml query aborting
    QMetaObject::invokeMethod(model.get(), "reload");
    QMetaObject::invokeMethod(model.get(), "reload");
    QCoreApplication::processEvents();
    QMetaObject::invokeMethod(model.get(), "reload");
    QMetaObject::invokeMethod(model.get(), "reload");
    QTRY_COMPARE(spyCount.size(), 0);
    QTRY_COMPARE(spyInsert.size(), 1);
    QTRY_COMPARE(spyRemove.size(), 1);

    QCOMPARE(spyInsert[0][1].toInt(), 0);
    QCOMPARE(spyInsert[0][2].toInt(), 8);

    QCOMPARE(spyRemove[0][1].toInt(), 0);
    QCOMPARE(spyRemove[0][2].toInt(), 8);
}

void tst_QQmlXmlListModel::threading()
{
    QFETCH(int, xmlDataCount);

    QQmlComponent component(&engine, testFileUrl("threading.qml"));

    QScopedPointer<QAbstractItemModel> m1(qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(m1 != nullptr);
    QScopedPointer<QAbstractItemModel> m2(qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(m2 != nullptr);
    QScopedPointer<QAbstractItemModel> m3(qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(m3 != nullptr);

    QTemporaryDir tempDir;

    for (int dataCount = 0; dataCount < xmlDataCount; ++dataCount) {
        QString data1, data2, data3;
        for (int i = 0; i < dataCount; ++i) {
            data1 += "name=A" + QString::number(i) + ",age=1" + QString::number(i)
                    + ",sport=Football;";
            data2 += "name=B" + QString::number(i) + ",age=2" + QString::number(i)
                    + ",sport=Athletics;";
            data3 += "name=C" + QString::number(i) + ",age=3" + QString::number(i)
                    + ",sport=Curling;";
        }

        ScopedFile f1(tempDir.filePath("file1.xml"), makeItemXmlAndData(data1).toLatin1());
        ScopedFile f2(tempDir.filePath("file2.xml"), makeItemXmlAndData(data2).toLatin1());
        ScopedFile f3(tempDir.filePath("file3.xml"), makeItemXmlAndData(data3).toLatin1());
        QVERIFY(f1.isCreated() && f2.isCreated() && f3.isCreated());

        m1->setProperty("source", QUrl::fromLocalFile(f1.fileName()));
        m2->setProperty("source", QUrl::fromLocalFile(f2.fileName()));
        m3->setProperty("source", QUrl::fromLocalFile(f3.fileName()));
        QCoreApplication::processEvents();

        QTRY_VERIFY(m1->rowCount() == dataCount && m2->rowCount() == dataCount
                    && m3->rowCount() == dataCount);

        for (int i = 0; i < dataCount; ++i) {
            QModelIndex index = m1->index(i, 0);
            QList<int> roles = m1->roleNames().keys();
            std::sort(roles.begin(), roles.end());
            QCOMPARE(m1->data(index, roles.at(0)).toString(),
                     QLatin1Char('A') + QString::number(i));
            QCOMPARE(m1->data(index, roles.at(1)).toString(),
                     QLatin1Char('1') + QString::number(i));
            QCOMPARE(m1->data(index, roles.at(2)).toString(), QString("Football"));

            index = m2->index(i, 0);
            roles = m2->roleNames().keys();
            std::sort(roles.begin(), roles.end());
            QCOMPARE(m2->data(index, roles.at(0)).toString(),
                     QLatin1Char('B') + QString::number(i));
            QCOMPARE(m2->data(index, roles.at(1)).toString(),
                     QLatin1Char('2') + QString::number(i));
            QCOMPARE(m2->data(index, roles.at(2)).toString(), QString("Athletics"));

            index = m3->index(i, 0);
            roles = m3->roleNames().keys();
            std::sort(roles.begin(), roles.end());
            QCOMPARE(m3->data(index, roles.at(0)).toString(),
                     QLatin1Char('C') + QString::number(i));
            QCOMPARE(m3->data(index, roles.at(1)).toString(),
                     QLatin1Char('3') + QString::number(i));
            QCOMPARE(m3->data(index, roles.at(2)).toString(), QString("Curling"));
        }

        // clear sources, so that we could reuse same file names later
        m1->setProperty("source", QUrl());
        m2->setProperty("source", QUrl());
        m3->setProperty("source", QUrl());
    }
}

void tst_QQmlXmlListModel::threading_data()
{
    QTest::addColumn<int>("xmlDataCount");

    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
    QTest::newRow("10") << 10;
}

void tst_QQmlXmlListModel::propertyChanges()
{
    QQmlComponent component(&engine, testFileUrl("propertychanges.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 9);

    QObject *role = model->findChild<QObject *>("role");
    QVERIFY(role);

    QSignalSpy nameSpy(role, SIGNAL(nameChanged()));
    QSignalSpy elementSpy(role, SIGNAL(elementNameChanged()));

    role->setProperty("name", "size");
    role->setProperty("elementName", "size");

    QCOMPARE(role->property("name").toString(), QString("size"));
    QCOMPARE(role->property("elementName").toString(), QString("size"));

    QCOMPARE(nameSpy.size(), 1);
    QCOMPARE(elementSpy.size(), 1);

    role->setProperty("name", "size");
    role->setProperty("elementName", "size");

    QCOMPARE(nameSpy.size(), 1);
    QCOMPARE(elementSpy.size(), 1);

    QSignalSpy sourceSpy(model.get(), SIGNAL(sourceChanged()));
    QSignalSpy modelQuerySpy(model.get(), SIGNAL(queryChanged()));

    model->setProperty("source", QUrl("model2.xml"));
    model->setProperty("query", "/Pets");

    QCOMPARE(model->property("source").toUrl(), QUrl("model2.xml"));
    QCOMPARE(model->property("query").toString(), QString("/Pets"));

    QTRY_COMPARE(model->rowCount(), 1);

    QCOMPARE(sourceSpy.size(), 1);
    QCOMPARE(modelQuerySpy.size(), 1);

    model->setProperty("source", QUrl("model2.xml"));
    model->setProperty("query", "/Pets");

    QCOMPARE(sourceSpy.size(), 1);
    QCOMPARE(modelQuerySpy.size(), 1);

    QTRY_COMPARE(model->rowCount(), 1);
}

void tst_QQmlXmlListModel::nestedElements()
{
    QQmlComponent component(&engine, testFileUrl("nestedElements.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    QTRY_COMPARE(model->rowCount(), 4);

    const QList<QVariantList> desiredResults = { QVariantList { "Polly", "Parrot", 12, "Small" },
                                                 QVariantList { "Penny", "Turtle", 4, "Small" },
                                                 QVariantList { "Spot", "Dog", 9, "Medium" },
                                                 QVariantList { "Tiny", "Elephant", 15, "Large" } };

    QVERIFY(model->rowCount() == desiredResults.size());

    for (qsizetype idx = 0; idx < model->rowCount(); ++idx) {
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole).toString(),
                 desiredResults.at(idx).at(0).toString());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 1).toString(),
                 desiredResults.at(idx).at(1).toString());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 2).toInt(),
                 desiredResults.at(idx).at(2).toInt());
        QCOMPARE(model->data(model->index(idx, 0), Qt::UserRole + 3).toString(),
                 desiredResults.at(idx).at(3).toString());
    }
}

void tst_QQmlXmlListModel::malformedData()
{
    QFETCH(QUrl, fileName);
    QFETCH(QString, errorMessage);

    // In this test we check that malformed xml document would not cause
    // infinite loop while parsing, and that the errors will be reported.

    QTest::ignoreMessage(
            QtWarningMsg,
            (testFileUrl("malformedData.qml").toString() + errorMessage).toUtf8().constData());

    QQmlComponent component(&engine, testFileUrl("malformedData.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
    model->setProperty("source", fileName);
    QTRY_VERIFY(model->rowCount() != 0);
}

void tst_QQmlXmlListModel::malformedData_data()
{
    QTest::addColumn<QUrl>("fileName");
    QTest::addColumn<QString>("errorMessage");

    QTest::addRow("tag mismatch top level")
            << testFileUrl("malformedTagTopLevel.xml")
            << QStringLiteral(
                       ":3:1: QML XmlListModel: Query error: \"Opening and ending tag mismatch.\"");
    QTest::addRow("missing tag nested level")
            << testFileUrl("malformedTagNestedLevel.xml")
            << QStringLiteral(
                       ":3:1: QML XmlListModel: Query error: \"Opening and ending tag mismatch.\"");
    QTest::addRow("invalid attribute name")
            << testFileUrl("malformedAttribute.xml")
            << QStringLiteral(":3:1: QML XmlListModel: Query error: \"Expected '>' or '/', but got "
                              "'[0-9]'.\"");
}

void tst_QQmlXmlListModel::roleCrash()
{
    // don't crash
    QQmlComponent component(&engine, testFileUrl("roleCrash.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
}

class SortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource)

public:
    SortFilterProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) { sort(0); }
    QObject *source() const { return sourceModel(); }
    void setSource(QObject *source) { setSourceModel(qobject_cast<QAbstractItemModel *>(source)); }
};

void tst_QQmlXmlListModel::proxyCrash()
{
    qmlRegisterType<SortFilterProxyModel>("SortFilterProxyModel", 1, 0, "SortFilterProxyModel");

    // don't crash
    QQmlComponent component(&engine, testFileUrl("proxyCrash.qml"));
    QScopedPointer<QAbstractItemModel> model(
            qobject_cast<QAbstractItemModel *>(component.create()));
    QVERIFY(model != nullptr);
}

QTEST_MAIN(tst_QQmlXmlListModel)

#include "tst_qqmlxmllistmodel.moc"
