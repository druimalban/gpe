CFLAGS = `gtk-config --cflags` -g

gpe-todo: main.o todo-ui.o todo-sql.o
	$(CC) -o $@ $^ `gtk-config --libs` -g -lsqlite

clean:
	rm gpe-todo main.o todo-ui.o todo-sql.o

