#pragma once

/*
 *  K4IniReader - Lightweight cross-platform INI reader
 *  Version v1.1.0
 *  GitHub page: https://github.com/Kevin4e/K4IniReader
 *  Author: Kevin4e
 *
 *  Target: C++17+
 */

/*
 *  MIT License
 *  Copyright (c) 2025 Kevin4e
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 *  and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */ 

#include <unordered_map>
#include <string>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <charconv>

class K4IniReader {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

    // Removes leading and trailing whitespaces from a string
    static inline void trim(std::string& s) noexcept {
        // Trim from start
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));

        // Trim from end
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    // Removes any inline comments from a string.
    // Supports comment markers: "//", ";", and "#". 
    // If multiple markers are present, removes from the first one found.
    static inline void removeInlineComment(std::string& s) noexcept {
        size_t commaOrHashtagPos = s.find_first_of(";#");
        size_t doubleSlashPos = s.find("//");

        if (commaOrHashtagPos != std::string::npos) {
            if (doubleSlashPos != std::string::npos)
                s.erase(std::min(commaOrHashtagPos, doubleSlashPos));
            else
                s.erase(commaOrHashtagPos);
        }
        else {
            if (doubleSlashPos != std::string::npos)
                s.erase(doubleSlashPos);
        }
    }

    // Searches for a key in a section.
    // Returns true if found and stores its value in 'out'.
    inline bool find(const std::string& s, const std::string& k, std::string& out) const noexcept {
        auto sectionIt = data.find(s);
        if (sectionIt == data.end()) return false; // The section was not found

        auto keyIt = sectionIt->second.find(k);
        if (keyIt == sectionIt->second.end()) return false; // The key was not found

        out = keyIt->second; // Get the value

        return true;
    }

public:
    // Extracts all the sections, keys, and their values from a .ini file
    K4IniReader(const std::string& fileName, size_t nSections = 32, size_t nKeys = 8) {
        std::ifstream file(fileName);
        if (!file.is_open()) return; // Don't 

        data.reserve(nSections); // Reserves sections

        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            removeInlineComment(line); // Removes inline comments before processing the line

            if (line.empty()) continue; // Skips empty lines

            size_t posBracketStart = line.find('['); // Finds the position of the open square bracket
            size_t posEqualSing = line.find('=');    // Finds the position of the equal sign

            if (posBracketStart != std::string::npos) { // If there is an open square bracket, checks if further ahead there's a close one.
                size_t posBracketEnd = line.find(']', posBracketStart);
                if (posBracketEnd == std::string::npos) continue; // If it wasn't found, skips to the next line

                std::string sectionExtracted = line.substr(posBracketStart + 1, posBracketEnd - posBracketStart - 1); // Extract the content between the two brackets
                trim(sectionExtracted); // Remove leading and trailing whitespaces
                currentSection = sectionExtracted; // Any key read from now on will be part of the section extracted (until a new section is found)
                data[currentSection].reserve(nKeys); // Reserve keys for this section
            }
            else if (posEqualSing != std::string::npos) { // If there's an equal sign
                std::string keyExtracted = line.substr(0, posEqualSing); // Extracts the key
                trim(keyExtracted); // Removes leading and trailing whitespaces

                std::string value = line.substr(posEqualSing + 1); // Extracts the value
                trim(value); // Removes leading and trailing whitespaces

                data[currentSection][keyExtracted] = value; // Inserts the key-value pair into the current section
            }
        }
    }

    // Reads a value of a key from a section
    template<typename T>
    T read(const std::string& section, const std::string& key, T defaultValue, bool toLowerString = false) const noexcept {
        std::string outValue;
        if (!find(section, key, outValue)) return defaultValue;

        if constexpr (std::is_same_v<T, bool>) { // If T is a boolean
            if (outValue == "true" || outValue == "1" || outValue == "on" || outValue == "yes")
                return true;

            if (outValue == "false" || outValue == "0" || outValue == "off" || outValue == "no")
                return false;

            return defaultValue;
        }

        else if constexpr (std::is_same_v<T, char>) // If T is a char or a wide one
            return outValue.empty() ? defaultValue : outValue[0];

        else if constexpr (std::is_arithmetic_v<T>) { // If T is an arithmetic type (numeric)
            T outParsedValue = defaultValue;

            // Tries converting the value read to a numeric type
            auto result = std::from_chars(outValue.data(), outValue.data() + outValue.size(), outParsedValue);

            // Returns the out value regardless of the conversion result;
            // if it failed, it will not modify the out value,
            // therefore the out value will still have the default one
            return outParsedValue; 
        }

        else if constexpr (std::is_same_v<T, std::string>) { // If T is a string
            if (toLowerString) {
                bool hasUpper = std::any_of(outValue.begin(), outValue.end(), [](unsigned char c) { return std::isupper(c); });
                if (hasUpper)
                    // Lowers the out value only if an upper character was found
                    std::transform(outValue.begin(), outValue.end(), outValue.begin(), [](unsigned char c) { return std::tolower(c); });
            }

            return outValue;
        }

        // Fallback return for unhandled types
        return defaultValue;
    }
};