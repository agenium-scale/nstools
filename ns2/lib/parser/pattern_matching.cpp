// MIT License
//
// Copyright (c) 2021 Agenium Scale
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <ns2/parser/analyser.hpp>
#include <ns2/parser/pattern_matching.hpp>
#include <ns2/parser/skip.hpp>
#include <ns2/parser/vector.hpp>

#include <algorithm>
#include <cassert>
#include <ctype.h>
#include <iostream>

using namespace ns2::ast;
using namespace ns2::parser::skip;

namespace ns2 {
namespace parser {
namespace match {

// ----------------------------------------------------------------------------

bool is_number(std::string const &str) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (isdigit(str[i]) == 0)
      return false;
  }
  return true;
}

// ----------------------------------------------------------------------------

bool is_identifier(std::string const &var) {
  if (var.empty())
    return false;
  if (!isalpha(var[0]) && var[0] != '_')
    return false;
  for (size_t ix = 1; ix < var.size(); ++ix) {
    if (!(isalpha(var[ix]) || isdigit(var[ix])) && var[ix] != '_')
      return false;
  }
  return true;
}
// ----------------------------------------------------------------------------

// https://en.cppreference.com/w/cpp/keyword
bool is_keyword(std::string const &str) {
  std::vector<std::string> keywords;
  keywords.reserve(50);
  keywords.push_back("auto");
  keywords.push_back("const");
  keywords.push_back("continue");
  keywords.push_back("catch");
  keywords.push_back("while");
  keywords.push_back("if");
  keywords.push_back("switch");
  keywords.push_back("for");
  keywords.push_back("false");
  keywords.push_back("true");
  keywords.push_back("extern");
  keywords.push_back("do");
  keywords.push_back("return");
  keywords.push_back("struct");
  keywords.push_back("template");
  keywords.push_back("enum");
  keywords.push_back("goto");
  keywords.push_back("break");
  keywords.push_back("case");
  keywords.push_back("namespace");
  keywords.push_back("throw");
  keywords.push_back("catch");
  keywords.push_back("this");
  keywords.push_back("static");
  keywords.push_back("return");
  keywords.push_back("virtual");
  keywords.push_back("using");
  keywords.push_back("typedef");
  keywords.push_back("new");
  keywords.push_back("delete");
  keywords.push_back("volatile");
  keywords.push_back("extern");
  keywords.push_back("typename");
  keywords.push_back("static");
  keywords.push_back("goto");
  keywords.push_back("friend");
  keywords.push_back("inline");
  keywords.push_back("class");
  keywords.push_back("typeid");

  for (size_t i = 0; i < keywords.size(); ++i) {
    if (keywords[i] == str) {
      return true;
    }
  }

  return false;
}
// ----------------------------------------------------------------------------

bool is_operator(lexer::lexeme_t::kind_t const &k) {
  std::vector<lexer::lexeme_t::kind_t> operators;
  operators.push_back(lexer::lexeme_t::KIND_T_PLUS);
  operators.push_back(lexer::lexeme_t::KIND_T_MINUS);
  operators.push_back(lexer::lexeme_t::KIND_T_ASTERISK);
  operators.push_back(lexer::lexeme_t::KIND_T_DOUBLEDOT);
  operators.push_back(lexer::lexeme_t::KIND_T_PERCENT);
  operators.push_back(lexer::lexeme_t::KIND_T_SLASH);

  for (size_t i = 0; i < operators.size(); ++i) {
    if (operators[i] == k) {
      return true;
    }
  }

  return false;
}

/*bool is_unary_operator(std::string const &str) {
  std::vector<std::string> operators = {"--", "++", "!", "-"};
  return std::find(operators.begin(), operators.end(), str) != operators.end();
}*/

// ----------------------------------------------------------------------------

bool is_var_declaration_keyword(std::string const &str) {
  return str == "static" || str == "const" || str == "volatile" ||
         str == "extern";
}

// ----------------------------------------------------------------------------

bool is_compound_stmt(size_t *position,
                      analyser::vector_index_lexeme_t const &lp,
                      std::vector<lexer::lexeme_t *> const &lexemes,
                      compound_stmt_t **node, int nb_lcurly) {
  assert(position != NULL);
  try {

    type_t function_type;
    return_stmt_t *return_stmt = NULL;
    std::vector<ast_node_t *> body;

    size_t i = *position;
    if (i >= lp.size()) {
      return false;
    }

    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_LEFTCURLY) {
      return false;
    }
    i++;
    nb_lcurly++;

    while (i < lp.size()) {
      if ((lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTCURLY &&
           nb_lcurly == 0) ||
          is_return_stmt(&i, lp, lexemes, &return_stmt)) {
        break;
      }
      // Left Curly => ++
      if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LEFTCURLY) {
        nb_lcurly++;
      } else if (lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTCURLY) {
        nb_lcurly--;
      }

      // Expression
      expr_t *new_node = NULL;
      is_expr(&i, lp, lexemes, &new_node);
      if (new_node != NULL) {
        body.push_back(new_node);
      }
      i++;
    }

    if (i >= lp.size() && nb_lcurly != 0) {
      return false;
    }

    if (return_stmt != NULL) {
      body.push_back(return_stmt);
      while (i < lp.size() &&
             lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTCURLY &&
             nb_lcurly == 0) {
        if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LEFTCURLY) {
          nb_lcurly++;
        }

        if (lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTCURLY) {
          nb_lcurly--;
        }
        i++;
      }
    }
    (*node) = new compound_stmt_t(body);
    (*node)->pos_start = int(lp[*position].first);
    (*node)->pos_end = int(lp[i].first - 1);
    *position = i;
    (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool is_return_stmt(size_t *position, analyser::vector_index_lexeme_t const &lp,
                    std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                    return_stmt_t **node) {
  assert(position != NULL);
  try {
    if (lp.size() == 0) {
      return false;
    }

    size_t i = *position;
    // Keyword "return"
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_WORD ||
        lp[i].second->data != "return") {
      return false;
    }
    ++i;
    if (i >= lp.size()) {
      return false;
    }
    // Expression
    expr_t *expr = NULL;
    bool ok = is_expr(&i, lp, lexemes, &expr);
    if (!ok || expr == NULL) {
      return false;
    }
    if (expr != NULL) {
      type_t type = expr->get_type();
      (*node) = new return_stmt_t(expr, type);
    } else {
      return false;
    }

    if (i >= lp.size()) {
      return false;
    }

    (*node)->pos_start = int(lp[*position].first);
    (*node)->pos_end = int(lp[i].first);
    *position = i;
    (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool is_func_declaration(size_t *position,
                         analyser::vector_index_lexeme_t const &lp,
                         std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                         function_decl_t **node) {
  assert(position != NULL);

  try {
    if (lp.size() == 0) {
      return false;
    }

    type_t function_type;
    compound_stmt_t *function_body = NULL;
    lexer::lexeme_t *function_name = NULL;
    std::vector<param_var_decl_t *> params;

    size_t i = *position;
    // static, const, ...
    while (is_var_declaration_keyword(lp[i].second->data)) {
      function_type.qualifier_before.push_back(lp[i].second);
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    }

    // Type
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_WORD ||
        is_keyword(lp[i].second->data)) {
      return false;
    }
    function_type.name = lp[i].second;
    ++i;
    if (i >= lp.size()) {
      return false;
    }

    // T<...>
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LESSTHAN) {
      ++i;
      std::map<std::pair<std::string, std::string>, int> other_betweens;
      other_betweens[std::make_pair("(", ")")] = 0;
      other_betweens[std::make_pair("{", "}")] = 0;
      other_betweens[std::make_pair("[", "]")] = 0;
      std::vector<analyser::vector_index_lexeme_t> args =
          analyser::get_args_between("<", ">", ",", 1, lp, &i, other_betweens);
      if (i >= lp.size()) {
        return false;
      }
      analyser::make_type_t(args, &function_type.template_types);
    }

    // T const ...
    // static, const, ...
    while (is_var_declaration_keyword(lp[i].second->data)) {
      function_type.qualifier_after.push_back(lp[i].second);
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    }

    // T(\*)+ | T&
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_AMPERSAND) {
      function_type.ref = lp[i].second;
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    } else {
      while (lp[i].second->kind == lexer::lexeme_t::KIND_T_ASTERISK) {
        function_type.asterisk.push_back(lp[i].second);
        ++i;
        if (i >= lp.size()) {
          return false;
        }
      };
    }

    // Function name
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_WORD) {
      return false;
    } else {
      function_name = lp[i].second;
    }
    ++i;
    if (i >= lp.size()) {
      return false;
    }

    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_LEFTPAREN) {
      return false;
    }

    i++;
    if (i >= lp.size()) {
      return false;
    }

    // Check if paramdecl
    {
      std::map<std::pair<std::string, std::string>, int> other_betweens;
      other_betweens[std::make_pair("<", ">")] = 0;
      other_betweens[std::make_pair("{", "}")] = 0;
      other_betweens[std::make_pair("[", "]")] = 0;
      std::vector<analyser::vector_index_lexeme_t> args =
          analyser::get_args_between("(", ")", ",", 1, lp, &i, other_betweens);
      for (size_t k = 0; k < args.size(); ++k) {
        type_t t;
        size_t i = 0;
        if ((args[k].size() >= i + 1) &&
            analyser::make_type_t(args[k], &t, &i)) {
          param_var_decl_t *param_var_decl = new param_var_decl_t();
          param_var_decl->set_type(t);
          param_var_decl->set_name(args[k].back().second);
          params.push_back(param_var_decl);
        } else {
          return false;
        }
      }
    }

    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_LEFTCURLY &&
        lp[i].second->kind != lexer::lexeme_t::KIND_T_SEMICOLON) {
      return false;
    }

    if (i >= lp.size()) {
      return false;
    }

    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_SEMICOLON) {
      goto end_func;
    }
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LEFTCURLY) {
      int nb_lcurly = 1;
      bool ok = is_compound_stmt(&i, lp, lexemes, &function_body, nb_lcurly);
      if (i >= lp.size() || function_body == NULL || !ok) {
        return false;
      }
    } else if (lp[i].second->kind != lexer::lexeme_t::KIND_T_SEMICOLON) {
      return false;
    }
  end_func:

    (*node) = new function_decl_t(function_type, function_name, params,
                                  function_body);
    (*node)->pos_start = int(lp[*position].first);
    (*node)->pos_end = int(lp[i].first);
    *position = i;
    (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));

    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

// Declaration Pattern
bool is_var_declaration(size_t *position,
                        analyser::vector_index_lexeme_t const &lp,
                        std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                        ast_node_t **node) {
  assert(position != NULL);
  try {
    if (lp.size() == 0) {
      return false;
    }
    size_t i = *position;
    type_t type;
    std::vector<lexer::lexeme_t *> identifier;
    std::map<std::string, std::vector<lexer::lexeme_t> > inits;
    // static, const, ...
    while (is_var_declaration_keyword(lp[i].second->data)) {
      type.qualifier_before.push_back(lp[i].second);
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    }

    // Type
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_WORD ||
        is_keyword(lp[i].second->data)) {
      return false;
    }

    type.name = lp[i].second;
    ++i;
    if (i >= lp.size()) {
      return false;
    }

    // T<...>
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LESSTHAN) {
      skip_between(&i, "<", ">", lp);
      if (i >= lp.size()) {
        return false;
      }
      // type.template_types.push_back(lp[i].second);
    }

    // T const ...
    if (lp[i].second->data == "const") {
      ++i;
      if (i >= lp.size()) {
        return false;
      }
      type.qualifier_after.push_back(lp[i].second);
    }

    // T(\*)+ | T&
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_AMPERSAND) {
      type.ref = lp[i].second;
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    } else {
      while (lp[i].second->kind == lexer::lexeme_t::KIND_T_ASTERISK) {
        type.asterisk.push_back(lp[i].second);
        ++i;
        if (i >= lp.size()) {
          return false;
        }
      };
    }

    // Identifier variable
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_WORD ||
        !is_identifier(lp[i].second->data)) {
      return false;
    }

    identifier.push_back(lp[i].second);

    ++i;
    // int a[1]; => VAR DECL
    // int a = 0, b[12], c(0), d{9}, e[] = {1, 2, 3},... => DECL_STMT {
    // VAR_DECL1, VAR_DECL2,....}
    int l_par = 0;
    int l_bracket = 0;

    // var (,var)*
    for (; i < lp.size(); ++i) {
      if (lp[i].second->data == ";") {
        break;
      }
      if (lp[i].second->data == ",") {
        ++i;
        if (i < lp.size() &&
            lp[i].second->kind == lexer::lexeme_t::KIND_T_WORD) {
          identifier.push_back(lp[i].second);
          continue;
        } else {
          return false;
        }
      }
      if (lp[i].second->data == "=") {
        ++i;
        skip_argument(&i, lp, l_par, l_bracket);
        i--;
      } else if (lp[i].second->data == "{") {
        l_bracket++;
      } else if (lp[i].second->data == "(") {
        l_par++;
      } else {
        if (!skip_argument(&i, lp, l_par, l_bracket)) {
          return false;
        }
      }
    }
    if (identifier.size() > 1) {
      std::vector<var_decl_t *> decls;
      for (size_t ii = 0; ii < identifier.size(); ii++) {
        var_decl_t *new_var_decl = new var_decl_t(type, identifier[ii]);
        decls.push_back(new_var_decl);
      }
      (*node) = new decl_stmt_t(decls);
    } else {
      (*node) = new var_decl_t(type, identifier[0], NULL);
    }
    (*node)->pos_start = int(lp[*position].first);
    (*node)->pos_end = int(lp[i].first);
    *position = i;
    (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool has_paren_bracket(std::pair<size_t, lexer::lexeme_t *> const &l) {
  return l.second->data == "(" || l.second->data == "{" ||
         l.second->data == ")" || l.second->data == "}";
}

// Assignement Pattern
bool is_assignement(size_t *var_position,
                    analyser::vector_index_lexeme_t const &lp) {
  analyser::vector_index_lexeme_t lp_bis = lp;

  analyser::vector_index_lexeme_t tmp;
  try {
    // Skip method parameters
    for (size_t i = 1; i < lp_bis.size(); ++i) {
      tmp.push_back(lp[i - 1]);
      if (lp_bis[i - 1].second->kind == lexer::lexeme_t::KIND_T_WORD &&
          lp_bis[i].second->data == "(") {
        skip_between(&i, "(", ")", lp_bis);
      }
      if (i + 1 == lp_bis.size()) {
        tmp.push_back(lp[i]);
      }
    }

    // Delete () {} to simplify analysis
    { tmp.erase(std::remove_if(tmp.begin(), tmp.end(), has_paren_bracket)); }

    size_t i = 0;

    //&VAR || *VAR
    do {
      if (i < tmp.size() &&
          (tmp[i].second->data == "*" || tmp[i].second->data == "&")) {
        ++i;
      } else {
        break;
      }
    } while (true);

    // VAR
    if (i < tmp.size() && tmp[i].second->kind == lexer::lexeme_t::KIND_T_WORD) {
      *var_position = i;
      ++i;
    } else {
      return false;
    }

    // VAR(->F | .F | [...])*
    do {
      if (i + 1 < tmp.size() && tmp[i].second->data == "." &&
          tmp[i + 1].second->kind == lexer::lexeme_t::KIND_T_WORD) {
        i += 2;
      } else if (i + 2 < tmp.size() && tmp[i].second->data == "-" &&
                 tmp[i + 1].second->data == ">" &&
                 tmp[i + 2].second->kind == lexer::lexeme_t::KIND_T_WORD) {
        i += 3;
      } else if (i < tmp.size() && tmp[i].second->data == "[") {
        skip_between(&i, "[", "]", tmp);
      } else {
        break;
      }
    } while (true);

    return i < tmp.size() && tmp[i].second->data == "=";
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

// Expression Pattern
bool is_expr(size_t *position, analyser::vector_index_lexeme_t const &lp,
             std::vector<ns2::lexer::lexeme_t *> const &lexemes,
             expr_t **node) {
  // In C++, an expression is everything with a value (variables, binop,
  // unaryop, callexpr with ret, ...).
  assert(position != NULL);
  size_t i = *position;
  try {
    if (lp.size() == 0) {
      return false;
    }

    if (is_binary_op(&i, lp, lexemes, node)) {
      std::cout << "is_binary_op\n";
      *position = i;
      return node != NULL;
    } else if (is_unary_op(&i, lp, lexemes, node)) {
      std::cout << "is_unary_op\n";
      *position = i;
      return node != NULL;
    } else if (is_callexpr(&i, lp, lexemes, node)) {
      std::cout << "is_callexpr\n";
      *position = i;
      return true;
    }
    /*else if (is_lambdaexpr(i, lp)) {
      std::cout << "is_lambdaexpr\n";
      *position = i;

    }*/
    else if (lp[i].second->kind == lexer::lexeme_t::KIND_T_STRING) {
      i++;
      std::cout << "is string literal\n";
      (*node) = new literal_t(lp[i].second, LITERAL_KIND_STRING);
      (*node)->pos_start = int(lp[*position].first);
      (*node)->pos_end = int(lp[i].first - 1);
      *position = i;
      (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
      return true;
    } else if (lp[i].second->kind == lexer::lexeme_t::KIND_T_WORD) {
      std::cout << "is number literal\n";
      if (is_identifier(lp[i].second->data) ||
          is_integer_b10(lp[i].second->data) ||
          is_integer_b16(lp[i].second->data)) {
        i++;
        (*node) = new literal_t(lp[i].second, LITERAL_KIND_NUMBER);
        (*node)->pos_start = int(lp[*position].first);
        (*node)->pos_end = int(lp[i].first - 1);
        *position = i;
        (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
        return true;
      }
    }
    return false;
  } catch (...) {
    return false;
  }
}

bool is_callexpr(size_t *position, analyser::vector_index_lexeme_t const &lp,
                 std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                 expr_t **node) {
  // WORD\((IDENTIFIER | EXPR\(PARAMEXPR*\))*\)
  assert(position != NULL);
  (*node) = NULL;
  try {
    if (lp.size() == 0) {
      return false;
    }
    int nb_lparen = 0;
    lexer::lexeme_t *function_name = NULL;
    std::vector<ast_node_t *> args;
    size_t i = *position;
    // Word => function_name
    if (is_identifier(lp[i].second->data)) {
      function_name = lp[i].second;
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    } else {
      return false;
    }

    // (
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_LEFTPAREN) {
      return false;
    }
    ++i;
    ++nb_lparen;
    if (i >= lp.size()) {
      return false;
    }

    // Params
    if (lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTPAREN) {
      --nb_lparen;
      goto r_paren;
    } else {
      do {
        expr_t *param = NULL;
        if (!is_expr(&i, lp, lexemes, &param)) {
          return false;
        }
        if (param != NULL) {
          args.push_back(param);
        }

        if (lp[i].second->kind == lexer::lexeme_t::KIND_T_RIGHTPAREN) {
          --nb_lparen;
        }

        if (lp[i].second->kind == lexer::lexeme_t::KIND_T_LEFTPAREN) {
          ++nb_lparen;
        }

        if (lp[i].second->kind == lexer::lexeme_t::KIND_T_COLON &&
            lp[i].second->data == ",") {
          i++;
        }

        if (i >= lp.size()) {
          return false;
        }
      } while (lp[i].second->kind != lexer::lexeme_t::KIND_T_RIGHTPAREN &&
               !(lp[i].second->kind == lexer::lexeme_t::KIND_T_PUNCTUATION &&
                 lp[i].second->data == ";") &&
               i < lp.size() && nb_lparen > 0);
    }

    // )
  r_paren:
    if (lp[i].second->kind != lexer::lexeme_t::KIND_T_RIGHTPAREN &&
        nb_lparen > 0) {
      return false;
    }
    ++i;
    if (i > lp.size()) {
      return false;
    }

    (*node) = new callexpr_t(function_name, args);
    (*node)->pos_start = int(lp[*position].first);
    (*node)->pos_end = int(lp[i].first - 1);
    *position = i;
    (*node)->set_data(slice(lexemes, (*node)->pos_start, (*node)->pos_end));
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool is_binary_op(size_t *position, analyser::vector_index_lexeme_t const &lp,
                  std::vector<ns2::lexer::lexeme_t *> const & /*lexemes*/,
                  expr_t ** /*node*/) {
  // (IDENTIFIER | CALLEXPR | LITTERAL) OP (IDENTIFIER | CALLEXPR | LITTERAL)
  assert(position != NULL);

  try {
    if (lp.size() == 0) {
      return false;
    }

    size_t i = *position;
    // (IDENTIFIER | CALLEXPR | LITTERAL) => (EXPR)
    if (true /*is_callexpr(&i, lp)*/) {
      if (i >= lp.size()) {
        return false;
      }
      goto op;
    }
    if (is_identifier(lp[i].second->data) || is_number(lp[i].second->data) ||
        lp[i].second->kind == lexer::lexeme_t::KIND_T_STRING) {
      ++i;
      if (i >= lp.size()) {
        return false;
      }
    } else {
      return false;
    }

  op:
    // OP
    if (!is_operator(lp[i].second->kind)) {
      return false;
    }
    ++i;
    if (i >= lp.size()) {
      return false;
    }

    // (IDENTIFIER | CALLEXPR | LITTERAL)
    if (true /*is_callexpr(&i, lp)*/) {
      if (i > lp.size()) {
        return false;
      }
      goto end;
    }
    if (is_identifier(lp[i].second->data) || is_number(lp[i].second->data) ||
        lp[i].second->kind == lexer::lexeme_t::KIND_T_STRING) {
      ++i;
      if (i > lp.size()) {
        return false;
      }
    } else {
      return false;
    }
  end:

    *position = i;
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool is_unary_op(size_t *position, analyser::vector_index_lexeme_t const &lp,
                 std::vector<ns2::lexer::lexeme_t *> const & /*lexemes*/,
                 expr_t ** /*node*/) {
  // OP (IDENTIFIER | CALLEXPR) or (IDENTIFIER | CALLEXPR) OP
  assert(position != NULL);
  bool op_before = false;

  try {
    if (lp.size() == 0) {
      return false;
    }
    size_t i = *position;
    // (IDENTIFIER | CALLEXPR | LITTERAL)
    if (true /*is_callexpr(&i, lp)*/) {
      if (i >= lp.size()) {
        return false;
      }
      op_before = true;
      goto op;
    } else if (is_identifier(lp[i].second->data) ||
               is_number(lp[i].second->data) ||
               lp[i].second->kind == lexer::lexeme_t::KIND_T_STRING) {
      ++i;
      if (i >= lp.size()) {
        return false;
      }
      op_before = true;
    }

  op:
    // OP
    if (!is_operator(lp[i].second->kind)) {
      return false;
    }
    ++i;
    if (i >= lp.size()) {
      return false;
    }

    // (IDENTIFIER | CALLEXPR | LITTERAL)
    if (true /*is_callexpr(&i, lp)*/ && !op_before) {
      if (i > lp.size()) {
        return false;
      }
      goto end;
    }
    if ((is_identifier(lp[i].second->data) || is_number(lp[i].second->data) ||
         lp[i].second->kind == lexer::lexeme_t::KIND_T_STRING) &&
        !op_before) {
      ++i;
      if (i > lp.size()) {
        return false;
      }
    } else {
      return false;
    }
  end:

    *position = i;
    return true;
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------

bool is_lambdaexpr(size_t *i, analyser::vector_index_lexeme_t const &lp) {
  // \[[& | = | (& | =){0, 1}IDENTIFIER]\] (PARAM_VAR_DECL*) (-> TYPE){0,1}
  // \{COMPOUND_STMT\}
  assert(i != NULL);

  try {
    if (lp.size() == 0) {
      return false;
    }

    return true;
  } catch (...) {
    return false;
  }
}

// ---------------------------------------------------------------------------

bool is_instruction_block(ns2::lexer::lexeme_t *lex) {
  return lex != NULL &&
         (lex->data == "for" || lex->data == "while" || lex->data == "if");
}

// ----------------------------------------------------------------------------

bool is_integer_b10(std::string const &str) {
  if (str.empty()) {
    return false;
  }
  for (size_t i = 0; i < str.size(); ++i) {
    if (!isdigit(str[i])) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------------------

bool is_integer_b16(std::string const &str) {
  if (str.empty()) {
    return false;
  }

  size_t i = 0;
  if (str[i] != '0') {
    return false;
  }
  ++i;
  if (i + 1 >= str.size() || (str[i] != 'x' && str[i] != 'X')) {
    return false;
  }
  ++i;
  for (; i < str.size(); ++i) {
    if (!isdigit(str[i]) && str[i] != 'a' && str[i] != 'A' && str[i] != 'b' &&
        str[i] != 'B' && str[i] != 'c' && str[i] != 'C' && str[i] != 'd' &&
        str[i] != 'D' && str[i] != 'e' && str[i] != 'E' && str[i] != 'f' &&
        str[i] != 'F') {
      return false;
    }
  }

  return true;
}

// ----------------------------------------------------------------------------

} // namespace match
} // namespace parser
} // namespace ns2
