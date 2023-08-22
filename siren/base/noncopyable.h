#pragma once

namespace siren {
class noncopyable {
   public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
    
   protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
}  // namespace siren