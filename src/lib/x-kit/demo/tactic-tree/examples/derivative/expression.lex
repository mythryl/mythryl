

       type Source_Position = Int;

       type Semantic_Value = tokens::Semantic_Value;
       type Token( X, Y ) = tokens::Token( X, Y );
       type Lex_Result = tokens::Token( Semantic_Value, Source_Position );

       pos = 0;

       fun error(err_str,_,_) = output(std_out,("error: "^err_str^"\n"));

type Lex_Arg =  {comment_nesting_depth : int ref, 
	        lineNum : int ref,
                stringtext : string ref,
	        linePos : int list ref, #  offsets of lines in file 
	        err: Source_Position * Source_Position * string -> unit
               } 

type Arg = Lex_Arg

eof = \\ ({comment_nesting_depth,err,linePos,lineNum,stringtext}: Lex_Arg) => 
  let pos = hd(*linePos) in tokens::EOF(pos,pos) end 


       %%
       %package expressionLex
       %header (functor expressionLexFun(package tokens : Expression_Tokens));
       %arg ({comment_nesting_depth,lineNum,err,linePos,stringtext});
       var = [A-Za-df-z];
       num = [0-9][0-9]*;
       ws = [\ \t\n];
       %%

"+" => (tokens::PLUS(0,0));
"-" => (tokens::MINUS(0,0));
"*" => (tokens::TIMES(0,0));
"/" => (tokens::DIVIDE(0,0));
"^" => (tokens::EXP (0,0));
"e" => (tokens::E(0,0));
"~" => (tokens::NEG(0,0));
"cos" => (tokens::COS(0,0));
"sin" => (tokens::SIN(0,0));


{num} => 
	(tokens::NUM (revfold (\\(a,r)=>ord(a)-ord("0")+10*r) (explode yytext) 0,0,0));       

"(" => (tokens::LPAREN(0,0));
")" => (tokens::RPAREN(0,0));

{ws}+ => (continue());
{var} => (tokens::VAR(yytext,0,0));
