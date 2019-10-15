CC= gcc
CFLAGS= -g
LIBOBJS = myalloc.o
LIB=myalloc
LIBFILE=lib$(LIB).a
TESTS = test
all: $(TESTS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

test : test.o $(LIB)
	$(CC) test.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

test1 : test1.o $(LIB)
	$(CC) test1.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

test2 : test2.o $(LIB)
	$(CC) test2.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

test3 : test3.o $(LIB)
	$(CC) test3.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

test4 : test4.o $(LIB)
	$(CC) test4.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

test5 : test5.o $(LIB)
	$(CC) test5.o $(CFLAGS) -o test -L. -l$(LIB) -lpthread

$(LIB) : $(LIBOBJS)
	ar -cvr $(LIBFILE) $(LIBOBJS)
	#ranlib $(LIBFILE) # may be needed on some systems
	ar -t $(LIBFILE)

clean:
	/bin/rm -f *.o $(TESTS) $(LIBFILE)
