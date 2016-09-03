#ifndef VIUA_CG_LEX_H
#define VIUA_CG_LEX_H

#pragma once

#include <string>
#include <vector>


namespace viua {
    namespace cg {
        namespace lex {
            class Token {
                std::string content;
                decltype(content.size()) line_number, character_in_line;

                public:

                auto line() const -> decltype(line_number);
                auto character() const -> decltype(character_in_line);
                auto str() const -> decltype(content);

                auto ends() const -> decltype(character_in_line);

                bool operator==(const std::string& s);
                bool operator!=(const std::string& s);

                Token(decltype(line_number) = 0, decltype(character_in_line) = 0, std::string = "");
            };

            struct InvalidSyntax {
                long unsigned line_number, character_in_line;
                std::string content;

                InvalidSyntax(long unsigned, long unsigned, std::string);
            };

            std::vector<Token> tokenise(const std::string&);
            std::vector<Token> standardise(std::vector<Token>);

            template<class T, typename... R> bool adjacent(T first, T second) {
                if (first.line() != second.line()) {
                    return false;
                }
                if (first.ends() != second.character()) {
                    return false;
                }
                return true;
            }
            template<class T, typename... R> bool adjacent(T first, T second, R... rest) {
                if (first.line() != second.line()) {
                    return false;
                }
                if (first.ends() != second.character()) {
                    return false;
                }
                return adjacent(second, rest...);
            }

            std::string join_tokens(const std::vector<Token> tokens, const decltype(tokens)::size_type from, const decltype(from) to);

            std::vector<Token> remove_spaces(std::vector<Token>);
            std::vector<Token> remove_comments(std::vector<Token>);
            std::vector<Token> reduce_newlines(std::vector<Token>);
            std::vector<Token> reduce_function_directive(std::vector<Token>);
            std::vector<Token> reduce_end_directive(std::vector<Token>);
            std::vector<Token> reduce_signature_directive(std::vector<Token>);
            std::vector<Token> reduce_double_colon(std::vector<Token>);
            std::vector<Token> reduce_function_signatures(std::vector<Token>);
            std::vector<Token> reduce_names(std::vector<Token>);

            std::vector<Token> unwrap_lines(std::vector<Token>, bool full = true);

            std::vector<Token> reduce(std::vector<Token>);
        }
    }
}


#endif