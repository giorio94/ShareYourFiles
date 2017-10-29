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

#ifndef USERINFO_HPP
#define USERINFO_HPP

#include "receptionpreferences.hpp"
#include "usericon.hpp"

#include <QString>

///
/// \brief The UserInfo class stores the main information representing a User.
///
/// Every information needed to be accessed externally from the User class is
/// contained here, in order to make it simple to return all the fields at once
/// when requested or to store them for later usage: in particular it contains
/// all the fields that can be read from or written to a SyfdDatagram.
/// Furthermore, it contains the information about the UserIcon and the
/// ReceptionPreferences.
///
/// This is a read only class that can only be read from outside the User
/// classes.
///
class UserInfo
{
    friend class User;
    friend class LocalUser;
    friend class PeerUser;

public:
    ///
    /// \brief Generates an invalid instance.
    ///
    explicit UserInfo() : m_valid(false) {}

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    UserInfo(const UserInfo &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    UserInfo &operator=(const UserInfo &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~UserInfo() = default;

    ///
    /// \brief Returns whether the UserInfo instance is valid or not.
    ///
    /// In case of invalid instance, that is when initialized through
    /// the default constructor, the content of the other fields is undefined.
    ///
    bool valid() const { return m_valid; }

    /// \brief Returns the UUID associated to the user.
    QString uuid() const { return m_uuid; }
    /// \brief Returns the first name associated to the user.
    QString firstName() const { return m_firstName; }
    /// \brief Returns the last name associated to the user.
    QString lastName() const { return m_lastName; }
    /// \brief Returns the names associated to the user.
    QString names() const
    {
        return QString(m_firstName + " " + m_lastName).trimmed();
    }

    /// \brief Returns the IPv4 address associated to the user.
    quint32 ipv4Address() const { return m_ipv4Address; }
    /// \brief Returns the TCP port associated to data requests.
    quint16 dataPort() const { return m_dataPort; }
    /// \brief Returns the TCP port associated to icon requests.
    quint16 iconPort() const { return m_iconPort; }

    /// \brief Returns an object representing the icon associated to the user.
    const UserIcon &icon() const { return m_icon; }

    /// \brief Returns an object representing the reception preferences
    /// regarding the current peer.
    const ReceptionPreferences &preferences() const { return m_preferences; }

private:
    ///
    /// \brief Generates a UserInfo instance by creating a copy of the
    /// parameters.
    /// \param uuid user's UUID.
    /// \param firstName user's first name.
    /// \param lastName user's last name.
    /// \param ipv4Address user's IPv4 address.
    /// \param dataPort TCP port used for data transfers.
    /// \param iconPort TCP port used for icon transfers.
    /// \param icon user's icon information.
    /// \param preferences file reception preferences.
    ///
    explicit UserInfo(const QString &uuid, const QString &firstName,
                      const QString &lastName, quint32 ipv4Address,
                      quint16 dataPort, quint16 iconPort, const UserIcon &icon,
                      const ReceptionPreferences &preferences)
            : m_valid(true),
              m_uuid(uuid),
              m_firstName(firstName),
              m_lastName(lastName),
              m_ipv4Address(ipv4Address),
              m_dataPort(dataPort),
              m_iconPort(iconPort),
              m_icon(icon),
              m_preferences(preferences)
    {
    }

    bool m_valid; ///< \brief Specifies whether the instance is valid or not.

    QString m_uuid;      ///< \brief UUID field.
    QString m_firstName; ///< \brief First name field.
    QString m_lastName;  ///< \brief Last name field.

    quint32 m_ipv4Address; ///< \brief IPv4 address field.
    quint16 m_dataPort;    ///< \brief TCP port for data requests field.
    quint16 m_iconPort;    ///< \brief TCP port for icon requests field.

    UserIcon m_icon; ///< \brief Icon information.

    /// \brief Preferences about the reception.
    ReceptionPreferences m_preferences;
};

#endif // USERINFO_HPP
