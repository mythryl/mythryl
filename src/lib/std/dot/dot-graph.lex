## dot-graph.lex
## COPYRIGHT (c) 1994 AT&T Bell Labortories.

# Scanner specification for "dot" graph files.

Source_Position = Int;
Semantic_Value = tokens::Semantic_Value;
Token (X,Y)  = tokens::Token(X,Y);
Lex_Result = Token (Semantic_Value, Source_Position);

Lexstate
    =
    { line_num:       Ref(Int),
      stringstart:    Ref(Int),
      comment_state:  Ref(Null_Or(Int)),
      charlist:       Ref(List(String)),
      complain:       String -> Void
    };

Arg = Lexstate;
       

fun make_symbol (s,i)
    =
    tokens::symbol(s,i,i+(size s));


fun add_string (charlist, s: String)
    =
    charlist :=  s ! *charlist;


fun make_string charlist
    =
    cat (reverse *charlist)
    before
        charlist := [];


fun eof ({ line_num, stringstart, comment_state, charlist, complain }: Lexstate)
    =
    {   case *comment_state
	    #
	    THE l =>  complain (sprintf "warning: non-terminated comment in line %d\n" l);
	    _     =>  ();
	esac;

	tokens::eof(0,0);
    };


%%
%header (generic package dotgraph_lex_g(package tokens: Graph_Tokens;));
%arg ({ line_num, stringstart, comment_state, charlist, complain });
%s qs com eolcom;
ws=[\t\ ];
idchars=[A-Za-z_0-9];
id=[_A-Za-z]{idchars}*;
num=[0-9]+;
frac="."{num};
real="-"?{num}?{frac};
whole="-"?{num}"."?;
%%
<initial>\n		=> (line_num := *line_num + 1; continue());
<initial>{ws}+		=> (continue());
<initial>\"		=> (charlist := [];
                            stringstart := yypos;
                            yybegin qs;
                            continue());
<initial>"/*"   	=> (comment_state := THE(*line_num); 
                            yybegin com; 
                            continue());
<initial>"//"   	=> (yybegin eolcom; 
                            continue());
<initial>"@"		=> (tokens::at(yypos,yypos+1));
<initial>"."		=> (tokens::dot(yypos,yypos+1));
<initial>"="		=> (tokens::equal(yypos,yypos+1));
<initial>")"    	=> (tokens::rparen(yypos,yypos+1));
<initial>"]"    	=> (tokens::rbracket(yypos,yypos+1));
<initial>"}"    	=> (tokens::rbrace(yypos,yypos+1));
<initial>"("    	=> (tokens::lparen(yypos,yypos+1));
<initial>"["    	=> (tokens::lbracket(yypos,yypos+1));
<initial>"{"    	=> (tokens::lbrace(yypos,yypos+1));
<initial>","    	=> (tokens::comma(yypos,yypos+1));
<initial>";"    	=> (tokens::semicolon(yypos,yypos+1));
<initial>":"    	=> (tokens::colon(yypos,yypos+1));
<initial>"->"		=> (tokens::edgeop(yypos,yypos+2));
<initial>"--"		=> (tokens::edgeop(yypos,yypos+2));
<initial>edge		=> (tokens::edge(yypos,yypos+4));
<initial>node		=> (tokens::node(yypos,yypos+4));
<initial>strict		=> (tokens::strict(yypos,yypos+6));
<initial>graph		=> (tokens::graph(yypos,yypos+5));
<initial>digraph	=> (tokens::digraph(yypos,yypos+7));
<initial>subgraph	=> (tokens::subgraph(yypos,yypos+8));
<initial>{id}		=> (make_symbol(yytext,yypos));
<initial>{whole}	=> (make_symbol(yytext,yypos));
<initial>{real}		=> (make_symbol(yytext,yypos));

<initial>\h		=> (complain "non-Ascii character";
                   	    continue());
<initial>.		=> (complain ("illegal token: " + (string::to_string yytext));
                    	    continue());

<com>\n			=> (line_num := *line_num + 1; continue());
<com>"*/"		=> (comment_state := NULL; yybegin initial; continue());
<com>.			=> (continue());

<eolcom>\n		=> (line_num := *line_num + 1; yybegin initial; continue());
<eolcom>.		=> (continue());

<qs>\"			=> (yybegin initial; 
                            make_symbol(make_string charlist,*stringstart));
<qs>\n			=> (complain "unclosed string";
			    yybegin initial;
                            make_symbol(make_string charlist,*stringstart));
<qs>[^"\\\n]*		=> (add_string(charlist,yytext); continue());
<qs>\\\n         	=> (continue());
<qs>\\\\         	=> (add_string(charlist,"\\\\"); continue());
<qs>\\\"         	=> (add_string(charlist,"\""); continue());
<qs>\\	         	=> (add_string(charlist,"\\"); continue());

