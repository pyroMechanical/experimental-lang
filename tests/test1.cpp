#include "semant.h"
#define BOOST_TEST_MODULE pilaf_test
#include <boost/test/included/unit_test.hpp>
BOOST_AUTO_TEST_SUITE(lexical_test);
BOOST_AUTO_TEST_CASE(lexical_test_empty)
{
    auto l = pilaf::initLexer("");
    auto t = pilaf::scanToken(&l).type;
    BOOST_CHECK(t == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_test_identifiers)
{
    auto l = pilaf::initLexer("_ __ _t t (+) (*) (&) (:) (->)");
    BOOST_CHECK(pilaf::scanToken(&l).type != pilaf::TokenTypes::IDENTIFIER); // "_" is NOT a plain identifier
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER); // "__" IS an identifier
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER); 
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_ERROR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_ERROR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_test_types)
{
    auto l = pilaf::initLexer("_ __T _T T");
    BOOST_CHECK(pilaf::scanToken(&l).type != pilaf::TokenTypes::TYPE); // "_" is NOT an identifier, it is pilaf::TokenTypes::UNDERSCORE
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::TYPE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::TYPE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::TYPE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_keywords)
{
    auto l = pilaf::initLexer("and break case class continue else false for fn if implement infix let lambda mutable not or prefix postfix return switch struct true typedef using union unsafe while");
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::AND);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::BREAK);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::CASE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::CLASS);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::CONTINUE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::ELSE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_FALSE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::FOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::FN);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IF);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IMPLEMENT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::INFIX);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::LET);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::LAMBDA);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::MUTABLE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::NOT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::PREFIX);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::POSTFIX);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::RETURN);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::SWITCH);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::STRUCT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_TRUE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::TYPEDEF);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::USING);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::UNION);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::UNSAFE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::WHILE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_test_operators)
{
    auto l = pilaf::initLexer("~ ! @ # $ % ^ & * - + = == | [ ] < > ? / ->");
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::AMPERSAND); // '&'
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::STAR); // '*'
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::EQUAL); // '='
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::BRACKET);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::CLOSE_BRACKET);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::ARROW); // '->'
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
    l = pilaf::initLexer("<* >* <& >& ?* ?&");
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OPERATOR);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_test_numbers)
{
    auto l = pilaf::initLexer("1 1.0f 1.0 01234567 0xDeAdBeEf 0b10100");
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::INT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::FLOAT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::DOUBLE);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::OCT_INT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::HEX_INT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::BIN_INT);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_CASE(lexical_test_whitespace)
{
    auto l = pilaf::initLexer("spaces      tabs\t\t\t\t\t\tnewlines\n\n\n\n\n\nend");
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::IDENTIFIER);
    BOOST_CHECK(pilaf::scanToken(&l).type == pilaf::TokenTypes::_EOF);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(parser_test);
BOOST_AUTO_TEST_CASE(parser_test_fixity)
{
    /*  Program
           |
        Binary "$"
       /      \
      x     Binary "^"
           /      \
          y        z */
    auto x = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "x", 1, 1});
    auto y = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "y", 1, 1});
    auto z = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "z", 1, 1});
    auto b1 = std::make_shared<pilaf::BinaryNode>(y, pilaf::Token{pilaf::TokenTypes::OPERATOR, "^", 1, 1}, z); 
    auto b2 = std::make_shared<pilaf::BinaryNode>(x, pilaf::Token{pilaf::TokenTypes::OPERATOR, "$", 1, 1}, b1);
    auto valid = std::make_shared<pilaf::ProgramNode>();
    valid->declarations.push_back(b2);
    BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("infix ($) 2; infix (^) 3; x $ y ^ z;")));
    /*    Program
             |
          Binary "%"
         /      \
      Binary "$" Binary "^"
     /      \   /      \
    x        y z        w*/
    auto result2 = pilaf::parse("infix ($) 3; infix (%) 1; infix (^) 4; x $ y % z ^ w;");
    auto w = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "w", 1, 1});
    auto b3 = std::make_shared<pilaf::BinaryNode>(x, pilaf::Token{pilaf::TokenTypes::OPERATOR, "$", 1, 1}, y);
    auto b4 = std::make_shared<pilaf::BinaryNode>(z, pilaf::Token{pilaf::TokenTypes::OPERATOR, "^", 1, 1}, w);
    auto b5 = std::make_shared<pilaf::BinaryNode>(b3, pilaf::Token{pilaf::TokenTypes::OPERATOR, "%", 1, 1}, b4);
    auto valid2 = std::make_shared<pilaf::ProgramNode>();
    valid2->declarations.push_back(b5);
    BOOST_CHECK(pilaf::compareAST(valid2, result2));
}
BOOST_AUTO_TEST_CASE(parser_test_if)
{
    auto x = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "x", 1, 1});
    auto y = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "y", 1, 1});
    auto z = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "z", 1, 1});
    auto if1 = std::make_shared<pilaf::IfStatementNode>(x, y, z);
    auto valid = std::make_shared<pilaf::ProgramNode>();
    valid->declarations.push_back(if1);
    BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("if(x) y; else z;")));
    if1 = std::make_shared<pilaf::IfStatementNode>(x, y, nullptr);
    valid->declarations.clear();
    auto if2 = std::make_shared<pilaf::IfStatementNode>(x, y, nullptr);
    valid->declarations.push_back(if2);
    BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("if(x) y;")));
}
BOOST_AUTO_TEST_CASE(parser_test_for)
{
    auto x = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "x", 1, 1});
    auto y = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "y", 1, 1});
    auto z = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "z", 1, 1});
    auto for1 = std::make_shared<pilaf::ForStatementNode>(x, y, x, z);
    auto valid = std::make_shared<pilaf::ProgramNode>();
    valid->declarations.push_back(for1);
    BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("for(x; y; x) z;")));
    auto for2 = std::make_shared<pilaf::ForStatementNode>(nullptr, nullptr, nullptr, z);
    auto valid2 = std::make_shared<pilaf::ProgramNode>();
    valid2->declarations.push_back(for2);
    BOOST_CHECK(pilaf::compareAST(valid2, pilaf::parse("for(;;) z;")));
}
//BOOST_AUTO_TEST_CASE(parser_test_switch)
//{
//    auto x = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "x", 1, 1});
//    auto y = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "y", 1, 1});
//    auto z = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "z", 1, 1});
//    auto v = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "v", 1, 1});
//    auto w = std::make_shared<pilaf::VariableNode>(pilaf::Token{pilaf::TokenTypes::IDENTIFIER, "w", 1, 1});
//    auto case1 = std::make_shared<pilaf::CaseNode>(y, z);
//    auto case2 = std::make_shared<pilaf::CaseNode>(v, w);
//    auto cases = std::vector<std::shared_ptr<pilaf::node>>{case1, case2};
//    auto switch1 = std::make_shared<pilaf::SwitchStatementNode>(x, cases);
//    auto valid = std::make_shared<pilaf::ProgramNode>();
//    valid->declarations.push_back(switch1);
//    BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("switch(x) { case y: z; case v: w; }")));
//}
BOOST_AUTO_TEST_CASE(parser_test_types)
{
    //auto result = pilaf::parse("Type");
    //auto t = std::make_shared<pilaf::TyBasic>(std::string("Type"));
    //auto tyNode = std::make_shared<pilaf::TypeNode>(t);
    //auto valid = std::make_shared<pilaf::ProgramNode>();
    //valid->declarations.push_back(tyNode);
    //BOOST_CHECK(pilaf::compareAST(valid, pilaf::parse("Type")));
    //result = pilaf::parse("()");
}
BOOST_AUTO_TEST_CASE(parser_test_unions)
{
    //auto result = pilaf::parse("union Nbool { False, True } let x = True;");
    
}
BOOST_AUTO_TEST_SUITE_END();