CFLAGS = `gtk-config --cflags` -g

gpe-tasklist: main.o todo-ui.o todo-sql.o
	$(CC) -o $@ $^ `gtk-config --libs` -g -lsqlite

clean:
	rm gpe-tasklist main.o todo-ui.o todo-sql.o

