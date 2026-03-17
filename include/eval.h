#pragma once

#include <optional>
#include <vector>
#include <string>

#define CH_ADD "\x80"
#define CH_SUB "\x81"
#define CH_MUL "\x82"
#define CH_DIV "\x83"
#define CH_PWR "\x84"
#define CH_SQRT "\x85"
#define CH_HALF "\x86"
#define CH_ENTER "\x87"
#define CH_CALC "\x88"
#define CH_PWR2 "\x89"
#define CH_RADIAL "\x8A"
#define CH_DIAGONAL "\x8B"
#define CH_BACKSPACE "\x7F"

class CalcEvaluator
{
    struct Token {
        int pos;
        enum Type { VALUE, CHAR } type;
        union {
            double value;
            char ch;
        };
        Token(int _pos, char _ch) : pos(_pos), type(CHAR), ch(_ch) {}
        Token(int _pos, double _value) : pos(_pos), type(VALUE), value(_value) {}
    };
    struct Node {
        Token token;
        Node *left, *right;
        Node(Token _token, Node *_left, Node *_right)
        : token(_token), left(_left), right(_right) {}
    };
    enum class Context {
        START,
        NUMBER,
        AFTER_PAREN,
    };
    const char *expression;
    std::vector<Token> tokens;
    std::vector<Token> stack;
    
    bool match(const char *pattern);
    bool reduce();
    
public:
    std::string error;
    int error_pos;

    CalcEvaluator(const char *_expression) : expression{_expression} {}
    std::optional<double> evaluate();
    bool tokenize();
    void dump_tokens();
    void fail(const std::string &msg, int pos);
};

