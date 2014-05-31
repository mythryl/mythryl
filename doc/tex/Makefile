
docs:	book.html

test.html: test.tex
	hevea test.tex

book.html: book.tex
	hevea book.tex
	sh/substitute-primes-and-bangs book.html
	hacha book.html
	sh/give-book-html-pages-better-names
	@-rm -f book*.html

clean:
	@-rm -f #*#
	@-rm -f book.hind
	@-rm -f book.hidx
	@-rm -f book.css
	@-rm -f *.html
	@-rm -f *.htoc
	@-rm -f *.haux
	@-rm -f *.gif
	@-rm -f *.png
	@-rm -f *.image.tex
	@-rm -f *~
	@-rm -f tmp*
	@-rm -rf src
	@-rm -f mythryl.tbz

