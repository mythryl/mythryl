# Mythryl-Yacc Parser Generator (c) 1989 Andrew W. Appel, David R. Tarditi
#
#   yacc.lex: Lexer specification



###           "This therefore, is mathematics:
###            she reminds you of the invisible forms of the soul;
###            she gives life to her own discoveries;
###            she awakens the mind and purifies the intellect;
###            she brings to light our intrinsic ideas;
###            she abolishes the oblivion and ignorance which are ours by birth..."
###
###                                                 -- Proclus



package tokens = tokens;

Semantic_Value = tokens::Semantic_Value;
Source_Position = Int;
Token( X, Y ) = tokens::Token( X, Y );
Lex_Result = Token( Semantic_Value, Source_Position );

Lex_Arg = header::Input_Source;
Arg = Lex_Arg;

include tokens;

error  = header::error;
lineno = header::lineno;
text   = header::text;

pcount        = REF 0;
comment_level = REF 0;
actionstart   = REF 0;

eof =   fn i =
             {   if  (*pcount > 0)

		     error i *actionstart
		         " eof encountered in action beginning here !";
		 fi;

                 eof_t (*lineno, *lineno);
             };

fun add s
    =
    text :=   s ! *text;


stipulate

    dictionary = [("%prec",prec_tag),("%term",term),
            ("%nonterm",nonterm), ("%eop",percent_eop),("%start",start),
            ("%prefer",prefer),("%subst",subst),("%change",change),
            ("%keyword",keyword),("%name",name),
            ("%verbose",verbose), ("%nodefault",nodefault),
            ("%value",value), ("%noshift",noshift),
            ("%header",percent_header),("%pure",percent_pure),
            ("%token_api_info",percent_token_api_info),
            ("%arg",percent_arg),
            ("%pos",percent_pos)];
herein
    fun lookup (s,left,right)
        =
        {  fun f ((a,d) ! b) =>   if (a==s)   d(left,right);   else f b; fi;
	       f NIL         =>   unknown(s,left,right);
           end;

	   f dictionary;
	};
end;

fun inc (ri as REF i) =   ri := i + 1;
fun dec (ri as REF i) =   ri := i - 1;

%%
%header (
generic package lex_mlyacc_g (package tokens : Mlyacc_Tokens;
		  package header : Header #  = header 
		  where Precedence == header::Precedence
		  also  Input_Source == header::Input_Source;) : (weak) Arg_Lexer
);
%arg (input_source);
%s aaa code fff comment linecomment string emptycomment;
ws = [\t\ ]+;
eol=("\n"|"\013\n"|"\013");
idchars = [A-Za-z_'0-9];
id=[A-Za-z]{idchars}*;
tyvar="'"{idchars}*;
qualid ={id}"::";
%%
<initial>"/*"[*=#-]* => (add yytext; yybegin comment; comment_level := 1;
		    continue() then yybegin initial);
<aaa>"/*"[*=#-]*	=> (yybegin emptycomment; comment_level := 1; continue());
<aaa>"#"{eol}     => (inc lineno; continue());
<aaa>"# "	        => (yybegin linecomment;  continue());
<aaa>"#\t"        => (yybegin linecomment;  continue());
<aaa>\#\!	        => (yybegin linecomment;  continue());
<aaa>\#\#	        => (yybegin linecomment;  continue());

<code>"/*"[*=#-]* => (add yytext; yybegin comment; comment_level := 1;
		    continue() then yybegin code);
<initial>[^%\013\n]+ => (add yytext; continue());
<initial>"%%"	 => (yybegin aaa; header (cat (reverse *text),*lineno,*lineno));
<initial,code,comment,fff,emptycomment>{eol}  => (add yytext; inc lineno; continue());
<initial>.	 => (add yytext; continue());


<linecomment>{eol}	=> (inc lineno; yybegin aaa; continue());
<linecomment>.		=> (continue());

<aaa>{eol}	=> (inc lineno; continue ());
<aaa>{ws}+	=> (continue());
<aaa>of		=> (of_t(*lineno,*lineno));
<aaa>for		=> (for_t(*lineno,*lineno));
<aaa>"{"		=> (lbrace(*lineno,*lineno));
<aaa>"}"		=> (rbrace(*lineno,*lineno));
<aaa>","		=> (comma(*lineno,*lineno));
<aaa>"*"		=> (asterisk(*lineno,*lineno));
<aaa>"->"		=> (arrow(*lineno,*lineno));
<aaa>"%left"	=> (prec(header::LEFT,*lineno,*lineno));
<aaa>"%right"	=> (prec(header::RIGHT,*lineno,*lineno));
<aaa>"%nonassoc" 	=> (prec(header::NONASSOC,*lineno,*lineno));
<aaa>"%"[a-z_]+	=> (lookup(yytext,*lineno,*lineno));
<aaa>{tyvar}	=> (tyvar(yytext,*lineno,*lineno));
<aaa>{qualid}	=> (iddot(yytext,*lineno,*lineno));
<aaa>[0-9]+	=> (int (yytext,*lineno,*lineno));
<aaa>"%%"		=> (delimiter(*lineno,*lineno));
<aaa>":"		=> (colon(*lineno,*lineno));
<aaa>"|"		=> (bar(*lineno,*lineno));
<aaa>{id}		=> (id ((yytext,*lineno),*lineno,*lineno));
<aaa>"("		=> (pcount := 1; actionstart := *lineno;
		    text := NIL; yybegin code; continue() then yybegin aaa);
<aaa>.		=> (unknown(yytext,*lineno,*lineno));
<code>"("	=> (inc pcount; add yytext; continue());
<code>")"	=> (dec pcount;
		    if (*pcount == 0)
			 prog (cat (reverse *text),*lineno,*lineno);
		    else
                         add yytext;
                         continue();
                    fi
                   );
<code>"\""	=> (add yytext; yybegin string; continue());
<code>[^()"\n\013]+ => (add yytext; continue());

<comment>"*/"	=> (add yytext; dec comment_level;
		    if (*comment_level==0)
			 bogus_value(*lineno,*lineno);
		    else
                        continue();
                    fi
		   );
<comment>"/*"[*=#-]* => (add yytext; inc comment_level; continue());
<comment>[^*()\n\013]+ => (add yytext; continue());

<emptycomment>[(*)]  => (continue());
<emptycomment>"*/"   => (dec comment_level;
		          if (*comment_level==0)
                              yybegin aaa;
                          fi;
			  continue ());
<emptycomment>"/*"[*=#-]*  => (inc comment_level; continue());
<emptycomment>[^*()\n\013]+ => (continue());

<string>"\""	=> (add yytext; yybegin code; continue());
<string>\\	=> (add yytext; continue());
<string>{eol}	=> (add yytext; error input_source *lineno "unclosed string";
 	            inc lineno; yybegin code; continue());
<string>[^"\\\n\013]+ => (add yytext; continue());
<string>\\\"	=> (add yytext; continue());
<string>\\{eol} => (add yytext; inc lineno; yybegin fff; continue());
<string>\\[\ \t] => (add yytext; yybegin fff; continue());

<fff>{ws}		=> (add yytext; continue());
<fff>\\		=> (add yytext; yybegin string; continue());
<fff>.		=> (add yytext; error input_source *lineno "unclosed string";
		    yybegin code; continue());

