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

#include "syfddatagram.hpp"
#include "Common/common.hpp"
#include "user.hpp"

#include <Logger.h>

#include <QDataStream>

// Static variables definition
const int SyfdDatagram::UUID_LEN = Constants::UUID_LEN;
const int SyfdDatagram::MIN_DATAGRAM_SIZE =
    MAGIC_LEN * sizeof(quint8) + sizeof(quint8) + sizeof(quint8) + UUID_LEN +
    2 * sizeof(quint32) + sizeof(quint32) + sizeof(quint16) + sizeof(quint16);
const int SyfdDatagram::MAX_DATAGRAM_SIZE =
    MIN_DATAGRAM_SIZE + 2 * STRING_LEN * sizeof(quint16) + HASH_LEN;

///
/// The datagram is constructed by making a deep copy of the requested fields
/// obtained from the LocalUser instance.
///
/// During the copy, many checks are performed in order to generate only valid
/// datagrams: in particular it is verified that all fields are filled and
/// contain correct (e.g. of expected length) data. In case one of these checks
/// fails, an invalid datagram is generated and the error is appended to the
/// log.
///
/// \see SyfdDatagram::valid()
///
SyfdDatagram::SyfdDatagram(const UserInfo &userInfo)
        : m_valid(false), m_flags(0)
{
    // UUID
    QUuid tmpUuid(userInfo.uuid());
    if (tmpUuid.isNull()) {
        LOG_WARNING()
            << "SyfdDatagram: trying to create an invalid SyfdDatagram "
               "- UUID ="
            << qUtf8Printable(userInfo.uuid());
        return;
    }
    // Convert the UUID to a standard representation
    m_uuid = tmpUuid.toRfc4122();

    // Names
    m_firstName = userInfo.firstName();
    m_lastName = userInfo.lastName();
    if (m_firstName.length() > SyfdDatagram::STRING_LEN ||
        m_lastName.length() > SyfdDatagram::STRING_LEN) {
        LOG_WARNING()
            << "SyfdDatagram: trying to create an invalid SyfdDatagram "
               "- first name ="
            << userInfo.firstName() << "- last name =" << userInfo.lastName();
        return;
    }

    // IP and ports
    m_ipv4Addr = userInfo.ipv4Address();
    m_dataPort = userInfo.dataPort();
    m_iconPort = userInfo.iconPort();
    if (m_ipv4Addr == 0 || m_dataPort == 0) {
        LOG_WARNING()
            << "SyfdDatagram: trying to create an invalid SyfdDatagram "
               "- IPv4 ="
            << qUtf8Printable(userInfo.ipv4Address())
            << "- TCP port =" << userInfo.dataPort();
        return;
    }

    // Icon info
    if (userInfo.icon().set()) {
        m_flags |= SyfdDatagram::Flags::FlagIcon;
        m_iconHash = userInfo.icon().hash();
        if (m_iconHash.length() != SyfdDatagram::HASH_LEN || m_iconPort == 0) {
            LOG_WARNING() << "SyfdDatagram: trying to create an invalid "
                             "SyfdDatagram (wrong icon information)";
            return;
        }
    } else {
        m_iconPort = 0;
    }

    // If everything is correct, set the datagram as valid
    m_valid = true;
}

///
/// The datagram is constructed by reading the data from the parameter
/// to fill the different fields. The byte array must fulfill the
/// SyfdDatagram format.
///
/// Since the byte array is received from the network, it is necessary to
/// manage problems related to different architectures and operating
/// systems that possibly are used by different people (e.g. byte ordering):
/// for this reason, a data stream is created to automatically provide
/// conversions (obviously the same is applied when byte arrays are generated
/// for output) and then data is read through the operator>>(). In case the
/// byte array cannot be converted correctly, an invalid datagram is
/// generated and the error is appended to the log.
///
/// \see SyfdDatagram::valid()
/// \see SyfdDatagram::toByteArray()
///
SyfdDatagram::SyfdDatagram(const QByteArray &data)
{
    // Build the stream to provide uniformity
    QDataStream stream(data);
    stream.setVersion(QDataStream::Version::Qt_5_0);
    stream.setByteOrder(QDataStream::ByteOrder::LittleEndian);

    // Build the datagram
    stream >> *this;

    if (!m_valid) {
        LOG_WARNING() << "SyfdDatagram: invalid datagram created from the"
                         " byte array";
    }
}

///
/// The byte array is created by concatenating all the fields contained
/// in the datagram according to the SyfdDatagram format.
///
/// Since the byte array is sent over the network, it is necessary to
/// manage problems related to different architectures and operating
/// systems that possibly are used by different people (e.g. byte ordering):
/// for this reason, a data stream is created to automatically provide
/// conversions (obviously the same is applied when byte arrays are read
/// for input) and then data is written through the operator<<(). In case the
/// datagram cannot be converted correctly, error messages are appended to the
/// log.
///
/// \see SyfdDatagram::SyfdDatagram(const QByteArray&)
///
QByteArray SyfdDatagram::toByteArray() const
{
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Version::Qt_5_0);
    stream.setByteOrder(QDataStream::ByteOrder::LittleEndian);

    stream << *this;
    return byteArray;
}

///
/// The datagram is written to the stream according to the SyfdDatagram
/// format specifications, by concatenating the initial header (magic string,
/// version and flags) and all the other fields.
///
/// In case of invalid datagram given as parameter, nothing is done and an
/// error is reported in the log. The same is done if the stream is initially
/// not in a valid state.
///
/// \see operator>>()
///
QDataStream &operator<<(QDataStream &stream, const SyfdDatagram &datagram)
{
    // Check if the stream is initially in a valid status
    if (stream.status() != QDataStream::Status::Ok) {
        LOG_WARNING() << "SyfdDatagram: stream initially in an invalid status";
        return stream;
    }

    // Check if the fields related to the icon are consistent
    bool iconOk =
        (datagram.flagIcon())
            ? (datagram.m_iconHash.length() == SyfdDatagram::HASH_LEN &&
               datagram.iconPort() != 0)
            : (datagram.iconPort() == 0);

    // Check if the datagram is valid, otherwise abort
    if (!datagram.valid() || datagram.flagInvalid() ||
        datagram.m_uuid.length() != SyfdDatagram::UUID_LEN ||
        datagram.m_firstName.length() > SyfdDatagram::STRING_LEN ||
        datagram.m_lastName.length() > SyfdDatagram::STRING_LEN ||
        datagram.m_ipv4Addr == 0 || datagram.m_dataPort == 0 || !iconOk) {

        LOG_ERROR() << "SyfdDatagram: trying to output an invalid datagram";
        return stream;
    }

    // Magic numbers
    stream << SyfdDatagram::MAGIC_0;
    stream << SyfdDatagram::MAGIC_1;
    stream << SyfdDatagram::MAGIC_2;
    stream << SyfdDatagram::MAGIC_3;

    // Version and flags
    stream << static_cast<quint8>(SyfdDatagram::Version::V1_0);
    stream << datagram.m_flags;

    // UUID
    int result = stream.writeRawData(datagram.m_uuid.constData(),
                                     datagram.m_uuid.length());
    if (result != datagram.m_uuid.length()) {
        LOG_WARNING() << "SyfdDatagram: error occurred while writing the"
                         " datagram (UUID)";
        return stream;
    }

    // Names
    stream << datagram.m_firstName;
    stream << datagram.m_lastName;

    // IP and ports
    stream << datagram.m_ipv4Addr;
    stream << datagram.m_dataPort;
    stream << datagram.m_iconPort;

    // Icon hash
    if (datagram.flagIcon()) {
        int result = stream.writeRawData(datagram.m_iconHash.constData(),
                                         datagram.m_iconHash.length());
        if (result != datagram.m_iconHash.length()) {
            LOG_WARNING() << "SyfdDatagram: error occurred while writing the "
                             "datagram (icon hash)";
            return stream;
        }
    }

    // Check if the stream is still valid
    if (stream.status() != QDataStream::Status::Ok) {
        LOG_WARNING() << "SyfdDatagram: error occurred while writing the"
                         " datagram";
    }

    // Return it to allow concatenations
    return stream;
}

///
/// The datagram is read from the stream according to the SyfdDatagram
/// format specifications, by getting the initial header (magic string,
/// version and flags) that is checked and then continuing with the other
/// fields.
///
/// In case of invalid data contained in the stream, an invalid datagram is
/// generated and an error is reported in the log. The same is done if the
/// stream is initially not in a valid state or if its end is reached.
///
/// \see operator<<()
///
QDataStream &operator>>(QDataStream &stream, SyfdDatagram &datagram)
{
    datagram.m_valid = false;

    // Magic numbers
    stream >> datagram.m_magic[0];
    stream >> datagram.m_magic[1];
    stream >> datagram.m_magic[2];
    stream >> datagram.m_magic[3];

    // Version and flags
    stream >> datagram.m_version;
    stream >> datagram.m_flags;

    // Check if some error occurred in this first part
    if (stream.status() != QDataStream::Status::Ok ||
        datagram.m_magic[0] != SyfdDatagram::MAGIC_0 ||
        datagram.m_magic[1] != SyfdDatagram::MAGIC_1 ||
        datagram.m_magic[2] != SyfdDatagram::MAGIC_2 ||
        datagram.m_magic[3] != SyfdDatagram::MAGIC_3 ||
        datagram.m_version != SyfdDatagram::Version::V1_0 ||
        datagram.flagInvalid()) {

        LOG_WARNING() << "SyfdDatagram: invalid format detected (header)";
        return stream;
    }

    // UUID
    datagram.m_uuid.resize(SyfdDatagram::UUID_LEN);
    if (stream.readRawData(datagram.m_uuid.data(), SyfdDatagram::UUID_LEN) !=
            SyfdDatagram::UUID_LEN ||
        QUuid::fromRfc4122(datagram.m_uuid).isNull()) {

        LOG_WARNING() << "SyfdDatagram: invalid format detected (UUID)";
        return stream;
    }

    // Names
    stream >> datagram.m_firstName;
    stream >> datagram.m_lastName;
    if (stream.status() != QDataStream::Status::Ok ||
        datagram.m_firstName.length() > SyfdDatagram::STRING_LEN ||
        datagram.m_lastName.length() > SyfdDatagram::STRING_LEN) {

        LOG_WARNING() << "SyfdDatagram: invalid format detected (names)";
        return stream;
    }

    // IP and ports
    stream >> datagram.m_ipv4Addr;
    stream >> datagram.m_dataPort;
    stream >> datagram.m_iconPort;
    bool iconOk = (datagram.flagIcon()) ? (datagram.iconPort() != 0)
                                        : (datagram.iconPort() == 0);

    if (stream.status() != QDataStream::Status::Ok ||
        datagram.m_ipv4Addr == 0 || datagram.m_dataPort == 0 || !iconOk) {

        LOG_WARNING() << "SyfdDatagram: invalid format detected (addresses)";
        return stream;
    }

    // Icon hash
    if (datagram.flagIcon()) {
        datagram.m_iconHash.resize(SyfdDatagram::HASH_LEN);
        if (stream.readRawData(datagram.m_iconHash.data(),
                               SyfdDatagram::HASH_LEN) !=
            SyfdDatagram::HASH_LEN) {

            LOG_WARNING()
                << "SyfdDatagram: invalid format detected (icon hash)";
            return stream;
        }
    }

    // Check that all data read was correct
    if (stream.status() != QDataStream::Status::Ok) {
        LOG_WARNING() << "SyfdDatagram: invalid format detected";
        return stream;
    }

    // Set the datagram as valid
    datagram.m_valid = true;

    // Return the stream to allow concatenations
    return stream;
}
