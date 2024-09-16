#ifndef QLISTPROVIDER_H
#define QLISTPROVIDER_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qobject.h>
#include <QtCore/qjsonarray.h>

class QListProvider : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ListProvider)
    Q_PROPERTY(QJsonArray json READ json WRITE setJson NOTIFY jsonChanged FINAL)
    Q_PROPERTY(QStringList strings READ strings WRITE setStrings NOTIFY stringsChanged FINAL)

    Q_PROPERTY(QList<QStringList> stringStrings READ stringStrings WRITE setStringStrings NOTIFY stringStringsChanged FINAL)

public:
    explicit QListProvider(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE QList<int> intList() const
    {
        QList<int> list;
        for (int i = 0; i < 3; ++i)
            list.append(i);
        return list;
    }

    QStringList strings() const { return m_strings; }
    void setStrings(const QStringList &strings)
    {
        if (strings != m_strings) {
            m_strings = strings;
            emit stringsChanged();
        }
    }

    QJsonArray json() const { return m_json; }
    void setJson(const QJsonArray &json)
    {
        if (json != m_json) {
            m_json = json;
            emit jsonChanged();
        }
    }

    QList<QStringList> stringStrings() const
    {
        return m_stringStrings;
    }
    void setStringStrings(const QList<QStringList> &stringStrings)
    {
        if (m_stringStrings != stringStrings) {
            m_stringStrings = stringStrings;
            emit stringStringsChanged();
        }
    }

signals:
    void jsonChanged();
    void stringsChanged();
    void stringStringsChanged();

private:
    QJsonArray m_json;
    QStringList m_strings;
    QList<QStringList> m_stringStrings;
};

#endif // QLISTPROVIDER_H
