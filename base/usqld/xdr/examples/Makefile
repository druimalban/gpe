examples:writer_example reader_example
LDFLAGS=-L../ -lxdr
CFLAGS=-I../

writer_example: example-schema.o writer-example.c
	gcc $(CFLAGS) $(LDFLAGS) example-schema.o writer-example.c -o writer_example

reader_example: example-schema.o reader-example.c
	gcc $(CFLAGS) $(LDFLAGS) example-schema.o reader-example.c -o reader_example

example-schema.o:example-schema.c example-schema.h
	gcc $(CFLAGS) -c example-schema.c

clean: 
	rm -f writer_example reader_example *~ *o