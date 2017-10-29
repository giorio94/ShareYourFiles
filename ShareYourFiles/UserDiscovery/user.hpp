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

#ifndef USER_HPP
#define USER_HPP

#include "Common/common.hpp"
#include "usericon.hpp"
#include "userinfo.hpp"

#include <QObject>
#include <QPointer>

class QJsonObject;

class SyfdDatagram;
class SyfftProtocolReceiver;
class SyfftProtocolSender;
class SyfftProtocolServer;
class SyfitProtocolServer;
class SyfitProtocolClient;

///
/// \brief The User class represents an abstract user of Share Your Files.
///
/// It contains the main information related to the user (the ones that can be
/// learned through a SyfdDatagram) represented by the UserInfo subclass,
/// stores some user preferences and mainly it acts as a proxy for data and
/// icon requests.
///
class User : public QObject
{
    Q_OBJECT

public:
    /// \brief A special string used when the user has no names set.
    static const QString NO_NAME;

    /// \brief The identifier representing an anonymous user.
    static const QString ANONYMOUS_UUID;

    /// \brief The UserInfo object representing an anonymous user.
    static const UserInfo ANONYMOUS_USERINFO;

    ///
    /// \brief Deleted constructor since the class is not instantiable.
    ///
    explicit User() = delete;

    ///
    /// \brief Deleted copy constructor since the class is not instantiable.
    /// \param other the instance to be copied.
    ///
    User(const User &other) = delete;

    ///
    /// \brief Deleted assignment operator since the class is not instantiable.
    /// \param other the instance to be copied.
    ///
    User &operator=(const User &other) = delete;

    ///
    /// \brief Virtual destructor.
    ///
    virtual ~User() = default;

    ///
    /// \brief Returns whether the User instance is valid or not.
    ///
    /// In case of invalid instance, that is when the initialization
    /// failed, the other fields are undefined.
    ///
    bool valid() const { return m_valid; }

    ///
    /// \brief Returns whether the instance has been modified and should be
    /// saved.
    ///
    bool toBeSaved() const { return m_toBeSaved; }

    ///
    /// \brief Returns a reference to the information associated with the user.
    ///
    const UserInfo &info() const { return *m_info; }

    ///
    /// \brief Returns whether the instance represents the local user or a peer.
    ///
    bool localUser() const { return m_age == User::Age::AgeInfinity; }

    ///
    /// \brief Saves the main information to the JSON object.
    /// \param json the object where data is added.
    ///
    void save(QJsonObject &json) const;

    ///
    /// \brief Sets the reception preferences for the current user.
    /// \param preferences the instance containing the values to be set.
    ///
    virtual void
    setReceptionPreferences(const ReceptionPreferences &preferences) = 0;

signals:
    /// \brief Signal emitted when some information related to the user is
    /// updated.
    void updated();

    /// \brief Signal emitted when the icon associated to the user is changed.
    void updatedIcon();

protected:
    ///
    /// \brief Common initialization to create a user instance.
    /// \param confPath the base path where configuration files are stored.
    /// \param parent the parent of the current object.
    ///
    explicit User(const QString &confPath, QObject *parent = Q_NULLPTR);

    ///
    /// \brief Common initialization to create a user instance from a JSON
    /// object.
    /// \param confPath the base path where configuration files are stored.
    /// \param json the JSON object containing information about the user.
    /// \param parent the parent of the current object.
    ///
    explicit User(const QString &confPath, const QJsonObject &json,
                  QObject *parent = Q_NULLPTR);

protected:
    ///
    /// \brief The Age enum specifies special values for the age field.
    ///
    enum Age {
        /// \brief Maximum age (the instance is to be considered expired).
        AgeMax = 4,
        /// \brief Value representing an expired instance.
        AgeUnconfirmed = 254,
        /// \brief Value representing the local user instance.
        AgeInfinity = 255,
    };

    /// \brief Specifies whether the User instance is valid or not.
    bool m_valid;

    /// \brief The age associated to the instance.
    quint8 m_age;
    /// \brief Flag set when the instance needs to be saved.
    mutable bool m_toBeSaved;

    /// \brief The base path where configuration files are stored.
    QString m_confPath;

    /// \brief Contains all the main information representing the user.
    QScopedPointer<UserInfo> m_info;
};

///
/// \brief The LocalUser class represents the local user of Share Your Files.
///
/// It extends the User base class by adding the methods necessary to modify and
/// customize the instance (e.g. change the names or the icon) and includes
/// the servers in charge of answering for file and icon transfer requests.
///
/// All the public members are thread-safe.
///
class LocalUser : public User
{
    Q_OBJECT

public:
    ///
    /// \brief A new instance representing the local user is created.
    /// \param confPath the base path where configuration files are stored.
    /// \param dataPath the default path where received files are stored.
    /// \param ipv4Address the address chosen to be used.
    /// \param parent the parent of the current object.
    ///
    explicit LocalUser(const QString &confPath, const QString &dataPath,
                       quint32 ipv4Address, QObject *parent = Q_NULLPTR);

    ///
    /// \brief A new instance representing a local user is created from a JSON
    /// object.
    /// \param confPath the base path where configuration files are stored.
    /// \param dataPath the default path where received files are stored.
    /// \param json the JSON object containing information about the user.
    /// \param ipv4Address the address chosen to be used.
    /// \param parent the parent of the current object.
    ///
    explicit LocalUser(const QString &confPath, const QString &dataPath,
                       const QJsonObject &json, quint32 ipv4Address,
                       QObject *parent = Q_NULLPTR);

    ///
    /// \brief Destroys the current instance.
    ///
    ~LocalUser();

    ///
    /// \brief Generates a new UUID.
    /// \param usedUuids the list of UUIDs used by the peers.
    ///
    void resetUuid(const QList<QString> &usedUuids);

    ///
    /// \brief Sets the first and the last names of the user.
    /// \param first first name.
    /// \param last last name.
    ///
    void setNames(const QString &first, const QString &last);

    ///
    /// \brief Sets the icon representing the user.
    /// \param icon the chosen image (or a null one to delete the current icon).
    /// \return true in case of success and false of failure.
    ///
    bool setIcon(const QImage &icon);

    ///
    /// \brief Sets the reception preferences for the current user.
    /// \param preferences the instance containing the values to be set.
    ///
    void
    setReceptionPreferences(const ReceptionPreferences &preferences) override;

    ///
    /// \brief Sets the mode in which the protocols operate.
    /// \param mode the mode the protocols have to switch to.
    /// \return a value indicating whether the outcome of the operation.
    ///
    bool setMode(Enums::OperationalMode mode);

    ///
    /// \brief Returns the mode the protocols are currently operating in.
    ///
    Enums::OperationalMode mode() const { return m_mode; }

    ///
    /// \brief Updates the local address used by the protocols.
    /// \param ipv4Address the address chosen to be used.
    ///
    void updateLocalAddress(quint32 ipv4Address);

signals:
    ///
    /// \brief Signal emitted when the names of the user changes.
    ///
    void namesChanged();

    ///
    /// \brief Signal emitted when the operational mode changes.
    /// \param mode the entered mode.
    ///
    void modeChanged(Enums::OperationalMode mode);

    ///
    /// \brief Signal emitted when a new connection is attempted by a peer.
    /// \param receiver the receiver instance associated with the connection.
    ///
    void connectionRequested(QSharedPointer<SyfftProtocolReceiver> receiver);

private:
    ///
    /// \brief Starts the server in charge of handling files transfer requests.
    ///
    void startSyfftProtocolServer();

    ///
    /// \brief Stops the server in charge of handling files transfer requests.
    ///
    void stopSyfftProtocolServer();

    ///
    /// \brief Starts the server in charge of handling icon requests.
    /// \param data icon data to be transfered.
    ///
    void startSyfitProtocolServer(const QByteArray &data);

    ///
    /// \brief Stops the server in charge of handling icon requests.
    ///
    void stopSyfitProtocolServer();

    /// \brief The server used to answer for files transfer requests.
    QPointer<SyfftProtocolServer> m_syfftServer;

    /// \brief The server used to answer for icon requests.
    QPointer<SyfitProtocolServer> m_syfitServer;

    QString m_dataPath;            ///< \brief The default data path.
    Enums::OperationalMode m_mode; ///< \brief The current operation mode.
};


///
/// \brief The PeerUser class represents a peer user of Share Your Files.
///
/// It extends the User base class by adding the methods necessary to update
/// the instance when a datagram is received, to increment the age and includes
/// the clients in charge of requesting file and icon transfers.
///
/// All the public members are thread-safe.
///
class PeerUser : public User
{
    Q_OBJECT

public:
    ///
    /// \brief A new instance representing a peer is created from a
    /// SyfdDatagram.
    /// \param confPath the base path where configuration files are stored.
    /// \param datagram the datagram representing the user to be created.
    /// \param localUuid the identifier of the local user.
    /// \param parent the parent of the current object.
    ///
    explicit PeerUser(const QString &confPath, const SyfdDatagram &datagram,
                      const QString &localUuid, QObject *parent = Q_NULLPTR);

    ///
    /// \brief A new instance representing a peer is created from a JSON object.
    /// \param confPath the base path where configuration files are stored.
    /// \param json the JSON object containing information about the user.
    /// \param localUuid the identifier of the local user.
    /// \param parent the parent of the current object.
    ///
    explicit PeerUser(const QString &confPath, const QJsonObject &json,
                      const QString &localUuid, QObject *parent = Q_NULLPTR);

    ///
    /// \brief Destroys the current instance.
    ///
    ~PeerUser();

    ///
    /// \brief Increments the age of the current instance.
    /// \return true in case of expiration and false otherwise.
    ///
    bool incrementAge();

    ///
    /// \brief Sets the current instance as unconfirmed.
    /// \return true in case of success and false if no change is made.
    ///
    bool setUnconfirmed();

    ///
    /// \brief Updates the information stored according to the datagram.
    /// \param datagram the datagram containing the information received.
    /// \return true in case some information is updated and false otherwise.
    ///
    bool update(const SyfdDatagram &datagram);

    ///
    /// \brief Returns whether the instance is related to an expired user or
    /// not.
    ///
    bool unconfirmed() const { return m_age == User::Age::AgeUnconfirmed; }
    ///
    /// \brief Sets the reception preferences for the current user.
    /// \param preferences the instance containing the values to be set.
    ///
    void
    setReceptionPreferences(const ReceptionPreferences &preferences) override;

    ///
    /// \brief Updates the local UUID cached in the instance.
    /// \param localUuid the new local UUID.
    ///
    void updateLocalUuid(const QString &localUuid) { m_localUuid = localUuid; }

    ///
    /// \brief Returns a pointer to a new instance used to send files to the
    /// peer.
    /// \param anonymous specifies whether the local UUID is advertised or not.
    /// \return a new SYFFT Protocol Sender instance.
    ///
    QSharedPointer<SyfftProtocolSender> newSyfftInstance(bool anonymous) const;

private:
    ///
    /// \brief Manages the update of icon information.
    /// \param iconSet flag indicating if the icon is set or not in the
    /// datagram.
    /// \param hash the SHA-1 hash representing the icon.
    ///
    void updatePeerIcon(bool iconSet, const QByteArray &hash);

    ///
    /// \brief Starts or updates the client in charge of requesting an icon.
    /// \param hash the SHA-1 hash representing the icon.
    ///
    void startSyfitProtocolClient(const QByteArray &hash);

    ///
    /// \brief Stops the client in charge of requesting an icon.
    ///
    void stopSyfitProtocolClient();

    /// \brief The server used for icon requests.
    QPointer<SyfitProtocolClient> m_syfitClient;

    /// \brief The identifier of the local user.
    QString m_localUuid;
};

#endif // USER_HPP
