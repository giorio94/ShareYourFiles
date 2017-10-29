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

#ifndef USERS_HPP
#define USERS_HPP

#include "userinfo.hpp"

#include <QHash>
#include <QObject>
#include <QPointer>

class QTimer;

class LocalUser;
class PeerUser;
class SyfdDatagram;
class SyfftProtocolSender;

///
/// \brief The LocalInstance class provides a simple wrapper to the LocalUser
/// instance, managing reading and writing operations from and to the
/// configuration file.
///
class LocalInstance : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a new instance of this class.
    /// \param confPath the base path where configuration files are stored.
    /// \param dataPath the default path where received files are stored.
    /// \param ipv4Address the address chosen to be used as source address.
    ///
    explicit LocalInstance(const QString &confPath, const QString &dataPath,
                           quint32 ipv4Address);

    ///
    /// \brief Returns the pointer to the LocalUser instance.
    ///
    LocalUser *data() { return m_instance.data(); }

    ///
    /// \brief Saves the changes to file and frees the memory used by the
    /// instance.
    ///
    ~LocalInstance();

private:
    QString m_confPath; ///< \brief The base path.

    /// \brief The actual LocalUser instance.
    QScopedPointer<LocalUser> m_instance;

    /// \brief The relative path (with respect to confPath), where the
    /// configuration file is located.
    static const QString JSON_PATH;
};


///
/// \brief The PeersList class provides a wrapper to a list of User instances
/// representing the peers, the other users of this application.
///
/// In particular, the class actually stores a hash map in order to provide fast
/// lookups of instances given their identifier (the UUID).
///
class PeersList : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a new instance of this class.
    /// \param confPath the base path where configuration files are stored.
    /// \param localUser the pointer to the instance representing the local
    /// user.
    ///
    explicit PeersList(const QString &confPath, LocalUser *localUser);

    ///
    /// \brief Saves the changes to file and frees the memory used by the
    /// instance.
    ///
    ~PeersList();

    ///
    /// \brief Updates the peers list given a SyfdDatagram.
    /// \param datagram the datagram received from the network.
    ///
    void update(const SyfdDatagram &datagram);

    ///
    /// \brief Returns the information relative to the specified user.
    /// \param uuid the identifier of the requested user.
    /// \return the requested information or an invalid instance if no user
    /// found.
    ///
    UserInfo peer(const QString &uuid) const;

    ///
    /// \brief Returns the information relative to the specified user.
    /// \param uuid the identifier of the requested user.
    /// \return the requested information or an invalid instance if no user
    /// found.
    ///
    UserInfo activePeer(const QString &uuid) const;

    ///
    /// \brief Returns the list of user information relative to the currently
    /// active peers.
    /// \return the generated hash table.
    ///
    QHash<QString, UserInfo> activePeers() const;

    ///
    /// \brief Sets the reception preferences for the given user.
    /// \param uuid the identifier of the requested user.
    /// \param preferences the instance containing the values to be set.
    ///
    void setReceptionPreferences(const QString &uuid,
                                 const ReceptionPreferences &preferences);

    ///
    /// \brief Resets to default the reception preferences for all users.
    ///
    void resetReceptionPreferences();

    ///
    /// \brief Returns a pointer to a new instance used to send files to the
    /// peer.
    /// \param uuid the identifier of the requested user.
    /// \param anonymous specifies whether the local UUID is advertised or not.
    /// \return a new SYFFT Protocol Sender instance.
    ///
    QSharedPointer<SyfftProtocolSender> newSyfftInstance(const QString &uuid,
                                                         bool anonymous) const;

    ///
    /// \brief Checks whether some user advertises the given names.
    /// \param firstName the user's first name to be checked.
    /// \param lastName the user's last name to be checked.
    ///
    void checkDuplicatedNames(const QString &firstName,
                              const QString &lastName);

signals:
    /// \brief Signal emitted when a new peer is added to the list.
    /// \param uuid the identifier of the added user.
    void peerAdded(QString uuid);

    /// \brief Signal emitted when a peer is removed from the list.
    /// \param uuid the identifier of the expired user.
    void peerExpired(QString uuid);

    /// \brief Signal emitted when a peer is updated.
    /// \param uuid the identifier of the updated user.
    void peerUpdated(QString uuid);

    /// \brief Signal emitted when a peer with the same names of the local user
    /// is detected.
    /// \param uuid the identifier of the responsible user.
    void duplicatedNameDetected(QString uuid);

private:
    ///
    /// \brief Adds a new user to the list of peers.
    /// \param instance the instance to be added.
    ///
    void addPeerToList(const QSharedPointer<PeerUser> &instance);

    ///
    /// \brief Increments the age of all the instances saved in the list.
    ///
    void incrementAge();

private:
    QString m_confPath; ///< \brief The base path.
    /// \brief The hash table containing the instances.
    QHash<QString, QSharedPointer<PeerUser>> m_instances;

    /// \brief The pointer to the instance representing the local user.s
    QPointer<LocalUser> m_localUser;

    /// \brief The timer used to increment the age of the instances.
    QPointer<QTimer> m_timerAge;

    /// \brief The relative path (with respect to confPath), where the
    /// configuration file is located.
    static const QString JSON_PATH;

    /// \brief Interval between executions of incrementAge().
    static const int AGING_INTERVAL = 5000;
};

#endif // USERS_HPP
