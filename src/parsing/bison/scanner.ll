%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <string>
# include "driver.h"
# include "parser.hh"

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
# undef yywrap
# define yywrap() 1

// Pacify warnings in yy_init_buffer (observed with Flex 2.6.4)
// and GCC 7.3.0.
#if defined __GNUC__ && 7 <= __GNUC__
# pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
%}

%option noyywrap nounput batch noinput
/* %option debug */

Whitespace      [ \b\t\f\r]
Letter          [a-zA-Z]
Digit           [0-9]
Identifier      {Letter}({Digit}|{Letter}|_)*
Integer         0|[1-9]{Digit}*
Float           {Digit}+"."{Digit}+
Ascii           [ -~]

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns (yyleng);
%}
%%
%{
  // A handy shortcut to the location held by the driver.
  yy::location& loc = drv.location;
  // Code run each time yylex is called.
  loc.step ();
%}
volatile|super|long|float|double|throws|throw|try|catch|finally|do|switch|break|continue|synchronized  {
             throw yy::parser::syntax_error
               (loc, "invalid token: '" + std::string(yytext) + "' is not a valid joosc token, only a Java token");
}

if    return yy::parser::make_IF(new AstNode(yy::parser::symbol_kind_type::S_IF), loc);
while     return yy::parser::make_WHILE(new AstNode(yy::parser::symbol_kind_type::S_WHILE), loc);
for     return yy::parser::make_FOR(new AstNode(yy::parser::symbol_kind_type::S_FOR), loc);
else    return yy::parser::make_ELSE(new AstNode(yy::parser::symbol_kind_type::S_ELSE), loc);
extends     return yy::parser::make_EXTENDS(new AstNode(yy::parser::symbol_kind_type::S_EXTENDS), loc);
new     return yy::parser::make_NEW(new AstNode(yy::parser::symbol_kind_type::S_NEW), loc);
public    return yy::parser::make_PUBLIC(new AstNode(yy::parser::symbol_kind_type::S_PUBLIC), loc);
implements    return yy::parser::make_IMPLEMENTS(new AstNode(yy::parser::symbol_kind_type::S_IMPLEMENTS), loc);
protected     return yy::parser::make_PROTECTED(new AstNode(yy::parser::symbol_kind_type::S_PROTECTED), loc);
private     return yy::parser::make_PRIVATE(new AstNode(yy::parser::symbol_kind_type::S_PRIVATE), loc);
static    return yy::parser::make_STATIC(new AstNode(yy::parser::symbol_kind_type::S_STATIC), loc);
abstract      return yy::parser::make_ABSTRACT(new AstNode(yy::parser::symbol_kind_type::S_ABSTRACT), loc);
this      return yy::parser::make_THIS(new AstNode(yy::parser::symbol_kind_type::S_THIS), loc);
void      return yy::parser::make_VOID(new AstNode(yy::parser::symbol_kind_type::S_VOID), loc);
final     return yy::parser::make_FINAL(new AstNode(yy::parser::symbol_kind_type::S_FINAL), loc);
import      return yy::parser::make_IMPORT(new AstNode(yy::parser::symbol_kind_type::S_IMPORT), loc);
class      return yy::parser::make_CLASS(new AstNode(yy::parser::symbol_kind_type::S_CLASS), loc);
package     return yy::parser::make_PACKAGE(new AstNode(yy::parser::symbol_kind_type::S_PACKAGE), loc);
interface     return yy::parser::make_INTERFACE(new AstNode(yy::parser::symbol_kind_type::S_INTERFACE), loc);
native    return yy::parser::make_NATIVE(new AstNode(yy::parser::symbol_kind_type::S_NATIVE), loc);
return    return yy::parser::make_RETURN(new AstNode(yy::parser::symbol_kind_type::S_RETURN), loc);
instanceof return yy::parser::make_INSTANCEOF(new AstNode(yy::parser::symbol_kind_type::S_INSTANCEOF), loc);
"{"       return yy::parser::make_OPENING_BRACE(new AstNode(yy::parser::symbol_kind_type::S_OPENING_BRACE), loc);
"}"       return yy::parser::make_CLOSING_BRACE(new AstNode(yy::parser::symbol_kind_type::S_CLOSING_BRACE), loc);
"["       return yy::parser::make_OPENING_BRACKET(new AstNode(yy::parser::symbol_kind_type::S_OPENING_BRACKET), loc);
"]"       return yy::parser::make_CLOSING_BRACKET(new AstNode(yy::parser::symbol_kind_type::S_CLOSING_BRACKET), loc);
"("       return yy::parser::make_OPENING_PAREN(new AstNode(yy::parser::symbol_kind_type::S_OPENING_PAREN), loc);
")"       return yy::parser::make_CLOSING_PAREN(new AstNode(yy::parser::symbol_kind_type::S_CLOSING_PAREN), loc);
";"       return yy::parser::make_SEMI_COLON(new AstNode(yy::parser::symbol_kind_type::S_SEMI_COLON), loc);
"."     return yy::parser::make_DOT(new AstNode(yy::parser::symbol_kind_type::S_DOT), loc);
"="     return yy::parser::make_ASSIGNMENT(new AstNode(yy::parser::symbol_kind_type::S_ASSIGNMENT), loc);

%{ // Types %}
int     return yy::parser::make_INT(new AstNode(yy::parser::symbol_kind_type::S_INT), loc);
boolean    return yy::parser::make_BOOLEAN(new AstNode(yy::parser::symbol_kind_type::S_BOOLEAN), loc);
char    return yy::parser::make_CHAR(new AstNode(yy::parser::symbol_kind_type::S_CHAR), loc); 
byte    return yy::parser::make_BYTE(new AstNode(yy::parser::symbol_kind_type::S_BYTE), loc);
short   return yy::parser::make_SHORT(new AstNode(yy::parser::symbol_kind_type::S_SHORT), loc);
"int\[\]"   return yy::parser::make_ARRAY(new AstNode(yy::parser::symbol_kind_type::S_ARRAY), loc);

%{ // Literals %}
true                return yy::parser::make_TRUE(new AstNode(yy::parser::symbol_kind_type::S_TRUE), loc);
false               return yy::parser::make_FALSE(new AstNode(yy::parser::symbol_kind_type::S_FALSE), loc);
\"({Ascii}|{Whitespace}|[\n\"\'\\\0])*\"     return yy::parser::make_STRING_LITERAL(new AstNode(yy::parser::symbol_kind_type::S_STRING_LITERAL), loc);
{Integer}           return yy::parser::make_INTEGER(new AstNode(yy::parser::symbol_kind_type::S_INTEGER), loc);
null                return yy::parser::make_NULL_TOKEN(new AstNode(yy::parser::symbol_kind_type::S_NULL_TOKEN), loc);
\'{Ascii}\'         return yy::parser::make_CHAR_LITERAL(new AstNode(yy::parser::symbol_kind_type::S_CHAR_LITERAL), loc);

%{ // Comments %}
"//".*     { }
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]      { }
\/\*\*.*\*\/    { } 

%{ // Operators %}
"!"     return yy::parser::make_NEGATE(new AstNode(yy::parser::symbol_kind_type::S_NEGATE), loc);
"+"     return yy::parser::make_PLUS(new AstNode(yy::parser::symbol_kind_type::S_PLUS), loc);
"-"     return yy::parser::make_MINUS(new AstNode(yy::parser::symbol_kind_type::S_MINUS), loc);
"*"     return yy::parser::make_ASTERISK(new AstNode(yy::parser::symbol_kind_type::S_ASTERISK), loc);
"/"     return yy::parser::make_DIVIDE(new AstNode(yy::parser::symbol_kind_type::S_DIVIDE), loc);
"%"     return yy::parser::make_MODULO(new AstNode(yy::parser::symbol_kind_type::S_MODULO), loc);
"<"     return yy::parser::make_LESS_THAN(new AstNode(yy::parser::symbol_kind_type::S_LESS_THAN), loc);
">"     return yy::parser::make_GREATER_THAN(new AstNode(yy::parser::symbol_kind_type::S_GREATER_THAN), loc);
"<="    return yy::parser::make_LESS_THAN_EQUAL(new AstNode(yy::parser::symbol_kind_type::S_LESS_THAN_EQUAL), loc);
">="    return yy::parser::make_GREATER_THAN_EQUAL(new AstNode(yy::parser::symbol_kind_type::S_GREATER_THAN_EQUAL), loc);
"=="    return yy::parser::make_BOOLEAN_EQUAL(new AstNode(yy::parser::symbol_kind_type::S_BOOLEAN_EQUAL), loc);
"!="    return yy::parser::make_NOT_EQUAL(new AstNode(yy::parser::symbol_kind_type::S_NOT_EQUAL), loc);
"&"     return yy::parser::make_AMPERSAND(new AstNode(yy::parser::symbol_kind_type::S_AMPERSAND), loc);
"|"     return yy::parser::make_PIPE(new AstNode(yy::parser::symbol_kind_type::S_PIPE), loc);

{Whitespace}+      loc.step ();
{Identifier}       return yy::parser::make_IDENTIFIER(new AstNode(yy::parser::symbol_kind_type::S_IDENTIFIER, yytext), loc);

.          {
             throw yy::parser::syntax_error
               (loc, "invalid character: " + std::string(yytext));
}
<<EOF>>    return yy::parser::make_EOF (loc);
[\n]+      loc.lines (yyleng); loc.step ();
%%

void Driver::scan_begin() {
  yy_flex_debug = trace_scanning;
  if (file.empty()|| file == "-")
    yyin = stdin;
  else if (!(yyin = fopen (file.c_str (), "r")))
    {
      std::cerr << "cannot open " << file << ": " << strerror(errno) << '\n';
      exit (EXIT_FAILURE);
    }
}

void Driver::scan_end() {
  fclose(yyin);
}

