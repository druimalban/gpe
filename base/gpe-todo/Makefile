CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall

gpe-todo: main.o todo-ui.o todo-sql.o list-ui.o
	$(CC) -o $@ $^ `gtk-config --libs` -g -lsqlite -L../libgpewidget -lgpewidget

clean:
	rm -f gpe-todo main.o todo-ui.o todo-sql.o

