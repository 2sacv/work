include cc.rule

target=$(shell find ./ -maxdepth 2 -mindepth 2 -name "Makefile" | xargs dirname | grep -v common)
target_clean=$(addsuffix _clean,$(target))

.PHONY:all clean  $(target) $(target_clean)
all:  $(target)

clean: $(target_clean)

$(target):
	@cd $@ && make

$(target_clean):
	@cd $(subst _clean,,$@) && make clean
