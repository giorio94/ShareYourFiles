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

#ifndef NETWORKINTERFACESLIST_HPP
#define NETWORKINTERFACESLIST_HPP

#include <QObject>
#include <QPair>
#include <QPointer>
#include <QVector>

class QHostAddress;
class QNetworkInterface;
class QTimer;

///
/// \brief The NetworkInterfacesList class represents the list of pairs network
/// interface and IPv4 address that can be used by SYF.
///
/// In particular, this utility class provides a simple way to get the list of
/// all the valid entries and to be notified when such a list changes.
///
class NetworkEntriesList : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief The type representing the tuple network interface - IPv4 address.
    ///
    typedef QPair<QString, quint32> Entry;

    /// \brief A special value used to represent an invalid entry.
    static const Entry InvalidEntry;

    ///
    /// \brief Builds a new instance of NetworkInterfacesList.
    /// \param parent the parent of the current object.
    ///
    explicit NetworkEntriesList(QObject *parent = Q_NULLPTR);

    /// \brief Returns whether the list of entries is empty or not.
    bool empty() const { return m_entries.empty(); }

    /// \brief Returns the number of detected entries.
    bool count() const { return m_entries.count(); }

    ///
    /// \brief Returns the list containing all the valid entries detected.
    /// \return a list containing the valid pairs interface - address.
    ///
    QVector<Entry> const entries();

    ///
    /// \brief Updates the cached list of valid entries.
    ///
    void updateEntries();

    ///
    /// \brief Checks whether the network interface fulfills the requirements of
    /// SYF or not.
    /// \param iface the network interface to be checked.
    /// \return a boolean value indicating the result.
    ///
    static bool validNetworkInterface(const QNetworkInterface &iface);

    ///
    /// \brief Checks whether the host address fulfills the requirements of SYF
    /// or not.
    /// \param address the host address to be checked.
    /// \return a boolean value indicating the result.
    ///
    static bool validHostAddress(const QHostAddress &address);

    ///
    /// \brief Returns a human readable representation of the given entry.
    /// \param entry the instance representing the network entry.
    /// \return the human readable string.
    ///
    static QString toString(const Entry &entry);

signals:
    ///
    /// \brief Signal emitted when a change in the list of interfaces is
    /// detected.
    ///
    void networkInterfacesChanged();

private:
    ///
    /// \brief Builds a list containing all the valid pairs interface - address.
    /// \return the constructed data structure.
    ///
    static QVector<Entry> buildEntriesList();

private:
    /// \brief The list of valid entries detected.
    QVector<Entry> m_entries;

    /// \brief The timer used to update the entries every UPDATE_INTERVAL ms.
    QPointer<QTimer> m_timer;

    /// \brief The interval between subsequent updates (in milliseconds).
    static const int UPDATE_INTERVAL = 30000;
};

#endif // NETWORKINTERFACESLIST_HPP
