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

#ifndef COMMON_HPP
#define COMMON_HPP

#include <QMetaEnum>
#include <QMetaType>
#include <QQmlEngine>

///
/// \brief The Constants namespace collects the common constants used throughout
/// SYF.
///
namespace Constants
{
    /// \brief Number of bytes required to store a UUID.
    const int UUID_LEN = 16;

    /// \brief A special value indicating that the interval of time is unknown.
    const quint64 UNKNOWN_INTERVAL = Q_UINT64_C(0xFFFFFFFFFFFFFFFF);
}

///
/// \brief The Enums namespace collects the common enumerations used throughout
/// SYF.
///
namespace Enums
{
    Q_NAMESPACE

    ///
    /// \brief The OperationalMode enum describes the operational mode chosen by
    /// the user.
    ///
    /// In particular, ShareYourFiles can operate in two different ways: online,
    /// where the user is able to both receive from and send to the other peers
    /// the data, or offline, where the other users do not discover the local
    /// one that, therefore, is only able to send files to the others but cannot
    /// receive them.
    ///
    enum class OperationalMode {
        /// \brief The local user is not advertised (all protocols stopped).
        Offline,
        /// \brief The local user is advertised (all protocols running).
        Online
    };

    Q_ENUM_NS(OperationalMode)
}

///
/// \brief The MetaTypeRegistration class provides type registration in the
/// qt system.
///
/// In particular, the class can be used to register the type T in the qt meta
/// type system (e.g. to be able to use it as a signal parameter in case of
/// queued connections). Since there is no standard way to register the type
/// only once, the goal can be reached by instantiating a static variable of
/// this class, whose constructor will perform the actual registration.
/// Be careful of the possibility of static initialization fiasco.
///
/// \tparam T the type to be registered.
///
template <typename T> class MetaTypeRegistration
{
public:
    ///
    /// \brief Registers the type T in the qt meta type system.
    /// \param typeName the name to be associated to the type.
    ///
    explicit MetaTypeRegistration(const char *typeName)
    {
        qRegisterMetaType<T>(typeName);
    }
};

///
/// \brief The MetaTypeRegistration class provides type registration in the
/// QML system.
///
/// In particular, the class can be used to register the type T in the QML
/// system (e.g. to be able to use it as a signal parameter in case of
/// connections from a c++ object to a QML one). Since there is no standard way
/// to register the type only once, the goal can be reached by instantiating a
/// static variable of this class, whose constructor will perform the actual
/// registration.
/// Be careful of the possibility of static initialization fiasco.
///
/// \tparam T the type to be registered.
///
template <typename T> class QmlTypeRegistration
{
public:
    ///
    /// \brief Registers the type T in the QML system.
    ///
    explicit QmlTypeRegistration() { qmlRegisterType<T>(); }
};

///
/// \brief Converts an enumeration value to the corresponding string.
/// \param e the value to be converted.
/// \return the string corresponding to the parameter.
/// \tparam E the enumeration to which the value belongs.
///
template <typename E> QString enum2str(E e)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<E>();
    return metaEnum.valueToKey(static_cast<int>(e));
}

///
/// \brief Converts a size in bytes to a human readable format.
/// \param size the value in bytes to be converted.
/// \return a string in the requested format.
///
QString sizeToHRFormat(quint64 size);

///
/// \brief Converts a transfer speed in bytes/s to a human readable format.
/// \param speed the value in bytes per second to be converted.
/// \return a string in the requested format.
///
QString speedToHRFormat(double speed);

///
/// \brief Converts a time interval to a human readable format.
/// \param ms the number of milliseconds representing the interval.
/// \return a string in the requested format.
///
QString intervalToHRFormat(quint64 ms);

#endif // COMMON_HPP
