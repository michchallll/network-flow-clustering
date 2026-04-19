flows: flows.c
	gcc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows.exe -lm

run: 
	./flows.exe data.txt 2 1.0 1.0 1.0 1.0

run_file:
	./flows.exe data.txt

run_valgrind:
	valgrind ./flows.exe data.txt 2 1.0 1.0 1.0 1.0

test:
	sh test.sh
