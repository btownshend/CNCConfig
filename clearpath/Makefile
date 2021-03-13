SDK=/home/bst/linuxcnc/clearpath/SDK
all: getstatus getstatus.so

getstatus.so: getstatus.o
	g++ -L"$(SDK)/sFoundation/Debug" -shared -o getstatus.so  ./getstatus.o   -lsFoundation20

getstatus: getstatus.o
	g++ -L"$(SDK)/sFoundation/Debug" -o getstatus  ./getstatus.o   -lsFoundation20

getstatus.o: getstatus.cpp
	g++ -I"$(SDK)/inc/inc-pub" -fPIC -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"getstatus.d" -MT"getstatus.o" getstatus.cpp

clean:
	rm *.d *.o *.so getstatus
