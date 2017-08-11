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

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
#include <viua/support/string.h>
using namespace std;


using Token = viua::cg::lex::Token;


auto assembler::ce::getmarks(const vector<viua::cg::lex::Token>& tokens)
    -> map<string, std::remove_reference<decltype(tokens)>::type::size_type> {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     */
    std::remove_reference<decltype(tokens)>::type::size_type instruction =
        0;  // we need separate instruction counter because number of lines is not exactly number of
            // instructions
    map<string, decltype(instruction)> marks;

    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".name:" or tokens.at(i) == ".import:") {
            do {
                ++i;
            } while (i < tokens.size() and tokens.at(i) != "\n");
            continue;
        }
        if (tokens.at(i) == ".mark:") {
            ++i;  // skip ".mark:" token
            assert_is_not_reserved_keyword(tokens.at(i), "marker name");
            marks.emplace(tokens.at(i), instruction);
            ++i;  // skip marker name
            continue;
        }
        if (tokens.at(i) == "\n") {
            ++instruction;
        }
    }

    return marks;
}

vector<string> assembler::ce::getlinks(const vector<viua::cg::lex::Token>& tokens) {
    /** This function will pass over all instructions and
     * gather .import: assembler instructions.
     */
    vector<string> links;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".import:") {
            ++i;  // skip '.import:' token
            if (tokens.at(i) == "\n") {
                throw viua::cg::lex::InvalidSyntax(tokens.at(i), "missing module name in import directive");
            }
            links.emplace_back(tokens.at(i));
            ++i;  // skip module name token
        }
    }
    return links;
}

static bool looks_like_name_definition(Token t) {
    return (t == ".function:" or t == ".closure:" or t == ".block:" or t == ".signature:" or
            t == ".bsignature:");
}
static vector<string> get_instruction_block_names(const vector<Token>& tokens, string directive,
                                                  void predicate(Token) = [](Token) {}) {
    vector<string> names;
    vector<string> all_names;
    map<string, Token> defined_where;

    const auto limit = tokens.size();
    string looking_for = ("." + directive + ":");
    for (decltype(tokens.size()) i = 0; i < limit; ++i) {
        if (looks_like_name_definition(tokens.at(i))) {
            const auto first_token_of_block_header_at = i;
            ++i;
            if (i >= limit) {
                throw tokens[i - 1];
            }

            if (tokens.at(i) == "[[") {
                do {
                    ++i;
                } while (tokens.at(i) != "]]");
                ++i;
            }

            if (defined_where.count(tokens.at(i)) > 0) {
                throw viua::cg::lex::TracedSyntaxError()
                    .append(viua::cg::lex::InvalidSyntax(tokens.at(i),
                                                         ("duplicated name: " + tokens.at(i).str())))
                    .append(viua::cg::lex::InvalidSyntax(defined_where.at(tokens.at(i)),
                                                         "already defined here:"));
            }

            if (tokens.at(first_token_of_block_header_at) != looking_for) {
                continue;
            }

            predicate(tokens.at(i));
            names.emplace_back(tokens.at(i).str());
            defined_where.emplace(tokens.at(i), tokens.at(i));
        }
    }

    return names;
}
vector<string> assembler::ce::getFunctionNames(const vector<Token>& tokens) {
    auto names = get_instruction_block_names(
        tokens, "function", [](Token t) { assert_is_not_reserved_keyword(t, "function name"); });
    for (const auto& each : get_instruction_block_names(tokens, "closure")) {
        names.push_back(each);
    }
    return names;
}
vector<string> assembler::ce::getSignatures(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "signature",
                                       [](Token t) { assert_is_not_reserved_keyword(t, "function name"); });
}
vector<string> assembler::ce::getBlockNames(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "block",
                                       [](Token t) { assert_is_not_reserved_keyword(t, "block name"); });
}
vector<string> assembler::ce::getBlockSignatures(const vector<Token>& tokens) {
    return get_instruction_block_names(tokens, "bsignature",
                                       [](Token t) { assert_is_not_reserved_keyword(t, "block name"); });
}


static map<string, vector<Token>> get_raw_block_bodies(const string& type, const vector<Token>& tokens) {
    map<string, vector<Token>> invokables;

    string looking_for = ("." + type + ":");
    string name;
    vector<Token> body;

    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == looking_for) {
            ++i;  // skip directive
            name = tokens[i];
            ++i;  // skip name
            ++i;  // skip '\n' token
            while (i < tokens.size() and tokens[i].str() != ".end") {
                if (tokens[i] == looking_for) {
                    throw viua::cg::lex::InvalidSyntax(
                        tokens[i], ("another " + type + " opened before assembler reached .end after '" +
                                    name + "' " + type));
                }
                body.push_back(tokens[i]);
                ++i;
            }
            ++i;  // skip .end token

            invokables[name] = body;
            name = "";
            body.clear();
        }
    }

    return invokables;
}
map<string, vector<Token>> assembler::ce::getInvokablesTokenBodies(const string& type,
                                                                   const vector<Token>& tokens) {
    return get_raw_block_bodies(type, tokens);
}
