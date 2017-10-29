/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "peersselectormodel.hpp"
#include "UserDiscovery/users.hpp"

PeersSelectorModel::PeersSelectorModel(quint32 filesNumber,
                                       const QString &filesSize,
                                       PeersList *source, QObject *parent)
        : QAbstractListModel(parent),
          m_source(source),
          m_data(source->activePeers().values()),
          m_selectedCount(0),
          m_filesNumber(filesNumber),
          m_filesSize(filesSize)
{
    connect(source, &PeersList::peerAdded, this, &PeersSelectorModel::addPeer);
    connect(source, &PeersList::peerExpired, this,
            &PeersSelectorModel::removePeer);
    connect(source, &PeersList::peerUpdated, this,
            &PeersSelectorModel::updatePeer);

    // Set all elements as not selected
    for (int i = 0; i < m_data.count(); i++) {
        m_selected << false;
    }
}

int PeersSelectorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.size();
}

QVariant PeersSelectorModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_data.size())
        return QVariant();

    const UserInfo &info = m_data.at(index.row());

    switch (role) {
    case Roles::FirstNameRole:
        return info.firstName();
    case Roles::LastNameRole:
        return info.lastName();
    case Roles::IconSetRole:
        return info.icon().set();
    case Roles::IconPathRole:
        return info.icon().path();
    case Roles::SelectedRole:
        return m_selected.at(index.row());
    }

    return QVariant();
}

void PeersSelectorModel::toggleSelected(const int index)
{
    if (index < 0 || index >= m_data.size())
        return;

    bool selected = m_selected[index] = !m_selected.at(index);
    QModelIndex mindex = createIndex(index, 0);
    emit dataChanged(mindex, mindex, {Roles::SelectedRole});

    selected ? m_selectedCount++ : m_selectedCount--;
    emit selectedCountChanged();
}

void PeersSelectorModel::completeSelection(bool confirm)
{
    emit selectionCompleted(confirm);
}

QStringList PeersSelectorModel::selectedItems() const
{
    QStringList selected;
    for (int i = 0; i < m_selected.length(); i++) {
        if (m_selected.at(i)) {
            selected << m_data.at(i).uuid();
        }
    }
    return selected;
}

QHash<int, QByteArray> PeersSelectorModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::FirstNameRole] = "firstName";
    roles[Roles::LastNameRole] = "lastName";
    roles[Roles::IconSetRole] = "iconSet";
    roles[Roles::IconPathRole] = "iconPath";
    roles[Roles::SelectedRole] = "selected";
    return roles;
}

void PeersSelectorModel::addPeer(const QString &uuid)
{
    UserInfo peer = m_source->activePeer(uuid);
    if (!peer.valid())
        return;

    int index = rowCount();
    beginInsertRows(QModelIndex(), index, index);
    m_data << peer;
    m_selected << false;
    endInsertRows();

    emit rowCountChanged();
}

void PeersSelectorModel::removePeer(const QString &uuid)
{
    int index = indexOf(uuid);
    if (index == -1)
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_data.removeAt(index);
    bool selected = m_selected.takeAt(index);
    endRemoveRows();

    emit rowCountChanged();
    if (selected) {
        m_selectedCount--;
        emit selectedCountChanged();
    }
}

void PeersSelectorModel::updatePeer(const QString &uuid)
{
    int index = indexOf(uuid);
    if (index == -1)
        return;

    UserInfo peer = m_source->activePeer(uuid);
    if (!peer.valid())
        return;

    m_data.replace(index, peer);
    QModelIndex mindex = createIndex(index, 0);
    emit dataChanged(mindex, mindex);
}

int PeersSelectorModel::indexOf(const QString &uuid) const
{
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data.at(i).uuid() == uuid) {
            return i;
        }
    }

    return -1;
}
