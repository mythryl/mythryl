# burg-lex
#
# COPYRIGHT (c) 1995 AT&T Bell Laboratories.
## Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
## released under Gnu Public Licence version 3.
#
# Mythryl-Lex specification for mythryl-burg.



package t 		= tokens;
package e		= error_message;

Source_Position	= Int;
Semantic_Value	= t::Semantic_Value;
Token (X, Y) 	= t::Token (X, Y);
Lex_Result		= Token (Semantic_Value, Source_Position);

my comment_nesting_depth= REF 0;
my line_num		= REF 0;
my verbatim_level	= REF 0;
my percent_count	= REF 0;
my raw_line		= REF "";
my raw_no_newline	= REF FALSE;
my raw:  Ref(List(String)) = REF [];
my reached_eop		= REF FALSE;

fun reset_state ()	= { comment_nesting_depth := 0;
			   line_num             := 0;
			   verbatim_level       := 0;
			   percent_count        := 0;
			   raw_line	       := "";
			   raw_no_newline	       := FALSE;
			   raw		       := [];
			   reached_eop	       := FALSE;
                          };
			   
fun inc (ri as REF i) =   ri := i + 1;
fun dec (ri as REF i) =   ri := i - 1;

fun increment_verbatim_level ()
    =
    if (*verbatim_level != 0 )
	e::impossible "nested verbatim levels";
    else
        inc verbatim_level;
    fi;

fun output_raw (s: String)
    =
    {   raw_line := *raw_line + s;
        raw_no_newline := TRUE;
    };

fun raw_next_line ()
    =
    {   raw            :=  *raw_line + "\n" ! *raw;
	raw_line       :=  "";
        raw_no_newline :=  FALSE;
    };

fun raw_stop ()
    =
    *raw_no_newline  ?:   raw_next_line ();


fun eof ()
    =
    {   if   (*comment_nesting_depth > 0)
            
              e::complain "unclosed comment";
	else
             if   (*verbatim_level != 0)
                 
		  e::complain "unclosed user input";
             fi; 
        fi;

	if *reached_eop 

             t::k_eof(*line_num,*line_num);
	else
	     {   raw_stop ();

		t::ppercent(reverse(*raw),*line_num,*line_num);
	     }
	     before {  raw := [];
		       reached_eop := TRUE;
		    };
	fi;
    };

%%

%s 			comment dump postlude;
%header			(generic package burg_lex_g( package tokens : Burg_Tokens; ));
idchars			= [A-Za-z_0-9];
id			= [A-Za-z]{idchars}*;
ws			= [\t\ ]*;
num			= [0-9]+;
line			= .*;




%%

<initial> "\n"		=> (inc line_num; continue());
<initial> "%{"		=> (increment_verbatim_level(); yybegin dump; continue());
<initial> "%%"		=> (inc percent_count; 
			    if (*percent_count == 2 )
			         yybegin postlude; continue();
			    else t::ppercent(reverse(*raw), *line_num, *line_num)
				 before raw := [];
                            fi
                           );
                           
<initial> {ws}		=> (continue());
<initial> \n		=> (inc line_num; continue());
<initial> "("		=> (t::k_lparen(*line_num,*line_num));
<initial> ")"		=> (t::k_rparen(*line_num,*line_num));
<initial> ","		=> (t::k_comma(*line_num,*line_num));
<initial> ":"		=> (t::k_colon(*line_num,*line_num));
<initial> ";"		=> (t::k_semicolon(*line_num,*line_num));
<initial> "="		=> (t::k_equal(*line_num,*line_num));
<initial> "|"		=> (t::k_pipe(*line_num,*line_num));
<initial> "%term"	=> (t::k_term(*line_num,*line_num));
<initial> "%start"	=> (t::k_start(*line_num,*line_num));
<initial> "%termprefix"	=> (t::k_termprefix(*line_num,*line_num));
<initial> "%ruleprefix"	=> (t::k_ruleprefix(*line_num,*line_num));
<initial> "%sig"	=> (t::k_sig(*line_num,*line_num));
<initial> "(*"		=> (yybegin comment; comment_nesting_depth:=1; continue());
<initial> {num}		=> (t::int(the(int::from_string yytext),*line_num,*line_num));
<initial> {id}		=> (t::id(yytext,*line_num,*line_num));

<comment> "(*"		=> (inc comment_nesting_depth; continue());
<comment> \n		=> (inc line_num; continue());
<comment> "*)"		=> (dec comment_nesting_depth;
			    if (*comment_nesting_depth==0 ) yybegin initial; fi;
			    continue());
<comment> .		=> (continue());

<dump> "%}"		=> (raw_stop(); dec verbatim_level;
			    yybegin initial; continue());
<dump> "\n"		=> (raw_next_line (); inc line_num; continue());
<dump> {line}		=> (output_raw yytext; continue());


<postlude> "\n"		=> (raw_next_line (); inc line_num; continue());
<postlude> {line}	=> (output_raw yytext; continue());
