TESTS=quadratic.i

GCC=gcc

test: ../typefind $(TESTS)
	for i in $(TESTS); do \
		../typefind $$i > $$i.new_test 2>&1; \
		if [ -f $$i.out ]; then \
				diff $$i.out $$i.new_test  > /dev/null 2>&1; \
				if (( $$? )); then \
						echo "Test $$i failed"; \
				else \
						mv -f $$i.new_test $$i.out; \
				fi; \
		else \
				mv -f $$i.new_test $$i.out; \
		fi; \
  done

%.i: %.c
	$(GCC) -E $< -o $@
	rm -f $@.out

clean:
	rm -f *.new_test *.i

distclean:
	rm -f *.out

