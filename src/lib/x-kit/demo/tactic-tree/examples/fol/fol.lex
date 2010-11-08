# fol.lex
#
# COPYRIGHT (c) 1992 by AT&T Bell Laboratories.  See COPYRIGHT file for details.





       type Source_Position = Int;

type Semantic_Value = tokens::Semantic_Value;
       type Token( X, Y ) = tokens::Token( X, Y );
       type Lex_Result = tokens::Token( Semantic_Value, Source_Position );

       pos = 0;

       fun error(err_str,_,_) = output(std_out,("error: "^err_str^"\n"));

type Lex_Arg =  {comment_nesting_depth : int ref, 
	        lineNum             : int ref,
                stringtext          : string ref,
	        linePos             : int list ref, #  offsets of lines in file 
	        err                 : Source_Position * Source_Position * string -> unit
               } 

type Arg = Lex_Arg

eof = fn ({comment_nesting_depth,err,linePos,lineNum,stringtext}: Lex_Arg) => 
  let pos = hd(*linePos) in tokens::EOF(pos,pos) end 


       %%
       %package fol_lex
       %header (package_macro fol_lex(package tokens : Fol_Tokens));
       %arg ({comment_nesting_depth,lineNum,err,linePos,stringtext});
       ident = [A-Za-z] [A-Za-z0-9_]*;
       num = [0-9][0-9]*;
       alphanum = [A-Za-z0-9]*;
       ws = [\ \t\n];
       %%

<INITIAL>"&" => (tokens::AND(0,0));
<INITIAL>"|" => (tokens::OR(0,0));
<INITIAL>"~" => (tokens::NEG(0,0));
<INITIAL>"->" => (tokens::IMPLIES(0,0));
<INITIAL>"exists" => (tokens::EXISTS(0,0));
<INITIAL>"forall" => (tokens::FORALL(0,0));

<INITIAL>"=" => (tokens::EQUAL(0,0));
<INITIAL>"<" => (tokens::LANGLE(0,0));
<INITIAL>">" => (tokens::RANGLE(0,0));
<INITIAL>"<=" => (tokens::LEQ(0,0));
<INITIAL>">=" => (tokens::GEQ(0,0));

<INITIAL>"+" => (tokens::PLUS(0,0));
<INITIAL>"-" => (tokens::MINUS(0,0));
<INITIAL>"*" => (tokens::TIMES(0,0));
<INITIAL>"/" => (tokens::DIVIDE(0,0));
<INITIAL>{num} => 
	(tokens::NUM (revfold (fn(a,r)=>ord(a)-ord("0")+10*r) (explode yytext) 0,0,0));       

<INITIAL>":" => (tokens::COLON(0,0));
<INITIAL>"(" => (tokens::LPAREN(0,0));
<INITIAL>")" => (tokens::RPAREN(0,0));
<INITIAL>"." => (tokens::DOT(0,0));
<INITIAL>"," => (tokens::COMMA(0,0));
<INITIAL>"'" => (tokens::QUOTE(0,0));

<INITIAL>"#" => (tokens::FORMPREFIX(0,0));
<INITIAL>"@" => (tokens::TERMPREFIX(0,0));



<INITIAL>{ws}+ => (continue());
<INITIAL>{ident} => (tokens::IDENT(yytext,0,0));
<INITIAL>. => (error (("lexer: ignoring bad character "^yytext),0,0); continue());






