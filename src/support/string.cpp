/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <viua/types/string.h>
using namespace std;


namespace str {
    bool startswith(const std::string& s, const std::string& w) {
        /*  Returns true if s stars with w.
         */
        return (s.compare(0, w.length(), w) == 0);
    }

    bool startswithchunk(const std::string& s, const std::string& w) {
        /*  Returns true if s stars with chunk w.
         */
        return (chunk(s) == w);
    }

    bool endswith(const std::string& s, const std::string& w) {
        /*  Returns true if s ends with w.
         */
        return (s.compare(s.length() - w.length(), s.length(), w) == 0);
    }


    bool isnum(const std::string& s, bool negatives) {
        /*  Returns true if s contains only numerical characters.
         *  Regex equivalent: `^[0-9]+$`
         */
        bool num = false;
        std::string::size_type start = 0;
        if (s[0] == '-' and negatives) {
            // must handle negative numbers
            start = 1;
        }
        for (auto i = start; i < s.size(); ++i) {
            switch (s[i]) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    num = true;
                    break;
                default:
                    num = false;
            }
            if (!num)
                break;
        }
        return num;
    }

    bool ishex(const std::string& s, bool) {
        /*  Returns true if s is a valid hexadecimal number.
         */
        static regex hexadecimal_number("^0x[0-9a-fA-F]+$");
        return regex_match(s, hexadecimal_number);
    }

    auto is_binary_literal(const string s) -> bool {
        static regex binary_literal{"^0(?:b[01]+|o[0-7]+|x[0-9a-f]+)$"};
        return regex_match(s, binary_literal);
    }

    auto is_boolean_literal(const string s) -> bool { return (s == "true" or s == "false"); }

    auto is_void(const string s) -> bool { return (s == "void"); }

    auto is_atom_literal(const string s) -> bool {
        /*
         * This seemingly naive check is sufficient, as this function should only
         * be called after the source code already lexed (and the lexer ensures that
         * strings are properly closed and escaped so it is sufficient to check
         * for opening and closing quotes here).
         */
        return (s.at(0) == '\'' and s.at(s.size() - 1) == '\'');
    }

    auto is_text_literal(const string s) -> bool {
        /*
         * Same as with with is_atom_literal().
         */
        return (s.at(0) == '"' and s.at(s.size() - 1) == '"');
    }

    auto is_timeout_literal(const string s) -> bool {
        if (s == "infinity") {
            return true;
        }

        const auto size = s.size();
        if (size < 2) {
            return false;
        }
        if (s.at(size - 2) == 'm' and s.at(size - 1) == 's' and str::isnum(s.substr(0, size - 2))) {
            return true;
        }
        if (s.at(size - 1) == 's' and str::isnum(s.substr(0, size - 1))) {
            return true;
        }
        return false;
    }

    bool isfloat(const std::string& s, bool negatives) {
        /*  Returns true if s contains only numerical characters.
         *  Regex equivalent: `^[0-9]+\.[0-9]+$`
         */
        bool is = false;
        std::string::size_type start = 0;
        if (s[0] == '-' and negatives) {
            // to handle negative numbers
            start = 1;
        }
        int dot = -1;
        for (auto i = start; i < s.size(); ++i) {
            if (s[i] == '.') {
                dot = static_cast<int>(i);
                break;
            }
        }
        is = isnum(sub(s, 0, dot), negatives) and isnum(sub(s, (static_cast<unsigned>(dot) + 1)));
        return is;
    }

    bool isid(const std::string& s) {
        /*  Returns true if s is a valid identifier.
         */
        static regex identifier("^[a-zA-Z_][a-zA-Z0-9_]*$");
        return regex_match(s, identifier);
    }


    string sub(const string& s, std::string::size_type b, long int e) {
        /*  Returns substring of s.
         *  If only s is passed, returns copy of s.
         */
        if (b == 0 and e == -1)
            return string(s);

        ostringstream part;
        part.str("");

        std::string::size_type end = 0;
        if (e < 0) {
            end = (s.size() - static_cast<std::string::size_type>(-1 * e) + 1);
        } else {
            end = static_cast<std::string::size_type>(e);
        }

        for (auto i = b; i < s.size() and i < end; ++i) {
            part << s[i];
        }

        return part.str();
    }


    string chunk(const string& s, bool ignore_leading_ws) {
        /*  Returns part of the string until first whitespace from left side.
         */
        ostringstream chnk;

        string str = (ignore_leading_ws ? lstrip(s) : s);

        for (std::string::size_type i = 0; i < str.size(); ++i) {
            if (str[i] == ' ' or str[i] == '\t' or str[i] == '\v' or str[i] == '\n')
                break;
            chnk << str[i];
        }
        return chnk.str();
    }

    vector<string> chunks(const string& s) {
        /*  Returns chunks of string.
         */
        vector<string> chnks;
        string tmp(lstrip(s));
        string chnk;
        while (tmp.size()) {
            chnk = chunk(tmp);
            tmp = lstrip(sub(tmp, chnk.size()));
            chnks.emplace_back(chnk);
        }
        return chnks;
    }


    string join(const string& s, const vector<string>& parts) {
        /** Join elements of vector with given string.
         */
        ostringstream oss;
        auto limit = parts.size();
        for (decltype(limit) i = 0; i < limit; ++i) {
            oss << parts[i];
            if (i < (limit - 1)) {
                oss << s;
            }
        }
        return oss.str();
    }


    string extract(const string& s) {
        /** Extracts *enquoted chunk*.
         *
         *  It is particularly useful if you have a string encoded in another string.
         *
         *  This function will return `"Hello 'Beautiful' World!"` when fed `"Hello 'Beautiful' World!" some
         * other (42) things;`, and
         *  will return `'Hello "Beautiful" World!'` when fed `'Hello "Beautiful" World!' some other (42)
         * things;`.
         *  Starting quote character is irrelevant.
         *
         *  In fact, this function will treat *the first character* of the string it is fed as a delimiter for
         *  string extraction - whatever that may be (e.g. the backtick character) so you can get creative.
         *  One character that is not recommended for use as a delimiter is the backslash as it is treated
         * specially (as
         *  the escape character) by this function.
         */
        if (s.size() == 0) {
            return string("");
        }

        ostringstream chnk;
        char quote;
        chnk << (quote = s[0]);

        std::string::size_type backs = 0;
        for (std::string::size_type i = 1; i < s.size(); ++i) {
            chnk << s[i];
            if (backs and s[i] != '\\' and s[i] != quote) {
                backs = 0;
                continue;
            }
            if (s[i] == quote and ((backs % 2) != 0)) {
                backs = 0;
                continue;
            } else if (s[i] == quote and ((backs % 2) == 0)) {
                break;
            }
            if (s[i] == '\\') {
                ++backs;
                // cout << "backs = " << backs << endl;
            }
        }

        return chnk.str();
    }


    string lstrip(const string& s) {
        /*  Removes whitespace from left side of the string.
         */
        std::string::size_type i = 0;
        while (i < s.size()) {
            if (not(s[i] == ' ' or s[i] == '\t' or s[i] == '\v' or s[i] == '\n')) {
                break;
            };
            ++i;
        }
        return sub(s, i);
    }


    std::string::size_type lshare(const string& s, const string& w) {
        std::string::size_type share = 0;
        for (std::string::size_type i = 0; i < s.size() and i < w.size(); ++i) {
            if (s[i] == w[i]) {
                ++share;
            } else {
                break;
            }
        }
        return share;
    }
    bool contains(const string& s, const char c) {
        bool it_does = false;
        for (std::string::size_type i = 0; i < s.size(); ++i) {
            if (s[i] == c) {
                it_does = true;
                break;
            }
        }
        return it_does;
    }


    string enquote(const string& s, const char closing) {
        /** Enquote the string.
         */
        ostringstream encoded;

        encoded << closing;
        for (std::string::size_type i = 0; i < s.size(); ++i) {
            if (s[i] == closing) {
                encoded << "\\";
            }
            encoded << s[i];
        }
        encoded << closing;

        return encoded.str();
    }

    string strdecode(const string& s) {
        /** Decode escape sequences in strings.
         *
         *  This function recognizes escape sequences as listed on:
         *  http://en.cppreference.com/w/cpp/language/escape
         *  The function does not recognize sequences for:
         *      - arbitrary octal numbers (escape: \nnn),
         *      - arbitrary hexadecimal numbers (escape: \xnn),
         *      - short arbitrary Unicode values (escape: \unnnn),
         *      - long arbitrary Unicode values (escape: \Unnnnnnnn),
         *
         *  If a character that does not encode an escape sequence is
         *  preceded by a backslash (\\) the function consumes the backslash and
         *  leaves only the character preceded by it in the output string.
         *
         */
        ostringstream decoded;
        char c;
        for (std::string::size_type i = 0; i < s.size(); ++i) {
            c = s[i];
            if (c == '\\' and i < (s.size() - 1)) {
                ++i;
                switch (s[i]) {
                    case '\'':
                        c = '\'';
                        break;
                    case '"':
                        c = '"';
                        break;
                    case '?':
                        c = '?';
                        break;
                    case '\\':
                        c = '\\';
                        break;
                    case 'a':
                        c = '\a';
                        break;
                    case 'b':
                        c = '\b';
                        break;
                    case 'f':
                        c = '\f';
                        break;
                    case 'n':
                        c = '\n';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case 'v':
                        c = '\v';
                        break;
                    default:
                        c = s[i];
                }
            }
            decoded << c;
        }
        return decoded.str();
    }
    string strencode(const string& s) {
        /** Encode escape sequences in strings.
         *
         *  This function recognizes escape sequences as listed on:
         *  http://en.cppreference.com/w/cpp/language/escape
         *  The function does not recognize sequences for:
         *      - arbitrary octal numbers (escape: \nnn),
         *      - arbitrary hexadecimal numbers (escape: \xnn),
         *      - short arbitrary Unicode values (escape: \unnnn),
         *      - long arbitrary Unicode values (escape: \Unnnnnnnn),
         *
         */
        ostringstream encoded;
        char c;
        bool escape = false;
        for (std::string::size_type i = 0; i < s.size(); ++i) {
            switch (s[i]) {
                case '\\':
                    escape = true;
                    c = '\\';
                    break;
                case '\a':
                    escape = true;
                    c = 'a';
                    break;
                case '\b':
                    escape = true;
                    c = 'b';
                    break;
                case '\f':
                    escape = true;
                    c = 'f';
                    break;
                case '\n':
                    escape = true;
                    c = 'n';
                    break;
                case '\r':
                    escape = true;
                    c = 'r';
                    break;
                case '\t':
                    escape = true;
                    c = 't';
                    break;
                case '\v':
                    escape = true;
                    c = 'v';
                    break;
                default:
                    escape = false;
                    c = s[i];
            }
            if (escape) {
                encoded << '\\';
            }
            encoded << c;
        }
        return encoded.str();
    }


    string stringify(const vector<string>& sv) {
        ostringstream oss;
        oss << '[';
        auto sz = sv.size();
        for (decltype(sz) i = 0; i < sz; ++i) {
            oss << enquote(sv[i]);
            if (i < (sz - 1)) {
                oss << ", ";
            }
        }
        oss << ']';
        return oss.str();
    }
}
