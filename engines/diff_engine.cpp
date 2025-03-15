#include "diff_engine.h"
#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>

std::vector<unsigned char> DiffEngine::computeDelta(const std::vector<unsigned char>& original,
                                               const std::vector<unsigned char>& modified) {
    std::vector<unsigned char> delta;

    // Простой формат дельты:
    // [offset (4 bytes)][size (4 bytes)][operation (1 byte)][data (if needed)]
    // operation: 0 - копировать из оригинала, 1 - вставить новые данные

    unsigned long long int i = 0;
    while (i < modified.size()) {
        unsigned long long int matchPos = 0;
        unsigned long long int matchLen = 0;

        if (i < modified.size()) {
            for (unsigned long long int j = 0; j < original.size(); ++j) {
                unsigned long long int k = 0;
                while (i + k < modified.size() && j + k < original.size() &&
                       modified[i + k] == original[j + k]) {
                    k++;
                }

                if (k > matchLen) {
                    matchLen = k;
                    matchPos = j;
                }
            }
        }

        if (matchLen > 4) {
            auto offset = static_cast<unsigned int>(matchPos);
            auto size = static_cast<unsigned int>(matchLen);

            delta.push_back(0);
            delta.insert(delta.end(), reinterpret_cast<unsigned char *>(&offset),
                         reinterpret_cast<unsigned char *>(&offset) + sizeof(offset));
            delta.insert(delta.end(), reinterpret_cast<unsigned char *>(&size),
                         reinterpret_cast<unsigned char *>(&size) + sizeof(size));

            i += matchLen;
        } else {
            unsigned long long int insertStart = i;
            while (i < modified.size()) {
                unsigned long long int bestMatch = 0;
                for (unsigned long long int j = 0; j < original.size() && bestMatch < 4; ++j) {
                    unsigned long long int k = 0;
                    while (i + k < modified.size() && j + k < original.size() &&
                           modified[i + k] == original[j + k]) {
                        k++;
                    }
                    bestMatch = std::max(bestMatch, k);
                }

                if (bestMatch >= 4) {
                    break;
                }
                i++;
            }

            unsigned long long int insertLen = i - insertStart;
            if (insertLen > 0) {
                delta.push_back(1);
                auto size = static_cast<unsigned int>(insertLen);
                delta.insert(delta.end(), reinterpret_cast<unsigned char *>(&size),
                             reinterpret_cast<unsigned char *>(&size) + sizeof(size));

                delta.insert(delta.end(), modified.begin() + insertStart,
                             modified.begin() + i);
            }
        }
    }

    return delta;
}

std::vector<unsigned char> DiffEngine::applyDelta(const std::vector<unsigned char>& original,
                                             const std::vector<unsigned char>& delta) {
    std::vector<unsigned char> result;
    unsigned long long int i = 0;

    while (i < delta.size()) {
        unsigned char op = delta[i++];

        if (op == 0) {  // операция копирования
            unsigned int offset, size;
            std::memcpy(&offset, &delta[i], sizeof(offset));
            i += sizeof(offset);
            std::memcpy(&size, &delta[i], sizeof(size));
            i += sizeof(size);

            result.insert(result.end(), original.begin() + offset,
                          original.begin() + offset + size);
        } else if (op == 1) {  // операция вставки
            unsigned int size;
            std::memcpy(&size, &delta[i], sizeof(size));
            i += sizeof(size);

            result.insert(result.end(), delta.begin() + i, delta.begin() + i + size);
            i += size;
        }
    }

    return result;
}

std::basic_string<char> DiffEngine::computeHash(const std::vector<unsigned char>& data) {
    // TO-DO реализовать SHA-1 или SHA-256
    unsigned long long int hash = 0;
    for (auto byte : data) {
        hash = hash * 31 + byte;
    }

    std::basic_stringstream<char> ss;
    ss << std::hex << hash;
    return ss.str();
}