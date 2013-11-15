prog: test_parser.o gps_utils.o
	gcc -o test test_parser.o gps_utils.o

test_parser.o: test_parser.c
	gcc -c test_parser.c

gps_utils.o: gps_utils.c
	gcc -c gps_utils.c
clean:
	rm -f *.o gps_utils test_parser

