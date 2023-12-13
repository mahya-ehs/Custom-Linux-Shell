all: library output

library:
	sudo apt-get install libreadline-dev

output: shell-project.o
	gcc shell-project.o -o output -lreadline

shell-project.o: shell-project.c
	gcc -c shell-project.c

clean:
	rm *.o output
