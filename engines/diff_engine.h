#ifndef DELTASYNC_DIFF_ENGINE_H
#define DELTASYNC_DIFF_ENGINE_H

#include <vector>
#include <cstring>
#include <ios>
#include <sstream>

class DiffEngine {
public:
    static std::vector<unsigned char> computeDelta(const std::vector<unsigned char>& original,
                                                   const std::vector<unsigned char>& modified);

    static std::vector<unsigned char> applyDelta(const std::vector<unsigned char>& original,
                                                 const std::vector<unsigned char>& delta);

    static std::basic_string<char> computeHash(const std::vector<unsigned char>& data);
};

#endif //DELTASYNC_DIFF_ENGINE_H
