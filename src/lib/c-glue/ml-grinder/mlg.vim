" Vim syntax file
" Language:	SML with MLGrinder tags
" Maintainer:	Allen Leung <leunga@cs.nyu.edu>
" Last change:	2001 

" Remove any old syntax stuff hanging around
syn clear

syn keyword smlStatement	structure signature functor struct sig end
syn keyword smlStatement	fun fn let in val rec of and raise handle as
syn keyword smlStatement	type eqtype datatype exception sharing where 
syn keyword smlStatement	withtype with abstype open local
syn keyword smlConditional	if else then case include
syn keyword smlRepeat		while op infix infixr nonfix do
"syn match smlLabel		"\#[a-zA-Z'][a-zA-Z0-9_']*\>" 

syn keyword smlTodo contained	TODO

" String
syn match   smlSpecial	contained "\\x\x\+\|\\\o\{1,3\}\|\\.\|\\$"
syn region  smlQuote	start=+"+ skip=+\\\\\|\\"+ end=+":exp+ contains=smlSpecial
syn region  smlQuote	start=+"+ skip=+\\\\\|\\"+ end=+":decl+ contains=smlSpecial
syn region  smlQuote	start=+"+ skip=+\\\\\|\\"+ end=+":ty+ contains=smlSpecial
syn region  smlQuote	start=+"+ skip=+\\\\\|\\"+ end=+":pat+ contains=smlSpecial
syn region  smlQuote	start=+"+ skip=+\\\\\|\\"+ end=+":sexp+ contains=smlSpecial
syn region  smlString	start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=smlSpecial
syn region  smlChar	start=+#"+ skip=+\\\\\|\\"+ end=+"+ contains=smlSpecial

syn match  smlIdentifier		"\<[a-zA-Z][a-zA-Z0-9_']*\>"
syn match  smlType			"'\<[a-zA-Z][a-zA-Z0-9_']*\>"

"syn match  smlDelimiter		"[(){}\[\]]"
syn match  smlNumber		"\<\d\+\>"
syn match  smlNumber		"\<\d\+\.\d\+[eE]\~?\d\+\>"
syn match  smlWord		"0xw[0-9a-fA-F]+"
syn match  smlWord		"0w\d+"

" If you don't like tabs
"syn match smlShowTab "\t"
"syn match smlShowTabc "\t"

syn region smlComment	start="(\*"  end="\*)" contains=smlTodo
"syn region smlComment	start="{"  end="}" contains=smlTodo

syn keyword smlOperator	andalso orelse not div mod
syn keyword smlOperator	false true 
syn keyword smlType	char string int real exn bool word list option 
syn keyword smlType	array vector unit ref
syn keyword smlType	"\->"
syn keyword smlType	"\*"

"syn keyword smlFunction	fun 

syn sync lines=250

if !exists("did_sml_syntax_inits")
  let did_sml_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link smlStatement			Statement
  hi link smlLabel			Label
  hi link smlConditional		Conditional
  hi link smlRepeat			Repeat
  hi link smlQuote			Statement
  hi link smlTodo			Todo
  hi link smlString			String
  hi link smlChar			String
  hi link smlNumber			Number
  hi link smlWord			Number
  hi link smlOperator			Operator
  hi link smlFunction			Function
  hi link smlType			Type
  hi link smlComment			Comment
  hi link smlStatement			Statement

"optional highlighting
  hi link smlDelimiter			Identifier

  "hi link smlShowTab			Error
  "hi link smlShowTabc		Error

  hi link smlIdentifier		Identifier
endif

let b:current_syntax = "sml"

" vim: ts=8
