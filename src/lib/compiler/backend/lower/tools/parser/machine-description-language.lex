## machine-description-language.lex

# Compiled by:
#     src/lib/compiler/backend/lower/tools/parser.make6

exception ERROR;

Source_Position = Int;
Semantic_Value = tokens::Semantic_Value;
Token( X, Y ) = tokens::Token( X, Y );
Lex_Result =  Token( Semantic_Value, Source_Position );

Lex_Arg = { source_map  : source_mapping::Sourcemap,
	    err     : (Source_Position, Source_Position, String) -> Void,
	    mdl_mode : Bool
          };

Arg = Lex_Arg;

include tokens;

comment_level = REF 0;
meta_level = REF 0;

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

fun eof { source_map, err, mdl_mode }
    = 
    {   pos = source_mapping::curr_pos source_map;

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
    scan err (unt32::scan number_string::DECIMAL) (s,strip 2 s) word pos;

fun whex (err,s,pos)
    = 
    scan err (unt32::scan number_string::HEX) (s,strip 3 s) word pos;

fun woctal  (err,s,pos) =  scan err (unt32::scan number_string::OCTAL)   (s,strip 3 s) word pos;
fun wbinary (err,s,pos) =  scan err (unt32::scan number_string::BINARY)  (s,strip 3 s) word pos;
fun decimal (err,s,pos) =  scan err (int::scan   number_string::DECIMAL) (s,s)         int  pos;

fun real (err,s,pos)
    =
    scan err (float::scan) (s,s) 
                       (fn (x,y,z) =  real_t(float::to_string x, y, z)) pos;

fun hex    (err,s,pos) =  scan err (int::scan number_string::HEX)    (s,strip 2 s) int pos;
fun octal  (err,s,pos) =  scan err (int::scan number_string::OCTAL)  (s,strip 2 s) int pos;
fun binary (err,s,pos) =  scan err (int::scan number_string::BINARY) (s,strip 2 s) int pos;

fun decimalinf (err,s,pos) =  scan err (integer::scan number_string::DECIMAL) (s,s)         intinf pos;
fun hexinf     (err,s,pos) =  scan err (integer::scan number_string::HEX)     (s,strip 2 s) intinf pos;
fun octalinf   (err,s,pos) =  scan err (integer::scan number_string::OCTAL)   (s,strip 2 s) intinf pos;
fun binaryinf  (err,s,pos) =  scan err (integer::scan number_string::BINARY)  (s,strip 2 s) intinf pos;

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

keywords    = hash_table::make_table (hash_string::hash_string, (==)) (13,NOT_FOUND) 
            :  hash_table::Hash_Table (String, (Int, Int) -> Token (Semantic_Value,Int));

mdlkeywords = hash_table::make_table (hash_string::hash_string, (==)) (13,NOT_FOUND) 
            : hash_table::Hash_Table (String, (Int, Int) -> Token (Semantic_Value, Int));

symbols     = hash_table::make_table (hash_string::hash_string, (==)) (13,NOT_FOUND)
            : hash_table::Hash_Table (String, (Int, Int) -> Token (Semantic_Value, Int));

my _ = apply (hash_table::set keywords) 
( NIL       @@
 ("_",wild) @@
 ("enum", datatype) @@
 ("type", type_t) @@
 ("end", end_t) @@
 ("fun", fun_t) @@
 ("fn", fn_t) @@
 ("my", my_t) @@
 ("raise", raise_t) @@
 ("handle", except_t) @@
 ("stipulate", let_t) @@
 ("local", local_t) @@
 ("exception", exception_t) @@
 ("package", package_t) @@
 ("api", api_t) @@
 ("generic", generic_t) @@
 ("sig", begin_api) @@
 ("struct", struct) @@
 ("sharing", sharing_t) @@
 ("where", where_t) @@
 ("withtype", withtype_t) @@
 ("if", if_t) @@
 ("then", then_t) @@
 ("else", else_t) @@
 ("herein", in_t) @@
 ("true", true) @@
 ("false", false) @@
 ("and", and_t) @@
 ("at", at) @@
 ("of", of_t) @@
 ("case", case_t) @@
 ("as", as_t) @@
 ("use", open) @@
 ("op", op_t) @@
 ("include", include_t) @@
 ("infix", infix_t) @@
 ("infixr", infixr_t) @@
 ("nonfix", nonfix_t) @@
 ("not", not) 
);

my _ = apply (hash_table::set mdlkeywords) 
( NIL @@
 ("architecture", architecture) @@
 ("assembly", assembly) @@
 ("storage", storage) @@
 ("locations", locations) @@
 ("equation", equation) @@
 ("at", at) @@
 ("vliw", vliw) @@
 ("field", field_t) @@
 ("fields", fields) @@
 ("signed", signed) @@
 ("unsigned", unsigned) @@
 ("superscalar", superscalar) @@
 ("bits", bits) @@
 ("ordering", ordering) @@
 ("little", little) @@
 ("big", big) @@
 ("endian", endian) @@
 ("register", register) @@
 ("as", as_t) @@
 ("cell", cell) @@
 ("cells", cells) @@
 ("cellset", cellset) @@
 ("pipeline", pipeline) @@
 ("cpu", cpu) @@
 ("resource", resource) @@
 ("reservation", reservation) @@
 ("table", table) @@
 ("latency", latency) @@
 ("predicated", predicated) @@
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
 #  ("branching", branching) @@ 
 ("candidate", candidate) @@
 ("rtl", rtl) @@
 ("debug", debug_t) @@
 ("aliasing", aliasing) @@
 ("aggregable",aggregable) 
);

my _ = apply (hash_table::set symbols) 
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
  ("^^", meld)
);

fun lookup (mdl_mode,s,yypos)
    =
    {   l = string::length s;

	fun id_fn () = id(unique_symbol::to_string
			(unique_symbol::from_string s), yypos, yypos + l);

        hash_table::lookup keywords s (yypos,yypos + l) 
	except _ =
	     if mdl_mode  
	       (hash_table::lookup mdlkeywords s (yypos,yypos + l) except _ =  id_fn());
	     else
               id_fn();
	     fi;
    };

fun lookup_sym (s,yypos)
    =
    {   l = string::length s;

        hash_table::lookup symbols s (yypos,yypos + l) 
	except _ =  symbol (unique_symbol::to_string
			   (unique_symbol::from_string s), yypos, yypos + l);
    };

%%

%header (generic package mdl_lex_g(tokens : Mdl_Tokens));
%arg ( { source_map,err,mdl_mode } );
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
<initial,comment,asm>\n		=> (source_mapping::newline source_map yypos; continue());
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
<initial,asm>"$"	=> (if mdl_mode  dollar(yypos,yypos+1);
                            else symbol("$",yypos,yypos+1); fi);
<initial,asm>"asm:"     => (if mdl_mode  
                              asm_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"mc:"      => (if mdl_mode  
                               mc_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"rtl:"     => (if mdl_mode  
                               rtl_colon(yypos,yypos+size yytext); else REJECT(); fi);
<initial,asm>"delayslot:" => (if mdl_mode 
                               delayslot_colon(yypos,size yytext); else REJECT(); fi);
<initial,asm>"padding:" => (if mdl_mode   
                               padding_colon(yypos,size yytext); else REJECT(); fi);
<initial,asm>"nullified:" => (if mdl_mode 
                                nullified_colon(yypos,size yytext); else REJECT(); fi);
<initial,asm>"candidate:" => (if mdl_mode   
                                candidate_colon(yypos,size yytext); else REJECT(); fi);
<initial,asm>{id}	=> (lookup(mdl_mode,yytext,yypos));
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
<initial>{symbol}	=> (if (yytext == *asm_lquote )
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
