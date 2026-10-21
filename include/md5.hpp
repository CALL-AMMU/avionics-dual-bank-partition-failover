#ifndef MD5_HPP
#define MD5_HPP

#include <string>
#include <cstdint>
#include <vector>

namespace avionics {

/**
 * @brief Standalone RFC 1321 compliant MD5 cryptographic hashing utility.
 * Used for dual-bank partition manifest and firmware integrity audits.
 */
class MD5 {
public:
    MD5();
    void update(const uint8_t* data, size_t length);
    void update(const std::string& str);
    std::string finalize();
    static std::string hash(const std::string& data);
    static std::string hash(const uint8_t* data, size_t length);

private:
    void transform(const uint8_t block[64]);
    uint32_t state_[4];
    uint32_t count_[2];
    uint8_t buffer_[64];
    bool finalized_;
};

} // namespace avionics

#endif // MD5_HPP
