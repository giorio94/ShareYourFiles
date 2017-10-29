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

#include "networkentrieslist.hpp"

#include <Logger.h>

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QTimer>

// Static variables definition
const NetworkEntriesList::Entry NetworkEntriesList::InvalidEntry =
    QPair<QString, quint32>(QString(), 0);

///
/// \brief Appends to the log the information about the detected entries.
/// \param entries the list of detected entries.
///
static void logEntries(const QVector<NetworkEntriesList::Entry> &entries)
{
    if (entries.isEmpty()) {
        LOG_WARNING() << "NetworkEntriesList: no valid entry detected";
        return;
    }

    foreach (const NetworkEntriesList::Entry &entry, entries) {
        LOG_INFO() << "NetworkEntriesList: detected"
                   << qUtf8Printable(NetworkEntriesList::toString(entry));
    }
}

///
/// The instance is created by caching the network interfaces list and by
/// starting the timer in charge of updating the list every UPDATE_INTERVAL.
///
NetworkEntriesList::NetworkEntriesList(QObject *parent)
        : QObject(parent),
          m_entries(buildEntriesList()),
          m_timer(new QTimer(this))
{
    // Connect the slot and start the timer to update the list
    connect(m_timer, &QTimer::timeout, this,
            &NetworkEntriesList::updateEntries);
    m_timer->start(NetworkEntriesList::UPDATE_INTERVAL);

    LOG_INFO() << "NetworkEntriesList: initialization...";
    logEntries(m_entries);
    LOG_INFO() << "NetworkEntriesList: initialization completed";
}

///
/// The function updates the cached list of entries and then returns it.
///
const QVector<NetworkEntriesList::Entry> NetworkEntriesList::entries()
{
    updateEntries();
    return m_entries;
}

///
/// The function recomputes the list of valid entries though the
/// buildEntriesList() method and compares it against the cached one: in case
/// they are different, the cached version is updated and the
/// networkInterfacesChanged() signal is emitted.
///
void NetworkEntriesList::updateEntries()
{
    // Get the new list of entries
    QVector<Entry> currentEntries = buildEntriesList();

    // Check if the list is different from the cached one
    if (currentEntries != m_entries) {
        // Update it and emit the networkInterfacesChanged() signal
        m_entries = currentEntries;

        LOG_INFO() << "NetworkEntriesList: updating...";
        logEntries(m_entries);
        LOG_INFO() << "NetworkEntriesList: updated";

        emit networkInterfacesChanged();
    }
}

///
/// The function checks the flags associated to the interface to verify if it
/// can be used by SYF: in particular, it must be up and running, able to send
/// multicast data and not be marked as loopback.
///
bool NetworkEntriesList::validNetworkInterface(const QNetworkInterface &iface)
{
    // The interface is not valid
    if (!iface.isValid())
        return false;

    // Check the flags to determine if the interface is valid
    QNetworkInterface::InterfaceFlags ifaceFlags = iface.flags();
    return (ifaceFlags & QNetworkInterface::IsUp) &&
           (ifaceFlags & QNetworkInterface::IsRunning) &&
           (ifaceFlags & QNetworkInterface::CanMulticast) &&
           !(ifaceFlags & QNetworkInterface::IsLoopBack);
}

///
/// The function checks the properties associated to the address to verify if it
/// can be used by as source address by SYF: in particular, it must be a valid
/// unicast IPv4 address.
///
bool NetworkEntriesList::validHostAddress(const QHostAddress &address)
{
    return !address.isNull() && !address.isLoopback() &&
           !address.isMulticast() &&
           address.protocol() == QAbstractSocket::IPv4Protocol;
}

///
/// The function returns a string representing the specified entry, including
/// both the network interface and the IPv4 address associated.
///
QString NetworkEntriesList::toString(const NetworkEntriesList::Entry &entry)
{
    return QNetworkInterface::interfaceFromName(entry.first)
               .humanReadableName() +
           " (" + QHostAddress(entry.second).toString() + ")";
}

///
/// The function scans all the network interfaces and discards the ones that are
/// not valid; for the remaining, then, the list of associated addresses is
/// scanned and every valid pair interface - address is added to the set to be
/// returned.
///
QVector<NetworkEntriesList::Entry> NetworkEntriesList::buildEntriesList()
{
    QVector<Entry> entries;

    // Loop among all available interfaces
    foreach (const QNetworkInterface &iface,
             QNetworkInterface::allInterfaces()) {

        // Skip invalid interfaces
        if (!validNetworkInterface(iface)) {
            continue;
        }

        // Loop among all addresses associated to the interface
        foreach (const QNetworkAddressEntry &addressEntry,
                 iface.addressEntries()) {

            QHostAddress address(addressEntry.ip());
            // If the address is valid, add the tuple to the list
            if (validHostAddress(address)) {
                entries.push_back(QPair<QString, quint32>(
                    iface.name(), address.toIPv4Address()));
            }
        }
    }

    return entries;
}
