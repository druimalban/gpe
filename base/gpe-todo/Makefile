CFLAGS = `gtk-config --cflags` -g -I../libgpewidget

gpe-todo: main.o todo-ui.o todo-sql.o list-ui.o
	$(CC) -o $@ $^ `gtk-config --libs` -g -lsqlite -L../libgpewidget -lgpewidget

clean:
	rm gpe-todo main.o todo-ui.o todo-sql.o

