CC = g++
CFLAGS = -Wall -pthread -lncurses
OBJS = main.o common.o vehicle.o intersection.o parking.o controller.o visualization.o

all: traffic_sim

traffic_sim: $(OBJS)
	$(CC) $(OBJS) -o traffic_sim $(CFLAGS)

main.o: main.cpp common.h intersection.h parking.h controller.h visualization.h
	$(CC) -c main.cpp

common.o: common.cpp common.h
	$(CC) -c common.cpp

vehicle.o: vehicle.cpp vehicle.h common.h
	$(CC) -c vehicle.cpp

intersection.o: intersection.cpp intersection.h common.h controller.h
	$(CC) -c intersection.cpp

parking.o: parking.cpp parking.h common.h vehicle.h
	$(CC) -c parking.cpp

controller.o: controller.cpp controller.h common.h intersection.h parking.h
	$(CC) -c controller.cpp

visualization.o: visualization.cpp visualization.h common.h intersection.h parking.h
	$(CC) -c visualization.cpp

clean:
	rm -f *.o traffic_sim

run: traffic_sim
	./traffic_sim

debug: CFLAGS += -g
debug: traffic_sim