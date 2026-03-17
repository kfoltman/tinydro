#include "eval.h"
#include "calc.h"
#include <math.h>
#include <string.h>

void CalcEvaluator::fail(const std::string &msg, int pos)
{
    error = msg;
    error_pos = pos;
    //printf("Error: %s\n", err);
    //printf("%s @ %d\n", expression, pos);
}

bool CalcEvaluator::match(const char *pattern)
{
    size_t slen = strlen(pattern);
    if (stack.size() < slen)
        return false;
    int sofs = stack.size() - slen;
    for (size_t i = 0; i < slen; ++i) {
        if (pattern[i] == '0') {
            if (stack[sofs + i].type != Token::VALUE)
                return false;
        } else {
            if (stack[sofs + i].type != Token::CHAR)
                return false;
            if (stack[sofs + i].ch != pattern[i])
                return false;
        }
    }
    return true;
}

bool CalcEvaluator::reduce()
{
    if(match("!0")) {
        int sofs = stack.size() - 2;
        stack[sofs] = Token{stack[sofs].pos, -stack[sofs + 1].value};
        stack.erase(stack.begin() + sofs + 1);
        return true;
    }
    if(match("=0")) {
        int sofs = stack.size() - 2;
        stack[sofs] = Token{stack[sofs].pos, stack[sofs + 1].value};
        stack.erase(stack.begin() + sofs + 1);
        return true;
    }
    if(match("0" CH_SQRT "0")) {
        int sofs = stack.size() - 3;
        double value1 = stack[sofs].value;
        double value2 = stack[sofs + 2].value;
        if (value1 < 0) {
            fail("Negative root", stack[sofs].pos);
            return false;
        }
        if (value2 < 0) {
            fail(CH_SQRT " of a negative", stack[sofs + 2].pos);
            return false;
        }
        stack[sofs] = Token{stack[sofs + 1].pos, pow(value2, 1.0 / value1)};
        stack.erase(stack.end() - 2, stack.end());
        return true;
    }
    if(match(CH_SQRT "0")) {
        int sofs = stack.size() - 2;
        double value = stack[sofs + 1].value;
        if (value < 0) {
            fail(CH_SQRT " of a negative", stack[sofs].pos);
            return false;
        }
        Token value2{stack[sofs + 1].pos, sqrt(value)};
        stack.erase(stack.end() - 2, stack.end());
        stack.push_back(value2);
        return true;
    }
    if(match("0" CH_PWR2 "0")) {
        int sofs = stack.size() - 3;
        Token value{stack[sofs + 1].pos, pow(stack[sofs].value, stack[sofs + 2].value)};
        stack.erase(stack.end() - 3, stack.end());
        stack.push_back(value);
        return true;
    }
    if(match("/0*0+") || match("/0*0-") || match("/0*0)") || match("/0*0\n")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value / stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("0*0+") || match("0*0-") || match("0*0)") || match("0*0\n")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value * stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("/0/0+") || match("/0/0-") || match("/0/0)") || match("/0/0\n")) {
        int sofs = stack.size() - 4;
        if (stack[sofs + 2].value == 0) {
            fail("Division by zero", stack[sofs].pos + 1);
            return false;
        }
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value * stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("0/0+") || match("0/0-") || match("0/0)") || match("0/0\n")) {
        int sofs = stack.size() - 4;
        if (stack[sofs + 2].value == 0) {
            fail("Division by zero", stack[sofs].pos + 1);
            return false;
        }
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value / stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("-0+0\n") || match("-0+0)")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value - stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("0+0\n") || match("0+0)")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value + stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("-0-0\n") || match("-0-0)")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value + stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("0-0\n") || match("0-0)")) {
        int sofs = stack.size() - 4;
        stack[sofs] = Token{stack[sofs + 1].pos, stack[sofs].value - stack[sofs + 2].value};
        stack.erase(stack.end() - 3, stack.end() - 1);
        return true;
    }
    if(match("(0)")) {
        int sofs = stack.size() - 3;
        stack[sofs] = Token{stack[sofs].pos, stack[sofs + 1].value};
        stack.erase(stack.end() - 2, stack.end());
        return true;
    }
    return false;
}

std::optional<double> CalcEvaluator::evaluate()
{
    std::optional<double> nothing;
    
    if (!tokenize())
        return nothing;
    //dump_tokens();
    for (const Token &t: tokens) {
        stack.push_back(t);
        while(reduce())
            ;
        if (!error.empty())
            return nothing;
    }
    if (match("0\n"))
        return stack[stack.size() - 2].value;
    else {
        fail("Syntax error", stack.empty() ? 0 : stack[0].pos);
        return nothing;
    }
}

void CalcEvaluator::dump_tokens()
{
    int i = 0;
    for (auto t: tokens) {
        if (t.type == Token::CHAR)
            printf("[%d] = %c\n", i, t.ch);
        else
            printf("[%d] = %f\n", i, t.value);
        ++i;
    }
}

bool CalcEvaluator::tokenize()
{
    char tmp[256];
    int start{0};
    Context context = Context::START;
    int i = 0;
    int len = strlen(expression);
    while(i <= len) {
        int tpos = i;
        char ch = expression[i++];
        bool bad_char = false;
        switch(context) {
            case Context::START:
                switch(ch) {
                    case '0'...'9':
                        start = i - 1;
                        context = Context::NUMBER;
                        break;
                    case '+':
                        tokens.push_back(Token(tpos, '=')); // unary plus
                        break;
                    case '-':
                        tokens.push_back(Token(tpos, '!')); // unary minus
                        break;
                    case CH_SQRT[0]:
                        tokens.push_back(Token(tpos, CH_SQRT[0])); // unary minus
                        break;
                    case '(':
                        tokens.push_back(Token(tpos, ch));
                        break;
                    case '\0':
                        break;
                    default:
                        bad_char = true;
                        break;
                }
                break;
            case Context::NUMBER:
                switch(ch) {
                    case '0'...'9': case '.':
                        break;
                    case '+': case '-': case '*': case '/':
                    case CH_SQRT[0]: case CH_PWR2[0]:
                        memcpy(tmp, &expression[start], i - start);
                        tokens.push_back(Token(start, strtod(tmp, nullptr)));
                        tokens.push_back(Token(tpos, ch));
                        start = i;
                        context = Context::START;
                        break;
                    case '\0':
                        memcpy(tmp, &expression[start], i - start);
                        tokens.push_back(Token(start, strtod(tmp, nullptr)));
                        break;
                    case ')':
                        memcpy(tmp, &expression[start], i - start);
                        tokens.push_back(Token(start, strtod(tmp, nullptr)));
                        tokens.push_back(Token(tpos, ')'));
                        start = i;
                        context = Context::AFTER_PAREN;
                        break;
                    default:
                        bad_char = true;
                        break;
                }
                break;
            case Context::AFTER_PAREN:
                switch(ch) {
                    case '+': case '-': case '*': case '/': 
                    case CH_SQRT[0]: case CH_PWR2[0]:
                        tokens.push_back(Token(tpos, ch));
                        start = i;
                        context = Context::START;
                        break;
                    case '\0':
                        break;
                    case ')':
                        tokens.push_back(Token(tpos, ')'));
                        break;
                    default:
                        bad_char = true;
                        break;
                }
                break;
        }
        if (bad_char) {
            fail(std::string("Bad char: '") + ch + "'", tpos);
            return false;
        }

    }
    tokens.push_back(Token(len, '\n'));
    return true;    
}

