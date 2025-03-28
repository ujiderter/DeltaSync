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
                                               const std::vector<unsigned char>& modified,
                                               size_t minMatchLength) {
    std::vector<unsigned char> delta;
    
    // Используем более эффективный алгоритм поиска совпадений
    size_t i = 0;
    while (i < modified.size()) {
        size_t bestMatchPos = 0;
        size_t bestMatchLen = 0;
        
        // Ищем самое длинное совпадение
        for (size_t j = 0; j < original.size(); j++) {
            size_t k = 0;
            while (i + k < modified.size() && 
                   j + k < original.size() && 
                   modified[i + k] == original[j + k]) {
                k++;
            }
            
            if (k > bestMatchLen) {
                bestMatchLen = k;
                bestMatchPos = j;
                if (bestMatchLen >= minMatchLength * 2) break; // ранний выход
            }
        }
        
        if (bestMatchLen >= minMatchLength) {
            // Операция копирования
            delta.push_back(0); // код операции
            
            // Добавляем offset и length
            uint32_t offset = static_cast<uint32_t>(bestMatchPos);
            uint32_t length = static_cast<uint32_t>(bestMatchLen);
            delta.insert(delta.end(), reinterpret_cast<unsigned char*>(&offset), 
                         reinterpret_cast<unsigned char*>(&offset) + sizeof(offset));
            delta.insert(delta.end(), reinterpret_cast<unsigned char*>(&length), 
                         reinterpret_cast<unsigned char*>(&length) + sizeof(length));
            
            i += bestMatchLen;
        } else {
            // Операция вставки новых данных
            size_t insertStart = i;
            i++; // минимум 1 байт
            
            // Ищем следующий хороший матч
            while (i < modified.size()) {
                bool foundGoodMatch = false;
                for (size_t j = 0; j < original.size(); j++) {
                    if (modified[i] == original[j]) {
                        size_t k = 1;
                        while (i + k < modified.size() && 
                               j + k < original.size() && 
                               modified[i + k] == original[j + k] && 
                               k < minMatchLength) {
                            k++;
                        }
                        if (k >= minMatchLength) {
                            foundGoodMatch = true;
                            break;
                        }
                    }
                }
                if (foundGoodMatch) break;
                i++;
            }
            
            uint32_t length = static_cast<uint32_t>(i - insertStart);
            delta.push_back(1); // код операции
            delta.insert(delta.end(), reinterpret_cast<unsigned char*>(&length), 
                         reinterpret_cast<unsigned char*>(&length) + sizeof(length));
            delta.insert(delta.end(), modified.begin() + insertStart, 
                         modified.begin() + i);
        }
    }
    
    return delta;
}

std::vector<unsigned char> DiffEngine::applyDelta(const std::vector<unsigned char>& original,
                                             const std::vector<unsigned char>& delta,
                                             bool verifyHash) {
    std::vector<unsigned char> result;
    size_t i = 0;
    
    while (i < delta.size()) {
        if (i + 1 >= delta.size()) throw std::runtime_error("Invalid delta format");
        
        unsigned char op = delta[i++];
        
        if (op == 0) { // copy
            if (i + sizeof(uint32_t)*2 > delta.size()) 
                throw std::runtime_error("Invalid copy operation in delta");
                
            uint32_t offset, length;
            memcpy(&offset, &delta[i], sizeof(offset));
            i += sizeof(offset);
            memcpy(&length, &delta[i], sizeof(length));
            i += sizeof(length);
            
            if (offset + length > original.size())
                throw std::runtime_error("Invalid offset/length in copy operation");
                
            result.insert(result.end(), original.begin() + offset, 
                         original.begin() + offset + length);
        } 
        else if (op == 1) { // insert
            if (i + sizeof(uint32_t) > delta.size())
                throw std::runtime_error("Invalid insert operation in delta");
                
            uint32_t length;
            memcpy(&length, &delta[i], sizeof(length));
            i += sizeof(length);
            
            if (i + length > delta.size())
                throw std::runtime_error("Invalid data length in insert operation");
                
            result.insert(result.end(), delta.begin() + i, delta.begin() + i + length);
            i += length;
        }
        else {
            throw std::runtime_error("Unknown operation in delta");
        }
    }
    
    if (verifyHash) {
        // Проверяем хеш, если он есть в конце дельты
        // (можно добавить опциональное хеширование)
    }
    
    return result;
}

std::basic_string<char> DiffEngine::computeHash(const std::vector<unsigned char>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::vector<unsigned char> DiffEngine::computeCompressedDelta(const std::vector<unsigned char>& original,
                                                      const std::vector<unsigned char>& modified) {
    std::vector<unsigned char> delta = computeDelta(original, modified);
    std::vector<unsigned char> compressed(delta.size() * 1.1 + 12); // zlib рекомендация
    
    z_stream stream;
    stream.next_in = delta.data();
    stream.avail_in = delta.size();
    stream.next_out = compressed.data();
    stream.avail_out = compressed.size();
    
    deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    
    compressed.resize(stream.total_out);
    return compressed;
}