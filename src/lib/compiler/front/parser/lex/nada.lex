## nada.lex
## Copyright 1989 by AT&T Bell Laboratories
## Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
## released per terms of SMLNJ-COPYRIGHT.



###       "He who will not apply new remedies
###        must expect old evils."
###
###                       -- Francis Bacon


include error_message;

package mada_token_table
    =
    mada_token_table_g( tokens );		# Defined in ROOT/src/lib/compiler/front/parser/lex/nada-token-table-g.pkg

# 'Lex_Result' is the one type which mythryl-lex
# requires us to define.  According to the manual:
#      Lexresult defines the type of values returned by the rule actions.

Semantic_Value    = tokens::Semantic_Value;
Source_Position   = Int;
Lex_Result        = tokens::Token( Semantic_Value, Source_Position );

Lex_Arg = {  comment_nesting_depth : Ref( Int ), 
	            line_number_db      : line_number_db::Sourcemap,
		    charlist            : Ref( List( String ) ),

		    stringtype          : Ref( Bool ),
		    stringstart         : Ref( Int ),            #  Start of current string or comment
		    brack_stack         : Ref( List( Ref( Int ) ) ),   #  For frags 

	            err : (Source_Position,  Source_Position) -> error_message::Plaint_Sink
  };

Arg = Lex_Arg;

Token( X, Y )
     =
     tokens::Token( X, Y );

# This is the one function which
# mythryl-lex requires us to define.
# According to the manual:              http://www.smlnj.org/doc/Lex/manual.html
#
#     The function "eof" is called by the lexer when
#     the end of the input stream is reached.
#
#     It will typically return a value signalling eof
#     or raise an exception.
#
#     It is called with the same argument as lex
#     (see %arg, below), and must return a value
#     of type Lex_Result.

fun eof ( { comment_nesting_depth, err, charlist, stringstart, line_number_db, ... } : Lex_Arg)
    =
    {   pos =
            int::max (   *stringstart + 2,
                        line_number_db::last_change line_number_db
			 );

        if   (*comment_nesting_depth > 0)
            
             err (*stringstart, pos) ERROR "unclosed comment" null_error_body;
        else 
             if   (*charlist != [])
                 
                  err
                      (*stringstart, pos)
                      ERROR
                      "unclosed string, character, or quotation"
                      null_error_body;

	     fi;
        fi;


	tokens::eof(pos,pos);
    };


fun add_string (charlist, s: String)
    =
    charlist := s ! *charlist;


fun add_char (charlist, c: Char)
    =
    add_string (charlist, string::from_char c);


fun make_string charlist
    =
    cat (reverse *charlist)
    then
        charlist := NIL;


stipulate
    fun convert radix (s, i)
        =
        #1 (the (multiword_int::scan radix substring::getc (substring::drop_first i (substring::from_string s))));
herein
    atoi   =   convert number_string::DECIMAL;
    xtoi   =   convert number_string::HEX;
end;

fun my_synch (src, pos, parts)
    =
    {   fun digit d
            =
	    char::to_int d - char::to_int '0';

	fun convert digits
            =
            fold_forward
                (fn (d, n) =  10*n + digit d)
                0
                (explode digits);

	r =   line_number_db::resynch src;

        case parts

	  [col, line]
             => 
	     r (   pos,
                   {   file_name => NULL,
                       line      => convert line,
                       column    => THE (convert col)
                   }
		   );

	   [file, col, line]
             => 
	     r (   pos,
                   {   file_name => THE file,
                       line      => convert line,
                       column    => THE (convert col)
                   }
		   );

	   _ => impossible "text in /*#line...*/";

        esac;
    };

# fun has_backquote s
#     =
#     {   fun loop i
#             =
#             (    (string::get(s,i) == '`')
#                  or
#                  loop (i+1)
#             )
# 	    except
#                 _ => FALSE;
# 
# 	loop 0;
#     };

fun inc (ri as REF i)   =   (ri := i + 1);
fun dec (ri as REF i)   =   (ri := i - 1);

/* End of user declarations section.
 * NB: There is apparently no way
 * to enter comments past this point
 * in the source   :-(   XXX BUGGO FIXME
 *
 * So, a few of notes here on the stuff that follows:
 *
 * o  The 'operator_id' regex only handles backquoted
 *    value_ids.  Most operators are of course '->' and
 *    the like, but those are all handled in relex-g.pkg.
 *
 * o  'eol' ("end of line") has three cases, one each for
 *    the Mac, Windows and POSIX newline conventions.
 *
 * o  CHAR and STRING are currently exact duplicates of
 *    each other except for the delimiting characters.  Lazy, ugly! :)
 *    They should either be specialized or merged.   CrT 2006-11-04 XXX BUGGO FIXME
 *
 * o  The QUOTE/ANTIQUOTE stuff is a lisp-macro type
 *    mechanism having nothing to do with strings or
 *    string quotation.  I'm not currently paying
 *    much attention to it -- it can wait until
 *    the basics are working smoothly.  CrT 2006-11-04 XXX BUGGO FIXME
 *
 * o  Double-backquote isn't implemented yet.
 *    (We'll need this by and by for shell escapes
 *    parallel to Perl's backquote construction.)  CrT 2006-11-04 XXX BUGGO FIXME
 */
%% 

%reject

%s comment char string indent quote antiquote lll ll llc llcq;

%header (generic package nada_lex_g(package tokens: Nada_Tokens;));

%arg ( {
  comment_nesting_depth,
  line_number_db,
  err,
  charlist,
  stringstart,
  stringtype,
  brack_stack});

idchars  =[A-Za-z_0-9];
valueid  =[a-z]{idchars}*[']*;


whitespace         =("\012"|[\t\ ])*;
nonempty_whitespace =("\012"|[\t\ ])+;
eol                =("\013\010"|"\010"|"\013");

someSym   =[!%&$+/:<=>?@~|#*]|\-|\^;
symbol     ={someSym}|"\\";

value_id       =({valueid}|[`]{symbol}+[`]);
type_id        =[A-Z][a-z]{idchars}*[']*;
typevar_id     =([A-Z]|[A-Z]_{idchars}*[']*);
operator_id    =[`]{valueid}[`];
constructor_id =[A-Z][A-Z]{idchars}*[']*;

backquote3 ="```";
backquote2 ="```";
backquote  ="`";

num    =[0-9]+;
frac   ="."{num};
exp    =[e](-?){num};

real   =(-?)(({num}{frac}?{exp})|({num}{frac}{exp}?));
hexnum =[0-9A-F]+;

%%

<initial>{whitespace}	=> (if ((size yytext) > 0)
                                 tokens::raw_whitespace(yypos,yypos+size yytext);
                            else continue();
                            fi);

<initial>{eol}	=> (line_number_db::newline line_number_db yypos;
                    tokens::raw_whitespace(yypos,yypos+size yytext));

<initial>"_"	=> (tokens::raw_underbar(yypos,yypos+1));
<initial>"&"	=> (tokens::raw_ampersand(yypos,yypos+1));
<initial>"$"	=> (tokens::raw_dollar(yypos,yypos+1));

<initial>"#"	=> (tokens::raw_sharp(yypos,yypos+1));
<initial>"!"	=> (tokens::raw_bang(yypos,yypos+1));
<initial>"~"	=> (tokens::raw_tilda(yypos,yypos+1));

<initial>"-"	=> (tokens::raw_dash(yypos,yypos+1));
<initial>"+"	=> (tokens::raw_plus(yypos,yypos+1));
<initial>"*"	=> (tokens::raw_star(yypos,yypos+1));

<initial>"/"	=> (tokens::raw_slash(yypos,yypos+1));
<initial>"%"	=> (tokens::raw_percent(yypos,yypos+1));
<initial>":"	=> (tokens::raw_colon(yypos,yypos+1));

<initial>"<"	=> (tokens::raw_langle(yypos,yypos+1));
<initial>">"	=> (tokens::raw_rangle(yypos,yypos+1));
<initial>"="	=> (tokens::raw_equal(yypos,yypos+1));

<initial>"?"	=> (tokens::raw_question(yypos,yypos+1));
<initial>"@"	=> (tokens::raw_atsign(yypos,yypos+1));
<initial>"^"	=> (tokens::raw_caret(yypos,yypos+1));

<initial>"|"	=> (tokens::raw_bar(yypos,yypos+1));
<initial>"\\"	=> (tokens::raw_backslash(yypos,yypos+1));
<initial>";"	=> (tokens::raw_semi(yypos,yypos+1));

<initial>"."	=> (tokens::raw_dot(yypos,yypos+1));
<initial>","	=> (tokens::raw_comma(yypos,yypos+1));

<initial>"{"	=> (tokens::raw_lbrace(yypos,yypos+1));
<initial>"}"	=> (tokens::raw_rbrace(yypos,yypos+1));

<initial>"["	=> (tokens::raw_lbracket(yypos,yypos+1));
<initial>"]"	=> (tokens::raw_rbracket(yypos,yypos+1));

<initial>"#!"{nonempty_whitespace}".*"{eol}
		=> (  line_number_db::newline line_number_db yypos; continue();
                                      tokens::shebang(yytext,yypos,yypos+size yytext)
                                   );
<initial>"#"{nonempty_whitespace}".*"{eol}
		=> (line_number_db::newline line_number_db yypos; continue());

<initial>"("		=> (if (null *brack_stack)
               		         ();
                	    else inc (head *brack_stack); fi;
                	    tokens::lparen(yypos,yypos+1));

<initial>")"		=> (if (null *brack_stack)
                	         ();
                	    else if   (*(head *brack_stack) == 1)
                                     
                                      brack_stack := tail  *brack_stack;
                	              charlist := [];
				      yybegin quote;
                	         else
                                      dec (head  *brack_stack);
                                 fi;
                            fi;
                	    tokens::rparen(yypos,yypos+1));

<initial>{value_id}       => (mada_token_table::check_value_id       (yytext, yypos));
<initial>{type_id}        => (mada_token_table::check_type_id        (yytext, yypos));
<initial>{typevar_id}     => (mada_token_table::check_typevar_id     (yytext, yypos));
<initial>{constructor_id} => (mada_token_table::check_constructor_id (yytext, yypos));
<initial>{operator_id}    => (mada_token_table::check_operator_id    (yytext, yypos));
<initial>{backquote3}    => (if *nada_parser::quotation
                                  yybegin quote;
                                  charlist := [];
                                  tokens::beginq(yypos,yypos+1);
                            else err(yypos, yypos+1)
                                     ERROR "quotation implementation error"
				     null_error_body;
                                  tokens::beginq(yypos,yypos+1);
                            fi
                            );

<initial>{real}	       => (tokens::real(yytext,yypos,yypos+size yytext));
<initial>[1-9][0-9]*   => (tokens::int(atoi(yytext, 0),yypos,yypos+size yytext));

<initial>{num}	       => (tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));
<initial>-{num}	       => (tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));

<initial>"0x"{hexnum}  => (tokens::int0(xtoi(yytext, 2),yypos,yypos+size yytext));
<initial>"-0x"{hexnum} => (tokens::int0(multiword_int::(-_)(xtoi(yytext, 3)),yypos,yypos+size yytext));

<initial>"0u"{num}     => (tokens::unt(atoi(yytext, 2),yypos,yypos+size yytext));
<initial>"0ux"{hexnum} => (tokens::unt(xtoi(yytext, 3),yypos,yypos+size yytext));

<initial>\"	=> (charlist := [""]; stringstart := yypos;
                    stringtype := TRUE; yybegin string; continue());

<initial>\'	=> (charlist := [""]; stringstart := yypos;
                    stringtype := FALSE; yybegin char; continue());

<initial>"/*#line"{nonempty_whitespace}
		=> 
                   (yybegin lll; stringstart := yypos; comment_nesting_depth := 1; continue());

<initial>"(#{nonempty_whitespace}"
			=> (yybegin comment; stringstart := yypos; comment_nesting_depth := 1; continue());

<initial>"{nonempty_whitespace}#)"
			=> (err (yypos,yypos+1) ERROR "unmatched close comment"
		        null_error_body;
		    continue());

<initial>\h	=> (err (yypos,yypos) ERROR "non-Ascii character"
		        null_error_body;
		    continue());
<initial>.	=> (err (yypos,yypos) ERROR "illegal token" null_error_body;
		    continue());
<lll>[0-9]+                 => (yybegin ll; charlist := [yytext]; continue());
<ll>\.                    => (/* cheat: take n > 0 dots */ continue());
<ll>[0-9]+                => (yybegin llc; add_string(charlist, yytext); continue());
<ll>0*               	  => (yybegin llc; add_string(charlist, "1");    continue()
		/* note hack, since mythryl-lex chokes on the empty string for 0* */);
<llc>{whitespace}\"	  => (yybegin llcq; continue());
<llcq>[^\"]*              => (add_string(charlist, yytext); continue());
<llcq>\""*/"              => (yybegin initial; my_synch(line_number_db, yypos+3, *charlist); 
		              comment_nesting_depth := 0; charlist := []; continue());
<lll,llc,llcq>"*/" => (err (*stringstart, yypos+1) WARNING 
                       "ill-formed /*#line...*/ taken as comment" null_error_body;
                     yybegin initial; comment_nesting_depth := 0; charlist := []; continue());
<lll,llc,llcq>.    => (err (*stringstart, yypos+1) WARNING 
                       "ill-formed /*#line...*/ taken as comment" null_error_body;
                     yybegin comment; continue());

<comment>"(#{nonempty_whitespace}"
			=> (inc comment_nesting_depth; continue());

<comment>{eol}		=> (line_number_db::newline line_number_db yypos; continue());

<comment>"{nonempty_whitespace}#)"
			=> (dec comment_nesting_depth; if (*comment_nesting_depth==0 ) yybegin initial;  fi; continue());

<comment>.		=> (continue());

<string>\"  	 	=> ( { s = make_string charlist;
                	       s = if (size s != 1 and not *stringtype)
                        	         err (*stringstart,yypos) ERROR
                                      "character constant not length 1"
                                       null_error_body;
                                       substring(s + "x",0,1);
                                    else s;
                                    fi;
                               t = (s,*stringstart,yypos+1);
                    	       yybegin initial;
                               if *stringtype  tokens::string t; else tokens::char t; fi;
                             }
                           );

<string>{eol}		=> (err (*stringstart,yypos) ERROR "unclosed string"
			        null_error_body;
			    line_number_db::newline line_number_db yypos;
			    yybegin initial; tokens::string(make_string charlist,*stringstart,yypos));

<string>\\{eol} 	=> (line_number_db::newline line_number_db (yypos+1);
			    yybegin indent; continue());

<string>\\{whitespace} 	=> (yybegin indent; continue());
<string>\\a		=> (add_string(charlist, "\007"); continue());
<string>\\b		=> (add_string(charlist, "\008"); continue());
<string>\\f		=> (add_string(charlist, "\012"); continue());
<string>\\n		=> (add_string(charlist, "\010"); continue());
<string>\\r		=> (add_string(charlist, "\013"); continue());
<string>\\t		=> (add_string(charlist, "\009"); continue());
<string>\\v		=> (add_string(charlist, "\011"); continue());
<string>\\\\		=> (add_string(charlist, "\\"); continue());
<string>\\\"		=> (add_string(charlist, "\""); continue());
<string>\\\^[@-_]	=> (add_char(charlist,
				char::from_int(char::to_int(string::get(yytext,2))-char::to_int '@'));
		    		continue());
<string>\\\^.		=>
	(err(yypos,yypos+2) ERROR "illegal control escape; must be one of \
	  \@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" null_error_body;
	 continue());
<string>\\[0-9]{3}	=>
 ( { x = char::to_int(string::get(yytext,1))*100
	     + char::to_int(string::get(yytext,2))*10
	     + char::to_int(string::get(yytext,3))
	     - ((char::to_int '0')*111);
   { if (x>255)
           err (yypos,yypos+4) ERROR "illegal ascii escape" null_error_body;
      else add_char(charlist, char::from_int x); fi;
      continue();};
   } );
<string>\\		=> (err (yypos,yypos+1) ERROR "illegal string escape"
		            null_error_body; 
		            continue());


<string>[\000-\031]  => (err (yypos,yypos+1) ERROR "illegal non-printing character in string" null_error_body;
                    continue());
<string>({idchars}|{someSym}|\[|\]|\(|\)|{backquote}|[,.;^{}])+|.  => (add_string(charlist,yytext); continue());


<char>\'  	 	=> ( {  s = make_string charlist;
                	        s = if (size s != 1 and not *stringtype)
                        	         err(*stringstart,yypos) ERROR
                                      "character constant not length 1"
                                       null_error_body;
                                       substring(s + "x",0,1);
                                    else s; fi;
                                t = (s,*stringstart,yypos+1);
                    	        yybegin initial;
                                if *stringtype  tokens::string t; else tokens::char t; fi;
                             } );

<char>{eol}		=> (err (*stringstart,yypos) ERROR "unclosed string"
			        null_error_body;
			    line_number_db::newline line_number_db yypos;
			    yybegin initial; tokens::string(make_string charlist,*stringstart,yypos));

<char>\\{eol} 	=> (line_number_db::newline line_number_db (yypos+1);
			    yybegin indent; continue());

<char>\\{whitespace} 	=> (yybegin indent; continue());
<char>\\a		=> (add_string(charlist, "\007"); continue());
<char>\\b		=> (add_string(charlist, "\008"); continue());
<char>\\f		=> (add_string(charlist, "\012"); continue());
<char>\\n		=> (add_string(charlist, "\010"); continue());
<char>\\r		=> (add_string(charlist, "\013"); continue());
<char>\\t		=> (add_string(charlist, "\009"); continue());
<char>\\v		=> (add_string(charlist, "\011"); continue());
<char>\\\\		=> (add_string(charlist, "\\"); continue());
<char>\\\"		=> (add_string(charlist, "\""); continue());
<char>\\\^[@-_]	=> (add_char(charlist,
				char::from_int(char::to_int(string::get(yytext,2))-char::to_int '@'));
		    		continue());
<char>\\\^.		=>
	(err(yypos,yypos+2) ERROR "illegal control escape; must be one of \
	  \@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" null_error_body;
	 continue());
<char>\\[0-9]{3}	=>
 ( {  x = char::to_int(string::get(yytext,1))*100
	     + char::to_int(string::get(yytext,2))*10
	     + char::to_int(string::get(yytext,3))
	     - ((char::to_int '0')*111);
      if (x>255)
           err (yypos,yypos+4) ERROR "illegal ascii escape" null_error_body;
      else add_char(charlist, char::from_int x); fi;

      continue();

  } );
<char>\\		=> (err (yypos,yypos+1) ERROR "illegal string escape"
		            null_error_body; 
		            continue());


<char>[\000-\031]  => (err (yypos,yypos+1) ERROR "illegal non-printing character in string" null_error_body;
                    continue());
<char>({idchars}|{someSym}|\[|\]|\(|\)|{backquote}|[,.;^{}])+|.  => (add_string(charlist,yytext); continue());




<indent>{eol}		=> (line_number_db::newline line_number_db yypos; continue());
<indent>{whitespace}	=> (continue());
<indent>\\		=> (yybegin string; stringstart := yypos; continue());
<indent>.		=> (err (*stringstart,yypos) ERROR "unclosed string"
		        	null_error_body; 
		    	yybegin initial; tokens::string(make_string charlist,*stringstart,yypos+1));




<quote>"^`"	=> (add_string(charlist, "`"); continue());
<quote>"^^"	=> (add_string(charlist, "^"); continue());
<quote>"^"      => (yybegin antiquote;
                    { x = make_string charlist;
                    tokens::chunkl(x,yypos,yypos+(size x));
                    } );
<quote>"'''"    => (#  Close a backquote3 scope 
                    yybegin initial;
                    { x = make_string charlist;
                    tokens::endq(x,yypos,yypos+(size x));
                    } );
<quote>{eol}    => (line_number_db::newline line_number_db yypos; add_string(charlist,"\n"); continue());
<quote>.        => (add_string(charlist,yytext); continue());





<antiquote>{eol} 	=> (line_number_db::newline line_number_db yypos; continue());
<antiquote>{whitespace} => (continue());
<antiquote>{value_id}	=> (yybegin quote; 
                    		{ hash = hash_string::hash_string yytext;
                    			tokens::antiquote_id(fast_symbol::raw_symbol(hash,yytext),
					yypos,yypos+(size yytext));
                    		} );

<antiquote>{symbol}+    => (yybegin quote; 
                    { hash = hash_string::hash_string yytext;
                    tokens::antiquote_id(fast_symbol::raw_symbol(hash,yytext),
				yypos,yypos+(size yytext));
                    } );
<antiquote>"("   => (yybegin initial;
                    brack_stack := ((REF 1) ! *brack_stack);
                    tokens::lparen(yypos,yypos+1));
<antiquote>.     => (err (yypos,yypos+1) ERROR
		       ("ml lexer: bad character after antiquote " + yytext)
		       null_error_body;
                    tokens::antiquote_id(fast_symbol::raw_symbol(0u0,""),yypos,yypos));
