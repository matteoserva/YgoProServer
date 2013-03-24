VERSION = 39
NEWVERS = `expr $(VERSION) + 1`
 
update-buildnum:
	@mv makefile.autoincr.inc makefile.autoincr.inc.bak
	@sed "s/^VERSION[ \t]*=[ \t]*[0-9]*/VERSION = $(NEWVERS)/" makefile.autoincr.inc.bak > makefile.autoincr.inc
	@rm makefile.autoincr.inc.bak
