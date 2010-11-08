
clean: 
	@-rm -f ID
	@-rm -f TAGS
	@-rm -f core
	@-rm -f OH7_FILES_TO_LOAD
	@-rm -f LIBRARY_CONTENTS
	@find . -name '*~' -print | xargs rm -f;
	@find . -name '#*#' -print | xargs rm -f;
	@find . -type f -name '*.frozen' -print | xargs rm -rf;
	@find . -type f -name '*.o7' -print | xargs rm -rf;
	@find . -type f -name '*.module-dependencies-summary' -print | xargs rm -rf;
	@find . -type f -name '*[a-z].version' -print | xargs rm -rf;
	@find . -type f -name '*.index' -print | xargs rm -f;
	@find . -type f -name '*.load.log' -print | xargs rm -f;
	@find . -type f -name '*.compile.log' -print | xargs rm -f;
	@find . -type f -name '*.EDIT_REQUESTS' -print | xargs rm -f;
	@find . -type f -name '*.UNEDITED' -print | xargs rm -f;
	@find . -type f -name '*.EDITED' -print | xargs rm -f;
	@find . -type f -name '*.EDITS' -print | xargs rm -f;
	@find . -type f -name '*.SEEN' -print | xargs rm -f;
	@find . -type f -name '*.skeleton' -print | xargs rm -f;
	@find . -type f -name 'tmp-make7-pid-*' -print | xargs rm -f;

