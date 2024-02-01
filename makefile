all: nested-linux pru-copy
nested-linux:
	make --directory=as4-linux
pru-copy:
	mkdir -p $(HOME)/cmpt433/public/pru/
	cp -r * $(HOME)/cmpt433/public/pru/
	@echo "DO NOT MODIFY: COPY ONLY" > $(HOME)/cmpt433/public/pru/COPY_ONLY
	@echo ""
	@echo "You must build the PRU code on the target, then install it:"
	@echo "(bbg)$$ cd /mount/remote/pru/as4-pru"
	@echo "(bbg)$$ make"
	@echo "(bbg)$$ sudo make install_PRU0"
