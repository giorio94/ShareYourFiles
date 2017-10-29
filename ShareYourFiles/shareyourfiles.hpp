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

#ifndef SHAREYOURFILES_HPP
#define SHAREYOURFILES_HPP

#include "Common/common.hpp"
#include "Common/networkentrieslist.hpp"
#include "FileTransfer/syfpprotocol.hpp"
#include "UserDiscovery/users.hpp"

#include <QObject>
#include <QPointer>

class QLockFile;

class LocalUser;
class SyfdProtocol;
class SyfpProtocolServer;

///
/// \brief The ShareYourFiles class provides access to the main data structures
/// used by Share Your Files.
///
/// The class is designed according to the singleton pattern: it is possible to
/// create and destroy the instance through the designed methods, while the
/// access is provided by the instance() function.
///
/// It encapsulates the instances of the objects representing the users of the
/// application and the ones related to the protocols used to advertise the
/// current user over the network and to receive requests from the SYFPicker
/// application. Through the provided functions, it is possible to access to the
/// pointers to such instances.
///
class ShareYourFiles : public QObject
{
    Q_OBJECT
public:
    ///
    /// \brief Creates the instance of ShareYourFiles.
    /// \param confPath the base path where the configuration files are stored.
    /// \param dataPath the base path where the data files are stored.
    /// \return a value indicating whether the operation succeeded or not.
    ///
    static bool createInstance(const QString &confPath,
                               const QString &dataPath);

    ///
    /// \brief Destroys the instance of ShareYourFiles.
    ///
    static void destroyInstance();

    ///
    /// \brief Returns the instance of ShareYourFiles.
    ///
    static ShareYourFiles *instance() { return m_instance; }

public:
    /// \brief Destroys the main data structures used by Share Your Files.
    ~ShareYourFiles();

    /// \brief Returns whether an error occurred during the initialization.
    bool error() const { return m_error; }
    /// \brief Returns a message describing the occurred error.
    QString errorMessage() const { return m_errorMessage; }

    /// \brief Returns the pointer to the LocalUser instance.
    LocalUser *localUser() { return m_localInstance->data(); }
    /// \brief Returns the pointer to the PeersList instance.
    PeersList *peersList() { return m_peersList; }

    /// \brief Returns the pointer to the SyfpProtocolServer instance.
    SyfpProtocolServer *syfpProtocolInstance() { return m_syfpInstance; }

    /// \brief Returns the pointer to the NetworkEntriesList instance.
    NetworkEntriesList *networkEntriesList() { return m_networkEntries; }

    /// \brief Returns the network entry currently in use.
    const NetworkEntriesList::Entry &currentNetworkEntry() const;

    ///
    /// \brief Changes the network entry to be used by the protocols.
    /// \param entry the instance representing the new entry.
    /// \return a value indicating whether the operation succeeded or failed.
    ///
    bool changeNetworkEntry(const NetworkEntriesList::Entry &entry);

signals:
    ///
    /// \brief Signal emitted when the current network entry is changed.
    /// \param entry the instance representing the selected entry.
    ///
    void networkEntryChanged(const NetworkEntriesList::Entry &entry);

private:
    ///
    /// \brief Creates the main data structures used by Share Your Files.
    /// \param confPath the base path where the configuration files are stored.
    /// \param dataPath the base path where the data files are stored.
    /// \param parent the parent of the current object.
    ///
    explicit ShareYourFiles(const QString &confPath, const QString &dataPath,
                            QObject *parent = Q_NULLPTR);

    ///
    /// \brief Tries to acquire the global lock.
    /// \param confPath the base path where the configuration files are stored.
    /// \return true in case of success and false otherwise.
    ///
    bool initLock(const QString &confPath);

    ///
    /// \brief Initializes the SYFP protocol server.
    /// \return true in case of success and false otherwise.
    ///
    bool initSYFPProtocol();

    ///
    /// \brief Initializes the list of network entries.
    /// \return true in case of success and false otherwise.
    ///
    bool initNetworkEntries();

    ///
    /// \brief Initializes the instances representing both the local user and
    /// the peers.
    /// \param confPath the base path where the configuration files are stored.
    /// \param dataPath the base path where the data files are stored.
    ///
    void initUserInstances(const QString &confPath, const QString &dataPath);

    ///
    /// \brief Initializes the SYFD protocol instance.
    /// \brief mode the operational mode the protocol has to start in.
    /// \return true in case of success and false otherwise.
    ///
    bool initSYFDProtocol(Enums::OperationalMode mode);

    ///
    /// \brief Function executed when the network entries list gets updated.
    ///
    void networkEntriesListUpdated();

private:
    /// \brief A value indicating whether an error occurred during the
    /// initialization or not.
    bool m_error;

    /// \brief A textual description of the occurred error (if any).
    QString m_errorMessage;

    /// \brief The object representing the local user.
    QPointer<LocalInstance> m_localInstance;

    /// \brief The object representing the list of peers.
    QPointer<PeersList> m_peersList;

    /// \brief The object representing the SYFP protocol instance.
    QPointer<SyfpProtocolServer> m_syfpInstance;

    /// \brief The object representing the SYFD protocol instance.
    QPointer<SyfdProtocol> m_syfdInstance;

    /// \brief The object representing the available pairs interface - IPv4
    /// address.
    QPointer<NetworkEntriesList> m_networkEntries;
    /// \brief The object representing the currently chosen network entry.
    NetworkEntriesList::Entry m_currentNetworkEntry;

    /// \brief The lock used to allow only one instance of Share Your Files.
    QScopedPointer<QLockFile> m_locker;

private:
    /// \brief The ShareYourFiles instance.
    static ShareYourFiles *m_instance;
};

#endif // SHAREYOURFILES_HPP
