## html.lex
## COPYRIGHT (c) 1995 AT&T Bell Laboratories.
## COPYRIGHT (c) 1996 AT&T Research.
## Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
## released per terms of SMLNJ-COPYRIGHT.



# A scanner for HTML.
#
# TODO:
#    Recognize the DOCTYPE element
#	<!DOCTYPE HTML PUBLIC "...">
#    Clean-up the scanning of start tags (do we need Err?).
#    Whitespace in PRE elements should be preserved, but how?


###                "[The BLINK tag in HTML] was a joke, okay?
###                 If we thought it would actually be used,
###                 we wouldn't have written it!"
###
###                                  -- Marc Andreessen


package t = tokens;
package elems = html_elements_g (
  package tokens = tokens;
  package err = err;
  package htmlattrs = htmlattrs;);

Source_Position = Int;
Semantic_Value = t::Semantic_Value;
Arg = (((String, Int, Int) -> Void), Null_Or String);
Token (X, Y) = t::Token (X, Y);
Lex_Result = Token (Semantic_Value, Source_Position);

fun eof _ = tokens::eof (0, 0);

# A buffer for collecting a string piecewise:
buffer = REF ([]:  List String);

fun add_str s
    =
    buffer :=  s ! *buffer;

fun get_str ()
    =
    string::cat (list::reverse *buffer)
    then
        buffer := [];

%%

%s com1 com2 stag;

%header (generic package html_lex_g (
  package tokens:  Html_Tokens;
  package err:  Html_Error;
  package htmlattrs:  Html_Attributes;)
 );

%arg (error_fn, file);

%full
%count

alpha=[A-Za-z];
digit=[0-9];
namechar=[-A-Za-z0-9.];
tag=({alpha}{namechar}*);
ws = [\ \t];

%%

<initial>"<"{tag}
	=> (add_str yytext; yybegin stag; continue());
<stag>">"
	=> (add_str yytext;
	    yybegin initial;
	    case (elems::start_tag file (get_str(), *yylineno, *yylineno))
	        THE tag => tag;
	        NULL    => continue();
	    esac
           );
<stag>\n
	=> (add_str " "; continue());
<stag>{ws}+
	=> (add_str yytext; continue());
<stag>{namechar}+
	=> (add_str yytext; continue());
<stag>"="
	=> (add_str yytext; continue());
<stag>"\""[^\"\n]*"\""
	=> (add_str yytext; continue());
<stag>"'"[^'\n]*"'"
	=> (add_str yytext; continue());
<stag>.
	=> (add_str yytext; continue());

<initial>"</"{tag}{ws}*">"
	=>  (case (elems::end_tag file (yytext, *yylineno, *yylineno))
	         NULL    =>  continue();
	         THE tag =>  tag;
 	    esac);

<initial>"<!--"
	=> (yybegin com1; continue());
<com1>"--"
	=> (yybegin com2; continue());
<com1>\n
	=> (continue());
<com1>.
	=> (continue());
<com2>"--"
	=> (yybegin com1; continue());
<com2>">"
	=> (yybegin initial; continue());
<com2>\n
	=> (continue());
<com2>{ws}
	=> (continue());
<com2>.
	=> (error_fn("bad comment syntax", *yylineno, *yylineno+1);
	    yybegin initial;
	    continue());

<initial>"&#"[A-Za-z]+";"
	=> (
/** At some point, we should support &#SPACE; and &#TAB; **/
	    continue());

<initial>"&#"[0-9]+";"
	=> (t::char_ref (yytext, *yylineno, *yylineno));

<initial>"&"{tag}";"
	=> (t::entity_ref (yytext, *yylineno, *yylineno));

<initial>"\n"
	=> (continue());
<initial>{ws}
	=> (continue());

<initial>[^<]+
	=> (t::pcdata (yytext, *yylineno, *yylineno));
<initial>.
	=> (error_fn (cat [
		"bogus character #\"", char::to_string (string::get (yytext, 0)),
		"\" in PCDATA\n"
	      ], *yylineno, *yylineno+1);
	    continue());

