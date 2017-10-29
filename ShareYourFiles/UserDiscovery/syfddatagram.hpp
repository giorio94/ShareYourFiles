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

#ifndef SYFDDATAGRAM_HPP
#define SYFDDATAGRAM_HPP

#include <QString>
#include <QUuid>

class UserInfo;

///
/// \brief The SyfdDatagram class represents a datagram used by the SYFD
/// protocol.
///
/// SyfdDatagram (Share Your Files Discovery Datagram) represents both the
/// format of the data that must be transmitted and received by the SyfdProtocol
/// and this adapter class.
///
/// It can be used to convert the raw bytes received from the network to
/// structured data for user creation or update. In the same way, given a
/// LocalUser instance, it is possible to create a SyfdDatagram that will
/// eventually be converted to an array of bytes and sent through the network.
///
/// The data transmitted over the network must fulfill the following format,
/// where the numbers are in little endian order (4 bytes per row):
/**
    \verbatim
    |----------------|----------------|----------------|----------------|
    |       'S'      |       'Y'      |       'F'      |       'D'      |
    |----------------|----------------|----------------|----------------|
    |     Version    |      Flags     |              UUID               |
    |----------------|----------------|----------------|----------------|
    |                           UUID (continues)                        |
    |----------------|----------------|----------------|----------------|
    |                           UUID (continues)                        |
    |----------------|----------------|----------------|----------------|
    |                           UUID (continues)                        |
    |----------------|----------------|----------------|----------------|
    |         UUID (continues)        |           First name (1)        |
    |----------------|----------------|----------------|----------------|
    |           Last name (2)         |              IPv4               |
    |----------------|----------------|----------------|----------------|
    |        IPv4 (continues)         |            Data port            |
    |----------------|----------------|----------------|----------------|
    |            Icon Port            |            Icon hash            |
    |----------------|----------------|----------------|----------------|
    |                        Icon hash (continues)                      |
    |----------------|----------------|----------------|----------------|
    |                        Icon hash (continues)                      |
    |----------------|----------------|----------------|----------------|
    |                        Icon hash (continues)                      |
    |----------------|----------------|----------------|----------------|
    |                        Icon hash (continues)                      |
    |----------------|----------------|----------------|----------------|
    |       Icon hash (continues)     | ------------------------------- |
    |----------------|----------------|----------------|----------------|

    \endverbatim
**/
///
///  * Version: represents the SyfdDatagram version;
///  * Flags: provides some information about the datagram;
///  * UUID: a 128 bits number that uniquely identifies a user on the LAN;
///  * First name: a string of at most 16 characters represented as 4 bytes for
///                the length followed by the data in UTF-16;
///  * Last name: a string of at most 16 characters represented as 4 bytes for
///               the length followed by the data in UTF-16;
///  * IPv4: 32 bits number representing the IPv4 address of the host;
///  * Data port: 16 bits number representing the TCP port on which the host is
///               listening for data requests;
///  * Icon port: 16 bits number representing the TCP port on which the host is
///               listening for icon requests (0 in case no icon set);
///  * Icon hash: 160 bits SHA-1 hash of the file representing the user icon
///               (omitted if no icon set);
///
/// N.B. (1) and (2) not in scale.
///
/// \see SyfdProtocol
///
class SyfdDatagram
{
public:
    ///
    /// \brief Constructs an invalid SyfdDatagram.
    ///
    explicit SyfdDatagram() { m_valid = false; }

    ///
    /// \brief Constructs a SyfdDatagram given a UserInfo object.
    /// \param userInfo the instance containing the data used to build the
    /// datagram.
    ///
    explicit SyfdDatagram(const UserInfo &userInfo);

    ///
    /// \brief Constructs a SyfdDatagram given a byte array.
    /// \param data the array of bytes received from the network.
    ///
    explicit SyfdDatagram(const QByteArray &data);

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    SyfdDatagram(const SyfdDatagram &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    SyfdDatagram &operator=(const SyfdDatagram &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~SyfdDatagram() = default;

    ///
    /// \brief Converts the SyfdDatagram to a byte array.
    /// \return the created byte array.
    ///
    QByteArray toByteArray() const;

    ///
    /// \brief Returns whether the SyfdDatagram is valid or not.
    ///
    /// In case of invalid datagram, that is when generated from either
    /// a LocalUser instance or a byte array with some errors, the content
    /// of all the other fields is undefined and, therefore, it is necessary
    /// to check the validity before using them.
    ///
    bool valid() const { return m_valid; }

    /// \brief Returns whether the flags are valid or not.
    bool flagInvalid() const { return m_flags & Flags::FlagInvalid; }
    /// \brief Returns whether the quit flag is set or not.
    bool flagQuit() const { return m_flags & Flags::FlagQuit; }
    /// \brief Returns whether the icon flag is set or not.
    bool flagIcon() const { return m_flags & Flags::FlagIcon; }

    /// \brief Sets the quit flag to true.
    void setFlagQuit() { m_flags |= SyfdDatagram::Flags::FlagQuit; }
    /// \brief Sets the quit flag to false.
    void clearFlagQuit() { m_flags &= ~SyfdDatagram::Flags::FlagQuit; }

    /// \brief Returns the UUID stored in the SyfdDatagram.
    QString uuid() const { return QUuid::fromRfc4122(m_uuid).toString(); }
    /// \brief Returns the first name stored in the SyfdDatagram.
    QString firstName() const { return m_firstName; }
    /// \brief Returns the last name stored in the SyfdDatagram.
    QString lastName() const { return m_lastName; }

    /// \brief Returns the IPv4 address stored in the SyfdDatagram.
    quint32 ipv4Addr() const { return m_ipv4Addr; }
    /// \brief Returns the TCP port associated to data requests stored in the
    /// SyfdDatagram.
    quint16 dataPort() const { return m_dataPort; }
    /// \brief Returns the TCP port associated to icon requests stored in the
    /// SyfdDatagram.
    quint16 iconPort() const { return m_iconPort; }

    ///
    /// \brief Returns the icon's SHA-1 hash stored in the datagram.
    ///
    /// Since the icon is optional and the hash is required only if it is
    /// present, this field is undefined in case the icon flag is not set.
    ///
    const QByteArray &iconHash() const { return m_iconHash; }

    /// \brief Number of bytes required by the magic string.
    static const int MAGIC_LEN = 4;
    /// \brief Number of bytes required to store a UUID.
    static const int UUID_LEN;
    /// \brief Maximum length of the strings (first and last name).
    static const int STRING_LEN = 16;
    /// \brief Number of bytes required to store a SHA-1 hash.
    static const int HASH_LEN = 20;

    /// \brief Minimum number of bytes needed by a correct datagram.
    static const int MIN_DATAGRAM_SIZE;
    /// \brief Maximum number of bytes needed by a correct datagram.
    static const int MAX_DATAGRAM_SIZE;

private:
    ///
    /// \brief Writes a SyfdDatagram to a QDataStream.
    /// \param stream the stream where data is written to.
    /// \param datagram the datagram to be written.
    /// \return the stream given as parameter to allow concatenations.
    ///
    friend QDataStream &operator<<(QDataStream &stream,
                                   const SyfdDatagram &datagram);
    ///
    /// \brief Reads a SyfdDatagram from a QDataStream.
    /// \param stream the stream from where data is read.
    /// \param datagram the datagram to be filled.
    /// \return the stream given as parameter to allow concatenations.
    ///
    friend QDataStream &operator>>(QDataStream &stream, SyfdDatagram &datagram);

    static const quint8 MAGIC_0 =
        'S'; ///< \brief First character of the magic string
    static const quint8 MAGIC_1 =
        'Y'; ///< \brief Second character of the magic string
    static const quint8 MAGIC_2 =
        'F'; ///< \brief Third character of the magic string
    static const quint8 MAGIC_3 =
        'D'; ///< \brief Forth character of the magic string

    /// \brief The Version enum provides the valid values for the SyfdDatagram
    /// version field.
    enum Version {
        V1_0 = 1 ///< \brief Version 1.0.
    };

    ///
    /// \brief The Flags enum provides the valid values for the SyfdDatagram
    /// flags field.
    ///
    /// The values are set in a way that represent the eight bits available in
    /// the field, in order to allow setting, clearing or querying a specific
    /// flag at a time using the classic bit manipulation operators.
    ///
    enum Flags {
        FlagQuit = 0x1, ///< \brief The user is about to quit (if set).
        FlagIcon = 0x2, ///< \brief The user has an associated icon (if set).

        /// \brief The flags field contains invalid values.
        FlagInvalid = ~(FlagQuit | FlagIcon)
    };

    /// \brief Specifies whether the SyfdDatagram is valid or not.
    bool m_valid;

    ///\brief Magic string to identify the SyfdDatagram
    quint8 m_magic[MAGIC_LEN];
    quint8 m_version; ///< \brief Version of the SyfdDatagram.
    quint8 m_flags;   ///< \brief Flags field.

    QByteArray m_uuid;   ///< \brief UUID field.
    QString m_firstName; ///< \brief First name field.
    QString m_lastName;  ///< \brief Last name field.

    quint32 m_ipv4Addr; ///< \brief IPv4 address field.
    quint16 m_dataPort; ///< \brief TCP port for data requests field.
    quint16 m_iconPort; ///< \brief TCP port for icon requests field.

    QByteArray m_iconHash; ///< \brief Icon's SHA-1 hash field.
};

#endif // SYFDDATAGRAM_HPP
