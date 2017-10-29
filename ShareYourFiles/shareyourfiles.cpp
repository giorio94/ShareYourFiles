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

#include "shareyourfiles.hpp"
#include "Common/threadpool.hpp"
#include "FileTransfer/syfpprotocol.hpp"
#include "UserDiscovery/syfddatagram.hpp"
#include "UserDiscovery/syfdprotocol.hpp"
#include "UserDiscovery/user.hpp"

#include <Logger.h>

#include <QDir>
#include <QLockFile>

// Static variables definition
ShareYourFiles *ShareYourFiles::m_instance = Q_NULLPTR;

///
/// The singleton instance is created through the ShareYourFiles constructor,
/// and stored for later retrieval. A boolean value is returned indicating the
/// outcome of the operation.
///
bool ShareYourFiles::createInstance(const QString &confPath,
                                    const QString &dataPath)
{
    LOG_ASSERT_X(m_instance == Q_NULLPTR,
                 "ShareYourFiles: instance already created");
    m_instance = new ShareYourFiles(confPath, dataPath);
    return !m_instance->error();
}

///
/// The singleton instance is deleted through the ShareYourFiles destructor, and
/// the pointer is reset.
///
void ShareYourFiles::destroyInstance()
{
    delete m_instance;
    m_instance = Q_NULLPTR;
}

///
/// The main data structures and protocols necessary for the application
/// operation are methodically created and initialized. In case some operation
/// fails, the error flag is set to true.
///
ShareYourFiles::ShareYourFiles(const QString &confPath, const QString &dataPath,
                               QObject *parent)
        : QObject(parent), m_error(true)
{
    LOG_INFO() << "ShareYourFiles: initialization...";
    LOG_INFO() << "ShareYourFiles: configuration path -" << confPath;

    // Try to acquire the global lock
    if (!initLock(confPath)) {
        return;
    }

    // Create the thread pool
    ThreadPool::createInstance();

    // Initialize the SYFP protocol
    if (!initSYFPProtocol()) {
        return;
    }

    // Initialize the network entries
    if (!initNetworkEntries()) {
        return;
    }

    // Initialize the user instances
    initUserInstances(confPath, dataPath);

    // Initialize the SYFD protocol
    if (!initSYFDProtocol(Enums::OperationalMode::Online)) {
        return;
    }

    m_error = false;
    m_errorMessage = tr("Success.");

    LOG_INFO() << "ShareYourFiles: initialization completed";
}

///
/// The main data structures and protocols necessary for the application
/// operation are methodically terminated and destroyed.
///
ShareYourFiles::~ShareYourFiles()
{
    LOG_INFO() << "ShareYourFiles: stopping...";

    // Terminate the SYFP protocol
    if (m_syfpInstance) {
        m_syfpInstance->deleteLater();
    }

    // Terminate the SYFD protocol
    if (m_syfdInstance) {
        m_syfdInstance->deleteLater();
    }

    // Destroy the user instances
    delete m_peersList;
    delete m_localInstance;

    delete m_networkEntries;

    // Terminate the threads
    ThreadPool::destroyInstance();

    // Release the global lock
    m_locker.reset();

    LOG_INFO() << "ShareYourFiles: stopped";
}

///
/// The function returns the network entry currently in use by the protocols.
///
const NetworkEntriesList::Entry &ShareYourFiles::currentNetworkEntry() const
{
    return m_currentNetworkEntry;
}

///
/// The function is in charge of changing the network entry (i.e. the pair
/// network interface and IPv4 address) used by the protocols. In particular,
/// the current instance of the SYFD Protocol is stopped and then restarted
/// with the new association; the source addresses used by the other protocols
/// are also updated.
///
bool ShareYourFiles::changeNetworkEntry(const NetworkEntriesList::Entry &entry)
{
    // In case the target is equal to the current entry, just return
    if (entry == m_currentNetworkEntry) {
        return true;
    }

    LOG_INFO() << "ShareYourFiles: changing selected network entry to"
               << qUtf8Printable(NetworkEntriesList::toString(entry));

    // Get the current list of entries
    QVector<NetworkEntriesList::Entry> entries = m_networkEntries->entries();

    // If the specified entry is not valid, return
    if (!entries.contains(entry)) {
        LOG_ERROR() << "ShareYourFiles: failed changing the network entry";
        return false;
    }

    // Save the current operational mode
    Enums::OperationalMode mode = m_localInstance->data()->mode();

    // Stop the running protocols and go offline
    m_currentNetworkEntry = NetworkEntriesList::InvalidEntry;
    if (m_syfdInstance) {
        // Disconnect the signal to avoid propagating the mode change when the
        // protocol is stopped
        disconnect(m_syfdInstance, &SyfdProtocol::modeChanged, Q_NULLPTR,
                   Q_NULLPTR);
        bool result = QMetaObject::invokeMethod(m_syfdInstance, "stop",
                                                Qt::BlockingQueuedConnection);
        LOG_ASSERT_X(result, "Failed invoking SyfdProtocol::stop()");
        m_syfdInstance->deleteLater();
        m_syfdInstance.clear();
    }
    m_localInstance->data()->updateLocalAddress(0);

    // Restart them associated to the new entry
    m_currentNetworkEntry = entry;
    m_localInstance->data()->updateLocalAddress(m_currentNetworkEntry.second);
    if (!initSYFDProtocol(mode)) {
        LOG_ERROR() << "ShareYourFiles: failed changing the network entry";
        // In case of error, force an entry update
        m_networkEntries->updateEntries();
        return false;
    }

    emit networkEntryChanged(m_currentNetworkEntry);
    return true;
}

///
/// The function attempts to acquire the global lock used to guarantee that only
/// one instance of ShareYourFiles is running at any given time.
/// In case an error occurs, false is returned and the error message is set to
/// an appropriate value.
///
bool ShareYourFiles::initLock(const QString &confPath)
{
    // Try to make the directory where the configuration files are stored
    if (!QDir(confPath).mkpath(".")) {
        LOG_ERROR()
            << "ShareYourFiles: impossible to create the configuration path"
            << confPath;
        m_errorMessage = tr("Failed creating the directory where the "
                            "configuration files will be stored.");
        return false;
    }

    // Try to acquire the lock
    m_locker.reset(new QLockFile(confPath + "/" + QString(TARGET) + ".lock"));
    m_locker->setStaleLockTime(0);
    if (!m_locker->tryLock()) {
        // In case of error, print it
        LOG_ERROR() << "ShareYourFiles: impossible to acquire the global lock";
        m_errorMessage =
            tr("Another instance of " TARGET " is already running.");
        return false;
    }

    return true;
}

///
/// The function attempts to initialize the SYFP protocol server, the one used
/// to receive the list of files to be shared from the SYFPicker utility.
/// In case an error occurs, false is returned and the error message is set to
/// an appropriate value.
///
bool ShareYourFiles::initSYFPProtocol()
{
    // Initialize the SYFP protocol instance
    m_syfpInstance = new SyfpProtocolServer();

    // Set the server name and start it
    QString serverName = "SYFPickerProtocol";
    bool result = m_syfpInstance->start(serverName);

    // In case an error occurred, print it
    if (!result) {
        LOG_ERROR() << "ShareYourFiles: failed starting the SYFP protocol";
        m_errorMessage = tr("Failed starting the SYFP protocol.");
        return false;
    }

    // Otherwise move the server to its thread
    m_syfpInstance->moveToThread(ThreadPool::syfpThread());
    return true;
}

///
/// The function initializes the object representing the list of network entries
/// (i.e. pairs network interface and IPv4 address) that are valid and can be
/// used by the other network protocols.
///
bool ShareYourFiles::initNetworkEntries()
{
    m_networkEntries = new NetworkEntriesList();

    // In case no valid entries found, print the error
    if (m_networkEntries->empty()) {
        m_currentNetworkEntry = NetworkEntriesList::InvalidEntry;
        LOG_ERROR() << "ShareYourFiles: no valid network entry found";
        m_errorMessage = tr("No valid network interfaces detected.");
        return false;
    }

    // Set the current network entry as the first of the list
    m_currentNetworkEntry = m_networkEntries->entries().at(0);
    LOG_INFO() << "ShareYourFiles: selected network entry"
               << qUtf8Printable(
                      NetworkEntriesList::toString(m_currentNetworkEntry));

    // Connect the slot executed when some network entry changes
    connect(m_networkEntries, &NetworkEntriesList::networkInterfacesChanged,
            this, &ShareYourFiles::networkEntriesListUpdated);
    return true;
}

///
/// The function initializes the objects representing both the local user
/// instance and the list of peers.
///
void ShareYourFiles::initUserInstances(const QString &confPath,
                                       const QString &dataPath)
{
    LOG_INFO() << "ShareYourFiles: user instances initialization...";

    m_localInstance =
        new LocalInstance(confPath, dataPath, m_currentNetworkEntry.second);
    m_peersList = new PeersList(confPath, m_localInstance->data());

    // Function executed when the names of the local user are changed
    // to check if some peer advertises the same names
    connect(m_localInstance->data(), &LocalUser::namesChanged, this, [this]() {
        UserInfo info = m_localInstance->data()->info();
        m_peersList->checkDuplicatedNames(info.firstName(), info.lastName());
    });

    // Connect the slot to update the cached datagram representing the local
    // user (the method is executed in the destination thread)
    connect(m_localInstance->data(), &LocalUser::updated, this, [this]() {
        // If the SYFD Protocol is not running, just return
        if (m_syfdInstance.isNull()) {
            return;
        }

        SyfdDatagram datagram(m_localInstance->data()->info());
        bool result = QMetaObject::invokeMethod(
            m_syfdInstance, "updateDatagram", Qt::QueuedConnection,
            Q_ARG(SyfdDatagram, datagram));

        LOG_ASSERT_X(
            result,
            "ShareYourfiles: failed invoking SyfdProtocol::updateDatagram()");
    });

    LOG_INFO() << "ShareYourFiles: user instances initialization completed";
}

///
/// The function attempts to initialize the SYFD protocol instance, the one used
/// to advertize the local user and to receive the information about the other
/// peers.
/// In case an error occurs, false is returned and the error message is set to
/// an appropriate value.
///
bool ShareYourFiles::initSYFDProtocol(Enums::OperationalMode mode)
{
    // Initialize the SYFD protocol instance
    m_syfdInstance = new SyfdProtocol(m_currentNetworkEntry);

    // In case an error occurred, print it
    if (!m_syfdInstance->valid()) {
        m_syfdInstance->deleteLater();
        LOG_ERROR() << "ShareYourFiles: failed starting the SYFD protocol";
        m_errorMessage = tr("Failed starting the SYFP protocol");
        return false;
    }

    // Connect the slot to update the peer list when a datagram is received
    connect(m_syfdInstance, &SyfdProtocol::datagramReceived, m_peersList,
            &PeersList::update);

    // Connect the slots to maintain the operational mode sync'ed
    connect(m_syfdInstance, &SyfdProtocol::modeChanged, m_localInstance->data(),
            &LocalUser::setMode);
    connect(m_localInstance->data(), &LocalUser::modeChanged, m_syfdInstance,
            [this](Enums::OperationalMode mode) {
                m_syfdInstance->setMode(mode, false);
            });

    // Connect the slot to force the network entries list update when an
    // error occurs while sending datagrams
    QObject::connect(m_syfdInstance, &SyfdProtocol::error, m_networkEntries,
                     &NetworkEntriesList::updateEntries);

    // Start the SYFD protocol instance and move it to its thread
    m_syfdInstance->start(mode, SyfdDatagram(m_localInstance->data()->info()));
    m_syfdInstance->moveToThread(ThreadPool::syfdThread());
    return true;
}

///
/// The function, executed when a change is detected in the list of available
/// network entries, tries to adopt corrective actions in order to let the
/// network protocols continue correctly. In particular, in case the entry
/// currently in use is no more available, the SYFD protocol is stopped and, if
/// possible, started again with another network entry; the local addresses
/// cached in the LocalUser instance are also updated.
///
void ShareYourFiles::networkEntriesListUpdated()
{
    // Get the new list of entries
    QVector<NetworkEntriesList::Entry> entries = m_networkEntries->entries();
    // If the currently used entry is still valid, just return
    if (entries.contains(m_currentNetworkEntry)) {
        return;
    }

    if (m_currentNetworkEntry != NetworkEntriesList::InvalidEntry) {
        LOG_ERROR() << "ShareYourFiles:"
                    << qUtf8Printable(
                           NetworkEntriesList::toString(m_currentNetworkEntry))
                    << "no more available";
    }

    // Stop the running protocols and go offline
    m_currentNetworkEntry = NetworkEntriesList::InvalidEntry;
    if (m_syfdInstance) {
        bool result = QMetaObject::invokeMethod(m_syfdInstance, "stop",
                                                Qt::BlockingQueuedConnection);
        LOG_ASSERT_X(result, "Failed invoking SyfdProtocol::stop()");
        m_syfdInstance->deleteLater();
        m_syfdInstance.clear();
    }
    m_localInstance->data()->updateLocalAddress(0);

    // Restart the protocol with the new interface
    if (!entries.isEmpty()) {
        m_currentNetworkEntry = entries[0];
        m_localInstance->data()->updateLocalAddress(
            m_currentNetworkEntry.second);

        if (!initSYFDProtocol(Enums::OperationalMode::Offline)) {
            // In case of error, just force another update
            m_networkEntries->updateEntries();
            return;
        }

        LOG_WARNING()
            << "ShareYourFiles: network entry changed automatically to"
            << qUtf8Printable(
                   NetworkEntriesList::toString(m_currentNetworkEntry));
    }

    emit networkEntryChanged(m_currentNetworkEntry);
}
