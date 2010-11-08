## make7.lex
## (C) 1999 Lucent Technologies, Bell Laboratories
## Author: Matthias Blume (blume@kurims.kyoto-u.ac.jp)



# lexical analysis (Mythryl-Lex specification) for make7 description files




###                     "Let's say the docs present a simplified view of reality...       :-)"
###
###                                      --Larry Wall in  <6940@jpl-devvax.JPL.NASA.GOV>



package s = make7grammar_actions;

Semantic_Value = tokens::Semantic_Value;
Source_Position = Int;

Token( X, Y )   = tokens::Token( X, Y );
Lex_Result = Token( Semantic_Value, Source_Position );

Lex_Arg = {    enter_comment: Void -> Void,
	       leave_comment: Void -> Bool,
	       new_string: Source_Position -> Void,
	       add_string: Char -> Void,
	       add_character: (String, Int) -> Void,
	       add_number: (String, Source_Position) -> Void,
	       get_string: (Source_Position, ((String, Source_Position, Source_Position) -> Lex_Result)) -> Lex_Result,
	       handle_eof: Void -> Source_Position,
	       newline: Source_Position -> Void,
	       obsolete: (Source_Position, Source_Position) -> Void,
	       error: (Source_Position, Source_Position) -> String -> Void,
	       sync: (Source_Position, String) -> Void,
	       in_section2: Ref( Bool )
	      };

Arg = Lex_Arg;

fun eof (arg: Lex_Arg)
    =
    {   pos =   arg.handle_eof ();

	tokens::eof (pos, pos);
    };

fun error_tok (t, p)
    =
    {   fun find_graph i
            =
	    if (char::is_graph (string::get (t, i)) )
                i;
	    else
                find_graph (i + 1);
            fi;

	fun find_error i
            =
	    if (string::get (t, i) == 'e' )
                i;
	    else
                find_error (i + 1);
            fi;

	start =   find_graph (5 + find_error 0);
	msg =   string::extract (t, start, NULL);

	tokens::errorx (msg, p + 1, p + size t);
    };

fun plain t (_: Ref( Bool ), arg)
    =
    t arg : Lex_Result;

fun is_token (r, arg)
    =
    {   r := TRUE;   tokens::is arg;   }: Lex_Result;

make7_ids = [ ("Group",               plain tokens::group),
	      ("GROUP",                 plain tokens::group),
	      ("group",                 plain tokens::group),
	      ("group_exports",         plain tokens::group),
	      ("LIBRARY_GROUP_EXPORTS", plain tokens::group),
	      ("Library", plain tokens::library),
	      ("LIBRARY", plain tokens::library),
	      ("library", plain tokens::library),
	      ("library_exports", plain tokens::library),
	      ("LIBRARY_EXPORTS", plain tokens::library),
	      ("IS", is_token),
	      ("group_components", is_token),
	      ("LIBRARY_GROUP_COMPONENTS", is_token),
	      ("LIBRARY_COMPONENTS", is_token),
	      ("*", plain tokens::star),
	      ("-", plain tokens::dash),
	      ("Source", plain tokens::source),
	      ("SOURCE", plain tokens::source),
	      ("source", plain tokens::source)];

ml_ids = [    ("pkg",               tokens::pkg_t),
              ("package",           tokens::package_t),
	      ("structure",         tokens::package_t),
	      ("api",               tokens::api_t),
	      ("generic",           tokens::generic_t),
	      ("funsig",            tokens::generic_api_t),
	      ("generic_api", tokens::generic_api_t)];

pp_ids = [("defined", plain tokens::defined),
	      ("div", plain (fn (x, y) =  tokens::mulsym (s::DIV, x, y))),
	      ("mod", plain (fn (x, y) =  tokens::mulsym (s::MOD, x, y))),
	      ("and", plain tokens::and_t),
	      ("or", plain tokens::or_t),
	      ("not", plain tokens::not_t)];

fun id_token (t, p, idlist, default, chstate, in_section2)
    =
    case (list::find
             (fn (id, _) =  id == t)
	     ml_ids
    )

	 THE (_, tok)
	    =>
	    {   chstate ();
		tok (p, p + size t);
	    };

	NULL
	    =>
	   case (list::find
		    (fn (id, _) =  id == t)
		    idlist
           )

		THE (_, tok)
		    =>
		    tok (in_section2, (p, p + size t));

		NULL
		    =>
		    default (t, p, p + size t);
	   esac;  

    esac;

/* states:

     initial -> C
       |
       +------> P -> PC
       |        |
       |        +--> PM -> PMC
       |
       +------> M -> MC
       |
       +------> S -> SS

   "COMMENT" -- COMMENT
   "P"       -- PREPROC
   "M"       -- MLSYMBOL
   "S"       -- STRING
   "SS"      -- STRINGSKIP
*/

%%

%s comment linecomment ppp pc pm pmc mmm mc sstring ss;

%header(generic package make7_lex_g (package tokens: Cm_Tokens;));

%arg ( { enter_comment, leave_comment,
        new_string, add_string, add_character, add_number, get_string,
        handle_eof,
        newline,
	obsolete,
	error,
	sync,
	in_section2 });

idchars=[A-Za-z'_0-9];
id=[A-Za-z]{idchars}*;
cmextrachars=[.;,!%&$+/<=>?@~|#*]|\-|\^;
cmidchars={idchars}|{cmextrachars};
cmid={cmidchars}+;
ws=("\012"|[\t\ ]);
eol=("\013\010"|"\013"|"\010");
neol=[^\013\010];
symbol=[!%&$+/:<=>?@~|#*]|\-|\^|"\\";
digit=[0-9];
sharp="#";
%%

<initial>"#"{eol}       => (newline yypos; continue());
<initial>"# "	        => (yybegin linecomment;  continue());
<initial>"#\t"          => (yybegin linecomment;  continue());
<initial>\#\!	        => (yybegin linecomment;  continue());
<initial>\#\#	        => (yybegin linecomment;  continue());
<initial>"/*"[*=#-]*    => (enter_comment (); yybegin comment; continue ());

<linecomment>{eol}	=> (newline yypos; yybegin initial; continue());
<linecomment>.		=> (continue());

<ppp>"/*"[*=#-]*          => (enter_comment (); yybegin pc; continue ());
<pm>"/*"[*=#-]*         => (enter_comment (); yybegin pmc; continue ());
<mmm>"/*"[*=#-]*          => (enter_comment (); yybegin mc; continue ());

<comment,pc,pmc,mc>"/*"[*=#-]* => (enter_comment (); continue ());

<comment>"*/"           => (if (leave_comment () ) yybegin initial;  fi;
			    continue ());
<pc>"*/"                => (if (leave_comment () ) yybegin ppp;  fi;
			    continue ());
<pmc>"*/"                => (if (leave_comment () ) yybegin pm;  fi;
			    continue ());
<mc>"*/"                => (if (leave_comment () ) yybegin mmm;  fi;
			    continue ());
<comment,pc,pmc,mc>{eol}  => (newline yypos; continue ());
<comment,pc,pmc,mc>.      => (continue ());

<initial,ppp,pm,mmm>"*/"	=> (error (yypos, yypos+2)
			      "unmatched comment delimiter";
			    continue ());

<initial>"\""		=> (yybegin sstring; new_string yypos; continue ());

<sstring>"\\a"		=> (add_string '\a'; continue ());
<sstring>"\\b"		=> (add_string '\b'; continue ());
<sstring>"\\f"		=> (add_string '\f'; continue ());
<sstring>"\\n"		=> (add_string '\n'; continue ());
<sstring>"\\r"		=> (add_string '\r'; continue ());
<sstring>"\\t"		=> (add_string '\t'; continue ());
<sstring>"\\v"		=> (add_string '\v'; continue ());

<sstring>"\\^"@		=> (add_string (char::from_int 0); continue ());
<sstring>"\\^"[a-z]	        => (add_character (yytext, char::to_int 'a'); continue ());
<sstring>"\\^"[A-Z]	        => (add_character (yytext, char::to_int 'A'); continue ());
<sstring>"\\^["		=> (add_string (char::from_int 27); continue ());
<sstring>"\\^\\"		=> (add_string (char::from_int 28); continue ());
<sstring>"\\^]"		=> (add_string (char::from_int 29); continue ());
<sstring>"\\^^"		=> (add_string (char::from_int 30); continue ());
<sstring>"\\^_"		=> (add_string (char::from_int 31); continue ());

<sstring>"\\"[0-9][0-9][0-9]	=> (add_number (yytext, yypos); continue ());

<sstring>"\\\""		=> (add_string (char::from_int 34); continue ());
<sstring>"\\\\"		=> (add_string '\\'; continue ());

<sstring>"\\"{eol}	        => (yybegin ss; newline (yypos + 1); continue ());
<sstring>"\\"{ws}+	        => (yybegin ss; continue ());

<sstring>"\\".		=> (error (yypos, yypos+2)
			     ("illegal escape character in string " + yytext);
			    continue ());

<sstring>"\""		        => (yybegin initial; get_string (yypos, tokens::file_native));
<sstring>{eol}		=> (newline yypos;
			    error (yypos, yypos + size yytext)
			      "illegal linebreak in string";
			    continue ());

<sstring>.		        => (add_string (string::get (yytext, 0)); continue ());

<ss>{eol}	        => (newline yypos; continue ());
<ss>{ws}+	        => (continue ());
<ss>"\\"	        => (yybegin sstring; continue ());
<ss>.		        => (error (yypos, yypos+1)
			     ("illegal character in stringskip " + yytext);
			    continue ());

<initial,ppp>"("		=> (tokens::lparen (yypos, yypos + 1));
<initial,ppp>")"		=> (tokens::rparen (yypos, yypos + 1));
<initial>":"		=> (tokens::colon (yypos, yypos + 1));
<ppp>"+"		        => (tokens::addsym (s::PLUS, yypos, yypos + 1));
<ppp>"-"		        => (tokens::addsym (s::MINUS, yypos, yypos + 1));
<ppp>"*"		        => (tokens::mulsym (s::TIMES, yypos, yypos + 1));
<ppp>"<>"		        => (tokens::eqsym (s::NE, yypos, yypos + 2));
<ppp>"!="                 => (obsolete (yypos, yypos + 2);
			    tokens::eqsym (s::NE, yypos, yypos+2));
<ppp>"<="		        => (tokens::ineqsym (s::LE, yypos, yypos + 2));
<ppp>"<"		        => (tokens::ineqsym (s::LT, yypos, yypos + 1));
<ppp>">="		        => (tokens::ineqsym (s::GE, yypos, yypos + 2));
<ppp>">"		        => (tokens::ineqsym (s::GT, yypos, yypos + 1));
<ppp>"=="                 => (obsolete (yypos, yypos + 2);
			    tokens::eqsym (s::EQ, yypos, yypos + 2));
<ppp>"="		        => (tokens::eqsym (s::EQ, yypos, yypos + 1));
<ppp>"~"		        => (tokens::tilde (yypos, yypos + 1));

<ppp>{digit}+	        => (tokens::number
			     (the (int::from_string yytext)
			      except _ =
				  {   error (yypos, yypos + size yytext)
				            "number too large";
				      0;
                                  },
			      yypos, yypos + size yytext));

<ppp>{id}                 => (id_token (yytext, yypos, pp_ids, tokens::make7_id,
				     fn () =  yybegin pm, in_section2));
<ppp>"/"                  => (obsolete (yypos, yypos + 1);
			    tokens::mulsym (s::DIV, yypos, yypos + 1));
<ppp>"%"                  => (obsolete (yypos, yypos + 1);
			    tokens::mulsym (s::MOD, yypos, yypos + 1));
<ppp>"&&"                 => (obsolete (yypos, yypos + 2);
			    tokens::and_t (yypos, yypos + 2));
<ppp>"||"                 => (obsolete (yypos, yypos + 2);
			    tokens::or_t (yypos, yypos + 2));
<ppp>"!"                  => (obsolete (yypos, yypos + 1);
			    tokens::not_t (yypos, yypos + 1));

<mmm>({id}|{symbol}+)        => (yybegin initial;
			    tokens::ml_id (yytext, yypos, yypos + size yytext));
<pm>({id}|{symbol}+)       => (yybegin ppp;
			    tokens::ml_id (yytext, yypos, yypos + size yytext));

<initial,ppp>{eol}{ws}*{sharp}"if" => (yybegin ppp;
				     newline yypos;
				     tokens::if_t (yypos, yypos + size yytext));
<initial,ppp>{eol}{ws}*{sharp}"elif" => (yybegin ppp;
				     newline yypos;
				     tokens::elif_t (yypos, yypos + size yytext));
<initial,ppp>{eol}{ws}*{sharp}"else" => (yybegin ppp;
				     newline yypos;
				     tokens::else_t (yypos, yypos + size yytext));
<initial,ppp>{eol}{ws}*{sharp}"endif" => (yybegin ppp;
				      newline yypos;
				      tokens::endif (yypos,
						    yypos + size yytext));
<initial,ppp>{eol}{ws}*{sharp}"error"{ws}+{neol}* => (newline yypos;
						    error_tok (yytext, yypos));
<initial,mmm,pm>{eol}     => (newline yypos; continue ());
<ppp>{eol}                => (yybegin initial; newline yypos; continue ());

<initial,mmm,pm,ppp>{ws}+   => (continue ());

<mmm,pm>.                 => (error (yypos, yypos+1)
			    ("illegal character at start of ML symbol: " +
			     yytext);
			    continue ());

<initial>{cmid}		=> (id_token (yytext, yypos,
				     if *in_section2  []; else make7_ids; fi,
				     tokens::file_standard,
				     fn () =  yybegin mmm, in_section2));


<initial>.		=> (error (yypos, yypos+1)
			    ("illegal character: " + yytext);
			    continue ());

{eol}{ws}*{sharp}"line"{ws}+{neol}* => (newline yypos;
					sync (yypos, yytext);
					continue ());
