/**
 * @file Buffer.h
 * @author kami (taotaotao0412@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-07-09
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <assert.h>

#include <algorithm>
#include <string>
#include <vector>

namespace siren {
namespace net {

class Buffer {
   public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }
    void swap(Buffer& rhs) {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    [[nodiscard]] size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    [[nodiscard]] size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    [[nodiscard]] size_t prependableBytes() const { return readerIndex_; }

    [[nodiscard]] const char* peek() const { return begin() + readerIndex_; }

    void retrieve(int len) {
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }
    /**
     * @brief reset readerIndex_ and writerIndex_
     */
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    char* beginWrite() { return begin() + writerIndex_; }

    void hasWritten(size_t len) {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void append(const char* /*restrict*/ data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void append(const void* /*restrict*/ data, size_t len) {
        append(static_cast<const char*>(data), len);
    }

    void prepend(const void* data, size_t len) {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {

            buffer_.resize(writerIndex_ + len);
        } else {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    ssize_t readFd(int);

   private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    char* begin() { return buffer_.data(); }

    const char* begin() const { return buffer_.data(); }

    static const char kCRLF[];
};
}  // namespace net

}  // namespace siren
