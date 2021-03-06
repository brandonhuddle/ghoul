/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <iostream>
#include "Lexer.hpp"
#include <cctype>

using namespace gulc;

TokenType Lexer::peekType() {
    return peekToken().tokenType;
}

TokenMetaType Lexer::peekMeta() {
    return peekToken().metaType;
}

std::string const& Lexer::peekCurrentSymbol() {
    return peekToken().currentSymbol;
}

TextPosition Lexer::peekStartPosition() {
    return peekToken().startPosition;
}

TextPosition Lexer::peekEndPosition() {
    return peekToken().endPosition;
}

bool Lexer::peekHasLeadingWhitespace() {
    return peekToken().hasLeadingWhitespace;
}

const Token& Lexer::peekToken() {
    if (_nextToken.tokenType == TokenType::NIL) {
        _nextToken = lexOneToken();
    }

    return _nextToken;
}

Token Lexer::nextToken() {
    Token result = peekToken();

    if (_nextToken.tokenType != TokenType::ENDOFFILE) {
        _nextToken.tokenType = TokenType::NIL;
        //_nextToken.tokenMetaType = TokenMetaType::NIL;
        _nextToken.currentSymbol = "";
        _nextToken.currentChar = 0;
        _nextToken.startPosition = TextPosition(0, 0, 0);
        _nextToken.endPosition = TextPosition(0, 0, 0);
    }

    return result;
}

bool Lexer::consumeType(TokenType type) {
    TokenType currentType = peekType();

    if (currentType == type) {
        nextToken();
        return true;
    } else {
        return false;
    }
}

LexerCheckpoint Lexer::createCheckpoint() {
    Token checkpointNextToken = Token(_nextToken.tokenType, _nextToken.metaType, _nextToken.currentSymbol,
                                      _nextToken.currentChar, _nextToken.startPosition, _nextToken.endPosition,
                                      _nextToken.hasLeadingWhitespace);
    return LexerCheckpoint(checkpointNextToken, _currentLine, _currentColumn, _currentIndex);
}

void Lexer::returnToCheckpoint(const LexerCheckpoint& checkpoint) {
    _nextToken = checkpoint._nextToken;
    _currentLine = checkpoint.currentLine;
    _currentColumn = checkpoint.currentColumn;
    _currentIndex = checkpoint.currentIndex;
}

bool Lexer::getRightShiftState() const {
    return _rightShiftEnabled;
}

void Lexer::setRightShiftState(bool enabled) {
    _rightShiftEnabled = enabled;
}

void Lexer::printError(const std::string& errorText, int errorCode) {
    std::cout << "gulc lexer error[" << _filePath << ", " << _currentLine << ", " << _currentColumn << "]: "
              << errorText << std::endl;
    std::exit(errorCode);
}

void Lexer::errorUnexpectedEOF() {
    printError("End of file reached unexpectedly, cannot continue!");
}

Token Lexer::lexOneToken() {
    TextPosition startPosition(_currentIndex, _currentLine, _currentColumn);
    std::string tmpTokenText;
    Token result(TokenType::NIL, TokenMetaType::NIL, "", 0,
                 startPosition, TextPosition(_currentIndex, _currentLine, _currentColumn),
                 false);

#define PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY() if (!tmpTokenText.empty()) return parseToken(tmpTokenText, startPosition, result.hasLeadingWhitespace);
#define RETURN_GENERIC_TOKEN(nTokenType, nMetaType, nSymbol, nChar) ++_currentIndex; ++_currentColumn; return Token((nTokenType), (nMetaType), (nSymbol), (nChar), startPosition, TextPosition(_currentIndex, _currentLine, _currentColumn), result.hasLeadingWhitespace);
#define CHECK_NEXT_CHAR() (_currentIndex + 1) < _sourceCode.length() && _sourceCode[_currentIndex + 1]
#define CHECK_AND_RETURN_EOF() if (_currentIndex == _sourceCode.length()) { result.tokenType = TokenType::ENDOFFILE; return result; }
#define ERROR_IF_EOF() if (_currentIndex == _sourceCode.length()) { errorUnexpectedEOF(); }

    // Check to see if the EOF has been reached and return EOF if it has
    CHECK_AND_RETURN_EOF();

    for (; _sourceCode[_currentIndex] == '\r' || _sourceCode[_currentIndex] == '\n' || isspace(_sourceCode[_currentIndex]); ++_currentIndex) {
        if (_sourceCode[_currentIndex] == '\r' || _sourceCode[_currentIndex] == '\n') {
            ++_currentLine;
            _currentColumn = 1;

            // If the character was '\r' then remove the '\n' that comes next
            if (_sourceCode[_currentIndex] == '\r' && (_currentIndex + 1) < _sourceCode.length() && _sourceCode[_currentIndex + 1] == '\n') {
                ++_currentIndex;
            }
        } else if (isspace(_sourceCode[_currentIndex])) {
            ++_currentColumn;
        }

        result.hasLeadingWhitespace = true;
    }

    startPosition = TextPosition(_currentIndex, _currentLine, _currentColumn);

    for (; _currentIndex < _sourceCode.length(); ++_currentIndex, ++_currentColumn) {
        if (_sourceCode[_currentIndex] == '\r' || _sourceCode[_currentIndex] == '\n') {
            PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
            result.hasLeadingWhitespace = true;

            ++_currentLine;
            _currentColumn = 1;

            // If the character was '\r' then remove the '\n' that comes next
            if (_sourceCode[_currentIndex] == '\r' &&
                (_currentIndex + 1) < _sourceCode.length() &&
                _sourceCode[_currentIndex + 1] == '\n') {
                ++_currentIndex;
            }
        } else if (std::isspace(static_cast<unsigned char>(_sourceCode[_currentIndex]))) {
            PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
            result.hasLeadingWhitespace = true;
        } else {
            switch (_sourceCode[_currentIndex]) {
                case '"': {
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    // TODO: Add support for Unicode and ASCII escape sequences
                    std::string tmpString;
                    bool isEscaped = false;

// Add the specified escaped char to 'tmpString' if 'isEscaped' is true (and then set 'isEscaped' to false)
// else add the unescaped char to 'tmpString'
#define COND_ADD_ESCAPED_OR_NORM_CHAR(escapedChar, unescapedChar) \
if (isEscaped) { tmpString += static_cast<char>(escapedChar); isEscaped = false; } \
else { tmpString += (unescapedChar); }

                    // We increment once to ignore the current double-quote
                    for (++_currentIndex, ++_currentColumn;
                         _currentIndex < _sourceCode.length();
                         ++_currentIndex, ++_currentColumn) {
                        switch (_sourceCode[_currentIndex]) {
                            case '"':
                                if (isEscaped) {
                                    isEscaped = false;
                                    tmpString += '"';
                                } else {
                                    // TODO: Support "String literal "  "concatenation when there are two double quotes"
                                    RETURN_GENERIC_TOKEN(TokenType::STRING, TokenMetaType::VALUE,
                                                         tmpString, 0);
                                }
                                break;
                            case '\\':
                                // If we're currently escaped then add a new '\\'
                                if (isEscaped) tmpString += '\\';
                                // Toggle if we're escaped or not
                                isEscaped = !isEscaped;
                                break;
                            case '\'':
                                // Within a string ' can optionally be escaped or unescaped so we ignore 'isEscaped'
                                if (isEscaped) isEscaped = false;
                                tmpString += _sourceCode[_currentIndex];
                                break;
                            case '0':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\0', '0');
                                break;
                            case 'a':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\a', 'a');
                                break;
                            case 'b':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\b', 'b');
                                break;
                            case 'f':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\f', 'f');
                                break;
                            case 'n':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\n', 'n');
                                break;
                            case 'r':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\r', 'r');
                                break;
                            case 't':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\t', 't');
                                break;
                            case 'v':
                                COND_ADD_ESCAPED_OR_NORM_CHAR('\v', 'v');
                                break;
                            case '\r':
                            case '\n':
                                printError("Strings cannot span multiple lines! "
                                           "Please replace the newline with '\\n' or '\\r\\n'!");
                                // This will never be reached...
                                break;
                            default:
                                if (isEscaped) {
                                    printError(std::string("Unknown escape character '\\") +
                                               static_cast<char>(_sourceCode[_currentIndex]) + "'!");
                                } else {
                                    tmpString += _sourceCode[_currentIndex];
                                }
                                break;
                        }
                    }

                    // If we reach this point then the text file most likely reached EOF without an ending double quote
                    errorUnexpectedEOF();
                }
                case '\'': {
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    ++_currentIndex;
                    ++_currentColumn;

                    ERROR_IF_EOF();

                    // Disallow empty char constants...
                    if (_sourceCode[_currentIndex] == '\'') {
                        printError("Char constants cannot be empty!");
                    }

                    unsigned int resultChar = 0;

                    // TODO: Support Unicode and ASCII escape sequences
                    switch (_sourceCode[_currentIndex]) {
                        case '\\':
                            ++_currentIndex;
                            ++_currentColumn;

                            ERROR_IF_EOF();

                            switch (_sourceCode[_currentIndex]) {
                                case '"':
                                    // Within a char " can optionally be escaped or unescaped
                                    resultChar = '\"';
                                    break;
                                case '\\':
                                    resultChar = '\\';
                                    break;
                                case '\'':
                                    resultChar = '\'';
                                    break;
                                case '0':
                                    resultChar = '\0';
                                    break;
                                case 'a':
                                    resultChar = '\a';
                                    break;
                                case 'b':
                                    resultChar = '\b';
                                    break;
                                case 'f':
                                    resultChar = '\f';
                                    break;
                                case 'n':
                                    resultChar = '\n';
                                    break;
                                case 'r':
                                    resultChar = '\r';
                                    break;
                                case 't':
                                    resultChar = '\t';
                                    break;
                                case 'v':
                                    resultChar = '\v';
                                    break;
                                case '\r':
                                case '\n':
                                    printError("Chars cannot span multiple lines! "
                                               "Please replace the newline with '\\n' or '\\r\\n'!");
                                    // This will never be reached...
                                    break;
                                default:
                                    printError(std::string("Unknown escape character '\\") +
                                               static_cast<char>(_sourceCode[_currentIndex]) + "'!");
                                    // This will never be reached...
                                    break;
                            }

                            ++_currentIndex;
                            ++_currentColumn;

                            break;
                        default:
                            resultChar = static_cast<unsigned int>(_sourceCode[_currentIndex]);
                            ++_currentIndex;
                            ++_currentColumn;
                            break;
                    }

                    ERROR_IF_EOF();

                    if (_sourceCode[_currentIndex] != '\'') {
                        printError("Expected an ending single quote!");
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::CHARACTER, TokenMetaType::VALUE, "", resultChar);
                    }

                    // It shouldn't be possible to reach here.
                    errorUnexpectedEOF();
                    break;
                }
                case '{':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::LCURLY, TokenMetaType::SPECIAL, "{", 0);
                case '}':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::RCURLY, TokenMetaType::SPECIAL, "}", 0);
                case '[':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::LSQUARE, TokenMetaType::SPECIAL, "[", 0);
                case ']':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::RSQUARE, TokenMetaType::SPECIAL, "]", 0);
                case ';':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::SEMICOLON, TokenMetaType::SPECIAL, ";", 0);
                case '(':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::LPAREN, TokenMetaType::SPECIAL, "(", 0);
                case ')':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::RPAREN, TokenMetaType::SPECIAL, ")", 0);
                case '@':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::ATSYMBOL, TokenMetaType::SPECIAL, "@", 0);
                case '`':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::GRAVE, TokenMetaType::SPECIAL, "`", 0);
                case '=':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::EQUALEQUALS, TokenMetaType::OPERATOR, "==", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::EQUALS, TokenMetaType::OPERATOR, "=", 0);
                    }
                case '>':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::GREATEREQUALS, TokenMetaType::OPERATOR, ">=", 0);
                    } else if (_rightShiftEnabled && CHECK_NEXT_CHAR() == '>') {
                        ++_currentIndex;
                        ++_currentColumn;

                        if (CHECK_NEXT_CHAR() == '=') {
                            // TODO: Shouldn't we increment the index and column?
                            RETURN_GENERIC_TOKEN(TokenType::RIGHTEQUALS, TokenMetaType::OPERATOR, ">>=", 0);
                        } else {
                            RETURN_GENERIC_TOKEN(TokenType::RIGHT, TokenMetaType::OPERATOR, ">>", 0);
                        }
                    } else {
                        if (_rightShiftEnabled) {
                            RETURN_GENERIC_TOKEN(TokenType::GREATER, TokenMetaType::OPERATOR, ">", 0);
                        } else {
                            RETURN_GENERIC_TOKEN(TokenType::TEMPLATEEND, TokenMetaType::OPERATOR, ">", 0);
                        }
                    }
                case '<':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::LESSEQUALS, TokenMetaType::OPERATOR, "<", 0);
                    } else if (CHECK_NEXT_CHAR() == '<') {
                        ++_currentIndex;
                        ++_currentColumn;

                        if (CHECK_NEXT_CHAR() == '=') {
                            // TODO: Shouldn't we increment the index and column?
                            RETURN_GENERIC_TOKEN(TokenType::LEFTEQUALS, TokenMetaType::OPERATOR, "<<=", 0);
                        } else {
                            RETURN_GENERIC_TOKEN(TokenType::LEFT, TokenMetaType::OPERATOR, "<<", 0);
                        }
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::LESS, TokenMetaType::OPERATOR, "<", 0);
                    }
                case '!':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::NOTEQUALS, TokenMetaType::OPERATOR, "!=", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::NOT, TokenMetaType::OPERATOR, "!", 0);
                    }
                case '~':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::TILDE, TokenMetaType::OPERATOR, "~", 0);
                case '+':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::PLUSEQUALS, TokenMetaType::OPERATOR, "+=", 0);
                    } else if (CHECK_NEXT_CHAR() == '+') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::PLUSPLUS, TokenMetaType::OPERATOR, "++", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::PLUS, TokenMetaType::OPERATOR, "+", 0);
                    }
                case '-':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::MINUSEQUALS, TokenMetaType::OPERATOR, "-=", 0);
                    } else if (CHECK_NEXT_CHAR() == '-') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::MINUSMINUS, TokenMetaType::OPERATOR, "--", 0);
                    } else if (CHECK_NEXT_CHAR() == '>') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::ARROW, TokenMetaType::OPERATOR, "->", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::MINUS, TokenMetaType::OPERATOR, "-", 0);
                    }
                case '*':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::STAREQUALS, TokenMetaType::OPERATOR, "*=", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::STAR, TokenMetaType::OPERATOR, "*", 0);
                    }
                case '/':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::SLASHEQUALS, TokenMetaType::OPERATOR, "/=", 0);
                    } else if (CHECK_NEXT_CHAR() == '/') {
                        for (_currentColumn = 1, ++_currentLine;
                             _currentIndex < _sourceCode.length();
                             ++_currentIndex) {
                            if (_sourceCode[_currentIndex] == '\r') {
                                if (CHECK_NEXT_CHAR() == '\n') {
                                    ++_currentIndex;
                                    // We'll fall through to the next '\n' check
                                }
                            }

                            if (_sourceCode[_currentIndex] == '\n') {
                                break;
                            }
                        }

                        continue;
                    } else if (CHECK_NEXT_CHAR() == '*') {
                        for (++_currentIndex, ++_currentColumn;
                             _currentIndex < _sourceCode.length();
                             ++_currentIndex, ++_currentColumn) {
                            if (_sourceCode[_currentIndex] == '\r' || _sourceCode[_currentIndex] == '\n') {
                                _currentColumn = 1;
                                ++_currentLine;

                                if (_sourceCode[_currentIndex] == '\r' && CHECK_NEXT_CHAR() == '\n') {
                                    ++_currentIndex;
                                }
                            } else if (_sourceCode[_currentIndex] == '*') {
                                if (CHECK_NEXT_CHAR() == '/') {
                                    ++_currentIndex;
                                    ++_currentColumn;
                                    break;
                                }
                            }
                        }

                        continue;
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::SLASH, TokenMetaType::OPERATOR, "/", 0);
                    }
                case '%':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::PERCENTEQUALS, TokenMetaType::OPERATOR, "%=", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::PERCENT, TokenMetaType::OPERATOR, "%", 0);
                    }
                case '#':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    // TODO: Implement this for '#region'
                    printError("Preprocessor commands not yet supported!");
                    break;
                case ',':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::COMMA, TokenMetaType::SPECIAL, ",", 0);
                case '&':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::AMPERSANDEQUALS, TokenMetaType::OPERATOR, "&=", 0);
                    } else if (CHECK_NEXT_CHAR() == '&') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::AMPERSANDAMPERSAND, TokenMetaType::OPERATOR, "&&", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::AMPERSAND, TokenMetaType::OPERATOR, "&", 0);
                    }
                case '|':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::PIPEEQUALS, TokenMetaType::OPERATOR, "|=", 0);
                    } else if (CHECK_NEXT_CHAR() == '|') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::PIPEPIPE, TokenMetaType::OPERATOR, "||", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::PIPE, TokenMetaType::OPERATOR, "|", 0);
                    }
                case '^':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '=') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::CARETEQUALS, TokenMetaType::OPERATOR, "^=", 0);
                    } else if (CHECK_NEXT_CHAR() == '^') {
                        ++_currentIndex;
                        ++_currentColumn;

                        if (CHECK_NEXT_CHAR() == '=') {
                            // TODO: Shouldn't we increment the index and column?
                            RETURN_GENERIC_TOKEN(TokenType::CARETCARETEQUALS, TokenMetaType::OPERATOR, "^^=", 0);
                        } else {
                            RETURN_GENERIC_TOKEN(TokenType::CARETCARET, TokenMetaType::OPERATOR, "^^", 0);
                        }
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::CARET, TokenMetaType::OPERATOR, "^", 0);
                    }
                case '.':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();
                    RETURN_GENERIC_TOKEN(TokenType::PERIOD, TokenMetaType::OPERATOR, ".", 0);
                case ':':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == ':') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::COLONCOLON, TokenMetaType::OPERATOR, "::", 0);
                    } else {
                        RETURN_GENERIC_TOKEN(TokenType::COLON, TokenMetaType::OPERATOR, ":", 0);
                    }
                case '?':
                    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

                    if (CHECK_NEXT_CHAR() == '?') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::QUESTIONQUESTION, TokenMetaType::OPERATOR, "??", 0);
                    } else if (CHECK_NEXT_CHAR() == '.') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::QUESTIONPERIOD, TokenMetaType::OPERATOR, "?.", 0);
                    } else if (CHECK_NEXT_CHAR() == '[') {
                        ++_currentIndex;
                        ++_currentColumn;
                        RETURN_GENERIC_TOKEN(TokenType::QUESTIONLSQUARE, TokenMetaType::OPERATOR, "?[", 0);
                    } else if (CHECK_NEXT_CHAR() == '-') {
                        ++_currentIndex;
                        ++_currentColumn;

                        if (CHECK_NEXT_CHAR() == '>') {
                            ++_currentIndex;
                            ++_currentColumn;

                            RETURN_GENERIC_TOKEN(TokenType::QUESTIONARROW, TokenMetaType::OPERATOR, "?->", 0);
                        } else {
                            --_currentIndex;
                            --_currentColumn;
                        }
                    }

                    RETURN_GENERIC_TOKEN(TokenType::QUESTION, TokenMetaType::OPERATOR, "?", 0);
                default: {
                    char checkChar = _sourceCode[_currentIndex];

                    if (isalnum(checkChar) || checkChar == '_') {
                        tmpTokenText += _sourceCode[_currentIndex];
                    } else {
                        printError("'" + std::string(1, checkChar) + "' is not a valid token character!");
                    }
                    break;
                }
            }
        }
    }

    // Before checking for EOF check to see if 'tmpTokenText' has a value we can return
    PARSE_AND_RETURN_IF_TOKEN_TEXT_NOT_EMPTY();

    // Check to see if the EOF has been reached and return EOF if it has
    CHECK_AND_RETURN_EOF();

    // This technically should never be reached...
    return result;
}

Token Lexer::parseToken(std::string& tokenText, TextPosition startPosition, bool hasLeadingWhitespace) {
    Token result(TokenType::NIL, TokenMetaType::NIL, {}, 0,
                 startPosition, TextPosition(_currentIndex, _currentLine, _currentColumn), hasLeadingWhitespace);

    if (std::isdigit(static_cast<unsigned char>(tokenText[0]))) {
        result.metaType = TokenMetaType::VALUE;
        result.tokenType = TokenType::NUMBER;
        result.currentSymbol = tokenText;
        // Known Modifiers --------------------------------------------------------------------------------------------
    } else if (tokenText == "public") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::PUBLIC;
        result.currentSymbol = "public";
    } else if (tokenText == "private") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::PRIVATE;
        result.currentSymbol = "private";
    } else if (tokenText == "protected") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::PROTECTED;
        result.currentSymbol = "protected";
    } else if (tokenText == "internal") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::INTERNAL;
        result.currentSymbol = "internal";
    } else if (tokenText == "static") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::STATIC;
        result.currentSymbol = "static";
    } else if (tokenText == "const") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::CONST;
        result.currentSymbol = "const";
    } else if (tokenText == "mut") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::MUT;
        result.currentSymbol = "mut";
    } else if (tokenText == "immut") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::IMMUT;
        result.currentSymbol = "immut";
    } else if (tokenText == "extern") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::EXTERN;
        result.currentSymbol = "extern";
    } else if (tokenText == "volatile") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::VOLATILE;
        result.currentSymbol = "volatile";
    } else if (tokenText == "abstract") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::ABSTRACT;
        result.currentSymbol = "abstract";
    } else if (tokenText == "sealed") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::SEALED;
        result.currentSymbol = "sealed";
    } else if (tokenText == "virtual") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::VIRTUAL;
        result.currentSymbol = "virtual";
    } else if (tokenText == "override") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::OVERRIDE;
        result.currentSymbol = "override";
    } else if (tokenText == "in") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::IN;
        result.currentSymbol = "in";
    } else if (tokenText == "out") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::OUT;
        result.currentSymbol = "out";
    } else if (tokenText == "ref") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::REF;
        result.currentSymbol = "ref";
    } else if (tokenText == "inout") {
        result.metaType = TokenMetaType::MODIFIER;
        result.tokenType = TokenType::INOUT;
        result.currentSymbol = "inout";
    // Known Keywords ---------------------------------------------------------------------------------------------
    } else if (tokenText == "trait") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TRAIT;
        result.currentSymbol = "trait";
    } else if (tokenText == "class") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::CLASS;
        result.currentSymbol = "class";
    } else if (tokenText == "struct") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::STRUCT;
        result.currentSymbol = "struct";
    } else if (tokenText == "union") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::UNION;
        result.currentSymbol = "union";
    } else if (tokenText == "enum") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::ENUM;
        result.currentSymbol = "enum";
    } else if (tokenText == "operator") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::OPERATOR;
        result.currentSymbol = "operator";
    } else if (tokenText == "prefix") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::PREFIX;
        result.currentSymbol = "prefix";
    } else if (tokenText == "infix") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::INFIX;
        result.currentSymbol = "infix";
    } else if (tokenText == "postfix") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::POSTFIX;
        result.currentSymbol = "postfix";
    } else if (tokenText == "explicit") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::EXPLICIT;
        result.currentSymbol = "explicit";
    } else if (tokenText == "implicit") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::IMPLICIT;
        result.currentSymbol = "implicit";
    } else if (tokenText == "namespace") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::NAMESPACE;
        result.currentSymbol = "namespace";
    } else if (tokenText == "sizeof") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::SIZEOF;
        result.currentSymbol = "sizeof";
    } else if (tokenText == "alignof") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::ALIGNOF;
        result.currentSymbol = "alignof";
    } else if (tokenText == "offsetof") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::OFFSETOF;
        result.currentSymbol = "offsetof";
    } else if (tokenText == "nameof") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::NAMEOF;
        result.currentSymbol = "nameof";
    } else if (tokenText == "traitsof") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TRAITSOF;
        result.currentSymbol = "traitsof";
    } else if (tokenText == "if") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::IF;
        result.currentSymbol = "if";
    } else if (tokenText == "else") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::ELSE;
        result.currentSymbol = "else";
    } else if (tokenText == "do") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::DO;
        result.currentSymbol = "do";
    } else if (tokenText == "repeat") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::REPEAT;
        result.currentSymbol = "repeat";
    } else if (tokenText == "while") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::WHILE;
        result.currentSymbol = "while";
    } else if (tokenText == "for") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::FOR;
        result.currentSymbol = "for";
    } else if (tokenText == "switch") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::SWITCH;
        result.currentSymbol = "switch";
    } else if (tokenText == "case") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::CASE;
        result.currentSymbol = "case";
    } else if (tokenText == "default") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::DEFAULT;
        result.currentSymbol = "default";
    } else if (tokenText == "continue") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::CONTINUE;
        result.currentSymbol = "continue";
    } else if (tokenText == "break") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::BREAK;
        result.currentSymbol = "break";
    } else if (tokenText == "goto") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::GOTO;
        result.currentSymbol = "goto";
    } else if (tokenText == "return") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::RETURN;
        result.currentSymbol = "return";
    } else if (tokenText == "asm") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::ASM;
        result.currentSymbol = "asm";
    } else if (tokenText == "import") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::IMPORT;
        result.currentSymbol = "import";
    } else if (tokenText == "as") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::AS;
        result.currentSymbol = "as";
    } else if (tokenText == "is") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::IS;
        result.currentSymbol = "is";
    } else if (tokenText == "has") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::HAS;
        result.currentSymbol = "has";
    } else if (tokenText == "try") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TRY;
        result.currentSymbol = "try";
    } else if (tokenText == "catch") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::CATCH;
        result.currentSymbol = "catch";
    } else if (tokenText == "finally") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::FINALLY;
        result.currentSymbol = "finally";
    } else if (tokenText == "throw") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::THROW;
        result.currentSymbol = "throw";
    } else if (tokenText == "throws") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::THROWS;
        result.currentSymbol = "throws";
    } else if (tokenText == "requires") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::REQUIRES;
        result.currentSymbol = "requires";
    } else if (tokenText == "ensures") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::ENSURES;
        result.currentSymbol = "ensures";
    } else if (tokenText == "where") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::WHERE;
        result.currentSymbol = "where";
    } else if (tokenText == "func") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::FUNC;
        result.currentSymbol = "func";
    } else if (tokenText == "prop") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::PROP;
        result.currentSymbol = "prop";
    } else if (tokenText == "let") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::LET;
        result.currentSymbol = "let";
    } else if (tokenText == "var") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::VAR;
        result.currentSymbol = "var";
    } else if (tokenText == "init") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::INIT;
        result.currentSymbol = "init";
    } else if (tokenText == "deinit") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::DEINIT;
        result.currentSymbol = "deinit";
    } else if (tokenText == "call") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::CALL;
        result.currentSymbol = "call";
    } else if (tokenText == "subscript") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::SUBSCRIPT;
        result.currentSymbol = "subscript";
    } else if (tokenText == "typealias") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TYPEALIAS;
        result.currentSymbol = "typealias";
    } else if (tokenText == "typesuffix") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TYPESUFFIX;
        result.currentSymbol = "typesuffix";
    } else if (tokenText == "extension") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::EXTENSION;
        result.currentSymbol = "extension";
    } else if (tokenText == "fallthrough") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::FALLTHROUGH;
        result.currentSymbol = "fallthrough";
    } else if (tokenText == "true") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::TRUE;
        result.currentSymbol = "true";
    } else if (tokenText == "false") {
        result.metaType = TokenMetaType::KEYWORD;
        result.tokenType = TokenType::FALSE;
        result.currentSymbol = "false";
    } else {
        result.metaType = TokenMetaType::VALUE;
        result.tokenType = TokenType::SYMBOL;
        result.currentSymbol = tokenText;
    }

    return result;
}
