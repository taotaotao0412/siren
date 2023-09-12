#pragma once

#include <mutex>

#include "siren/base/noncopyable.h"

namespace siren {
template <typename T>
class Singleton : noncopyable {
   public:
    Singleton() = delete;
    ~Singleton() = delete;

    static T& instance() {
        std::call_once(flag_, init);
        return *value_;
    }

   private:
    static T* value_;
    static std::once_flag flag_;
    static void init() { value_ = new T(); }

    static void destroy() {
        delete value_;
        value_ == nullptr;
    }
};
template <typename T>
std::once_flag Singleton<T>::flag_;

template <typename T>
T* Singleton<T>::value_ = nullptr;

}  // namespace siren