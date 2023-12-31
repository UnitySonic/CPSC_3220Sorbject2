CC = gcc
CFLAGS = -Wall -g


libmythreads.a: mythreads.o
	ar -cvrs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^

cooptest: cooperative_test.c libmythreads.a 
	$(CC) $(CFLAGS) -o  $@ $^

pretest: preemptive_test.c libmythreads.a
	$(CC) $(CFLAGS) -o $@ $^

project2.tgz:
	tar cvzf $@ README makefile mythreads.h mythreads.c


clean:
	rm *.o
	rm *.a
	rm cooptest
	rm pretest
