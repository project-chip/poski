
all:
	make -C tests

test:
	make -C tests test

gtest:
	make -C tests test_gtest

clean:
	make -C tests clean
