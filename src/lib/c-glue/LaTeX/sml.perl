#
# How to deal with the sml package
#

package main;

my(@sml_keywords) = 
   qw(functor structure signature sig struct sharing where 
      let open fun val rec end case raise handle withtype
      datatype type infix infixr nonfix fn local in 
      while do of true false
     );
my($smltypevar) = { "a" => "&alpha;",
                    "b" => "&beta;",
                    "c" => "&gamma;",
                    "d" => "&delta;",
                    "e" => "&epsilon;"
                  };
my($smlsyms) = ":";

my(@sml_types) = qw(string bool int word list array vector unit option);

my($sml_keywords) = join("|",@sml_keywords); 

my($sml_types)    = join("|",@sml_types);

sub smlkeyword
{  my($keyword) = @_;
   return "<B>" . $keyword . "</B>";
}

sub smltype
{  my($type) = @_;
   return "<code>" . $type . "</code>";
}

sub smlstring
{  my($s) = @_;
   return "<code>" . $s . "</code>";
}


sub smlnum
{  my($type) = @_;
   return "<code>" . $type . "</code>";
}

sub smlconst 
{  my($const) = @_;
   return "<code>" . $const . "</code>";
}

sub smltypevar
{  my($text,$typevar) = @_;
   if ($smltypevar->{$typevar})
   {  $typevar = $smltypevar->{$typevar}; }
   else 
   {  $typevar = "'" . $typevar; 
   }
   return $text . "<I>" .  $typevar . "</I>";
}

$alltt_rx = "smldisp";

sub do_env_smldisp {
    local ($_) = @_;
    local($closures,$reopens,$alltt_start,$alltt_end);
    #check the nature of the last opened tag
    local($last_tag) = pop (@open_tags);
    local($decl) = $declarations{$last_tag};
    if ( $decl =~ m|</.*$|) { $decl = $& }
    if (($last_tag)&&!($decl =~ /$block_close_rx/)) {
        # need to close tags, for re-opening inside
        push (@open_tags, $last_tag) if ($last_tag);
        ($closures,$reopens) = &preserve_open_tags();
        $alltt_start = "<DIV$env_id>";
        $alltt_end = "</DIV>";
        $env_id = '';
    } else {
        push (@open_tags, $last_tag) if ($last_tag);
    }

    # This allows paragraph/quote/DIV etc. tags to be preserved
    local(@open_tags,@save_open_tags) = ((),());

    local($cnt) = ++$global{'max_id'};
    $_ = join('',"$O$cnt$C\\tt$O", ++$global{'max_id'}, $C
                , $_ , $O, $global{'max_id'}, "$C$O$cnt$C");

    $_ = &translate_environments($_);
    $_ = &translate_commands($_) if (/\\/);

    # preserve space-runs, using &nbsp;
    while (s/(\S) ( +)/$1$2;SPMnbsp;/g){};
    s/(<BR>) /$1;SPMnbsp;/g;

#RRM: using <PRE> tags doesn't allow images, etc.
#    $_ = &revert_to_raw_tex($_);
#    &mark_string; # ???
#    s/\\([{}])/$1/g; # ???
#    s/<\/?\w+>//g; # no nested tags allowed
#    join('', $closures,"<PRE$env_id>$_</PRE>", $reopens);
#    s/<P>//g;
#    join('', $closures,"<PRE$env_id>", $_, &balance_tags(), '</PRE>', $reopens);

    $_ = join('', $closures, 
              $alltt_start, 
              $reopens, $_
        , &balance_tags(), $closures, 
              $alltt_end, 
               $reopens);

    # print $_;

    s|<TT>||g;
    s|</TT>||g;

    s/([^<>#\w])(0w\d+|0wx[0-9a-hA-H]+|\d+|0x[0-9a-hA-H]+)([^<>\w])/
        $1. &smlnum($2) . $3/eg;
    s|\b($sml_keywords)\b|&smlkeyword($1)|eg;
    s|\b($sml_types)\b|&smltype($1)|eg;
    s|([^\w])'([a-z]+)\b|&smltypevar($1,$2)|eg;
    #s|([^<>#\w])([A-Z_]+)([^<>#\w])|$1.&smlconst($2).$3|eg;
    #s/([^<>#\w])("([^\\]|\\.)*")([^<>#\w])/$1.&smlstring($2).$4/eg;

    $_ = "<div id=\"sml\"><BR>\n" . $_ . "<BR></div>\n";
}

1;
