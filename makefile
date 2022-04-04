# Compile clava.c to executable and link ncurses

clava: clava.c
	cc clava.c -o clava -lncurses

debug: clava.c
	cc clava.c -o clava -lncurses -g

clean:
	rm clava
