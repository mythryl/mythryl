package tokens = tokens

type Source_Position = Int
  type Semantic_Value = tokens::Semantic_Value
  type Token( X, Y ) = tokens::Token( X, Y )
type Lex_Result = Token( Semantic_Value, Source_Position )

pos = REF 0
  fun eof () = tokens::EOF(*pos,*pos)
fun error (e,l : int,_) = file.output (file.stdout, string.cat [
	"line ", (int.to_string l), ": ", e, "\n"
      ])

%%
%header (generic package CalcLexFun(package tokens: Calc_TOKENS));
alpha=[A-Za-z];
digit=[0-9];
ws = [\ \t];
%%
\n       => (pos := *pos + 1; lex());
{ws}+    => (lex());
{digit}+ => (tokens::NUM (the (int.from_string yytext), *pos, *pos));

"+"      => (tokens::PLUS(*pos,*pos));
"*"      => (tokens::TIMES(*pos,*pos));
";"      => (tokens::SEMI(*pos,*pos));
{alpha}+ => (if yytext="print"
	     then tokens::PRINT(*pos,*pos)
	     else tokens::ID(yytext,*pos,*pos)
            );
"-"      => (tokens::SUB(*pos,*pos));
"^"      => (tokens::CARAT(*pos,*pos));
"/"      => (tokens::DIV(*pos,*pos));
"."      => (error ("ignoring bad character "$yytext,*pos,*pos);
             lex());


