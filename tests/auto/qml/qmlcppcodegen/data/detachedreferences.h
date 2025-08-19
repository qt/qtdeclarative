#ifndef DETACHEDREFERENCES_H
#define DETACHEDREFERENCES_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qobject.h>

class DetachedReferences : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantMap map READ getMap WRITE setMap NOTIFY mapChanged)
    Q_PROPERTY(QVariantHash hash READ getHash WRITE setHash NOTIFY hashChanged)
    Q_PROPERTY(QVariantList list READ getList WRITE setList NOTIFY listChanged)

public:
    explicit DetachedReferences(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE QVariantMap getMap() const
    {
        return m_map;
    }

    void setMap(const QVariantMap &newMap)
    {
        if (m_map == newMap)
            return;
        m_map = newMap;
        emit mapChanged();
    }

    Q_INVOKABLE QVariantHash getHash() const
    {
        return m_hash;
    }

    void setHash(const QVariantHash &newHash)
    {
        if (m_hash == newHash)
            return;
        m_hash = newHash;
        emit hashChanged();
    }

    Q_INVOKABLE QVariantList getList() const
    {
        return m_list;
    }

    void setList(const QVariantList &newList)
    {
        if (m_list == newList)
            return;
        m_list = newList;
        emit listChanged();
    }

signals:
    void mapChanged();
    void hashChanged();
    void listChanged();

private:
    QVariantMap m_map;
    QVariantHash m_hash;
    QVariantList m_list;
};

#endif // DETACHEDREFERENCES_H
