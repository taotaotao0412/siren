/**
 * @file Endian.h
 * @author kami (taotaotao0412@gmail.com)
 * @brief copy from siren-master
 * @version 0.1
 * @date 2023-07-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <endian.h>
#include <stdint.h>

namespace siren {
namespace net {
namespace sockets {

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
inline uint64_t hostToNetwork64(uint64_t host64) { return htobe64(host64); }

inline uint32_t hostToNetwork32(uint32_t host32) { return htobe32(host32); }

inline uint16_t hostToNetwork16(uint16_t host16) { return htobe16(host16); }

inline uint64_t networkToHost64(uint64_t net64) { return be64toh(net64); }

inline uint32_t networkToHost32(uint32_t net32) { return be32toh(net32); }

inline uint16_t networkToHost16(uint16_t net16) { return be16toh(net16); }

template <typename T>
inline T hostToNetwork(T t) {
    if constexpr (sizeof(T) == sizeof(uint16_t)) {
        return htobe16(t);
    }
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return htobe32(t);
    }
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return htobe64(t);
    }
    return t;
}

template <typename T>
inline T NetworkToHost(T t) {
    if constexpr (sizeof(T) == sizeof(uint16_t)) {
        return be16toh(t);
    }
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return be32toh(t);
    }
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return be64toh(t);
    }
    return t;
}

#pragma GCC diagnostic pop

}  // namespace sockets
}  // namespace net
}  // namespace siren
