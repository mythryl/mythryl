package tokens = tokens
package Interface = Interface
include Interface

type Source_Position = Interface.Source_Position
  type Semantic_Value = tokens::Semantic_Value
  type Token( X, Y ) = tokens::Token( X, Y )
type Lex_Result= Token( Semantic_Value, Source_Position )

  eof = fn () => tokens::EOF(*line,*line)
fun makeInt (s : String) = s

%%
%header (generic package FolLexFun(package tokens: Fol_Tokens
			   package interface: Interface) : Lexer);
lcstart=[a-z!&$+/<=>?@~|#*`]|\-;
ucstart=[A-Z_];
idchars={lcstart}|{ucstart}|[0-9];
lcid={lcstart}{idchars}*;
ucid={ucstart}{idchars}*;
ws=[\t\ ]*;
num=[0-9]+;
%%
<INITIAL>{ws}	=> (lex());
<INITIAL>\n	=> (next_line(); lex());
<INITIAL>":-"	=> (tokens::BACKARROW(*line,*line));
<INITIAL>","	=> (tokens::COMMA(*line,*line));
<INITIAL>";"	=> (tokens::SEMICOLON(*line,*line));
<INITIAL>"."    => (tokens::DOT(*line,*line));
<INITIAL>"("	=> (tokens::LPAREN(*line,*line));
<INITIAL>")"	=> (tokens::RPAREN(*line,*line));
<INITIAL>"->"	=> (tokens::ARROW(*line,*line));
<INITIAL>"=>"	=> (tokens::DOUBLEARROW(*line,*line));
<INITIAL>"|"	=> (tokens::BAR(*line,*line));
<INITIAL>"true" => (tokens::TRUE(*line,*line));
<INITIAL>"forall" => (tokens::FORALL(*line,*line));
<INITIAL>"exists" => (tokens::EXISTS(*line,*line));
<INITIAL>{lcid} => (tokens::LCID (yytext,*line,*line));
<INITIAL>{ucid} => (tokens::UCID (yytext,*line,*line));
<INITIAL>{num}	=> (tokens::INT (makeInt yytext,*line,*line));
<INITIAL>.	=> (error ("ignoring illegal character" + yytext,
			   *line,*line); lex());
