all: clean installcheck

install:

clean:
	rm -f test_encode

test_encode: test_encode.c
	clang -fPIC ../src/encode.o test_encode.c -o test_encode -g3 -O3 -fsanitize=address -fno-omit-frame-pointer -lzstd

installcheck: test_encode
	./test_encode

