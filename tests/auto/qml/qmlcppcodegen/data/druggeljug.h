#ifndef DRUGGELJUG_H
#define DRUGGELJUG_H

#include <QtCore/qobject.h>
#include <QtCore/qdatetime.h>
#include <qqmlregistration.h>

#define STORE_FUNCTION(type, name, member, signal) \
    Q_INVOKABLE void name(type value) {       \
        if (value != member) {               \
            member = value;                  \
            emit signal(value);              \
        }                                    \
    }

class Druggeljug : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int myInt MEMBER m_myInt NOTIFY myIntChanged FINAL)
    Q_PROPERTY(uint myUint MEMBER m_myUint NOTIFY myUintChanged FINAL)
    Q_PROPERTY(qint8 myInt8 MEMBER m_myInt8 NOTIFY myInt8Changed FINAL)
    Q_PROPERTY(quint8 myUint8 MEMBER m_myUint8 NOTIFY myUint8Changed FINAL)
    Q_PROPERTY(qint16 myInt16 MEMBER m_myInt16 NOTIFY myInt16Changed FINAL)
    Q_PROPERTY(quint16 myUint16 MEMBER m_myUint16 NOTIFY myUint16Changed FINAL)
    Q_PROPERTY(qint32 myInt32 MEMBER m_myInt32 NOTIFY myInt32Changed FINAL)
    Q_PROPERTY(quint32 myUint32 MEMBER m_myUint32 NOTIFY myUint32Changed FINAL)
    Q_PROPERTY(qint64 myInt64 MEMBER m_myInt64 NOTIFY myInt64Changed FINAL)
    Q_PROPERTY(quint64 myUint64 MEMBER m_myUint64 NOTIFY myUint64Changed FINAL)

    Q_PROPERTY(QTime myTime MEMBER m_myTime NOTIFY myTimeChanged)
    Q_PROPERTY(QDate myDate MEMBER m_myDate NOTIFY myDateChanged)

public:
    Druggeljug(QObject* parent = nullptr) : QObject(parent) {}

    STORE_FUNCTION(int, storeMyInt, m_myInt, myIntChanged)
    STORE_FUNCTION(uint, storeMyUint, m_myUint, myUintChanged)
    STORE_FUNCTION(qint8, storeMyInt8, m_myInt8, myInt8Changed)
    STORE_FUNCTION(quint8, storeMyUint8, m_myUint8, myUint8Changed)
    STORE_FUNCTION(qint16, storeMyInt16, m_myInt16, myInt16Changed)
    STORE_FUNCTION(quint16, storeMyUint16, m_myUint16, myUint16Changed)
    STORE_FUNCTION(qint32, storeMyInt32, m_myInt32, myInt32Changed)
    STORE_FUNCTION(quint32, storeMyUint32, m_myUint32, myUint32Changed)
    STORE_FUNCTION(qint64, storeMyInt64, m_myInt64, myInt64Changed)
    STORE_FUNCTION(quint64, storeMyUint64, m_myUint64, myUint64Changed)

    QTime myTime() const { return m_myTime; }
    QDate myDate() const { return m_myDate; }

private:
    int m_myInt = 0;
    uint m_myUint = 0;
    qint8 m_myInt8 = 0;
    quint8 m_myUint8 = 0;
    qint16 m_myInt16 = 0;
    quint16 m_myUint16 = 0;
    qint32 m_myInt32 = 0;
    quint32 m_myUint32 = 0;
    qint64 m_myInt64 = 0;
    quint64 m_myUint64 = 0;

    QTime m_myTime = QTime(11, 55, 0);
    QDate m_myDate = QDate(2017, 9, 3);

signals:
    void myIntChanged(int);
    void myUintChanged(uint);
    void myInt8Changed(qint8);
    void myUint8Changed(quint8);
    void myInt16Changed(qint16);
    void myUint16Changed(quint16);
    void myInt32Changed(qint32);
    void myUint32Changed(quint32);
    void myInt64Changed(qint64);
    void myUint64Changed(quint64);
    void myTimeChanged();
    void myDateChanged();
};

#endif
