## lexer
#
# COPYRIGHT (c) 1989,1992 by AT&T Bell Laboratories
#
# A scanner for mapping mapping ML code to pretty print form.
#
# TODO: spaces at the beginning of multi-line comments.


package vb = view_buffer;
package kw = ml_keywords;

Lex_Result
  = EOF
  | NL
  | COM List( Lex_Result )
  | STR List( Lex_Result )
  | TOK { space:  Int,
          kind:   vb::Token_Kind,
          text:   String
        }
  ;

comment_nesting_depth
    =
    REF 0;

result_stk
    =
    REF ([]:  List( Lex_Result ));

char_list = REF ([]:  List( String ));

fun make_string ()
    =
    cat (reverse *char_list)
    before
        char_list := [];

col   =  REF 0;
space =  REF 0;

fun tab ()
    =
    {   n = *col;
        skip = 8 - (n & 0x7);
	#
	space := *space + skip;
	col   := n + skip;
    };

fun expand_tab ()
    =
    {   n = *col;
        skip = 8 - (n & 0x7);
	#
	char_list := (number_string::pad_left ' ' skip "") ! *char_list;
	col := n + skip;
    };

fun add_string s
    =
    {   char_list := s ! *char_list;
	#
        col := *col + size s;
    };

fun token tok
    =
    {   space := 0;
	#
        col := *col + size tok.text;
	#
        TOK tok;
    };

fun newline ()
    =
    {   space := 0;
        col   := 0;
        NL;
    };

fun push_line kind
    =
    {   tok = TOK { space => *space, kind, text => make_string() };

	space := 0;
	newline();
        result_stk :=  NL ! tok ! *result_stk;
    };

fun dump_stk kind
    =
    {   tok = TOK { space => *space, kind, text => make_string() };

	space := 0;

	reverse (tok ! *result_stk)
        before
            result_stk := [];
    };

fun mk_id    s =  token (kw::make_token { space => *space, text => s });

fun mk_sym   s =  token ({ space => *space, kind => vb::SYMBOL, text => s });
fun mk_tyvar s =  token ({ space => *space, kind => vb::IDENT,  text => s });
fun mk_con   s =  token ({ space => *space, kind => vb::SYMBOL, text => s });

fun eof ()
    =
    {   char_list  := [];
        result_stk := [];
	#
        space := 0;
        col   := 0;
	#
        comment_nesting_depth := 0;
	#
        EOF;
    };

fun error s
    =
    raise exception FAIL s;

%% 
%header (package approximate_ml_lex);
%s ccc sss fff;

idchars=[A-Za-z'_0-9];
id=[A-Za-z]{idchars}*;
sym=[!%&$+/:<=>?@~|#*`]|\\|\-|\^;
num=[0-9]+;
frac="."{num};
exp="E"(~?){num};
real=(~?)(({num}{frac}?{exp})|({num}{frac}{exp}?));
hexnum=[0-9a-fA-F]+;

%%

<initial,fff>\t		=> (tab(); continue());
<initial,fff>" "	=> (space := *space + 1; col := *col + 1; continue());
<initial>\n		=> (newline());
<initial>"_"		=> (mk_sym yytext);
<initial>","		=> (mk_sym yytext);
<initial>"{"		=> (mk_sym yytext);
<initial>"}"		=> (mk_sym yytext);
<initial>"["		=> (mk_sym yytext);
<initial>"]"		=> (mk_sym yytext);
<initial>";"		=> (mk_sym yytext);
<initial>"("		=> (mk_sym yytext);
<initial>")"		=> (mk_sym yytext);
<initial>"."		=> (mk_sym yytext);
<initial>"..."		=> (mk_sym yytext);
<initial>"'"{idchars}*	=> (mk_tyvar yytext);

<initial>({sym}+|{id})	=> (mk_id yytext);

<initial>{real}		=> (mk_con yytext);

<initial>{num}		=> (mk_con yytext);
<initial>~{num}		=> (mk_con yytext);
<initial>"0x"{hexnum}	=> (mk_con yytext);
<initial>"~0x"{hexnum}	=> (mk_con yytext);

<initial>"(*"	=> (yybegin ccc; add_string yytext; comment_nesting_depth := 1; continue());
<initial>"*)"	=> (error "unmatched close comment");
<ccc>"(*"	=> (add_string yytext; comment_nesting_depth := *comment_nesting_depth + 1; continue());
<ccc>\n		=> (push_line vb::COMMENT; continue());
<ccc>"*)"	=> (add_string yytext;
		    comment_nesting_depth := *comment_nesting_depth - 1;
		    if (*comment_nesting_depth == 0)
		        yybegin initial;
                        COM (dump_stk vb::COMMENT);
		    else
                        continue();
                    fi
                   );
<ccc>\t		=> (expand_tab(); continue());
<ccc>.		=> (add_string yytext; continue());

<initial>\"	=> (yybegin sss; add_string yytext; continue());
<sss>\"	        => (yybegin initial; add_string yytext; STR(dump_stk vb::SYMBOL));
<sss>\n		=> (error "unexpected newline in unclosed string");
<sss>\\\n	=> (yybegin fff; push_line vb::SYMBOL; continue());
<sss>\t		=> (expand_tab(); continue());
<sss>\\\"	=> (add_string yytext; continue());
<sss>\\		=> (add_string yytext; continue());
<sss>[^"\\\n\t]*	=> (add_string yytext; continue());

<fff>\n		=> (result_stk := (newline ()) ! *result_stk; continue());
<fff>\\		=> (yybegin sss; add_string yytext; continue());
<fff>.		=> (error "unclosed string");

<initial>\h	=> (error "non-Ascii character");
<initial>.	=> (error "illegal character");
