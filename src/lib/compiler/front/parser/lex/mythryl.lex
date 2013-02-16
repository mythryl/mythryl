# mythryl.lex



###            "Certain programming errors cannot always
###             be detected [by a compiler], and must be
###             cheaply detectable at run time; in no case
###             can they be allowed to give rise to machine-
###             or implementation-dependent effects, which
###             are inexplicable in terms of the language
###             itself.  This is a criterion to which I
###             give the name 'security'."
###
###                          -- C.A.R. Hoare, 1973



include error_message;

package mythryl_token_table
    =
    mythryl_token_table_g( tokens );	# Defined in ROOT/src/lib/compiler/front/parser/lex/mythryl-token-table-g.pkg

Semantic_Value  =  tokens::Semantic_Value;
Source_Position =  Int;
Lex_Result      =  tokens::Token( Semantic_Value, Source_Position );

Lex_Arg = { comment_nesting_depth : Ref Int, 
	    line_number_db        : line_number_db::Sourcemap,
	    stringlist            : Ref List String,
	    #
	    stringtype            : Ref Bool,
	    stringstart           : Ref Int,            #  Start of current string or comment
	    brack_stack           : Ref List Ref Int,   #  For frags 

	    err : (Source_Position, Source_Position) -> error_message::Plaint_Sink
          };

Arg = Lex_Arg;

Token (X, Y)
     =
     tokens::Token (X, Y);

fun eof ( { comment_nesting_depth, err, stringlist, stringstart, line_number_db, ... } : Lex_Arg)
    =
    {   pos = int::max (   *stringstart + 2,
                           line_number_db::last_change line_number_db
                       );

        if (*comment_nesting_depth > 0)
	    #
	    err (*stringstart,pos) ERROR "unclosed comment" null_error_body;
	    #
        elif (*stringlist != [])
	    #
	    err
		(*stringstart, pos)
		ERROR
		"unclosed string, character, or quotation"
		null_error_body;
        fi;

        tokens::eof(pos,pos);
    };


fun add_string (stringlist, s: String)
    =
    stringlist := s ! *stringlist;


fun add_char (stringlist, c: Char)
    =
    add_string (stringlist, string::from_char c);


fun make_string stringlist
    =
    cat (reverse *stringlist)
    before
        stringlist := NIL;

							# hash_string		is from   src/lib/src/hash-string.pkg
hash_string
    =
    hash_string::hash_string;

							# number_string		is from   src/lib/std/src/number-string.pkg
							# integer		is from   src/lib/std/multiword-int.pkg
							# substring		is from   src/lib/std/substring.pkg
stipulate

    fun convert radix (s, i)
        =
	#1 (the (multiword_int::scan radix substring::getc (substring::drop_first i (substring::from_string s))));
herein
    # At some point we should support underbars in integer constants.
    # Just doing a s/_//g at this point should do, at least as a first cut.  XXX BUGGO FIXME
    #
    atoi   =   convert  number_string::DECIMAL;
    otoi   =   convert  number_string::OCTAL;
    xtoi   =   convert  number_string::HEX;
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

	    _    =>
                 impossible "text in /*#line...*/";

        esac;
    };

fun has_quote s
    =
    {   fun loop i
            =
            (    (string::get(s,i) == '`')
                 or
                 loop (i+1)
            )
	    except _ =   FALSE;

	loop 0;
    };

fun inc (ri as REF i)   =   (ri := i + 1);
fun dec (ri as REF i)   =   (ri := i - 1);


# initial vs postfix states:
#
# We want to use '-' as both a binary infix operator (subtraction)
# and a unary prefix operator (negation).  Similarly, we want to
# use '*' for both multiplication (a*b) and dereferencing (*ptr),
# and we want to use '.' for both (a.b) and (.a b).
#
# We choose to make the distinction based on whitespace:
#          a-b      binary case    (Recognized in postfix state.)
#         a - b     binary case    (Recognized in initial state.)
#         a -b      unary case.    (Recognized in initial state.)
#
# To do this, we need to keep track of whether we "just saw
# some whitespace".  We use the distinction between the initial
# and postfix states to track this information:  When we are
# in initial state then "We just saw whitespace" (i.e. a unary
# prefix operator is a possibility next), otherwise we are in
# postfix state.  Note that if we just saw a '(', for example,
# then we also say that we "just saw whitespace", since a unary op
# here would make sense but a binary op would not.
#   Hence our two states are essentially:
#   postfix: Just saw something like an identifier, so only postfix and infix operators are possible.
#   initial: Just saw something like whitespace,    so only  prefix and infix operators are possible.

# XXX BUGGO FIXME stuff like
#     <initial>"(*_)"		=> (tokens::pre_star(yypos+2,yypos+3));
#   where the token start/end values are bogus results in
#   bogus values propagating all through the system to where
#   they can eventually (e.g.) foul up do-edits and such.
#
#   It would be much better to give correct values here,
#   and then to adjust the symbol itself to exclude the
#   backquotes much later, in an action in the grammar.



# NB: Unlike SML/NJ, we recognize paths like a::b::c here in
#     the lexer, as single tokens, rather than waiting to
#     resolve them in the parser via rules.  The point of
#     this is that it effectively extends the parser's
#     lookahead in critical cases where we need it:
#         foo::bar::Zot
#         foo::bar::zot
#         foo::var::ZOT
#     can be distinguished and different reductions done
#     if they are single tokens resolved in the lexer, but
#     if they are sequences of tokens resolved in the parser,
#     then they all look like just "foo" for lookahead
#     purposes, which is to say, identical, and various rules
#     that now work become shift/reduce errors.

# NB:	I found that
#           <initial>"#PRE"{uppercase_id}{ws}	=> (yybegin pre_compile_code;  continue());
#	compiled ok but that when I did
#	    <initial>"#PRE_COMPILE_CODE"{ws}	=> (yybegin pre_compile_code;  continue());
#	and did "make compiler" I get a linktime segfault:
#	 	 ...
#                load-compiledfiles.c:   Reading   file          COMPILED_FILES_TO_LOAD
#                /mythryl7/mythryl7.110.58/mythryl7.110.58/bin/mythryl-runtime-intel32: Fatal error:  Bogus fault not in Mythryl: sig = 11, code = 0x805879b, pc = 0x805879b)
#                sh/make-compiler-executable:   Compiler link failed, no mythryld executable
#	It appears that we may have hit some sort of 64K type limit here;
#	attempting to add
#           <initial>"#PRE_{uppercase_id}{ws}	=> (yybegin postcompile_code;  continue());
#           <initial>"#POST"{uppercase_id}{ws}	=> (yybegin postcompile_code;  continue());
#	also produced a segfault. :-(	XXX BUGGO FIXME -- 2011-09-11 CrT
#       2012-02-22 CrT: The above was probably the Great Heisenbug, which is now fixed.
#                       It would be worth trying this again.
#       In the meantime, I switched to just #DO for #PRE_COMPILE_CODE and dropped #POSTCOMPILE_CODE entirely.



%% 
%reject
%s aaa comment string char stringgap backticks dot_backticks dot_qquotes dot_quotes dot_brokets dot_barets dot_slashets dot_hashets qqq aq lll ll llc llcq postfix pre_compile_code;
%header (generic package mythryl_lex_g(package tokens : Mythryl_Tokens;));
%arg ( {
  comment_nesting_depth,
  line_number_db,
  err,
  stringlist,
  stringstart,
  stringtype,
  brack_stack});
idchars=[A-Za-z_0-9];
uppercase_id=[A-Z][A-Z'_0-9]*[A-Z][A-Z'_0-9]*;
mixedcase_id=[A-Z][A-Za-z'_0-9]*[a-z][A-Za-z'_0-9]*;
lowercase_id=[a-z]('|[a-z_0-9])*;
id=[A-Za-z]('?{idchars})*'*;
ws=("\012"|[\t\ ])*;
nrws=("\012"|[\t\ ])+;
eol=("\013\010"|"\010"|"\013");
symbol_sans_backslash=[!%&$+/:<=>?@~|*]|\-|\^;
symbol={symbol_sans_backslash}|"\\";
backtick="`";
hash="#";
full_sym={symbol};
num=[0-9]+;
frac="."{num};
exp=[eE]([-]?){num};
float=([-]?)(({num}{frac}?{exp})|({num}{frac}{exp}?));
hexnum=[0-9a-fA-F]+;

uppercase_path=({lowercase_id}::)+{uppercase_id};
mixedcase_path=({lowercase_id}::)+{mixedcase_id};
lowercase_path=({lowercase_id}::)+{lowercase_id};
operators_path=({lowercase_id}::)+( \("_"?{symbol}+"_"?\) | "(|_|)" | "(<_>)" | "(/_/)" | "({_})" | "(_[])" | "(_[]:=)" );

%%
<initial>{ws}	=> (continue());
<initial>{eol}	=> (line_number_db::newline line_number_db yypos; continue());
<initial>"_"	=> (tokens::wild(yypos,yypos+1));
<initial>","	=> (tokens::comma(yypos,yypos+1));
<initial>".{"	=> (tokens::dot_lbrace(yypos,yypos+2));
<initial>"}"	=> (yybegin postfix; tokens::rbrace(yypos,yypos+1));
<initial>"["	=> (tokens::lbracket(yypos,yypos+1));
<initial>"#["	=> (tokens::vectorstart(yypos,yypos+1));
<initial>"]"	=> (yybegin postfix; tokens::rbracket(yypos,yypos+1));
<initial>";"	=> (tokens::semi(yypos,yypos+1));
<initial>"("{full_sym}+")" => (mythryl_token_table::check_passive_symbol_id(yytext,yypos));
<initial>"("	=> (if ((null *brack_stack))
                         ();
                    else inc (head *brack_stack); fi;
                    tokens::lparen(yypos,yypos+1));
<initial>")"	=> (yybegin postfix;
                    if (null *brack_stack)
                         ();
                    else if  (*(head *brack_stack) == 1)
                               brack_stack := tail *brack_stack;
                               stringlist := [];
                               yybegin qqq;
                         else
                               dec (head *brack_stack);
                         fi;
                    fi;
                    tokens::rparen(yypos,yypos+1));
<initial>"&"{nrws}	=> (tokens::amper(yypos,yypos+1));
<initial>"@"{nrws}	=> (tokens::atsign(yypos,yypos+1));
<initial>"\\"{nrws}	=> (tokens::back(yypos,yypos+1));
<initial>"!"{nrws}	=> (tokens::uppercase_id (fast_symbol::raw_symbol ((hash_string "!"), "!"), yypos, yypos+1));
<initial>"|"{nrws}	=> (tokens::bar(yypos,yypos+1));
<initial>"$"{nrws}	=> (tokens::buck(yypos,yypos+1));
<initial>"^"{nrws}	=> (tokens::caret(yypos,yypos+1));
<initial>"-"{nrws}	=> (tokens::dash(yypos,yypos+1));
<initial>"{"{nrws}	=> (tokens::lbrace(yypos,yypos+1));
<initial>"<"{nrws}	=> (tokens::langle(yypos,yypos+1));
<initial>">"{nrws}	=> (tokens::rangle(yypos,yypos+1));
<initial>"%"{nrws}	=> (tokens::percnt(yypos,yypos+1));
<initial>"+"{nrws}	=> (tokens::plus(yypos,yypos+1));
<initial>"?"{nrws}	=> (tokens::qmark(yypos,yypos+1));
<initial>"/"{nrws}	=> (tokens::slash(yypos,yypos+1));
<initial>"*"{nrws}	=> (tokens::star(yypos,yypos+1));
<initial>"~"{nrws}	=> (tokens::tilda(yypos,yypos+1));
<initial>"++"{nrws}	=> (tokens::plus_plus(yypos,yypos+2));
<initial>"--"{nrws}	=> (tokens::dash_dash(yypos,yypos+2));
<initial>".."{nrws}	=> (tokens::dotdot(yypos,yypos+2));
<initial>"."{nrws}	=> (tokens::operators_id (fast_symbol::raw_symbol ((hash_string " . "), " . "), yypos, yypos+1));
<initial>"&"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::amper(yypos,yypos+1));
<initial>"@"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::atsign(yypos,yypos+1));
<initial>"\\"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::back(yypos,yypos+1));
<initial>"!"{eol}	=> (line_number_db::newline line_number_db yypos; (tokens::uppercase_id (fast_symbol::raw_symbol ((hash_string "!"), "!"), yypos, yypos+1)));
<initial>"|"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::bar(yypos,yypos+1));
<initial>"$"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::buck(yypos,yypos+1));
<initial>"^"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::caret(yypos,yypos+1));
<initial>"-"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::dash(yypos,yypos+1));
<initial>"{"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::lbrace(yypos,yypos+1));
<initial>"<"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::langle(yypos,yypos+1));
<initial>">"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::rangle(yypos,yypos+1));
<initial>"%"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::percnt(yypos,yypos+1));
<initial>"+"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::plus(yypos,yypos+1));
<initial>"?"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::qmark(yypos,yypos+1));
<initial>"/"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::slash(yypos,yypos+1));
<initial>"*"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::star(yypos,yypos+1));
<initial>"~"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::tilda(yypos,yypos+1));
<initial>"."{eol}	=> (line_number_db::newline line_number_db yypos; (tokens::operators_id (fast_symbol::raw_symbol ((hash_string " . "), " . "), yypos, yypos+1)));
<initial>"++"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::plus_plus(yypos,yypos+2));
<initial>"--"{eol}	=> (line_number_db::newline line_number_db yypos; tokens::dash_dash(yypos,yypos+2));
<initial>".."{eol}	=> (line_number_db::newline line_number_db yypos; tokens::dotdot(yypos,yypos+2));
<initial>"(&_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "&_"), "&_"), yypos+1, yypos+3) );
<initial>"(@_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "@_"), "@_"), yypos+1, yypos+3) );
<initial>"(\\_)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "\\_"),"\\_"),yypos+1, yypos+3) );
<initial>"(!_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "!_"), "!_"), yypos+1, yypos+3) );
<initial>"($_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "$_"), "$_"), yypos+1, yypos+3) );
<initial>"(^_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "^_"), "^_"), yypos+1, yypos+3) );
<initial>"(-_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "-_"), "-_"), yypos+1, yypos+3) );
<initial>"(%_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "%_"), "%_"), yypos+1, yypos+3) );
<initial>"(+_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "+_"), "+_"), yypos+1, yypos+3) );
<initial>"(?_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "?_"), "?_"), yypos+1, yypos+3) );
<initial>"(/_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/_"), "/_"), yypos+1, yypos+3) );
<initial>"(*_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "*_"), "*_"), yypos+1, yypos+3) );
<initial>"(~_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "~_"), "~_"), yypos+1, yypos+3) );
<initial>"(&)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "&"), "&"), yypos+1, yypos+2) );
<initial>"(@)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "@"), "@"), yypos+1, yypos+2) );
<initial>"(\\)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "\\"),"\\"),yypos+1, yypos+2) );
<initial>"(!)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "!"), "!"), yypos+1, yypos+2) );
<initial>"(|)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "|"), "|"), yypos+1, yypos+2) );
<initial>"($)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "$"), "$"), yypos+1, yypos+2) );
<initial>"(^)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "^"), "^"), yypos+1, yypos+2) );
<initial>"(-)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "-"), "-"), yypos+1, yypos+2) );
<initial>"(%)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "%"), "%"), yypos+1, yypos+2) );
<initial>"(<)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "<"), "<"), yypos+1, yypos+2) );
<initial>"(>)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string ">"), ">"), yypos+1, yypos+2) );
<initial>"(+)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "+"), "+"), yypos+1, yypos+2) );
<initial>"(?)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "?"), "?"), yypos+1, yypos+2) );
<initial>"(/)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/"), "/"), yypos+1, yypos+2) );
<initial>"(*)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "*"), "*"), yypos+1, yypos+2) );
<initial>"(~)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "~"), "~"), yypos+1, yypos+2) );
<initial>"( . )"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string " . "), " . "), yypos, yypos+1));
<initial>"(_&)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_&"), "_&"), yypos+1, yypos+3) );
<initial>"(_@)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_@"), "_@"), yypos+1, yypos+3) );
<initial>"(_\\)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_\\"),"_\\"),yypos+1, yypos+3) );
<initial>"(_!)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_!"), "_!"), yypos+1, yypos+3) );
<initial>"(_$)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_$"), "_$"), yypos+1, yypos+3) );
<initial>"(_^)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_^"), "_^"), yypos+1, yypos+3) );
<initial>"(_-)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_-"), "_-"), yypos+1, yypos+3) );
<initial>"(_%)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_%"), "_%"), yypos+1, yypos+3) );
<initial>"(_+)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_+"), "_+"), yypos+1, yypos+3) );
<initial>"(_?)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_?"), "_?"), yypos+1, yypos+3) );
<initial>"(_/)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_/"), "_/"), yypos+1, yypos+3) );
<initial>"(_*)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_*"), "_*"), yypos+1, yypos+3) );
<initial>"(_~)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_~"), "_~"), yypos+1, yypos+3) );
<initial>"(|_|)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "|_|"),"|_|"),yypos+1, yypos+4) );
<initial>"(<_>)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "<_>"),"<_>"),yypos+1, yypos+4) );
<initial>"(/_/)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/_/"),"/_/"),yypos+1, yypos+4) );
<initial>"({_})"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "{_}"),"{_}"),yypos+1, yypos+4) );
<initial>"(_[])"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string"_[]"),"_[]"),yypos+1, yypos+5) );
<initial>"(_[]:=)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string"_[]:="),"_[]:="),yypos+1, yypos+7) );
<initial>"."		=> (tokens::pre_dot(yypos,yypos+1));
<initial>".="		=> (tokens::dot_eq(yypos,yypos+2));
<initial>"|"		=> (tokens::pre_bar(yypos, yypos+1));
<initial>"<"		=> (tokens::pre_langle(yypos, yypos+1));
<initial>"{"		=> (tokens::pre_lbrace(yypos, yypos+1));
<initial>"/"		=> (tokens::pre_slash(yypos, yypos+1));
<initial>"++"		=> (tokens::pre_plusplus(yypos, yypos+2));
<initial>"--"		=> (tokens::pre_dashdash(yypos, yypos+2));
<initial>".."		=> (tokens::pre_dotdot(yypos, yypos+2));
<initial>"!"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "!_"),"!_"), yypos, yypos+1));
<initial>"*"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "*_"),"*_"), yypos, yypos+1));
<initial>"-"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "-_"),"-_"), yypos, yypos+1));
<initial>"\\"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "\\_"),"\\_"), yypos, yypos+1));
<initial>"&"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "&_"), "&_"), yypos, yypos+1));
<initial>"@"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "@_"), "@_"), yypos, yypos+1));
<initial>"$"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "$_"), "$_"), yypos, yypos+1));
<initial>"^"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "^_"), "^_"), yypos, yypos+1));
<initial>"%"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "%_"), "%_"), yypos, yypos+1));
<initial>"+"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "+_"), "+_"), yypos, yypos+1));
<initial>"?"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "?_"), "?_"), yypos, yypos+1));
<initial>"/"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "/_"), "/_"), yypos, yypos+1));
<initial>"~"		=> (tokens::prefix_op_id (fast_symbol::raw_symbol ((hash_string "~_"), "~_"), yypos, yypos+1));
<initial>"..."		=> (tokens::dotdotdot(yypos,yypos+3));
<initial>": (weak)"	=> (tokens::weak_package_cast(yypos,yypos+8));
<initial>": (partial)"	=> (tokens::partial_package_cast(yypos,yypos+11));
<initial>"/*"[*=#-]*	=> (yybegin aaa; stringstart := yypos; comment_nesting_depth := 1; continue());
<initial>"*/"	=> (err (yypos,yypos+1) ERROR "unmatched close comment"
		        null_error_body;
		    continue());
<initial>"_"?[A-Z][0-9]*"'"*      => (mythryl_token_table::new_check_type_var(yytext,yypos));
<initial>"_"?[A-Z]_{lowercase_id}  => (mythryl_token_table::new_check_type_var(yytext,yypos));
<initial>"#"{lowercase_id} => (yybegin postfix; mythryl_token_table::check_implicit_thunk_parameter(yytext,yypos));
<initial>"-F"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::isfile"),     yypos, yypos+2));
<initial>"-D"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::isdir"),      yypos, yypos+2));
<initial>"-P"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::ispipe"),     yypos, yypos+2));
<initial>"-L"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::issymlink"),  yypos, yypos+2));
<initial>"-C"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::ischardev"),  yypos, yypos+2));
<initial>"-B"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::isblockdev"), yypos, yypos+2));
<initial>"-S"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::issocket"),   yypos, yypos+2));
<initial>"-R"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::mayread"),    yypos, yypos+2));
<initial>"-W"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::maywrite"),   yypos, yypos+2));
<initial>"-X"           => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string "scripting_globals::mayexecute"), "scripting_globals::mayexecute"), yypos, yypos+2));
<initial>"("{lowercase_id}")" => (yybegin postfix; mythryl_token_table::check_passive_id(yytext, yypos));
<initial>{lowercase_id} => (yybegin postfix; mythryl_token_table::check_id(yytext, yypos));
<initial>{mixedcase_id} => (yybegin postfix; tokens::mixedcase_id (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{uppercase_id} => (yybegin postfix; tokens::uppercase_id (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{operators_path} => (yybegin postfix; tokens::operators_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{uppercase_path} => (yybegin postfix; tokens::uppercase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{mixedcase_path} => (yybegin postfix; tokens::mixedcase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{lowercase_path} => (yybegin postfix; tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<initial>{full_sym}+    => (if (*mythryl_parser::support_smlnj_antiquotes)
                                 if (has_quote yytext)
                                      REJECT();
                                 else mythryl_token_table::check_symbol_id(yytext,yypos);
                                 fi;
                            else mythryl_token_table::check_symbol_id(yytext,yypos);
                            fi
                           );
<initial>{hash}            => (mythryl_token_table::check_symbol_id(yytext,yypos));
<initial>{symbol}+         => (mythryl_token_table::check_symbol_id(yytext,yypos));
<initial>{backtick}        => (    yybegin backticks;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                            /* if (*mythryl_parser::support_smlnj_antiquotes)
                                  yybegin qqq;
                                   stringlist := [];
                                   tokens::beginq(yypos,yypos+1);
                            else  err(yypos, yypos+1)
                                     ERROR "smlnj_antiquotes implementation error"
				     null_error_body;
                                  tokens::backticks(yypos,yypos+1); */
                             );

<initial>"\.\`"            => (    yybegin dot_backticks;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\""            => (    yybegin dot_qquotes;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\'"            => (    yybegin dot_quotes;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\<"            => (    yybegin dot_brokets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\|"            => (    yybegin dot_barets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\/"            => (    yybegin dot_slashets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>"\.\#"            => (    yybegin dot_hashets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<initial>{float}         => (yybegin postfix; tokens::float(yytext, yypos, yypos + size yytext));
<initial>[1-9][0-9]*     => (yybegin postfix; tokens::int(atoi(yytext, 0),yypos,yypos+size yytext));
<initial>"0"{num}        => (yybegin postfix; tokens::int0(otoi(yytext, 1),yypos,yypos+size yytext));
<initial>{num}	         => (yybegin postfix; tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));
<initial>[-]{num}        => (yybegin postfix; tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));
<initial>"0x"{hexnum}    => (yybegin postfix; tokens::int0(xtoi(yytext, 2),yypos,yypos+size yytext));
<initial>[-]"0x"{hexnum} => (yybegin postfix; tokens::int0(multiword_int::(-_)(xtoi(yytext, 3)),yypos,yypos+size yytext));
<initial>"0u"{num}       => (yybegin postfix; tokens::unt(atoi(yytext, 2),yypos,yypos+size yytext));
<initial>"0ux"{hexnum}   => (yybegin postfix; tokens::unt(xtoi(yytext, 3),yypos,yypos+size yytext));

<initial>\"	=> (stringlist := [""]; stringstart := yypos;
                    stringtype := TRUE; yybegin string; continue());
<initial>\'	=> (stringlist := [""]; stringstart := yypos;
                    stringtype := FALSE; yybegin char; continue());
<initial>"/*#line"{nrws}  => 
                   (yybegin lll; stringstart := yypos; comment_nesting_depth := 1; continue());
<initial>"#"{eol} => (line_number_db::newline line_number_db yypos; continue());
<initial>"# "	=> (yybegin comment;  continue());
<initial>"#\t"  => (yybegin comment;  continue());
<initial>\#\!	=> (yybegin comment;  continue());
<initial>\#\#	=> (yybegin comment;  continue());

<initial>"#DO"{ws}[^;\013\010]+ =>  (tokens::pre_compile_code ((substring::to_string (substring::drop_first 4 (substring::from_string yytext))), yypos+4, yypos + size yytext));

<initial>\h	=> (err (yypos,yypos) ERROR "non-Ascii character"
		        null_error_body;
		    continue());
<initial>.	=> (err (yypos,yypos) ERROR "illegal token" null_error_body;
		    continue());


<postfix>{nrws}	=> (yybegin initial; continue());
<postfix>{eol}	=> (line_number_db::newline line_number_db yypos; yybegin initial; continue());
<postfix>"_"	=> (tokens::wild(yypos,yypos+1));
<postfix>","	=> (yybegin initial; tokens::comma(yypos,yypos+1));
<postfix>".{"	=> (yybegin initial; tokens::dot_lbrace(yypos,yypos+2));
<postfix>"{"	=> (yybegin initial; tokens::lbrace(yypos,yypos+1));
<postfix>"["	=> (yybegin initial; tokens::post_lbracket(yypos,yypos+1));
<postfix>"#["	=> (yybegin initial; tokens::vectorstart(yypos,yypos+1));
<postfix>"]"	=> (tokens::rbracket(yypos,yypos+1));
<postfix>";"	=> (yybegin initial; tokens::semi(yypos,yypos+1));
<postfix>"("{full_sym}+")" => (mythryl_token_table::check_passive_symbol_id(yytext,yypos));
<postfix>"("	=> (if (null *brack_stack)
                         ();
                    else inc (head *brack_stack);
                    fi;
                    yybegin initial; 
                    tokens::lparen(yypos,yypos+1));
<postfix>")"	=> (if (null *brack_stack)
                         ();
                    else if (*(head *brack_stack) == 1)
                               brack_stack := tail *brack_stack;
                                stringlist := [];
                                yybegin qqq;
                              
                         else dec (head *brack_stack);
                         fi;
                    fi;  
                    tokens::rparen(yypos,yypos+1));
<postfix>"&"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_&"),"_&"), yypos, yypos+1));
<postfix>"!"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_!"),"_!"), yypos, yypos+1));
<postfix>"@"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_@"),"_@"), yypos, yypos+1));
<postfix>"$"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_$"),"_$"), yypos, yypos+1));
<postfix>"\\"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_\\"),"_\\"), yypos, yypos+1));
<postfix>"^"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_^"),"_^"), yypos, yypos+1));
<postfix>"-"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_-"),"_-"), yypos, yypos+1));
<postfix>"%"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_%"),"_%"), yypos, yypos+1));
<postfix>"+"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_+"),"_+"), yypos, yypos+1));
<postfix>"?"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_?"),"_?"), yypos, yypos+1));
<postfix>"*"{nrws}	=> (yybegin initial; tokens::postfix_op_id (fast_symbol::raw_symbol ((hash_string "_*"),"_*"), yypos, yypos+1));
<postfix>"/"{nrws}	=> (yybegin initial; tokens::post_slash(yypos, yypos+1));
<postfix>"|"{nrws}	=> (yybegin initial; tokens::post_bar(yypos, yypos+1));
<postfix>">"{nrws}	=> (yybegin initial; tokens::post_rangle(yypos, yypos+1));
<postfix>"}"{nrws}	=> (yybegin initial; tokens::post_rbrace(yypos, yypos+1));
<postfix>"++"{nrws}	=> (yybegin initial; tokens::post_plusplus(yypos, yypos+2));
<postfix>"--"{nrws}	=> (yybegin initial; tokens::post_dashdash(yypos, yypos+2));
<postfix>".."{nrws}	=> (yybegin initial; tokens::post_dotdot(yypos, yypos+2));
<postfix>"&"		=> (tokens::amper(yypos,yypos+1));
<postfix>"@"		=> (tokens::atsign(yypos,yypos+1));
<postfix>"\\"		=> (tokens::back(yypos,yypos+1));
<postfix>"!"		=> (tokens::bang(yypos,yypos+1));
<postfix>"|"		=> (tokens::bar(yypos,yypos+1));
<postfix>"$"		=> (tokens::buck(yypos,yypos+1));
<postfix>"^"		=> (tokens::caret(yypos,yypos+1));
<postfix>"-"		=> (tokens::dash(yypos,yypos+1));
<postfix>"<"		=> (tokens::langle(yypos,yypos+1));
<postfix>">"		=> (tokens::rangle(yypos,yypos+1));
<postfix>"}"     	=> (tokens::rbrace(yypos,yypos+1));
<postfix>"%"		=> (tokens::percnt(yypos,yypos+1));
<postfix>"+"		=> (tokens::plus(yypos,yypos+1));
<postfix>"?"		=> (tokens::qmark(yypos,yypos+1));
<postfix>"/"		=> (tokens::slash(yypos,yypos+1));
<postfix>"*"		=> (tokens::star(yypos,yypos+1));
<postfix>"~"		=> (tokens::tilda(yypos,yypos+1));
<postfix>"++"		=> (tokens::plus_plus(yypos,yypos+2));
<postfix>"--"		=> (tokens::dash_dash(yypos,yypos+2));
<postfix>"(&_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "&_"), "&_"), yypos+1, yypos+3) );
<postfix>"(@_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "@_"), "@_"), yypos+1, yypos+3) );
<postfix>"(\\_)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "\\_"),"\\_"),yypos+1, yypos+3) );
<postfix>"(!_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "!_"), "!_"), yypos+1, yypos+3) );
<postfix>"($_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "$_"), "$_"), yypos+1, yypos+3) );
<postfix>"(^_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "^_"), "^_"), yypos+1, yypos+3) );
<postfix>"(-_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "-_"), "-_"), yypos+1, yypos+3) );
<postfix>"(%_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "%_"), "%_"), yypos+1, yypos+3) );
<postfix>"(+_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "+_"), "+_"), yypos+1, yypos+3) );
<postfix>"(?_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "?_"), "?_"), yypos+1, yypos+3) );
<postfix>"(/_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/_"), "/_"), yypos+1, yypos+3) );
<postfix>"(*_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "*_"), "*_"), yypos+1, yypos+3) );
<postfix>"(~_)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "~_"), "~_"), yypos+1, yypos+3) );
<postfix>"(&)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "&"), "&"), yypos+1, yypos+2) );
<postfix>"(@)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "@"), "@"), yypos+1, yypos+2) );
<postfix>"(\\)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "\\"),"\\"),yypos+1, yypos+2) );
<postfix>"(!)"		=> (tokens::uppercase_id (fast_symbol::raw_symbol ((hash_string "!"), "!"), yypos+1, yypos+2) );
<postfix>"(|)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "|"), "|"), yypos+1, yypos+2) );
<postfix>"($)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "$"), "$"), yypos+1, yypos+2) );
<postfix>"(^)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "^"), "^"), yypos+1, yypos+2) );
<postfix>"(-)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "-"), "-"), yypos+1, yypos+2) );
<postfix>"( . )"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string " . "), " . "), yypos, yypos+1));
<postfix>"(<)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "<"), "<"), yypos+1, yypos+2) );
<postfix>"(>)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string ">"), ">"), yypos+1, yypos+2) );
<postfix>"(%)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "%"), "%"), yypos+1, yypos+2) );
<postfix>"(+)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "+"), "+"), yypos+1, yypos+2) );
<postfix>"(?)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "?"), "?"), yypos+1, yypos+2) );
<postfix>"(/)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/"), "/"), yypos+1, yypos+2) );
<postfix>"(*)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "*"), "*"), yypos+1, yypos+2) );
<postfix>"(~)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "~"), "~"), yypos+1, yypos+2) );
<postfix>"(_&)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_&"), "_&"), yypos+1, yypos+3) );
<postfix>"(_@)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_@"), "_@"), yypos+1, yypos+3) );
<postfix>"(_\\)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_\\"),"_\\"),yypos+1, yypos+3) );
<postfix>"(_!)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_!"), "_!"), yypos+1, yypos+3) );
<postfix>"(_$)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_$"), "_$"), yypos+1, yypos+3) );
<postfix>"(_^)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_^"), "_^"), yypos+1, yypos+3) );
<postfix>"(_-)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_-"), "_-"), yypos+1, yypos+3) );
<postfix>"(_%)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_%"), "_%"), yypos+1, yypos+3) );
<postfix>"(_+)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_+"), "_+"), yypos+1, yypos+3) );
<postfix>"(_?)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_?"), "_?"), yypos+1, yypos+3) );
<postfix>"(_/)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_/"), "_/"), yypos+1, yypos+3) );
<postfix>"(_*)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_*"), "_*"), yypos+1, yypos+3) );
<postfix>"(_~)"		=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_~"), "_~"), yypos+1, yypos+3) );
<postfix>"(|_|)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "|_|"), "|_|"), yypos+1, yypos+4) );
<postfix>"(<_>)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "<_>"), "<_>"), yypos+1, yypos+4) );
<postfix>"(/_/)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "/_/"), "/_/"), yypos+1, yypos+4) );
<postfix>"({_})"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "{_}"), "{_}"), yypos+1, yypos+4) );
<postfix>"(_[])"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_[]"), "_[]"), yypos+1, yypos+5) );
<postfix>"(_[]:=)"	=> (tokens::passiveop_id (fast_symbol::raw_symbol ((hash_string "_[]:="), "_[]:="), yypos+1, yypos+7) );
<postfix>".="		=> (tokens::dot_eq(yypos,yypos+2));
<postfix>"->"		=> (tokens::postfix_arrow(yypos,yypos+2));
<postfix>"."		=> (tokens::dot(yypos,yypos+1));
<postfix>".."		=> (tokens::dotdot(yypos,yypos+2));
<postfix>"..."		=> (tokens::dotdotdot(yypos,yypos+3));
<postfix>": (weak)"	=> (tokens::weak_package_cast(yypos,yypos+8));
<postfix>": (partial)"	=> (tokens::partial_package_cast(yypos,yypos+11));
<postfix>"/*"[*=#-]*	=> (yybegin aaa; stringstart := yypos; comment_nesting_depth := 1; continue());
<postfix>"*/"	=> (err (yypos,yypos+1) ERROR "unmatched close comment"
		        null_error_body;
		    continue());
<postfix>"_"?[A-Z][0-9]*"'"*      => (mythryl_token_table::new_check_type_var(yytext,yypos));
<postfix>"_"?[A-Z]_{lowercase_id}  => (mythryl_token_table::new_check_type_var(yytext,yypos));
<postfix>"#"{lowercase_id} => (mythryl_token_table::check_implicit_thunk_parameter(yytext,yypos));
<initial>"-F"           => (mythryl_token_table::check_id("is_file",      yypos));
<initial>"-D"           => (mythryl_token_table::check_id("is_dir",       yypos));
<initial>"-P"           => (mythryl_token_table::check_id("is_pipe",      yypos));
<initial>"-L"           => (mythryl_token_table::check_id("is_symlink",   yypos));
<initial>"-C"           => (mythryl_token_table::check_id("is_char_dev",  yypos));
<initial>"-B"           => (mythryl_token_table::check_id("is_block_dev", yypos));
<initial>"-S"           => (mythryl_token_table::check_id("is_socket",    yypos));
<postfix>"("{lowercase_id}")" => (mythryl_token_table::check_passive_id(yytext, yypos));
<postfix>{lowercase_id} => (mythryl_token_table::check_id(yytext, yypos));
<postfix>{mixedcase_id} => (tokens::mixedcase_id (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<postfix>{uppercase_id} => (tokens::uppercase_id (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos+size (yytext)));
<postfix>{operators_path} => (tokens::operators_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos + size(yytext)));
<postfix>{uppercase_path} => (tokens::uppercase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos + size(yytext)));
<postfix>{mixedcase_path} => (tokens::mixedcase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos + size(yytext)));
<postfix>{lowercase_path} => (tokens::lowercase_path (fast_symbol::raw_symbol ((hash_string yytext), yytext), yypos, yypos + size(yytext)));
<postfix>{full_sym}+    => (if (*mythryl_parser::support_smlnj_antiquotes)
                                 if (has_quote yytext)
                                      REJECT();
                                 else mythryl_token_table::check_symbol_id(yytext,yypos);
                                 fi;
                            else mythryl_token_table::check_symbol_id(yytext,yypos);
                            fi
                           );
<postfix>{hash}            => (mythryl_token_table::check_symbol_id(yytext,yypos));
<postfix>{symbol}+         => (mythryl_token_table::check_symbol_id(yytext,yypos));
<postfix>{backtick}     => (       yybegin backticks;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                              /* if *mythryl_parser::support_smlnj_antiquotes
                                  yybegin qqq;
                                  stringlist := [];
                                  tokens::beginq(yypos,yypos+1);
                            
                                else err(yypos, yypos+1)
                                     ERROR "smlnj_antiquotes implementation error"
				     null_error_body;
                                  tokens::beginq(yypos,yypos+1);  */
                            );

<postfix>"\.\`"            => (    yybegin dot_backticks;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>"\.\""            => (    yybegin dot_qquotes;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>"\.\'"            => (    yybegin dot_quotes;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>"\.\<"            => (    yybegin dot_brokets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>"\.\|"            => (    yybegin dot_barets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>"\.\/"            => (    yybegin dot_hashets;
                                   stringlist := [];
                                   stringstart := yypos;
                                   continue()
                             );

<postfix>{float}         => (tokens::float(yytext,yypos,yypos+size yytext));
<postfix>[1-9][0-9]*     => (tokens::int(atoi(yytext, 0),yypos,yypos+size yytext));
<postfix>"0"{num}        => (tokens::int0(otoi(yytext, 1),yypos,yypos+size yytext));
<postfix>{num}	         => (tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));
<postfix>[-]{num}        => (tokens::int0(atoi(yytext, 0),yypos,yypos+size yytext));
<postfix>"0x"{hexnum}    => (tokens::int0(xtoi(yytext, 2),yypos,yypos+size yytext));
<postfix>[-]"0x"{hexnum} => (tokens::int0(multiword_int::(-_)(xtoi(yytext, 3)),yypos,yypos+size yytext));
<postfix>"0u"{num}       => (tokens::unt(atoi(yytext, 2),yypos,yypos+size yytext));
<postfix>"0ux"{hexnum}   => (tokens::unt(xtoi(yytext, 3),yypos,yypos+size yytext));
<postfix>\"	=> (stringlist := [""]; stringstart := yypos;
                    stringtype := TRUE; yybegin string; continue());
<postfix>\'	=> (stringlist := [""]; stringstart := yypos;
                    stringtype := FALSE; yybegin char; continue());
<postfix>"/*#line"{nrws}  => 
                   (yybegin lll; stringstart := yypos; comment_nesting_depth := 1; continue());
<postfix>"#"{eol} => (line_number_db::newline line_number_db yypos; yybegin initial; continue());
<postfix>"# "	=> (yybegin comment;  continue());
<postfix>"#\t"  => (yybegin comment;  continue());
<postfix>\#\!	=> (yybegin comment;  continue());
<postfix>\#\#	=> (yybegin comment;  continue());
<postfix>\h	=> (err (yypos,yypos) ERROR "non-Ascii character"
		        null_error_body;
		    continue());
<postfix>.	=> (err (yypos,yypos) ERROR "illegal token" null_error_body;
		    continue());


<lll>[0-9]+                 => (yybegin ll; stringlist := [yytext]; continue());
<ll>\.                    => (/* cheat: take n > 0 dots */ continue());
<ll>[0-9]+                => (yybegin llc; add_string(stringlist, yytext); continue());
<ll>0*               	  => (yybegin llc; add_string(stringlist, "1");    continue()
		/* note hack, since mythryl-lex chokes on the empty string for 0* */);
<llc>"*/"                 => (yybegin initial; my_synch(line_number_db, yypos+2, *stringlist); 
		              comment_nesting_depth := 0; stringlist := []; continue());
<llc>{ws}\"		  => (yybegin llcq; continue());
<llcq>[^\"]*              => (add_string(stringlist, yytext); continue());
<llcq>\""*/"              => (yybegin initial; my_synch(line_number_db, yypos+3, *stringlist); 
		              comment_nesting_depth := 0; stringlist := []; continue());
<lll,llc,llcq>"*/" => (err (*stringstart, yypos+1) WARNING 
                       "ill-formed /*#line...*/ taken as comment" null_error_body;
                     yybegin initial; comment_nesting_depth := 0; stringlist := []; continue());
<lll,llc,llcq>.    => (err (*stringstart, yypos+1) WARNING 
                       "ill-formed /*#line...*/ taken as comment" null_error_body;
                     yybegin aaa; continue());

<comment>{eol}	=> (line_number_db::newline line_number_db yypos; yybegin initial; continue());
<comment>.	=> (continue());


<aaa>"/*"[*=#-]* => (inc comment_nesting_depth; continue());
<aaa>{eol}	 => (line_number_db::newline line_number_db yypos; continue());
<aaa>"*/"	 => (dec comment_nesting_depth; if (*comment_nesting_depth==0 ) yybegin initial; fi; continue());
<aaa>.		 => (continue());

<string>\"       => ( { s = make_string stringlist;
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
                    });
<string>{eol}	=> (err (*stringstart,yypos) ERROR "unclosed string"
		        null_error_body;
		    line_number_db::newline line_number_db yypos;
		    yybegin initial; tokens::string(make_string stringlist,*stringstart,yypos));
<string>\\{eol}	=> (line_number_db::newline line_number_db (yypos+1);
		    yybegin stringgap; continue());
<string>\\{ws} 	=> (yybegin stringgap; continue());
<string>\\a		=> (add_string(stringlist, "\007"); continue());
<string>\\b		=> (add_string(stringlist, "\008"); continue());
<string>\\f		=> (add_string(stringlist, "\012"); continue());
<string>\\n		=> (add_string(stringlist, "\010"); continue());
<string>\\r		=> (add_string(stringlist, "\013"); continue());
<string>\\t		=> (add_string(stringlist, "\009"); continue());
<string>\\v		=> (add_string(stringlist, "\011"); continue());
<string>\\\\		=> (add_string(stringlist, "\\"); continue());
<string>\\\"		=> (add_string(stringlist, "\""); continue());
<string>\\\^[@-_]	=> (add_char(stringlist,
			char::from_int(char::to_int(string::get(yytext,2))-char::to_int '@'));
		    continue());
<string>\\\^.	=>
	(err(yypos,yypos+2) ERROR "illegal control escape; must be one of \
	  \@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" null_error_body;
	 continue());
<string>\\[0-9]{3}	=>
 ( {  x = char::to_int(string::get(yytext,1))*100
	     + char::to_int(string::get(yytext,2))*10
	     + char::to_int(string::get(yytext,3))
	     - ((char::to_int '0')*111);
   {  if   (x > 255)
           err (yypos,yypos+4) ERROR "illegal ascii escape" null_error_body;
      else add_char(stringlist, char::from_int x);
      fi;
      continue();
   };
  });
<string>\\		=> (err (yypos,yypos+1) ERROR "illegal string escape"
		        null_error_body; 
		    continue());


<string>[\000-\031]  => (err (yypos,yypos+1) ERROR "illegal non-printing character in string" null_error_body;
                    continue());
<string>({idchars}|{symbol_sans_backslash}|\[|\]|\(|\)|{backtick}|{hash}|[',.;^{}])+|.  => (add_string(stringlist,yytext); continue());


<backticks>(\\\\)*\` =>   ( {   s = make_string stringlist;
				t = (s,*stringstart,yypos + size yytext);
				yybegin initial;
				tokens::backticks t;
			    }
			  );


<backticks>({ws}|{eol}|[\000-\031]|{idchars}|{symbol_sans_backslash}|\[|\]|\(|\)|{hash}|[',.;^{}])+|.  => (add_string(stringlist,yytext); continue());



<dot_backticks>({ws}|{eol}|[\000-\031]|{idchars}|{symbol}|\[|\]|\(|\)|{hash}|[',.;^{}'"])+
    =>
    (add_string(stringlist,yytext); continue());

<dot_backticks>\`\`
    =>
    (add_string(stringlist,"`"); continue());

<dot_backticks>\`
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_backticks t;
      }
    );



<dot_qquotes>({ws}|{eol}|[\000-\031]|{backtick}|{idchars}|{symbol}|\[|\]|\(|\)|{hash}|[',.;^{}'])+
    =>
    (add_string(stringlist,yytext); continue());

<dot_qquotes>\"\"
    =>
    (add_string(stringlist,"\""); continue());

<dot_qquotes>\"
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_qquotes t;
      }
    );



<dot_quotes>({ws}|{eol}|[\000-\031]|{backtick}|[A-Za-z_0-9]|{symbol}|\[|\]|\(|\)|{hash}|[,.;^{}"])+
    =>
    (add_string(stringlist,yytext); continue());

<dot_quotes>\'\'
    =>
    (add_string(stringlist,"'"); continue());

<dot_quotes>\'
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_quotes t;
      }
    );



<dot_brokets>({ws}|{eol}|[\000-\031]|{backtick}|{idchars}|[!%&$+/:<=?@~|*]|\-|\\|\^|\[|\]|\(|\)|{hash}|[',.;^{}"])+
    =>
    (add_string(stringlist,yytext); continue());

<dot_brokets>\>\>
    =>
    (add_string(stringlist,">"); continue());

<dot_brokets>\>
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_brokets t;
      }
    );



<dot_barets>({ws}|{eol}|[\000-\031]|{backtick}|{idchars}|[!%&$+/:<=>?@~*]|\-|\\|\^|\[|\]|\(|\)|{hash}|[',.;^{}"])+
    =>
    ( { add_string(stringlist,yytext);
        continue();
      }
    );

<dot_barets>\|\|
    =>
    ( { add_string(stringlist,"|");
        continue();
      }
    );

<dot_barets>\|
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_barets t;
      }
    );



<dot_slashets>({ws}|{eol}|[\000-\031]|{backtick}|{idchars}|[!%&$+|:<=>?@~*]|\-|\\|\^|\[|\]|\(|\)|{hash}|[',.;^{}"])+
    =>
    ( { add_string(stringlist,yytext);
        continue();
      }
    );

<dot_slashets>\/\/
    =>
    ( { add_string(stringlist,"/");
        continue();
      }
    );

<dot_slashets>\/
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_slashets t;
      }
    );



<dot_hashets>({ws}|{eol}|[\000-\031]|{backtick}|{idchars}|[!%&$+|/:<=>?@~*]|\-|\\|\^|\[|\]|\(|\)|[',.;^{}"])+
    =>
    ( { add_string(stringlist,yytext);
        continue();
      }
    );

<dot_hashets>\#\#
    =>
    ( { add_string(stringlist,"#");
        continue();
      }
    );

<dot_hashets>\#
    =>
    ( { s = make_string stringlist;
        t = (s,*stringstart,yypos + size yytext);
        yybegin initial;
        tokens::dot_hashets t;
      }
    );



<char>\'	=> ( {  s = make_string stringlist;
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
                    });
<char>{eol}	=> (err (*stringstart,yypos) ERROR "unclosed string"
		        null_error_body;
		    line_number_db::newline line_number_db yypos;
		    yybegin initial; tokens::string(make_string stringlist,*stringstart,yypos));
<char>\\{eol}     	=> (line_number_db::newline line_number_db (yypos+1);
		    yybegin stringgap; continue());
<char>\\{ws}   	=> (yybegin stringgap; continue());
<char>\\a		=> (add_string(stringlist, "\007"); continue());
<char>\\b		=> (add_string(stringlist, "\008"); continue());
<char>\\f		=> (add_string(stringlist, "\012"); continue());
<char>\\n		=> (add_string(stringlist, "\010"); continue());
<char>\\r		=> (add_string(stringlist, "\013"); continue());
<char>\\t		=> (add_string(stringlist, "\009"); continue());
<char>\\v		=> (add_string(stringlist, "\011"); continue());
<char>\\\\		=> (add_string(stringlist, "\\"); continue());
<char>\\\'		=> (add_string(stringlist,  "'"); continue());
<char>\\\^[@-_]	=> (add_char(stringlist,
			char::from_int(char::to_int(string::get(yytext,2))-char::to_int '@'));
		    continue());
<char>\\\^.	=>
	(err(yypos,yypos+2) ERROR "illegal control escape; must be one of \
	  \@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" null_error_body;
	 continue());
<char>\\[0-9]{3}	=>
 ( {  x = char::to_int(string::get(yytext,1))*100
	     + char::to_int(string::get(yytext,2))*10
	     + char::to_int(string::get(yytext,3))
	     - ((char::to_int '0')*111);
   {  if (x>255)
           err (yypos,yypos+4) ERROR "illegal ascii escape" null_error_body;
      else add_char(stringlist, char::from_int x);
      fi;
      continue();
   };
  });
<char>\\		=> (err (yypos,yypos+1) ERROR "illegal string escape"
		        null_error_body; 
		    continue());


<char>[\000-\031]  => (err (yypos,yypos+1) ERROR "illegal non-printing character in string" null_error_body;
                    continue());
<char>([A-Za-z_0-9]|{symbol_sans_backslash}|\[|\]|\(|\)|{backtick}|{hash}|[,.;^{}])+|.  => (add_string(stringlist,yytext); continue());


<stringgap>{eol}	=> (line_number_db::newline line_number_db yypos; continue());
<stringgap>{ws}		=> (continue());
<stringgap>\\		=> (yybegin string; stringstart := yypos; continue());
<stringgap>.		=> (err (*stringstart,yypos) ERROR "unclosed string"
		        null_error_body; 
		    yybegin initial; tokens::string(make_string stringlist,*stringstart,yypos+1));

<qqq>"^"	=> (add_string(stringlist, "`"); continue());
<qqq>"^^"	=> (add_string(stringlist, "^"); continue());
<qqq>"^"          => (yybegin aq;
                    {  x = make_string stringlist;

                    tokens::chunkl(x,yypos,yypos+(size x));
                    });
<qqq>"`"          => (/*  a closing backtick */
                    yybegin initial;
                    {  x = make_string stringlist;
                    tokens::endq(x,yypos,yypos+(size x));
                    });
<qqq>{eol}        => (line_number_db::newline line_number_db yypos; add_string(stringlist,"\n"); continue());
<qqq>.            => (add_string(stringlist,yytext); continue());

<aq>{eol}       => (line_number_db::newline line_number_db yypos; continue());
<aq>{ws}        => (continue());
<aq>{id}        => (yybegin qqq; 
                    { hash = hash_string yytext;

                    tokens::antiquote_id(fast_symbol::raw_symbol(hash,yytext),
				yypos,yypos+(size yytext));
                    });
<aq>{symbol}+      => (yybegin qqq; 
                    { hash = hash_string yytext;

                    tokens::antiquote_id(fast_symbol::raw_symbol(hash,yytext),
				yypos,yypos+(size yytext));
                    });
<aq>"("         => (yybegin initial;
                    brack_stack := ((REF 1) ! *brack_stack);
                    tokens::lparen(yypos,yypos+1));
<aq>.           => (err (yypos,yypos+1) ERROR
		       ("ml lexer: bad character after antiquote " + yytext)
		       null_error_body;
                    tokens::antiquote_id(fast_symbol::raw_symbol(0u0,""),yypos,yypos));
