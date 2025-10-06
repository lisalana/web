/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   String.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rguigneb <rguigneb@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 15:14:05 by rguigneb          #+#    #+#             */
/*   Updated: 2025/09/22 14:30:29 by rguigneb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <algorithm>

#include <string>
#include <cctype>

class String : public std::string
{
	public:
		String() : std::string() {}

		template<typename T>
		String(T s = "") : std::string(s) {}

		std::size_t count(char c, std::size_t start, std::size_t end) const throw() {
			if (start >= this->size() || end > this->size() || start >= end) {
				return 0;
			}
			return static_cast<std::size_t>(
				std::count(this->begin() + start, this->begin() + end, c)
			);
		}

		std::size_t count(char c) const throw() {
			return static_cast<std::size_t>(
				std::count(this->begin(), this->end(), c)
			);
		}

		std::size_t count(String str) const throw() {
			if (str.empty()) return 0;
			std::size_t n = 0;
			std::string::size_type pos = 0;

			while (true) {
				pos = this->find(str, pos);
				if (pos == std::string::npos) break;
				++n;
				pos += str.size();
			}
			return n;
		}

		bool endsWith(const std::string& suffix) const {
			if (suffix.size() > size()) return false;
			return compare(size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		bool startsWith(const std::string& prefix) const {
			if (prefix.size() > size()) return false;
			return compare(0, prefix.size(), prefix) == 0;
		}

		void trimStart() {
			size_t start = 0;
			while (start < size() && std::isspace((*this)[start])) {
				++start;
			}
			if (start > 0) {
				erase(0, start);
			}
		}

		void trimEnd() {
			if (empty()) return;
			size_t end = size() - 1;
			while (end != std::string::npos && std::isspace((*this)[end])) {
				if (end == 0) {
					clear();
					return;
				}
				--end;
			}
			erase(end + 1);
		}

		void trim() {
			trimEnd();
			trimStart();
		}

		static bool isHttpWhitespace(char c) {
			return c == ' ' || c == '\t';
		}

		void httpTrimStart() {
			size_t start = 0;
			while (start < size() && isHttpWhitespace((*this)[start])) {
				++start;
			}
			if (start > 0) {
				erase(0, start);
			}
		}

		void httpTrimEnd() {
			if (empty()) return;
			size_t end = size() - 1;
			while (end != std::string::npos && isHttpWhitespace((*this)[end])) {
				if (end == 0) {
					clear();
					return;
				}
				--end;
			}
			erase(end + 1);
		}

		void replaceFirst(const String& from, const String& to) {
			size_t start_pos = this->find(from);
			if (start_pos != std::string::npos) {
				this->std::string::replace(start_pos, from.length(), to);
			}
		}

		void httpTrim() {
			httpTrimEnd();
			httpTrimStart();
		}

		void trimQuotes() {
			if (size() >= 2 && (*this)[0] == '"' && (*this)[size() - 1] == '"') {
				erase(size() - 1);
				erase(0, 1);
			}
		}

		bool onlySpacesAfter(std::string::const_iterator it) const {
			for (; it != end(); ++it) {
				if (!std::isspace(*it)) {
					return false;
				}
			}
			return true;
		}

		bool onlySpacesAfter(std::size_t index) const {
			if (index >= size() - 1) return true;
			for (std::size_t i = index; i < size(); ++i) {
				if (!std::isspace((*this)[i])) {
					return false;
				}
			}
			return true;
		}

		std::size_t countCharUntilAnOther(std::size_t index, char c) const {
			if (index >= this->size()) {
				return 0;
			}

			std::size_t count = 0;
			for (std::size_t i = index; i < this->size(); i++) {
				if ((*this)[i] != c) {
					break;
				}
				count++;
			}
			return count;
		}

		bool isNumber() const {
        if (empty()) return false;

        size_t start = 0;
        bool hasSign = false;
        bool hasDigit = false;
        bool hasDecimal = false;
        bool hasExponent = false;

        if (at(0) == '+' || at(0) == '-') {
            hasSign = true;
            start = 1;
            if (size() == 1) return false;
        }

        for (size_t i = start; i < size(); ++i) {
            char c = at(i);

            if (std::isdigit(c)) {
                hasDigit = true;
            }
            else if (c == '.') {
                if (hasDecimal || hasExponent) return false;
                hasDecimal = true;
            }
            else if (c == 'e' || c == 'E') {
                if (hasExponent || !hasDigit) return false;
                hasExponent = true;
                hasDigit = false;

                if (i + 1 < size() && (at(i + 1) == '+' || at(i + 1) == '-')) {
                    ++i;
                    if (i + 1 >= size()) return false;
                }
            }
            else {
                return false;
            }
        }

        return hasDigit;
    }

		bool isInt() const {
			if (empty()) return false;

			std::string trimmed = *this;
			trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
			trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

			if (trimmed.empty()) return false;

			std::istringstream iss(trimmed);
			int value;

			return (iss >> value) && iss.eof();
		}

		int toInt() {
			this->trim();
			std::istringstream iss(*this);
			int value;

			if (!(iss >> value)) {
				throw std::invalid_argument("String is not a valid integer: " + *this);
			}

			return value;
		}

		void lower() {
			std::transform(this->begin(), this->end(), this->begin(), ::tolower);
		}

		static String fromInt(int value) {
			std::ostringstream oss;
			oss << value;
			return String(oss.str());
		}

		String replace(const String& from, const String& to, size_t start = 0) const {
			if (from.empty() || start >= this->size()) {
				return *this;
			}

			String result = *this;
			size_t pos = start;

			while ((pos = result.find(from, pos)) != std::string::npos) {
				result.std::string::replace(pos, from.length(), to);
				pos += to.length();
			}

			return result;
		}
};


