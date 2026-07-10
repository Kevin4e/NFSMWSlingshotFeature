#pragma once

/*
 *  K4IniReader - Lightweight cross-platform INI reader
 *  Version v1.2.0
 *  GitHub page: https://github.com/Kevin4e/K4IniReader
 *  Author: Kevin4e
 *
 *  Target: C++17+
 */

/*
 *  MIT License
 *  Copyright (c) 2025-2026 Kevin4e
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
#include <type_traits>

class K4IniReader {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;
    bool loaded = false;

    // Removes leading and trailing whitespaces from a string.
    static void trim(std::string& s) noexcept {
        // Trim from start
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));

        // Trim from end
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    }

    // Removes any inline comments from a string.
    // Supports comment markers: "//", ";", and "#". 
    // If multiple markers are present, removes from the first one found.
    static void removeInlineComment(std::string& s) noexcept {
        const size_t semicolonOrHashtagPos = s.find_first_of(";#");
        const size_t doubleSlashPos = s.find("//");

        if (semicolonOrHashtagPos != std::string::npos) {
            if (doubleSlashPos != std::string::npos)
                s.erase((std::min)(semicolonOrHashtagPos, doubleSlashPos)); // min() -> (min)() to avoid macro conflicts with Windows.h library
            else
                s.erase(semicolonOrHashtagPos);
        }
        else {
            if (doubleSlashPos != std::string::npos && (doubleSlashPos == 0 || std::isspace(static_cast<unsigned char>(s[doubleSlashPos - 1]))))
                s.erase(doubleSlashPos);
        }
    }

    // Lowers all the characters of a string
    static void lower(std::string& s) noexcept {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    // Determines if a string is empty or consists solely of whitespace
    static bool isEmpty(const std::string& s) noexcept {
        return std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isspace(c); });
    }

    // Searches for a key in a section.
    // Returns true if found and stores its value in 'out'.
    bool find(const std::string& s, const std::string& k, std::string& out) const noexcept {
        std::string sCopy = s;
        std::string kCopy = k;

        lower(sCopy);
        const auto sectionIt = data.find(sCopy);
        if (sectionIt == data.end()) return false; // The section was not found

        lower(kCopy);
        const auto keyIt = sectionIt->second.find(kCopy);
        if (keyIt == sectionIt->second.end()) return false; // The key was not found

        out = keyIt->second; // Get the value

        return true;
    }

public:
    // Extracts all the sections, keys, and their values from a .ini file.
    K4IniReader(const std::string& fileName, size_t nSections = 0, size_t nKeys = 0) {
        std::ifstream file(fileName);

        loaded = file.is_open();
        if (!loaded) return; // Failed to open file for reading

        data.reserve(nSections); // Reserves sections

        std::string line{};
        std::string currentSection{};

        while (std::getline(file, line)) {
            if (isEmpty(line)) continue; // Skips empty lines

            removeInlineComment(line); // Removes inline comments before processing the line
            trim(line);

            if (isEmpty(line)) continue; // Skips empty lines

            const size_t posEqualSing = line.find('=');  // Finds the position of the equal sign

            if (line.front() == '[') { // If the first character is an open square bracket, checks if further ahead there's a close one.
                const size_t posBracketEnd = line.find(']');
                if (posBracketEnd == std::string::npos) continue; // If it wasn't found, skips to the next line

                std::string sectionExtracted = line.substr(1, posBracketEnd - 1); // Extract the content between the two brackets
                trim(sectionExtracted); // Remove leading and trailing whitespaces
                if (sectionExtracted.empty()) continue; // Exit if the section is empty
                lower(sectionExtracted);
                currentSection = sectionExtracted; // Any key read from now on will be part of the section extracted (until a new section is found)
                data[currentSection].reserve(nKeys); // Reserve keys for this section
            }
            else if (posEqualSing != std::string::npos && !currentSection.empty()) { // If there's an equal sign and we're in a section
                std::string keyExtracted = line.substr(0, posEqualSing); // Extracts the key
                trim(keyExtracted); // Removes leading and trailing whitespaces
                if (keyExtracted.empty()) continue; // Exit if the key is empty
                lower(keyExtracted);

                std::string value = line.substr(posEqualSing + 1); // Extracts the value
                trim(value); // Removes leading and trailing whitespaces

                data[currentSection][keyExtracted] = value; // Inserts the key-value pair into the current section
            }
        }
    }

    // Reads a value of a key from a section.
    // 'toLower' parameter lowers all the letters of the value, effective for strings and booleans.
    template<typename T>
    T read(const std::string& section, const std::string& key, T defaultValue, bool toLowerString = false) const noexcept {
        std::string outValue{};
        if (!find(section, key, outValue))
            return defaultValue; // Exit the section or key wasn't found

        if constexpr (std::is_same_v<T, bool>) { // If T is a boolean
            lower(outValue);

            if (outValue == "true"  || outValue == "1" || outValue == "on" || outValue == "yes")
                return true;
            if (outValue == "false" || outValue == "0" || outValue == "off" || outValue == "no")
                return false;

            return defaultValue;
        }

        else if constexpr (std::is_same_v<T, char>) // If T is a char or a wide one
            return outValue.empty() ? defaultValue : outValue[0];

        else if constexpr (std::is_arithmetic_v<T>) { // If T is an arithmetic type (numeric)
            T outParsedValue = defaultValue;

            const size_t len = outValue.size();
            const char* start = outValue.data();
            const char* end = outValue.data() + len;

            std::from_chars_result conversionResult;

            if constexpr (std::is_integral_v<T>) { // If T is an integer data type
                int base = 10;

                if (len >= 2 && outValue[0] == '0') {
                    const char p = std::tolower(static_cast<unsigned char>(outValue[1]));

                    switch (p) {
                        case 'b': start += 2; base =  2; break; // Binary
                        case 'o': start += 2; base =  8; break; // Octal
                        case 'x': start += 2; base = 16; break; // Hexadecimal
                    }
                }

                conversionResult = std::from_chars(start, end, outParsedValue, base);
            }
            else // T is a floating point data type
                conversionResult = std::from_chars(start, end, outParsedValue); // Force base 10 conversion

            if (conversionResult.ec != std::errc() || conversionResult.ptr != end)
                return defaultValue;

            return outParsedValue;
        }

        else if constexpr (std::is_same_v<T, std::string>) { // If T is a string
            if (toLowerString)
                lower(outValue);

            return outValue;
        }

        // Fallback return for unhandled types
        return defaultValue;
    }

    // Allows the INI reader object to be used in boolean contexts to check if the file was successfully loaded
    // (e.g., if (iniReader) { ... })
    explicit operator bool() const noexcept {
        return loaded;
    }
};