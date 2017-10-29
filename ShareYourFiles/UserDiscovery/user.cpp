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

#include "user.hpp"
#include "Common/common.hpp"
#include "Common/threadpool.hpp"
#include "FileTransfer/syfftprotocolreceiver.hpp"
#include "FileTransfer/syfftprotocolsender.hpp"
#include "FileTransfer/syfftprotocolserver.hpp"
#include "syfddatagram.hpp"
#include "syfitprotocol.hpp"

#include <Logger.h>

#include <QImage>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QUuid>

// Static variables definition
const QString User::NO_NAME(QObject::tr("*** NO NAME ***"));
const QString User::ANONYMOUS_UUID(QUuid().toString());
const UserInfo User::ANONYMOUS_USERINFO(UserInfo(User::ANONYMOUS_UUID,
                                                 QObject::tr("Anonymous"), "",
                                                 0, 0, 0, UserIcon(),
                                                 ReceptionPreferences()));

///
/// \brief Returns the username of the user, as stored in environment variables.
/// \return the obtained name.
///
static QString getUserName()
{
    QString name = qgetenv("USER");
    if (name.isEmpty()) {
        name = qgetenv("USERNAME");
    }
    if (name.isEmpty()) {
        name = User::NO_NAME;
    }

    return name.left(SyfdDatagram::STRING_LEN);
}

///
/// \brief Read the icon information from JSON and then try reading the icon
/// itself.
/// \param confPath the base path where configuration files are stored.
/// \param uuid the identifier of the user.
/// \param json the JSON object containing information about the user.
/// \return the instance representing the icon or an invalid instance.
///
static UserIcon iconFromJson(const QString &confPath, const QString &uuid,
                             const QJsonObject &json)
{
    // Check if the icon hash is present
    if (!json.contains("IconHash")) {
        LOG_WARNING() << "User: icon set but missing or wrong hash - UUID:"
                      << qUtf8Printable(uuid);
        return UserIcon();
    } else {

        // Read the hash from the JSON object
        QString strHash = json["IconHash"].toString("");
        QRegularExpression hexMatcher(
            "^[0-9A-F]*$", QRegularExpression::CaseInsensitiveOption);

        // Verify if the string read is a valid hash
        if (strHash.length() != SyfdDatagram::HASH_LEN * 2 ||
            !hexMatcher.match(strHash).hasMatch()) {
            LOG_WARNING() << "User: icon set but missing or wrong hash - UUID:"
                          << qUtf8Printable(uuid);
            return UserIcon();
        }

        // Extract the hash from the string
        QByteArray iconHash =
            QByteArray::fromHex(QByteArray(strHash.toLocal8Bit()));

        // Try reading the icon and then return the obtained structure
        return UserIcon(confPath, uuid, iconHash);
    }
}


/******************************************************************************/


///
/// The function stores the persistent information (UUID, names,
/// icon info, ...) contained in the current instance to the
/// JSON object, in order to allow them to be saved for later retrieval.
///
/// \see LocalUser(const QString&, const QJsonObject&, quint32, QObject*)
/// \see PeerUser(const QString&, const QJsonObject&, QObject*)
///
void User::save(QJsonObject &json) const
{
    if (!m_valid) {
        LOG_ERROR() << "User: trying to save an invalid instance";
        return;
    }

    // Are you representing me?
    json["Me"] = localUser();

    // UUID and names
    json["UUID"] = m_info->m_uuid;
    json["First"] = m_info->m_firstName;
    json["Last"] = m_info->m_lastName;

    // Image
    json["Icon"] = m_info->m_icon.set();
    if (m_info->m_icon.set()) {
        json["IconHash"] = m_info->m_icon.hash().toHex().data();
    }

    // Reception preferences
    bool useDefaults = m_info->m_preferences.useDefaults();
    json["RP_UseDefaults"] = useDefaults;

    if (!useDefaults) {
        json["RP_Action"] =
            qVariantFromValue(m_info->m_preferences.action()).toString();
        json["RP_Path"] = m_info->m_preferences.path();
        json["RP_FolderUser"] = m_info->m_preferences.folderUser();
        json["RP_FolderDate"] = m_info->m_preferences.folderDate();
    }

    m_toBeSaved = false;
}

///
/// The instance is initialized by setting few common parameters used by all
/// constructors.
///
User::User(const QString &confPath, QObject *parent)
        : QObject(parent), m_valid(false), m_confPath(confPath)
{
}


///
/// The instance is created by reading the data stored in the JSON object. In
/// case the icon flag is set, it is checked if the cached version is present
/// and in that case it is loaded.
///
/// \see save(QJsonObject&) const
///
User::User(const QString &confPath, const QJsonObject &json, QObject *parent)
        : User(confPath, parent)
{
    m_toBeSaved = false;

    // Check if the object contains the mandatory fields
    if (!json.contains("UUID") || !json.contains("First") ||
        !json.contains("Last")) {
        LOG_WARNING() << "User: the json object does not contain the mandatory"
                         " fields or they are wrong";
        return;
    }

    // Load UUID and names
    QString uuid = json["UUID"].toString();
    QString firstName = json["First"].toString();
    QString lastName = json["Last"].toString();

    // Check if they are valid according to the SyfdDatagram specifications
    if (QUuid(uuid).isNull() || firstName.length() > SyfdDatagram::STRING_LEN ||
        lastName.length() > SyfdDatagram::STRING_LEN) {
        LOG_WARNING() << "User: the json object does not contain the mandatory"
                         " fields or they are wrong";
        return;
    }
    // In case both names are empty, replace them
    if (firstName.isEmpty() && lastName.isEmpty()) {
        firstName = User::NO_NAME;
    }

    // Check if it represents the local user and set the age accordingly
    bool me = json["Me"].toBool(false);
    m_age = (me) ? User::Age::AgeInfinity : User::Age::AgeUnconfirmed;

    // User icon
    UserIcon icon;

    // Check if the icon is set
    if (json["Icon"].toBool(false)) {

        // Get the icon
        icon = iconFromJson(confPath, uuid, json);

        // If the icon was not read correctly, remove the information
        if (!icon.set()) {
            m_toBeSaved = true;
        }
    }

    // Check if the reception preferences should use the default values
    ReceptionPreferences preferences;
    bool useDefaults = json["RP_UseDefaults"].toBool(true) ||
                       !json.contains("RP_Action") || !json.contains("RP_Path");

    // Otherwise read the values from the JSON document
    if (!useDefaults) {
        // Try to read the action to be taken
        ReceptionPreferences::Action action = ReceptionPreferences::Action::Ask;
        QVariant vaction = json["RP_Action"].toVariant();
        if (vaction.canConvert<ReceptionPreferences::Action>()) {
            action = vaction.value<ReceptionPreferences::Action>();
        }

        // Build the object
        preferences = ReceptionPreferences(action, json["RP_Path"].toString(),
                                           json["RP_FolderUser"].toBool(false),
                                           json["RP_FolderDate"].toBool(false));
    }

    // Create a new instance of UserInfo filled with the data
    // obtained from the JSON object
    m_info.reset(
        new UserInfo(uuid, firstName, lastName, 0, 0, 0, icon, preferences));

    m_valid = true;
}


/******************************************************************************/


///
/// The instance is created from default values, in particular the UUID
/// identifying the user is randomly generated, the first name is obtained by
/// the username and the icon is set to be empty.
///
LocalUser::LocalUser(const QString &confPath, const QString &dataPath,
                     quint32 ipv4Address, QObject *parent)
        : User(confPath, parent),
          m_dataPath(dataPath),
          m_mode(Enums::OperationalMode::Offline)
{
    LOG_ASSERT_X(
        ipv4Address != 0,
        "LocalUser: it is necessary to specify a not null IPv4 address");

    m_valid = true;
    m_toBeSaved = true;

    // Generate a new UUID randomly
    QString uuid = QUuid::createUuid().toString();

    // Create a new instance of UserInfo
    m_info.reset(
        new UserInfo(uuid,                     // UUID
                     getUserName(), QString(), // Names
                     ipv4Address, 0, 0,        // Address and unknown ports
                     UserIcon(),               // Not set icon
                     ReceptionPreferences(dataPath))); // Reception preferences

    // Set the invalid data port
    m_info->m_dataPort = SyfftProtocolServer::INVALID_PORT;

    // Set the age to infinity to indicate that this instance represents the
    // local user.
    m_age = User::Age::AgeInfinity;
}

///
/// The instance is created by reading the data stored in the JSON object as
/// done by the User constructor.
///
/// \see User(const QString&, const QJsonObject&, QObject*)
///
LocalUser::LocalUser(const QString &confPath, const QString &dataPath,
                     const QJsonObject &json, quint32 ipv4Address,
                     QObject *parent)
        : User(confPath, json, parent),
          m_dataPath(dataPath),
          m_mode(Enums::OperationalMode::Offline)
{
    // In case of invalid instance return
    if (!m_valid) {
        return;
    }
    m_valid = false;

    // Check if the generated instance represents a local user
    if (!localUser()) {
        LOG_ERROR("LocalUser: expected instance representing a local user "
                  "but got a peer");
        return;
    }

    // Set the addresses
    m_info->m_ipv4Address = ipv4Address;
    m_info->m_dataPort = SyfftProtocolServer::INVALID_PORT;
    m_info->m_iconPort =
        (m_info->m_icon.set()) ? SyfitProtocolServer::INVALID_PORT : 0;

    // If the reception preferences are not correct, set the default values
    if (m_info->m_preferences.useDefaults()) {
        m_info->m_preferences = ReceptionPreferences(dataPath);
        m_toBeSaved = true;
    }

    m_valid = true;
}

///
/// The instance is destroyed by terminating the SYFIT and SYFFT protocol
/// instances (if running) and letting the destructors to complete the job.
///
LocalUser::~LocalUser()
{
    stopSyfftProtocolServer();
    stopSyfitProtocolServer();
}

///
/// Every user of Share Your Files is identified univocally by a UUID, which
/// is generated randomly. This method allows to avoid the necessity of
/// central registration authorities or coordination which is quite unfeasible
/// in a distributed system: while the probability that a UUID will be
/// duplicated is not zero, it is so close to zero as to be negligible.
/// Anyway, in the very unlucky case of duplicated UUID detected, this function
/// can be used to generate a new UUID for the local user.
///
void LocalUser::resetUuid(const QList<QString> &usedUuids)
{
    // Go offline while changing the UUID
    Enums::OperationalMode mode = m_mode;
    setMode(Enums::OperationalMode::Offline);

    // Select a new UUID
    QString prevUuid = m_info->m_uuid;
    m_info->m_uuid = QUuid::createUuid().toString();
    while (usedUuids.contains(m_info->m_uuid)) {
        m_info->m_uuid = QUuid::createUuid().toString();
    }
    m_toBeSaved = true;

    LOG_ERROR() << "LocalUser: uuid reset" << qUtf8Printable(prevUuid) << "->"
                << qUtf8Printable(m_info->m_uuid);

    // Go back to the previous mode
    setMode(mode);

    emit updated();
}

///
/// The function copies the names given as parameter to the
/// user after having trimmed them to the maximum size allowed
/// by the SyfdDatagram specifications. Finally the updated signal
/// is emitted.
///
void LocalUser::setNames(const QString &first, const QString &last)
{
    QString newFirst = first.left(SyfdDatagram::STRING_LEN);
    QString newLast = last.left(SyfdDatagram::STRING_LEN);

    // Check if both are empty
    if (newFirst.isEmpty() && newLast.isEmpty()) {
        return;
    }

    // Check if equal to the previous one
    if (m_info->m_firstName == newFirst && m_info->m_lastName == newLast) {
        return;
    }

    m_info->m_firstName = newFirst;
    m_info->m_lastName = newLast;
    LOG_INFO() << "LocalUser:" << qUtf8Printable(m_info->m_uuid) << "updated,"
               << "first name =" << m_info->m_firstName
               << "- last name =" << m_info->m_lastName;

    m_toBeSaved = true;

    emit namesChanged();
    emit updated();
}

///
/// The function creates a UserIcon instance according to the
/// specified parameter (which in turn saves that image) and than
/// attaches it to the current user. Finally the updated signal
/// is emitted.
///
bool LocalUser::setIcon(const QImage &icon)
{
    // Request for removal
    if (icon.isNull()) {
        // Remove the current icon if present
        if (m_info->m_icon.set()) {
            m_info->m_icon = UserIcon();
            LOG_INFO() << "LocalUser:" << qUtf8Printable(m_info->m_uuid)
                       << "updated, icon removed";

            // Stop the server listening for icon requests
            stopSyfitProtocolServer();

            m_toBeSaved = true;

            emit updated();
            emit updatedIcon();
        }

        return true;
    }

    // Try creating the new instance
    UserIcon newIcon(m_confPath, m_info->m_uuid, icon);
    if (!newIcon.set()) {
        // Something went wrong
        LOG_ERROR() << "LocalUser: failed changing the icon of user"
                    << qUtf8Printable(m_info->m_uuid);
        return false;
    }

    // Read the data to be transferred
    QByteArray data = newIcon.readData();
    if (data.isEmpty()) {
        LOG_ERROR() << "LocalUser: impossible to read icon data just saved";
        return false;
    }

    // Success: replace the old one
    m_info->m_icon = newIcon;
    LOG_INFO() << "LocalUser:" << qUtf8Printable(m_info->m_uuid)
               << "updated, icon changed";

    // Start the server listening for icon requests if mode is online
    if (m_mode == Enums::OperationalMode::Online) {
        startSyfitProtocolServer(data);
    }

    m_toBeSaved = true;

    emit updated();
    emit updatedIcon();
    return true;
}

///
/// The function copies the specified preferences to the UserInfo instance
/// for later retrieval. In case useDefaults is set, a new ReceptionPreferences
/// is created from default values. Finally the updated signal is emitted.
///
void LocalUser::setReceptionPreferences(const ReceptionPreferences &preferences)
{
    // If useDefaults is set, reset the instance to the default values
    if (preferences.useDefaults()) {
        m_info->m_preferences = ReceptionPreferences(m_dataPath);
    }

    // Otherwise copy the instance
    else {
        m_info->m_preferences = preferences;
    }

    m_toBeSaved = true;
    emit updated();
}

///
/// This function allows to switch between offline mode (the SyfdDatagram
/// representing the local user is not advertised and therefore will not
/// be detected by the other users) and online mode (it is advertised).
///
/// In case the chosen mode is equal to the old one, nothing is done.
///
bool LocalUser::setMode(Enums::OperationalMode mode)
{
    // Same mode as before
    if (m_mode == mode)
        return true;

    // New mode: Online
    if (mode == Enums::OperationalMode::Online) {

        // Start the server for files transfer requests
        startSyfftProtocolServer();
        // In case of error, remain in offline mode
        if (m_info->m_dataPort == SyfftProtocolServer::INVALID_PORT) {
            stopSyfftProtocolServer();
            LOG_ERROR() << "LocalUser: aborting mode changing";
            return false;
        }

        // Start the server for icon requests if the icon is set
        if (m_info->m_icon.set()) {

            // Read the icon data to be transmitted
            QByteArray iconData = m_info->m_icon.readData();

            // In case of error remove the information about the icon
            if (iconData.isEmpty()) {
                m_info->m_icon = UserIcon();
                m_toBeSaved = true;
            }
            // Otherwise start the server
            else {
                startSyfitProtocolServer(iconData);
            }
        }
    }

    // New mode: Offline
    else {
        // Stop all protocols running
        stopSyfftProtocolServer();
        stopSyfitProtocolServer();
    }

    m_mode = mode;
    LOG_INFO() << "LocalUser: mode changed to"
               << qUtf8Printable(enum2str(m_mode));
    emit modeChanged(mode);
    emit updated();
    return true;
}

///
/// The function, in case the new address is different from the previous one,
/// updates the cached address after having changed the operational mode to
/// offline; finally, the previous mode is restored if possible.
///
void LocalUser::updateLocalAddress(quint32 ipv4Address)
{
    // Check if the new address is different from the previous one
    if (ipv4Address == m_info->m_ipv4Address) {
        return;
    }

    // Go offline while changing the address
    Enums::OperationalMode mode = m_mode;
    setMode(Enums::OperationalMode::Offline);

    m_info->m_ipv4Address = ipv4Address;

    // If the address is not null, try switching back to the previous mode
    if (ipv4Address != 0) {
        setMode(mode);
    }
}


///
/// The server is started by creating a new instance of the
/// SyfftProtocolServer and starting it: the selected local
/// port is also set to be advertised.
///
void LocalUser::startSyfftProtocolServer()
{
    // Delete the previous server (if any)
    stopSyfftProtocolServer();

    // In case the address is null, set the invalid port and return
    if (m_info->m_ipv4Address == 0) {
        m_info->m_dataPort = SyfftProtocolServer::INVALID_PORT;
        return;
    }

    // Create a new instance of the server
    m_syfftServer = new SyfftProtocolServer(m_info->m_uuid);
    // Retrigger the connectionRequested signal
    connect(m_syfftServer, &SyfftProtocolServer::connectionRequested, this,
            &LocalUser::connectionRequested);

    // Start it
    m_info->m_dataPort = m_syfftServer->start(m_info->m_ipv4Address);
    // Move it to the SYFFT Receiver thread
    m_syfftServer->moveToThread(ThreadPool::syfftReceiverThread());
}

///
/// The server is stopped by deleting the instance of the
/// SyfftProtocolServer and resetting the advertised port.
///
void LocalUser::stopSyfftProtocolServer()
{
    if (!m_syfftServer.isNull()) {
        m_info->m_dataPort = SyfftProtocolServer::INVALID_PORT;
        m_syfftServer->deleteLater();
        m_syfftServer.clear();
    }
}

///
/// The server is started by creating a new instance of the
/// SyfitProtocolServer and starting it: the selected local
/// port is also set to be advertised.
///
void LocalUser::startSyfitProtocolServer(const QByteArray &data)
{
    LOG_ASSERT_X(m_info->m_icon.set(),
                 "LocalUser: trying to start the SyfitProtocol"
                 "server but icon not set");

    // Delete the previous server (if any)
    stopSyfitProtocolServer();

    // In case the address is null, set the invalid port and return
    if (m_info->m_ipv4Address == 0) {
        m_info->m_iconPort = SyfitProtocolServer::INVALID_PORT;
        return;
    }

    // Create a new instance of the server
    m_syfitServer = new SyfitProtocolServer(data);
    // Start it
    m_info->m_iconPort = m_syfitServer->start(m_info->m_ipv4Address);
    // Move it to the SYFD thread
    m_syfitServer->moveToThread(ThreadPool::syfdThread());
}

///
/// The server is stopped by deleting the instance of the
/// SyfitProtocolServer and resetting the advertised port.
///
void LocalUser::stopSyfitProtocolServer()
{
    if (!m_syfitServer.isNull()) {
        m_info->m_iconPort =
            (m_info->m_icon.set()) ? SyfitProtocolServer::INVALID_PORT : 0;
        m_syfitServer->deleteLater();
        m_syfitServer.clear();
    }
}


/******************************************************************************/


///
/// The instance is created by copying the data stored in the datagram. In case
/// the icon flag is set, firstly it is checked if that icon is already cached
/// locally (it can be immediately loaded) and, otherwise, a request for it is
/// sent.
///
PeerUser::PeerUser(const QString &confPath, const SyfdDatagram &datagram,
                   const QString &localUuid, QObject *parent)
        : User(confPath, parent), m_localUuid(localUuid)
{
    m_valid = datagram.valid();
    LOG_ASSERT_X(datagram.valid(), "PeerUser: trying to create a user instance"
                                   " from an invalid SyfdDatagram");
    m_toBeSaved = true;

    // Create a new instance of UserInfo filled with the data obtained from
    // the datagram, with an empty icon and default preferences.
    m_info.reset(new UserInfo(datagram.uuid(), datagram.firstName(),
                              datagram.lastName(), datagram.ipv4Addr(),
                              datagram.dataPort(), datagram.iconPort(),
                              UserIcon(), ReceptionPreferences()));

    // In case both names are empty, replace them
    if (m_info->m_firstName.isEmpty() && m_info->m_lastName.isEmpty()) {
        m_info->m_firstName = User::NO_NAME;
    }

    // Get the icon if necessary
    updatePeerIcon(datagram.flagIcon(), datagram.iconHash());

    // Instance just created
    m_age = 0;
}

///
/// The instance is created by reading the data stored in the JSON object as
/// done by the User constructor. The instance set as expired and it will need
/// to be refreshed by a datagram before being considered confirmed.
///
/// \see User(const QString&, const QJsonObject&, QObject*)
///
PeerUser::PeerUser(const QString &confPath, const QJsonObject &json,
                   const QString &localUuid, QObject *parent)
        : User(confPath, json, parent), m_localUuid(localUuid)
{
    // Check if the generated instance represents a peer
    if (m_valid && localUser()) {
        LOG_ERROR("PeerUser: expected instance representing a peer "
                  "but got a local user");
        m_valid = false;
        return;
    }
}

///
/// The instance is destroyed by terminating the SYFIT protocol instance
/// (if running), and letting the destructors to complete the job.
///
PeerUser::~PeerUser() { stopSyfitProtocolClient(); }

///
/// Every time this function is executed, the age associated to the user
/// is incremented (it is reset to zero when the information is updated
/// through a datagram). In case the age reaches the AgeMax value,
/// the current instance is considered as expired, the updated signal is
/// emitted and true is returned.
///
bool PeerUser::incrementAge()
{
    if (m_age == User::Age::AgeUnconfirmed)
        return false;

    if (++m_age > User::Age::AgeMax) {
        m_age = User::Age::AgeUnconfirmed;
        emit updated();
        return true;
    }

    return false;
}

///
/// The functions allows to set a previously valid instance representing
/// a user as expired, for example when a SyfdDatagram with the quit flag
/// set is received.
///
bool PeerUser::setUnconfirmed()
{
    if (m_age != User::Age::AgeUnconfirmed) {
        m_age = User::Age::AgeUnconfirmed;
        emit updated();
        return true;
    } else {
        return false;
    }
}

///
/// The function analyzes the datagram received as parameter and
/// updates accordingly the information contained in the instance;
/// furthermore, the age is reset to zero. In case some fields are
/// actually updated, or if the instance was previously unconfirmed,
/// the updated signal is emitted and true is returned.
///
/// It is necessary that both the instance to be updated and the
/// datagram are valid and that the UUIDs are equal (guaranteed
/// by assertions).
///
bool PeerUser::update(const SyfdDatagram &datagram)
{
    // Preliminary checks
    LOG_ASSERT_X(m_valid, "PeerUser: trying to update an invalid instance");
    LOG_ASSERT_X(datagram.valid(),
                 "PeerUser: the datagram used for updating is invalid");
    LOG_ASSERT_X(m_info->m_uuid == datagram.uuid(),
                 "PeerUser: trying to update the instance with a datagram "
                 "corresponding to another user");

    bool updatedFlag = (m_age == User::Age::AgeUnconfirmed);
    m_age = 0;

    // Names
    if (m_info->m_firstName != datagram.firstName() ||
        m_info->m_lastName != datagram.lastName()) {

        m_info->m_firstName = datagram.firstName();
        m_info->m_lastName = datagram.lastName();
        LOG_INFO() << "PeerUser:" << qUtf8Printable(m_info->m_uuid)
                   << "updated (names)";

        m_toBeSaved = true;
        updatedFlag = true;
    }

    // Addresses
    if (m_info->m_ipv4Address != datagram.ipv4Addr() ||
        m_info->m_dataPort != datagram.dataPort() ||
        m_info->m_iconPort != datagram.iconPort()) {

        m_info->m_ipv4Address = datagram.ipv4Addr();
        m_info->m_dataPort = datagram.dataPort();
        m_info->m_iconPort = datagram.iconPort();
        LOG_INFO() << "PeerUser:" << qUtf8Printable(m_info->m_uuid)
                   << "updated (addresses)";

        updatedFlag = true;
    }

    // Icon
    updatePeerIcon(datagram.flagIcon(), datagram.iconHash());

    // Emit the signal if updated
    if (updatedFlag) {
        emit updated();
    }
    return updatedFlag;
}

///
/// The function copies the specified preferences to the UserInfo instance
/// for later retrieval and then emits the updated signal.
///
void PeerUser::setReceptionPreferences(const ReceptionPreferences &preferences)
{
    m_info->m_preferences = preferences;
    m_toBeSaved = true;

    emit updated();
}

///
/// The pointer to the instance is returned in order to be able
/// to use it to send files to the peer.
///
QSharedPointer<SyfftProtocolSender>
PeerUser::newSyfftInstance(bool anonymous) const
{
    SyfftProtocolSender::PeerStatus peerMode =
        m_age == User::Age::AgeUnconfirmed
            ? SyfftProtocolSender::PeerStatus::Offline
            : SyfftProtocolSender::PeerStatus::Online;

    // Prepare the SYFFT instance in charge to send files
    SyfftProtocolSender *instance = new SyfftProtocolSender(
        anonymous ? User::ANONYMOUS_UUID : m_localUuid, m_info->m_uuid,
        m_info->m_ipv4Address, m_info->m_dataPort, peerMode);

    // Move it to the sender thread
    instance->moveToThread(ThreadPool::syfftSenderThread());

    // Connect the handler to update the addresses and the mode
    connect(this, &PeerUser::updated, instance, [this, instance]() {
        instance->updatePeerStatus(
            m_age == Age::AgeUnconfirmed
                ? SyfftProtocolSender::PeerStatus::Offline
                : SyfftProtocolSender::PeerStatus::Online);
        instance->updatePeerAddress(m_info->m_ipv4Address, m_info->m_dataPort);
    });

    // Connect the handler to set offline mode when the current object is
    // destroyed
    connect(this, &PeerUser::destroyed, instance, [instance]() {
        instance->updatePeerStatus(SyfftProtocolSender::PeerStatus::Offline);
    });

    // Return a QSharedPointer to guarantee no memory leaks
    return QSharedPointer<SyfftProtocolSender>(instance, &QObject::deleteLater);
}

///
/// The method is in charge, given the data stored in the SyfdDatagram received,
/// to update accordingly the icon information: mainly, in case of a new icon
/// detected, the SyfitProtocolClient is started to perform the actual request.
///
void PeerUser::updatePeerIcon(bool iconSet, const QByteArray &hash)
{
    // Datagram advertising icon set
    if (iconSet) {

        // If the icon chached locally is not correct, start the SYFIT client
        if (!m_info->m_icon.set() || m_info->m_icon.hash() != hash) {
            startSyfitProtocolClient(hash);
        }
    }

    // Datagram advertising icon not set
    else {
        // Stop the SYFIT client if running
        stopSyfitProtocolClient();

        // Delete the cached icon if set
        if (m_info->m_icon.set()) {
            m_info->m_icon = UserIcon();

            LOG_INFO() << "PeerUser:" << qUtf8Printable(m_info->m_uuid)
                       << "updated (icon)";
            m_toBeSaved = true;
            emit updatedIcon();
        }
    }
}

///
/// The method is in charge to create a new instance of the SyfitProtocolClient
/// to request the user icon (if not already available) and to update the icon
/// data used by the client for the request.
///
void PeerUser::startSyfitProtocolClient(const QByteArray &hash)
{
    // If the client is not already running
    if (m_syfitClient.isNull()) {

        // Create a new instance of the client
        m_syfitClient = new SyfitProtocolClient(m_confPath, m_info->m_uuid);
        // Move it to the SYFD thread
        m_syfitClient->moveToThread(ThreadPool::syfdThread());

        // Connect the signal to be executed when the SYFIT protocol client
        // finishes
        connect(m_syfitClient, &SyfitProtocolClient::finished, this,
                [this](UserIcon icon) {

                    // Set the new icon and stop the protocol
                    m_info->m_icon = icon;
                    stopSyfitProtocolClient();

                    // Emit the updated signal
                    LOG_INFO() << "PeerUser:" << qUtf8Printable(m_info->m_uuid)
                               << "updated (icon)";
                    m_toBeSaved = true;
                    emit updatedIcon();
                });
    }

    // Update the fields stored by the client instance and start the protocol
    // if not yet running. The invokeMethod function guarantees that the
    // method is executed in a thread-safe manner (posted to the client's
    // event loop)
    bool result = QMetaObject::invokeMethod(
        m_syfitClient.data(), "updateAndStart",
        Q_ARG(quint32, m_info->m_ipv4Address),
        Q_ARG(quint16, m_info->m_iconPort), Q_ARG(QByteArray, hash));
    LOG_ASSERT_X(result, "PeerUser: failed invoking the updateAndStart method");
}

///
/// The client is stopped by deleting the instance of the SyfitProtocolClient.
///
void PeerUser::stopSyfitProtocolClient()
{
    if (!m_syfitClient.isNull()) {
        m_syfitClient->deleteLater();
        m_syfitClient.clear();
    }
}
