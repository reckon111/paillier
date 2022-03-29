CFLAGS1 = -lpthread 
CFLAGS2 = -lpthread -lgmp -lmysqlclient
CC = gcc
all:gendata clig query timecnt timecnt_naive
gendata : Gendata.c linklist.o my_semaphore.o 
	$(CC) -g Gendata.c linklist.o my_semaphore.o -o gendata $(CFLAGS1) 

linklist.o : linklist.c
	$(CC) -c linklist.c 

my_semaphore.o : my_semaphore.c
	$(CC) -c my_semaphore.c 

clig: clientpaillier_G.c paillier.o linklist.o my_semaphore.o 
	$(CC) -g clientpaillier_G.c paillier.o linklist.o my_semaphore.o -o clig $(CFLAGS2)

paillier.o:
	$(CC) -c paillier.c $(CFLAGS2)

query: query.c paillier.o
	$(CC) -g query.c paillier.o -o query -lgmp

timecnt: timecnt.c paillier.o
	$(CC) -g timecnt.c paillier.o -o timecnt -lgmp

timecnt_naive: timecnt_naive.c paillier.o
	$(CC) -g timecnt_naive.c paillier.o -o timecnt_naive -lgmp

clean:
	rm *.o gendata clig query timecnt timecnt_naive