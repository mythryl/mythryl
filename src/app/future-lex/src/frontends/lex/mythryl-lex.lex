type Source_Position = int
type Semantic_Value = Tok::Semantic_Value
type Token( X, Y ) = Tok::Token( X, Y )
type Lex_Result = Token( Semantic_Value, Source_Position )

use Tok

eof =   \\ () => eofx(-1,-1)
error = /* \\ (e,l : int,_) =>
      output(std_out,"line " + (makestring l) +
	     ": " + e + "\n") */
     \\ _ => ()

local
text = REF ([] : String list)
in
fun clrAction () = (text := ["("])
fun updAction (str) = (text := str . *text)
fun getAction () = (cat (reverse *text))
end

#  what to do (i.e. switch start states) after recognizing an action 
afterAction = REF (\\ () => ())

#  paren counting for actions 
pcount = REF 0
inquote = REF false
fun inc r = if *inquote then () else r := *r + 1
fun dec r = if *inquote then () else r := *r - 1

package SIS = RegExp::SymSet
fun uniChar s = let
      fun toW32 (c : char::char) : one_word_unt::word = 
	(case c of '0' => 0w0 | '1' => 0w1 | '2' => 0w2 | '3' => 0w3
	 	 | '4' => 0w4 | '5' => 0w5 | '6' => 0w6 | '7' => 0w7
	 	 | '8' => 0w8 | '9' => 0w9 | 'a' => 0w10 | 'A' => 0w10
		 | 'b' => 0w11 | 'B' => 0w11 | 'c' => 0w12 | 'C' => 0w12
		 | 'd' => 0w13 | 'D' => 0w13 | 'e' => 0w14 | 'E' => 0w14
		 | 'f' => 0w15 | 'F' => 0w15
		 | _ => raise exception FAIL "invalid unicode escape sequence")
      fun iter ('u' . _, v) = v
        | iter (c . cs,   v) = iter (cs, ((one_word_unt::from_int 16)*v + (toW32 c))
	| iter _ = raise exception FAIL "invalid unicode escape sequence"
      uni = iter (list::reverse (string::explode s), 0w0)
      in iter (list::reverse (string::explode s), 0w0)
      end

highAscii = SIS::interval(0w128, 0w255)

%%

%header (generic package mllex_lex_g(package Tok: MLLex_TOKENS));
%s DEFS RE RECB CHARILK LEXSTATES ACTION STRING;
%count

ws	= [\ \n\t\013];
alpha	= [a-zA-Z];
num	= [0-9];
hex	= {num} | [a-fA-F];
id	= {alpha}({alpha} | {num} | "_" | "'")*;

%%

<INITIAL> "%%"	=> (yybegin DEFS; lexmark(*yylineno, *yylineno));
<INITIAL> ([^%] | [^%]* % [^%])*
		=> (decls(yytext, *yylineno, *yylineno));

<DEFS> {ws}	=> (lex());
<DEFS> "%%"	=> (yybegin RE; lexmark(*yylineno, *yylineno));
<DEFS> "%s"	=> (yybegin LEXSTATES; states(*yylineno, *yylineno));
<DEFS> "%header" {ws}* "("
		=> (clrAction(); pcount := 1; inquote := false; 
	            yybegin ACTION;
		    afterAction := (\\ () => yybegin DEFS);
		    header(*yylineno, *yylineno));
<DEFS> "%package"
		=> (struct(*yylineno, *yylineno));
<DEFS> "%arg" {ws}* "("
		=> (clrAction(); pcount := 1; inquote := false;
                    yybegin ACTION;
		    afterAction := (\\ () => yybegin DEFS);
		    arg(*yylineno, *yylineno));
<DEFS> "%count" => (count(*yylineno, *yylineno));
<DEFS> "%reject"=> (rejecttok(*yylineno, *yylineno));
<DEFS> "%unicode" 
		=> (unicode(*yylineno, *yylineno));
<defs> "%full"
		=> (lex());
<DEFS> {id}	=> (id(yytext, *yylineno, *yylineno));
<DEFS> "="	=> (yybegin RE; eq(*yylineno, *yylineno));

<RE> {ws}	=> (lex());
<RE> "?"	=> (qmark(*yylineno, *yylineno));
<RE> "*"	=> (star(*yylineno, *yylineno));
<RE> "+"	=> (plus(*yylineno, *yylineno));
<RE> "|"	=> (bar(*yylineno, *yylineno));
<RE> "("	=> (lp(*yylineno, *yylineno));
<RE> ")"	=> (rp(*yylineno, *yylineno));
<RE> "$"	=> (dollar(*yylineno, *yylineno));
<RE> "/"	=> (slash(*yylineno, *yylineno));
<RE> "."	=> (dot(*yylineno, *yylineno));

<RE> "{"	=> (yybegin RECB; lex());
<RE> "\""       => (yybegin STRING; lex());
<RE> "["	=> (yybegin CHARILK; lb(*yylineno, *yylineno));
<RE> "<"	=> (yybegin LEXSTATES; lt(*yylineno, *yylineno));
<RE> ">"	=> (gt(*yylineno, *yylineno));
<RE> "=>" {ws}*	"("
		=> (clrAction(); pcount := 1; inquote := false;
                    yybegin ACTION;
		    afterAction := (\\ () => yybegin RE);
		    arrow(*yylineno, *yylineno));
<RE> ";"	=> (yybegin DEFS; semi(*yylineno, *yylineno));

<RECB>{ws}	=> (lex());
<RECB>{id}	=> (id(yytext, *yylineno, *yylineno));
<RECB>{num}+	=> (reps(the (int::from_string yytext), *yylineno, *yylineno));
<RECB>","	=> (comma(*yylineno, *yylineno));
<RECB>"}"	=> (yybegin RE; rcb(*yylineno, *yylineno));

<CHARILK>"-]"	=> (yybegin RE; rbd(*yylineno, *yylineno));
<CHARILK>"]"	=> (yybegin RE; rb(*yylineno, *yylineno));
<CHARILK>"-"	=> (dash(*yylineno, *yylineno));
<CHARILK>"^"	=> (carat(*yylineno, *yylineno));

<STRING> "\""	=> (yybegin RE; lex());

<RE,STRING,CHARILK>"\\" ({num}{3} | [btnr] | "\\" | "\"")
		=> (char(the (string::from_string yytext), *yylineno, *yylineno));
<RE,STRING,CHARILK>"\\u"{hex}{4}
		=> (unichar(uniChar yytext, *yylineno, *yylineno));
<RE,STRING,CHARILK>"\\".
		=> (char(string::substring (yytext, 1, 1), *yylineno, *yylineno));
<RE,STRING,CHARILK>.	
		=> (char(yytext, *yylineno, *yylineno));

<LEXSTATES>{id} => (lexstate(yytext, *yylineno, *yylineno));
<LEXSTATES>{ws}	=> (lex());
<LEXSTATES> "," => (comma(*yylineno, *yylineno));
<LEXSTATES> ">" => (yybegin RE; gt(*yylineno, *yylineno));
<LEXSTATES> ";" => (yybegin DEFS; semi(*yylineno, *yylineno));

<ACTION> ";"	=> (if *pcount = 0
		    then (*afterAction ();
			  act(getAction(), *yylineno, *yylineno))
		    else (updAction ";"; lex()));
<ACTION> "("	=> (updAction "("; inc pcount; lex());
<ACTION> ")"	=> (updAction ")"; dec pcount; lex());
<ACTION> "\\\"" => (updAction "\\\""; lex());
<ACTION> "\\\\"	=> (updAction "\\\\"; lex());
<ACTION> "\\"	=> (updAction "\\"; lex());
<ACTION> "\""   => (updAction "\""; inquote := not *inquote; lex());
<ACTION> [^;()\"\\]*
		=> (updAction yytext; lex());


