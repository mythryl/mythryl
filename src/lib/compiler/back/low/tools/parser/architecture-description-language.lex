## architecture-description-language.lex -- derived from ~/src/sml/nj/smlnj-110.60/MLRISC/Tools/Parser/adl.lex
#
# Here we define the lexer for our Architecture Description Language,
# which contains a large subset of SML, including SML/NJ extensions.
#
# Architecture Description Language is the syntax used in our architecture description files
#
#     src/lib/compiler/back/low/intel32/one_word_int.architecture-description
#     src/lib/compiler/back/low/pwrpc32/pwrpc32.architecture-description
#     src/lib/compiler/back/low/sparc32/sparc32.architecture-description
#     src/lib/compiler/back/low/tools/basis.adl
#     
# Our responsibility in this file is breaking up raw text from those files into
# a stream of typed tokens which get fed into the parser specified in
#
#     src/lib/compiler/back/low/tools/parser/architecture-description-language.grammar
#
# to produce a parsetree in the format specified in
#
#     src/lib/compiler/back/low/tools/adl-syntax/adl-raw-syntax-form.pkg
#
# which then gets run through
#
#     src/lib/compiler/back/low/tools/arch/architecture-description.pkg
#
# to produce our Architecture_Description record.
# The information in this record drives the modules
#
#     src/lib/compiler/back/low/tools/arch/make-sourcecode-for-machcode-xxx-package.pkg
#     src/lib/compiler/back/low/tools/arch/make-sourcecode-for-registerkinds-xxx-package.pkg
#     src/lib/compiler/back/low/tools/arch/make-sourcecode-for-translate-machcode-to-asmcode-xxx-g-package.pkg
#     src/lib/compiler/back/low/tools/arch/make-sourcecode-for-translate-machcode-to-execode-xxx-g-package.pkg
#     ...
#
# which generate corresponding compiler backend lowhalf packages such as
#
#     src/lib/compiler/back/low/intel32/code/machcode-intel32.codemade.api
#     src/lib/compiler/back/low/intel32/code/machcode-intel32-g.codemade.pkg
#     src/lib/compiler/back/low/intel32/code/registerkinds-intel32.codemade.pkg
#     src/lib/compiler/back/low/intel32/emit/translate-machcode-to-asmcode-intel32-g.codemade.pkg
#     src/lib/compiler/back/low/intel32/emit/translate-machcode-to-execode-intel32-g.codemade.pkg.unused
#     ...

# Compiled by:
#     src/lib/compiler/back/low/tools/architecture-parser.lib



exception ERROR;

Source_Position = Int;

Semantic_Value = tokens::Semantic_Value;
    #
    # Semantic_Value is a datatype including one constructor
    # for every grammar terminal listed in the %term declaration in
    #     src/lib/compiler/back/low/tools/parser/architecture-description-language.grammar
    #     src/lib/compiler/back/low/tools/parser/architecture-description-language.grammar.pkg	
    # is the the mythryl-yacc generated file actually containing
    # the 'package tokens' definition.


Token( X, Y ) = tokens::Token( X, Y );
Lex_Result =  Token( Semantic_Value, Source_Position );

Lex_Arg = { line_number_db  : line_number_database::Sourcemap,
	    err     : (Source_Position, Source_Position, String) -> Void,
	    adl_mode : Bool
          };

Arg = Lex_Arg;

include tokens;						# tokens	is from   src/lib/compiler/back/low/tools/parser/architecture-description-language.grammar.pkg
							# (architecture-description-language.grammar.pkg is synthesized by mythryl-yacc during compilation.)

comment_level = REF 0;
meta_level = REF 0;

# These four never get set to any other value in existing code.
# They are probably intended as an end-user customization hook:
#
asm_lquote = REF "``";
asm_rquote = REF "''";
asm_lmeta  = REF "<";
asm_rmeta  = REF ">";

exception ERROR;

fun init ()
    =
    {   comment_level := 0;
        meta_level    := 0;
        asm_lquote    := "``";
        asm_rquote    := "''";
        asm_lmeta     := "<";
        asm_rmeta     := ">";
    };

fun eof { line_number_db, err, adl_mode }
    = 
    {   pos = line_number_database::curr_pos line_number_db;

        eof_t(pos,pos);
    };

fun debug _
    =
    ();

fun check (err, _, _, THE w)
        =>
        w;

    check (err, pos, s, NULL)
        => 
        {   err(pos,pos+size s,"bad literal " + s);
            raise exception ERROR;
        };
end;

fun strip k s
    =
    string::substring(s,k,string::length s - k);

fun scan err fmt (s,s') tok pos
    = 
    tok(check(err,pos,s,number_string::scan_string fmt s'),
              pos,pos + size s) 
    except
        _ = id (s, pos, pos);

fun wdecimal (err,s,pos)
    = 
    scan err (one_word_unt::scan number_string::DECIMAL) (s,strip 2 s) unt pos;

fun whex (err,s,pos)
    = 
    scan err (one_word_unt::scan number_string::HEX) (s,strip 3 s) unt pos;

fun woctal  (err,s,pos) =  scan err (one_word_unt::scan number_string::OCTAL)   (s,strip 3 s) unt  pos;
fun wbinary (err,s,pos) =  scan err (one_word_unt::scan number_string::BINARY)  (s,strip 3 s) unt  pos;
fun decimal (err,s,pos) =  scan err (int::scan   number_string::DECIMAL) (s,s)         int  pos;

fun real (err,s,pos)
    =
    scan err (eight_byte_float::scan) (s,s) 
                       (fn (x,y,z) =  real_t(eight_byte_float::to_string x, y, z)) pos;

fun hex    (err,s,pos) =  scan err (int::scan number_string::HEX)    (s,strip 2 s) int pos;
fun octal  (err,s,pos) =  scan err (int::scan number_string::OCTAL)  (s,strip 2 s) int pos;
fun binary (err,s,pos) =  scan err (int::scan number_string::BINARY) (s,strip 2 s) int pos;

fun decimalinf (err,s,pos) =  scan err (multiword_int::scan number_string::DECIMAL) (s,s)         integer pos;
fun hexinf     (err,s,pos) =  scan err (multiword_int::scan number_string::HEX)     (s,strip 2 s) integer pos;
fun octalinf   (err,s,pos) =  scan err (multiword_int::scan number_string::OCTAL)   (s,strip 2 s) integer pos;
fun binaryinf  (err,s,pos) =  scan err (multiword_int::scan number_string::BINARY)  (s,strip 2 s) integer pos;

fun string (err,s,pos)
    = 
    string_t(
      check(err,pos,s,string::from_string(string::substring(s,1,string::length s - 2))),
      pos, pos + size s);

fun char (err,s,pos)
    = 
    char_t(check(err,pos,s,char::from_string(string::substring(s,2,string::length s - 3))),
	 pos,pos + size s);

fun trans_asm s
    = 
    string::implode(loop(string::explode s))
    where
        fun loop ('\\' ! '<' ! s) =>   '<' ! loop s;
	    loop('\\' ! '>' ! s) =>   '>' ! loop s;
	    loop(c ! s)          =>    c  ! loop s;
	    loop []              =>    []         ;
        end;
    end;

fun asmtext (err,s,pos)
    = 
    asmtext_t (check (err, pos, s, string::from_string(trans_asm s)),pos,pos + size s);

infix val @@ ;

fun x @@ y =  y ! x ;

exception NOT_FOUND;

keywords    = hashtable::make_hashtable (hash_string::hash_string, (==)) { size_hint => 13, not_found_exception => NOT_FOUND }
            :  hashtable::Hashtable (String, (Int, Int) -> Token (Semantic_Value,Int));

adlkeywords = hashtable::make_hashtable (hash_string::hash_string, (==)) { size_hint => 13, not_found_exception => NOT_FOUND }
            : hashtable::Hashtable (String, (Int, Int) -> Token (Semantic_Value, Int));

symbols     = hashtable::make_hashtable (hash_string::hash_string, (==)) { size_hint => 13, not_found_exception => NOT_FOUND }
            : hashtable::Hashtable (String, (Int, Int) -> Token (Semantic_Value, Int));

# Set up hashtable
#
#     keywords
# 
# mapping strings to corresponding token-creation functions.
#
# The token-creation functions (type_t, end_t, fun_t, ... )
# here are from package tokens in 
#     src/lib/compiler/back/low/tools/parser/architecture-description-language.grammar.pkg
# with definitions like
#     fun type_t (p1, p2) = token::TOKEN (parser_data::lr_table::TERM 7,  (parser_data::values::TM_VOID, p1, p2));
#     fun end_t  (p1, p2) = token::TOKEN (parser_data::lr_table::TERM 1,  (parser_data::values::TM_VOID, p1, p2));
#     fun fun_t  (p1, p2) = token::TOKEN (parser_data::lr_table::TERM 76, (parser_data::values::TM_VOID, p1, p2));
#
my _ = apply (hashtable::set keywords) 
( NIL       @@
 ("_",wild) @@
 ("datatype", datatype) @@
 ("type", type_t) @@
 ("end", end_t) @@
 ("fun", fun_t) @@
 ("fn", fn_t) @@
 ("val", my_t) @@
 ("raise", raise_t) @@
 ("handle", except_t) @@
 ("let", let_t) @@
 ("local", local_t) @@
 ("exception", exception_t) @@
 ("structure", package_t) @@
 ("signature", api_t) @@
 ("functor", generic_t) @@
 ("sig", begin_api) @@
 ("struct", struct) @@
 ("sharing", sharing_t) @@
 ("where", where_t) @@
 ("withtype", withtype_t) @@
 ("if", if_t) @@
 ("then", then_t) @@
 ("else", else_t) @@
 ("in", in_t) @@
 ("true", true) @@
 ("false", false) @@
 ("and", and_t) @@
 ("at", at) @@
 ("of", of_t) @@
 ("case", case_t) @@
 ("as", as_t) @@
 ("open", open) @@
 ("op", op_t) @@
 ("include", include_t) @@
 ("infix", infix_t) @@
 ("infixr", infixr_t) @@
 ("nonfix", nonfix_t) @@
 ("not", not) 
);

my _ = apply (hashtable::set adlkeywords) 
( NIL @@
 ("architecture", architecture) @@
 ("assembly", assembly) @@
 ("storage", storage) @@
 ("locations", locations) @@
 ("at", at) @@
 ("field", field_t) @@
 ("fields", fields) @@
 ("signed", signed) @@
 ("unsigned", unsigned) @@
 ("bits", bits) @@
 ("ordering", ordering) @@
 ("little", little) @@
 ("big", big) @@
 ("endian", endian) @@
 ("register", register) @@
 ("as", as_t) @@
 ("cell", cell) @@
 ("cells", cells) @@
 ("registerset", registerset) @@
 ("pipeline", pipeline) @@
 ("cpu", cpu) @@
 ("resource", resource) @@
 ("latency", latency) @@
 ("base_op", base_op) @@
 ("instruction", instruction) @@
 ("formats", formats) @@
 ("uppercase", uppercase) @@
 ("lowercase", lowercase) @@
 ("verbatim", verbatim) @@
 ("span", span) @@
 ("dependent", dependent) @@
 ("always", always) @@
 ("never", never) @@
 ("delayslot", delayslot) @@
 #  ("equation", equation) @@
 #  ("reservation", reservation) @@
 #  ("table", table) @@
 #  ("predicated", predicated) @@
 #  ("branching", branching) @@ 
 #  ("candidate", candidate) @@
 ("rtl", rtl) @@
 ("debug", debug_t) @@
 ("aliasing", aliasing) @@
 ("aggregable",aggregable) 
);

my _ = apply (hashtable::set symbols) 
(
  NIL @@
  ("=",	eq) @@
  ("*",	times) @@
  (":",	colon) @@
  (":>",colongreater) @@
  ("|", bar) @@
  ("->", arrow) @@
  ("=>", darrow) @@
  ("#", hash) @@
  ("!", deref) @@
  ("^^", meld)		# Called 'concat' in SML/NJ 
);

fun lookup (adl_mode,s,yypos)
    =
    {   l = string::length s;

	fun id_fn () = id(unique_symbol::to_string
			(unique_symbol::from_string s), yypos, yypos + l);

        hashtable::lookup keywords s (yypos,yypos + l) 
	except _ =
	     if adl_mode  
	       (hashtable::lookup adlkeywords s (yypos,yypos + l) except _ =  id_fn());
	     else
               id_fn();
	     fi;
    };

fun lookup_sym (s,yypos)
    =
    {   l = string::length s;

        hashtable::lookup symbols s (yypos,yypos + l) 
	except _ =  symbol (unique_symbol::to_string
			   (unique_symbol::from_string s), yypos, yypos + l);
    };

%%

%header (generic package adl_lex_g(tokens : Adl_Tokens));
%arg ( { line_number_db,err,adl_mode } );
%reject

alpha=[A-Za-z];
digit=[0-9];
id=[A-Za-z_][A-Za-z0-9_\']*;
tyvar=\'{id};
decimal={digit}+;
integer=-?{decimal};
real={integer}\.{decimal}(e{integer})?;
octal=0[0-7]+;
hex=0x[0-9a-fA-F]+;
binary=0b[0-1]+;
wdecimal=0w{digit}+;
woctal=0w0[0-7]+;
whex=0wx[0-9a-fA-F]+;
wbinary=0wb[0-1]+;
ws=[\ \t];
string=\"([^\\\n\t"]|\\.)*\";
char=#\"([^\\\n\t"]|\\.)*\";
symbol1=(\-|[=\.+~/*:!@#$%^&*|?])+;
symbol2=`+|'+|\<+|\>+|\=\>|~\>\>;
symbol3=\\.;
asmsymbol={symbol1}|{symbol2}|{symbol3};
symbol=(\-|[=+~/*:!@#$%^&*|?<>])+|``|'';
asmtext=([^\n\t<>']+|');
inf=i;

%s comment asm asmquote;

%%
<initial,comment,asm>\n		=> (line_number_database::newline line_number_db yypos; continue());
<initial,comment,asm>{ws}	=> (continue());
<asmquote>\n		=> (err(yypos,yypos+size yytext,
                                "newline in assembly text!"); continue());
<initial>"(*"		=> (comment_level := 1; yybegin comment; continue());
<initial>"/*"[*=-]*	=> (comment_level := 1; yybegin comment; continue());
<initial,asm>{integer}	=> (decimal(err,yytext,yypos));
<initial,asm>{hex}	=> (hex(err,yytext,yypos));
<initial,asm>{octal}	=> (octal(err,yytext,yypos));
<initial,asm>{binary}	=> (binary(err,yytext,yypos));
<initial,asm>{integer}{inf}	=> (decimalinf(err,yytext,yypos));
<initial,asm>{hex}{inf}		=> (hexinf(err,yytext,yypos));
<initial,asm>{octal}{inf}	=> (octalinf(err,yytext,yypos));
<initial,asm>{binary}{inf}	=> (binaryinf(err,yytext,yypos));
<initial,asm>{wdecimal}	=> (wdecimal(err,yytext,yypos));
<initial,asm>{whex}	=> (whex(err,yytext,yypos));
<initial,asm>{woctal}	=> (woctal(err,yytext,yypos));
<initial,asm>{wbinary}	=> (wbinary(err,yytext,yypos));
<initial,asm>{string}	=> (string(err,yytext,yypos));
<initial,asm>{char}	=> (char(err,yytext,yypos));
<initial,asm>{real}	=> (real(err,yytext,yypos));
<initial,asm>"$"	=> (if adl_mode  dollar(yypos,yypos+1);
                            else         symbol("$",yypos,yypos+1);
                            fi);

<initial,asm>"asm:"       => (if adl_mode       asm_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"mc:"        => (if adl_mode        mc_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"rtl:"       => (if adl_mode       rtl_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"delayslot:" => (if adl_mode delayslot_colon(yypos,      size yytext); else REJECT(); fi);
<initial,asm>"padding:"   => (if adl_mode   padding_colon(yypos,      size yytext); else REJECT(); fi);
<initial,asm>"nullified:" => (if adl_mode nullified_colon(yypos,      size yytext); else REJECT(); fi);
<initial,asm>"candidate:" => (if adl_mode candidate_colon(yypos,      size yytext); else REJECT(); fi);

<initial,asm>{id}	=> (lookup(adl_mode,yytext,yypos));
<initial,asm>{tyvar}	=> (tyvar(yytext,yypos,yypos + size yytext));
<initial,asm>"("	=> (lparen(yypos,yypos+1));
<initial,asm>")"	=> (rparen(yypos,yypos+1));
<initial,asm>"["	=> (lbracket(yypos,yypos+1));
<initial,asm>"#["	=> (lhashbracket(yypos,yypos+1));
<initial,asm>"]"	=> (rbracket(yypos,yypos+1));
<initial,asm>"{"	=> (lbrace(yypos,yypos+1));
<initial,asm>"}"	=> (rbrace(yypos,yypos+1));
<initial,asm>","	=> (comma(yypos,yypos+1));
<initial,asm>";"	=> (semicolon(yypos,yypos+1));
<initial,asm>"."	=> (dot(yypos,yypos+1));
<initial,asm>".."	=> (dotdot(yypos,yypos+2));
<initial,asm>"..."	=> (dotdot(yypos,yypos+3));
<initial>{symbol}	=> (if (yytext == *asm_lquote)
				debug("lquote " + yytext + "\n");
				yybegin asmquote; 
                                ldquote(yypos,yypos+size yytext);
			    else
			        lookup_sym(yytext,yypos);
                            fi
                           );
<asmquote>{asmsymbol}	=> (if (yytext == *asm_rquote )
				 if (*meta_level != 0 )
                                    err(yypos,yypos+size yytext,
                                       "Mismatch between " + *asm_lmeta +
                                          " and " + *asm_rmeta);
                                 fi;
				 debug("rquote " + yytext + "\n");
                                 yybegin initial; 
                                 rdquote(yypos,yypos+size yytext);
			    else if (yytext == *asm_lmeta )
				     meta_level := *meta_level + 1;
				     debug("lmeta " + yytext + "\n");
				     yybegin asm;
                                     lmeta(yypos,yypos+size yytext);
			         else
			             asmtext(err,yytext,yypos);
                                fi;
                            fi);
<asm>{asmsymbol}	=> (if (yytext == *asm_rmeta )
				 meta_level := *meta_level - 1;
				 debug("rmeta " + yytext + "(" + int::to_string *meta_level + ")\n");
				 if (*meta_level == 0 ) yybegin asmquote; fi;
				 rmeta(yypos,yypos+size yytext);
			    else
			        lookup_sym(yytext,yypos);
                            fi
                            );
<asmquote>{asmtext}	=> (debug("text=" + yytext + "\n"); 
                            asmtext(err,yytext,yypos));
<comment>"*)"		=> (comment_level := *comment_level - 1;
			    if (*comment_level == 0 ) yybegin initial; fi; 
			    continue());
<comment>"*/"		=> (comment_level := *comment_level - 1;
			    if (*comment_level == 0 ) yybegin initial; fi; 
			    continue());
<comment>"(*"		=> (comment_level := *comment_level + 1; continue());

<comment>"/*"[*=-]*	=> (comment_level := *comment_level + 1; continue());
<comment>.		=> (continue());
.			=> (err(yypos,yypos+size yytext,
                                "unknown character " + string::to_string yytext);
                            continue());
