#ifndef DELTASYNC_DIFF_ENGINE_H
#define DELTASYNC_DIFF_ENGINE_H

#include <vector>
#include <cstring>
#include <ios>
#include <sstream>
#include <openssl/sha.h>

class DiffEngine {
public:
     static std::vector<unsigned char> computeDelta(const std::vector<unsigned char>& original,
                                                 const std::vector<unsigned char>& modified,
                                                 size_t minMatchLength = 8);

    static std::vector<unsigned char> applyDelta(const std::vector<unsigned char>& original,
                                         const std::vector<unsigned char>& delta,
                                         bool verifyHash = false);

    static std::basic_string<char> computeHash(const std::vector<unsigned char>& data);

    static std::vector<unsigned char> computeCompressedDelta(const std::vector<unsigned char>& original,
                                                      const std::vector<unsigned char>& modified);
};

#endif //DELTASYNC_DIFF_ENGINE_H
