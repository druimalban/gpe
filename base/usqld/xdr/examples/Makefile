examples:writer_example reader_example


writer_example: example-schema.o writer-example.c
	gcc $(CFLAGS) example-schema.o writer-example.c -o writer_example

writer_example: example-schema.o reader-example.c
	gcc $(CFLAGS) example-schema.o reader-example.c -o reader_example

