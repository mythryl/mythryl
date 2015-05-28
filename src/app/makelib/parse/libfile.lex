## libfile.lex
## (C) 1999 Lucent Technologies, Bell Laboratories
## Author: Matthias Blume (blume@kurims.kyoto-u.ac.jp)



# lexical analysis (Mythryl-Lex specification)
# for foo.lib (library definition) files.




###                     "Let's say the docs present a simplified view of reality...       :-)"
###
###                                      --Larry Wall in  <6940@jpl-devvax.JPL.NASA.GOV>



package lga =   libfile_grammar_actions;		# libfile_grammar_actions	is from   src/app/makelib/parse/libfile-grammar-actions.pkg

Semantic_Value = tokens::Semantic_Value;
Source_Position = Int;

Token( X, Y )   = tokens::Token( X, Y );
Lex_Result = Token( Semantic_Value, Source_Position );


Lex_Arg = {

    # Define the lexer's input argument record.  This
    # actually gets constructed and passed to the lexer in
    #
    #     src/app/makelib/parse/libfile-parser-g.pkg

    # We use these two to track comment-nesting depth:
    #	
    enter_comment: Void -> Void,					# We call this when entering a comment scope (/*... )
    leave_comment: Void -> Bool,					# We call this when leaving  a comment scope (... */)	

    enter_qquote: Source_Position -> Void,				# We call this when entering a double-quoted string literal scope ("...)
    append_char_to_qquote: Char -> Void,				# We call this to add a      char to  a double-quoted string literal.
    append_control_char_to_qquote: (String, Char) -> Void,		# We call this to add a ^A   char to a double-quoted string literal.
    append_escaped_char_to_qquote: (String, Source_Position) -> Void,	# We call this to add a \013 char to a double-quoted string literal.
    leave_qquote							# We call this when leaving  a double-quoted string literal scope (...")
	:
	( Source_Position,
	  ((String, Source_Position, Source_Position) -> Lex_Result)
	)
	->
	Lex_Result,
    #
    handle_eof_by_complaining_about_unclosed_comments_and_strings
	:
	Void -> Source_Position,					# We return final line number.

    newline:	    Source_Position -> Void,				# We call this each time we see a newline -- we use this to track current line number.
    #
    complain_about_obsolete_syntax
	:
	(Source_Position, Source_Position) -> Void,			# We use this to flag an obsolete syntactic construct.
    #
    report_error: (Source_Position, Source_Position) -> String -> Void,	# String is the error message.
    handle_line_directive: (Source_Position, String) -> Void,		# Handle #line directive resetting effective line number.
    in_section2: Ref( Bool )						# Initially FALSE, set TRUE once we've seen LIBRARY_COMPONENTS or SUBLIBRARY_COMNPONENTS token.
};

Arg = Lex_Arg;

fun eof (arg: Lex_Arg)
    =
    {   end_of_file_position
	    =
	    arg.handle_eof_by_complaining_about_unclosed_comments_and_strings ();

	tokens::eof
	  ( end_of_file_position,
	    end_of_file_position
	  );
    };

fun error_tok (t, p)
    =
    {   fun find_graph i
            =
	    if (char::is_graph (string::get_byte_as_char (t, i)))   i;
	    else  			                            find_graph (i + 1);
            fi;

	fun find_error i
            =
	    if (string::get_byte_as_char (t, i) == 'e')   i;
	    else                                          find_error (i + 1);
            fi;

	start =   find_graph (5 + find_error 0);
	msg =   string::extract (t, start, NULL);

	tokens::errorx (msg, p + 1, p + size t);
    };

fun plain t (_: Ref( Bool ), arg)
    =
    t arg: Lex_Result;

fun library_components_token (in_section2, arg)
    =
    {   in_section2 := TRUE;			# We've seen LIBRARY_COMPONENTS or SUBLIBRARY_COMPONENTS, so we are now in the second section of the .lib file.
	tokens::library_components arg;		# LIBRARY_COMPONENTS and SUBLIBRARY_COMPONENTS are the same token.
    }
    : Lex_Result;

makelib_ids = [ ("LIBRARY_EXPORTS",       plain tokens::library_exports),
		("SUBLIBRARY_EXPORTS",    plain tokens::sublibrary_exports),
		("API_OR_PKG_EXPORTS",	  plain tokens::api_or_pkg_exports),
		("SUBLIBRARY_COMPONENTS", library_components_token),
		("LIBRARY_COMPONENTS",	  library_components_token),
		("*",			  plain tokens::star),
		("-",			  plain tokens::dash)
	];

ml_ids = [    ("pkg",		tokens::pkg_t		),
	      ("api",		tokens::api_t		),
	      ("generic",	tokens::generic_t	),
	      ("funsig",	tokens::generic_api_t	),
	      ("generic_api",	tokens::generic_api_t	)
	];

pp_ids = [    ("defined",	plain tokens::defined	),
	      ("and",		plain tokens::and_t	),
	      ("or",		plain tokens::or_t	),
	      ("not",		plain tokens::not_t	)
	];

fun id_token (t, p, idlist, default, chstate, in_section2)
    =
    case (list::find
             (\\ (id, _) =  id == t)
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
		     (\\ (id, _) =  id == t)
		     idlist
	    )
		THE (_, tok) =>   tok (in_section2, (p, p + size t));	# We pass 'in_section2' only so that library_components_token() can set it TRUE.
		NULL	     =>	         default (t, p, p + size t);
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

%s comment linecomment ppp pc pm pmc mmm mc qquote ss;

%header(generic package makelib_lex_g (package tokens: Libfile_Tokens;));

%arg ({ enter_comment,
	leave_comment,
	enter_qquote,
	append_char_to_qquote,
	append_control_char_to_qquote,
	append_escaped_char_to_qquote,
	leave_qquote,
        handle_eof_by_complaining_about_unclosed_comments_and_strings,
        newline,
	complain_about_obsolete_syntax,
	report_error,
	handle_line_directive,
	in_section2
      });

idchars=[A-Za-z'_0-9];
id=[A-Za-z]{idchars}*;
cmextrachars=[.;,!%&$+/<=>?@~|#*]|\-|\^;
cmidchars={idchars}|{cmextrachars};
cmid={cmidchars}+;
ws=("\x0c"|[\t\ ]);
eol=("\x0d\x0a"|"\x0d"|"\x0a");
neol=[^\x0d\x0a];
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

<initial,ppp,pm,mmm>"*/" => (report_error (yypos, yypos+2) "unmatched comment delimiter";
			    continue ());

<initial>"\""		=> (yybegin qquote; enter_qquote yypos; continue ());

<qquote>"\\a"		=> (append_char_to_qquote '\a'; continue ());
<qquote>"\\b"		=> (append_char_to_qquote '\b'; continue ());
<qquote>"\\f"		=> (append_char_to_qquote '\f'; continue ());
<qquote>"\\n"		=> (append_char_to_qquote '\n'; continue ());
<qquote>"\\r"		=> (append_char_to_qquote '\r'; continue ());
<qquote>"\\t"		=> (append_char_to_qquote '\t'; continue ());
<qquote>"\\v"		=> (append_char_to_qquote '\v'; continue ());

<qquote>"\\^"@		=> (append_char_to_qquote (char::from_int 0); continue ());
<qquote>"\\^"[a-z]	=> (append_control_char_to_qquote (yytext, 'a'); continue ());
<qquote>"\\^"[A-Z]	=> (append_control_char_to_qquote (yytext, 'A'); continue ());
<qquote>"\\^["		=> (append_char_to_qquote (char::from_int 27); continue ());
<qquote>"\\^\\"		=> (append_char_to_qquote (char::from_int 28); continue ());
<qquote>"\\^]"		=> (append_char_to_qquote (char::from_int 29); continue ());
<qquote>"\\^^"		=> (append_char_to_qquote (char::from_int 30); continue ());
<qquote>"\\^_"		=> (append_char_to_qquote (char::from_int 31); continue ());

<qquote>"\\"[0-7][0-7][0-7]	=> (append_escaped_char_to_qquote (yytext, yypos); continue ());

<qquote>"\\\""		=> (append_char_to_qquote (char::from_int 34); continue ());
<qquote>"\\\\"		=> (append_char_to_qquote '\\'; continue ());

<qquote>"\\"{eol}	=> (yybegin ss; newline (yypos + 1); continue ());
<qquote>"\\"{ws}+	=> (yybegin ss; continue ());

<qquote>"\\".		=> (report_error (yypos, yypos+2) ("illegal escape character in string " + yytext);
			    continue ());

<qquote>"\""		=> (yybegin initial; leave_qquote (yypos, tokens::file_native));
<qquote>{eol}		=> (newline yypos;
			    report_error (yypos, yypos + size yytext) "illegal linebreak in string";
			    continue ());

<qquote>.		=> (append_char_to_qquote (string::get_byte_as_char (yytext, 0)); continue ());

<ss>{eol}	        => (newline yypos; continue ());
<ss>{ws}+	        => (continue ());
<ss>"\\"	        => (yybegin qquote; continue ());
<ss>.		        => (report_error (yypos, yypos+1) ("illegal character in stringskip " + yytext);
			    continue ());

<initial,ppp>"("		=> (tokens::lparen (yypos, yypos + 1));
<initial,ppp>")"		=> (tokens::rparen (yypos, yypos + 1));
<initial>":"			=> (tokens::colon (yypos, yypos + 1));

<ppp>"+"		        => (tokens::addsym (lga::PLUS, yypos, yypos + 1));
<ppp>"-"		        => (tokens::addsym (lga::MINUS, yypos, yypos + 1));
<ppp>"*"		        => (tokens::mulsym (lga::TIMES, yypos, yypos + 1));

<ppp>"!="		        => (tokens::eqsym   (lga::NE, yypos, yypos + 2));
<ppp>"=="                       => (tokens::eqsym   (lga::EQ, yypos, yypos + 2));
<ppp>"<="		        => (tokens::ineqsym (lga::LE, yypos, yypos + 2));
<ppp>"<"		        => (tokens::ineqsym (lga::LT, yypos, yypos + 1));
<ppp>">="		        => (tokens::ineqsym (lga::GE, yypos, yypos + 2));
<ppp>">"		        => (tokens::ineqsym (lga::GT, yypos, yypos + 1));

<ppp>"~"		        => (tokens::tilde (yypos, yypos + 1));

<ppp>{digit}+	        => (tokens::number
			     (the (int::from_string yytext)
			      except _ =
				  {   report_error (yypos, yypos + size yytext) "number too large";
				      0;
                                  },
			      yypos, yypos + size yytext));

<ppp>{id}                 => (id_token (yytext, yypos, pp_ids, tokens::makelib_id,
				     \\ () =  yybegin pm, in_section2));
<ppp>"/"                  => (tokens::mulsym (lga::DIV, yypos, yypos + 1));
<ppp>"%"                  => (tokens::mulsym (lga::MOD, yypos, yypos + 1));

<mmm>({id}|{symbol}+)	  => (yybegin initial;	tokens::ml_id (yytext, yypos, yypos + size yytext));
<pm>({id}|{symbol}+)	  => (yybegin ppp;	tokens::ml_id (yytext, yypos, yypos + size yytext));

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

<mmm,pm>.                 => (report_error (yypos, yypos+1) ("illegal character at start of ML symbol: " + yytext);
			    continue ());

<initial>{cmid}		 => (id_token
                              ( yytext, yypos,
				#
				if *in_section2  [];		# We have seen LIBRARY_COMPONENTS/SUBLIBRARY_COMPONENTS so stop  recognizing  "LIBRARY_EXPORTS", "LIBRARY_COMPONENTS", "*", "-" ... as special tokens.
                                else             makelib_ids;	# Not yet seen LIBRARY_COMPONENTS/SUBLIBRARY_COMPONENTS so still recognizing  "LIBRARY_EXPORTS", "LIBRARY_COMPONENTS", "*", "-" ... as special tokens.
                                fi,
				#
				tokens::file_standard,
				\\ () =  yybegin mmm,
                                in_section2
			      )
			    );


<initial>.		=> (report_error (yypos, yypos+1) ("illegal character: " + yytext);
			    continue ());

{eol}{ws}*{sharp}"line"{ws}+{neol}* => (newline yypos;
					handle_line_directive (yypos, yytext);
					continue ());
