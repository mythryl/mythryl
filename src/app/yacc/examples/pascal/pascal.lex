package tokens = tokens
type Source_Position = Int
  type Semantic_Value = tokens::Semantic_Value
  type Token( X, Y )  = tokens::Token( X, Y )
type Lex_Result = Token( Semantic_Value, Source_Position )

include tokens

lineNum = REF 0
eof = fn () => EOF(*lineNum,*lineNum)


package KeyWord : sig
	     		my find : String ->
				 (int * int -> (Semantic_Value,int) token) option
	  	    end =
  pkg

	TableSize = 211
	HashFactor = 5

	hash = fn s =>
	   fold_forward (fn (c,v)=>(v*HashFactor+(ord c)) mod TableSize) 0 (explode s)


	hashtable = rw_vector.array(TableSize,NIL) :
		 (String * (int * int -> (Semantic_Value,int) token)) list rw_vector.rw_vector


	add = fn (s,v) =>
	 let i = hash s
	 in rw_vector.update(hashtable,i,(s,v) . (rw_vector.sub(hashtable, i)))
	 end

        find = fn s =>
	  let i = hash s
	      fun f ((key,v) . r) = if s=key then THE v else f r
	        | f NIL = NULL
	  in  f (rw_vector.sub(hashtable, i))
	  end
 
	my _ = 
	    (list.apply add
	[("and",YAND),
	 ("array",YARRAY),
	 ("begin",YBEGIN),
	 ("case",YCASE),
	 ("const",YCONST),
	 ("div",YDIV),
	 ("do",YDO),
	 ("downto",YDOWNTO),
	 ("else",YELSE),
	 ("end",YEND),
	 ("extern",YEXTERN),
	 ("file",YFILE),
	 ("for",YFOR),
	 ("forward",YFORWARD),
	 ("function",YFUNCTION),
	 ("goto",YGOTO),
	 ("hex",YHEX),
	 ("if",YIF),
	 ("in",YIN),
	 ("label",YLABEL),
	 ("mod",YMOD),
	 ("NIL",YNIL),
	 ("not",YNOT),
	 ("oct",YOCT),
	 ("of",YOF),
	 ("or",YOR),
	 ("packed",YPACKED),
	 ("procedure",YPROCEDURE),
	 ("program",YPROG),
	 ("record",YRECORD),
	 ("repeat",YREPEAT),
	 ("set",YSET),
	 ("then",YTHEN),
	 ("to",YTO),
	 ("type",YTYPE),
	 ("until",YUNTIL),
	 ("var",YVAR),
	 ("while",YWHILE),
	 ("with",YWITH)
	])
   end
   use KeyWord

%%

%header (generic package PascalLexFun(package tokens: Pascal_Tokens));
%s C B;
alpha=[A-Za-z];
digit=[0-9];
optsign=("+"|"-")?;
integer={digit}+;
frac="."{digit}+;
exp=(e|E){optsign}{digit}+;
octdigit=[0-7];
ws = [\ \t];
%%
<INITIAL>{ws}+	=> (lex());
<INITIAL>\n+	=> (lineNum := *lineNum + (string.size yytext); lex());
<INITIAL>{alpha}+ => (case find yytext of THE v => v(*lineNum,*lineNum)
						  | _ => YID(*lineNum,*lineNum));
<INITIAL>{alpha}({alpha}|{digit})*  => (YID(*lineNum,*lineNum));
<INITIAL>{optsign}{integer}({frac}{exp}?|{frac}?{exp}) => (YNUMB(*lineNum,*lineNum));
<INITIAL>{optsign}{integer} => (YINT(*lineNum,*lineNum));
<INITIAL>{octdigit}+(b|B) => (YBINT(*lineNum,*lineNum));
<INITIAL>"'"([^']|"''")*"'" => (YSTRING(*lineNum,*lineNum));
<INITIAL>"(*" =>   (yybegin C; lex());
<INITIAL>".."	=> (YDOTDOT(*lineNum,*lineNum));
<INITIAL>"."	=> (YDOT(*lineNum,*lineNum));
<INITIAL>"("	=> (YLPAR(*lineNum,*lineNum));
<INITIAL>")"	=> (YRPAR(*lineNum,*lineNum));
<INITIAL>";"	=> (YSEMI(*lineNum,*lineNum));
<INITIAL>","	=> (YCOMMA(*lineNum,*lineNum));
<INITIAL>":"	=> (YCOLON(*lineNum,*lineNum));
<INITIAL>"^"	=> (YCARET(*lineNum,*lineNum));
<INITIAL>"["	=> (YLBRA(*lineNum,*lineNum));
<INITIAL>"]"	=> (YRBRA(*lineNum,*lineNum));
<INITIAL>"~"	=> (YTILDE(*lineNum,*lineNum));
<INITIAL>"<"	=> (YLESS(*lineNum,*lineNum));
<INITIAL>"="	=> (YEQUAL(*lineNum,*lineNum));
<INITIAL>">"	=> (YGREATER(*lineNum,*lineNum));
<INITIAL>"+"	=> (YPLUS(*lineNum,*lineNum));
<INITIAL>"-"	=> (YMINUS(*lineNum,*lineNum));
<INITIAL>"|"	=> (YBAR(*lineNum,*lineNum));
<INITIAL>"*"	=> (YSTAR(*lineNum,*lineNum));
<INITIAL>"/"	=> (YSLASH(*lineNum,*lineNum));
<INITIAL>"{"	=> (yybegin B; lex());
<INITIAL>.	=> (YILLCH(*lineNum,*lineNum));
<C>\n+		=> (lineNum := (*lineNum) + (string.size yytext); lex());
<C>[^()*\n]+	=> (lex());
<C>"(*"		=> (lex());
<C>"*)"		=> (yybegin INITIAL; lex());
<C>[*()]	=> (lex());
<B>\n+		=> (lineNum := (*lineNum) + (string.size yytext); lex());
<B>[^{}\n]+	=> (lex());
<B>"{"		=> (lex());
<B>"}"		=> (yybegin INITIAL; lex());
