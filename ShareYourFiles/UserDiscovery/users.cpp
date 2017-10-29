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

#include "users.hpp"
#include "syfddatagram.hpp"
#include "user.hpp"

#include <Logger.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

// Static variables definition
const QString LocalInstance::JSON_PATH = "/me.json";
const QString PeersList::JSON_PATH = "/peers.json";


///
/// \brief Reads a JSON document from a file.
/// \param filePath the path of the file to be read.
/// \param error the description of the occurred error (if any).
/// \return the JSON document read or a null one in case of error.
///
static QJsonDocument readFromFile(const QString &filePath, QString &error)
{
    // Check if path exists and create it otherwise
    QDir directory = QFileInfo(filePath).absoluteDir();
    if (!directory.mkpath(".")) {
        LOG_ERROR() << "Users: impossible to create directory"
                    << directory.absolutePath();
        return QJsonDocument();
    }

    // Open the file
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        error = file.errorString();
        return QJsonDocument();
    }

    // Read data from file
    QByteArray data = file.readAll();
    file.close();

    // Convert data to JSON document
    return QJsonDocument::fromJson(data);
}

///
/// \brief Saves a JSON document to a file.
/// \param filePath the path of the file to be written.
/// \param json the JSON document to be saved.
/// \param error the description of the occurred error (if any).
/// \return true in case of success and false in case of failure.
///
static bool saveToFile(const QString &filePath, const QJsonDocument &json,
                       QString &error)
{
    // Check if path exists and create it otherwise
    QDir directory = QFileInfo(filePath).absoluteDir();
    if (!directory.mkpath(".")) {
        LOG_ERROR() << "Users: impossible to create directory"
                    << directory.absolutePath();
        return false;
    }

    // Open the file
    QSaveFile file(filePath);
    if (!file.open(QSaveFile::WriteOnly)) {
        error = file.errorString();
        return false;
    }

    // Write data to the file
    QByteArray data = json.toJson();
    if (file.write(data) != data.length()) {
        error = file.errorString();
        return false;
    }

    // Commit the data
    if (!file.commit()) {
        error = file.errorString();
        return false;
    }

    return true;
}

///
/// \brief Saves a User instance to file.
/// \param filePath the path of the file to be written.
/// \param instance the User instance to be saved.
///
/// The instance is saved only if it is marked to be needed (through the
/// to be saved flag).
///
static void saveToFile(const QString &filePath, const LocalUser &instance)
{
    // Check if data has to be saved
    if (!instance.toBeSaved()) {
        return;
    }

    // Fill the JSON object
    QJsonObject jsonObj;
    instance.save(jsonObj);

    // Save it to file
    QString error;
    if (!saveToFile(filePath, QJsonDocument(jsonObj), error)) {
        LOG_ERROR() << "LocalUser: failed saving information to file"
                    << filePath << "-" << error;
    } else {
        LOG_INFO() << "LocalUser: information correctly saved to file"
                   << filePath;
    }
}

///
/// \brief Saves a list of User instances to file.
/// \param filePath the path of the file to be written.
/// \param instances the instances to be saved.
///
static void saveToFile(QString filePath,
                       const QList<QSharedPointer<PeerUser>> &instances)
{
    // Build an array given the current peers
    QJsonArray jsonArray;
    foreach (QSharedPointer<PeerUser> peer, instances) {
        // Save the user as JSON
        QJsonObject jsonObj;
        peer->save(jsonObj);

        // Add to the array
        jsonArray.append(jsonObj);
    }

    // Save it to file
    QString error;
    if (!saveToFile(filePath, QJsonDocument(jsonArray), error)) {
        LOG_ERROR() << "PeersList: failed saving information to file"
                    << filePath << "-" << error;
    } else {
        LOG_INFO() << "PeersList: information correctly saved to file"
                   << filePath;
    }
}


/******************************************************************************/


///
/// The LocalUser instance is created by initially trying to read if
/// some configuration was saved by previous executions. In this case
/// the file is read, converted to a JSON document and then the User
/// instance is constructed from that data. On the other hand, if no
/// configuration is found, or if it is corrupted, a brand new instance
/// is generated.
///
LocalInstance::LocalInstance(const QString &confPath, const QString &dataPath,
                             quint32 ipv4Address)
        : m_confPath(confPath)
{
    QString path(m_confPath + LocalInstance::JSON_PATH), error;
    LOG_INFO() << "LocalInstance: initialization"
               << qUtf8Printable("(json file: \"" + path + "\")...");

    // Try to read the saved instance
    QJsonDocument json = readFromFile(path, error);
    if (!json.isNull()) {
        // Construct the instance from JSON data
        m_instance.reset(
            new LocalUser(confPath, dataPath, json.object(), ipv4Address));
    } else {
        LOG_WARNING()
            << "LocalInstance: impossible to read information from file:"
            << error;
    }

    // If failed to create a new instance
    if (!m_instance || !m_instance->valid() || !m_instance->localUser()) {
        LOG_INFO()
            << "LocalInstance: creating a new instance from default parameters";
        m_instance.reset(new LocalUser(confPath, dataPath, ipv4Address));
    }

    LOG_ASSERT_X(m_instance->valid(),
                 "LocalUser: generated an invalid instance");
    LOG_INFO() << "LocalInstance - uuid:"
               << qUtf8Printable(m_instance->info().uuid());
    LOG_INFO() << "LocalInstance - first name:"
               << m_instance->info().firstName();
    LOG_INFO() << "LocalInstance - last name:" << m_instance->info().lastName();

    // Save the new instance if necessary
    saveToFile(path, *m_instance);

    // Function executed when some information of the local user is changed
    // to save the changes to file
    connect(m_instance.data(), &LocalUser::updated, this,
            [this, path]() { saveToFile(path, *m_instance); });

    LOG_INFO() << "LocalInstance: initialization completed";
}

///
/// The method saves the changes to file (if necessary) and then destroys
/// the instance (done automatically by the destructor of the shared pointer)
///
LocalInstance::~LocalInstance()
{
    saveToFile(m_confPath + LocalInstance::JSON_PATH, *m_instance);
}


/******************************************************************************/


///
/// The PeersList instance is created by initially trying to read if
/// some configuration was saved from previous executions. In this case
/// the file is read, converted to a JSON document and then the detected
/// users are added to the list (marked as unconfirmed since no datagram
/// as been received yet). In case no configuration is found, an empty list
/// is created.
///
PeersList::PeersList(const QString &confPath, LocalUser *localUser)
        : m_confPath(confPath),
          m_localUser(localUser),
          m_timerAge(new QTimer(this))
{
    QString path(m_confPath + PeersList::JSON_PATH), error;
    LOG_INFO() << "PeersList: initialization"
               << qUtf8Printable("(json file: \"" + path + "\")...");

    // Get the information related to the local user
    UserInfo me = m_localUser->info();

    // Try to read the saved instances
    QJsonDocument json = readFromFile(path, error);
    if (!json.isNull() && json.isArray()) {
        QJsonArray peers = json.array();

        // Insert each instance into the data structure if valid
        foreach (QJsonValue obj, peers) {
            QSharedPointer<PeerUser> peer(
                new PeerUser(confPath, obj.toObject(), me.uuid()));

            if (peer->valid() && peer->info().uuid() != me.uuid()) {
                QString uuid = peer->info().uuid();

                addPeerToList(peer);
                LOG_INFO() << "Peerslist:" << qUtf8Printable(uuid) << "added";
            } else {
                LOG_WARNING() << "Peerslist: invalid record found";
            }
        }
    } else {
        LOG_WARNING() << "Peerslist: impossible to read information from file:"
                      << error;
    }

    // Initialize the timer used to increase the age of the peers
    connect(m_timerAge, &QTimer::timeout, this, [this]() { incrementAge(); });
    m_timerAge->start(AGING_INTERVAL);

    LOG_INFO() << "PeersList: initialization completed";
}

///
/// The function, to be executed every time a new datagram is received from
/// the network, is in charge of keeping the peers list updated.
///
/// The datagram is initially checked to verify if the quit flag is set:
/// in this case the quitting user is marked as expired and the peerExpired()
/// signal is emitted.
///
/// In case the flag is not set, on the other hand, it is verified if the
/// cached list already contains the user: in this case the user instance
/// is updated according to the information contained in the datagram (in
/// case some information is changed the peerUpdated() signal is triggered).
/// Otherwise, a new User instance is created from the datagram and added
/// to the list.
///
/// During the update, some checks are performed to verify if a duplicated UUID
/// is detected (in that case the local UUID is reset) or if some other user
/// advertises the same names of the local one (identification would become
/// much harder) and in that case the duplicatedNamesDetected() signal is
/// emitted.
///
void PeersList::update(const SyfdDatagram &datagram)
{
    LOG_ASSERT_X(datagram.valid(),
                 "PeersList: trying to update with an invalid datagram");

    QString uuid = datagram.uuid();

    // Check if user is quitting
    if (datagram.flagQuit()) {
        // Check if the user is in the list
        if (m_instances.contains(uuid)) {
            // Mark as expired
            if (m_instances.value(uuid)->setUnconfirmed()) {
                LOG_INFO() << "PeersList:" << qUtf8Printable(uuid) << "quitted";
                emit peerExpired(uuid);
            }
        }
        return;
    }

    // Get the instance representing the local user
    UserInfo me = m_localUser->info();

    // Check if the user is already in list
    if (m_instances.contains(uuid)) {
        // Update it
        PeerUser &peer = *m_instances.value(uuid);
        bool wasUnconfirmed = peer.unconfirmed();
        if (peer.update(datagram)) {
            LOG_ASSERT_X(peer.valid(),
                         "PeersList: updated user became invalid");

            // Emit the appropriated signals
            if (wasUnconfirmed) {
                LOG_INFO() << "PeersList:" << qUtf8Printable(uuid)
                           << "refreshed";
                emit peerAdded(uuid);
            } else {
                emit peerUpdated(uuid);
            }

            // Check for duplicated name
            if (me.firstName() == peer.info().firstName() &&
                me.lastName() == peer.info().lastName()) {
                LOG_WARNING() << "PeersList:" << qUtf8Printable(uuid)
                              << "has the same name of the local user";
                emit duplicatedNameDetected(uuid);
            }
        }
        return;
    }

    // Check if the new UUID equals mine
    if (me.uuid() == uuid) {
        LOG_ERROR() << "PeersList: duplicated UUID detected";
        m_localUser->resetUuid(m_instances.keys());

        // Update the cached UUID
        me = m_localUser->info();
        foreach (QSharedPointer<PeerUser> peer, m_instances.values()) {
            peer->updateLocalUuid(me.uuid());
        }

        return;
    }

    // Add the new user to the list
    QSharedPointer<PeerUser> peer(
        new PeerUser(m_confPath, datagram, me.uuid()));
    LOG_ASSERT_X(peer->valid(), "PeersList: created an invalid user");

    addPeerToList(peer);
    LOG_INFO() << "PeersList:" << qUtf8Printable(uuid) << "added";
    emit peerAdded(uuid);

    // Check for duplicated name
    if (me.firstName() == peer->info().firstName() &&
        me.lastName() == peer->info().lastName()) {
        LOG_WARNING() << "PeersList:" << qUtf8Printable(uuid)
                      << "has the same name of the local user";
        emit duplicatedNameDetected(uuid);
    }

    return;
}

///
/// The function searches the peers list for the user identified by the
/// specified UUID. In case it is found, a copy of the UserInfo instance is
/// returned while, if the user is not present, an invalid UserInfo instance is
/// returned.
///
UserInfo PeersList::peer(const QString &uuid) const
{
    if (uuid == User::ANONYMOUS_UUID) {
        return User::ANONYMOUS_USERINFO;
    }

    if (!m_instances.contains(uuid)) {
        return UserInfo();
    }
    return m_instances.value(uuid)->info();
}

///
/// The function searches the peers list for the user identified by the
/// specified UUID. In case it is found, a copy of the UserInfo instance is
/// returned while, if the user is not present or currently not active, an
/// invalid UserInfo instance is returned.
///
UserInfo PeersList::activePeer(const QString &uuid) const
{
    if (uuid == User::ANONYMOUS_UUID) {
        return User::ANONYMOUS_USERINFO;
    }

    if (!m_instances.contains(uuid) || m_instances.value(uuid)->unconfirmed()) {
        return UserInfo();
    }
    return m_instances.value(uuid)->info();
}

///
/// The function builds a new hash table containing a copy of all the UserInfo
/// information relative to the active peers, which is then returned.
///
QHash<QString, UserInfo> PeersList::activePeers() const
{
    QHash<QString, UserInfo> peers;

    // Scan the list of peers and add every valid information to the new table
    foreach (QSharedPointer<PeerUser> peer, m_instances.values()) {
        if (!peer->unconfirmed()) {
            peers.insert(peer->info().uuid(), peer->info());
        }
    }
    return peers;
}

///
/// The function proceeds by setting preferences for the specified user. In
/// case the given UUID does not exist, nothing is performed.
///
void PeersList::setReceptionPreferences(const QString &uuid,
                                        const ReceptionPreferences &preferences)
{
    if (!m_instances.contains(uuid)) {
        return;
    }
    m_instances.value(uuid)->setReceptionPreferences(preferences);
}

///
/// The function proceeds by scanning all the peer list and setting the default
/// preferences for each of them.
///
void PeersList::resetReceptionPreferences()
{
    // Scan the list of peers reset the preferences
    ReceptionPreferences defaultPreferences;
    foreach (QSharedPointer<PeerUser> peer, m_instances.values()) {
        peer->setReceptionPreferences(defaultPreferences);
    }
}

///
/// The function returns a new SYFFT Protocol Sender instance associated with
/// the specified user; if the user is not present or currently not active,
/// Q_NULLPTR is returned.
///
QSharedPointer<SyfftProtocolSender>
PeersList::newSyfftInstance(const QString &uuid, bool anonymous) const
{
    if (!m_instances.contains(uuid) || m_instances.value(uuid)->unconfirmed()) {
        return Q_NULLPTR;
    }
    return m_instances.value(uuid)->newSyfftInstance(anonymous);
}

///
/// Every time this function is executed, the list of peers is scanned
/// and the age of every instance is incremented. In case one of them
/// expires, that is the maximum allowed age is reached, it is marked
/// as unconfirmed and the peerExpired() signal is triggered.
///
void PeersList::incrementAge()
{
    // Increment the age of all instances
    foreach (QSharedPointer<PeerUser> peer, m_instances.values()) {
        if (peer->incrementAge()) {
            QString uuid = peer->info().uuid();
            LOG_INFO() << "PeersList:" << qUtf8Printable(uuid) << "expired";
            emit peerExpired(uuid);
        }
    }
}

///
/// The method scans the list of peers and checks whether someone uses the names
/// given as parameters: in case of correspondence, the duplicatedNameDetected()
/// signal is emitted.
///
void PeersList::checkDuplicatedNames(const QString &firstName,
                                     const QString &lastName)
{
    // Scan all instances
    foreach (QSharedPointer<PeerUser> peer, m_instances.values()) {
        // Skip unconfirmed users
        if (!peer->unconfirmed()) {
            UserInfo info = peer->info();
            if (info.firstName() == firstName && info.lastName() == lastName) {
                // Duplicated name detected
                QString uuid = info.uuid();

                LOG_WARNING() << "PeersList:" << qUtf8Printable(uuid)
                              << "has the same name of the local user";

                emit duplicatedNameDetected(uuid);
                return;
            }
        }
    }
}

///
/// The function inserts a new user to list, after having connected the
/// signal in charge of retriggering the signal emitted in case of icon
/// updated.
///
void PeersList::addPeerToList(const QSharedPointer<PeerUser> &instance)
{
    QString uuid = instance->info().uuid();

    // Connect the signal in case of icon updated
    connect(instance.data(), &User::updatedIcon, this,
            [this, uuid]() { emit peerUpdated(uuid); });

    // Add the new user to the list
    m_instances.insert(uuid, instance);
}

///
/// The method saves the changes the peer list to file and then destroys
/// the instances (done automatically by the destructor of the shared pointers)
///
PeersList::~PeersList()
{
    saveToFile(m_confPath + PeersList::JSON_PATH, m_instances.values());
}
