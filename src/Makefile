
CXX_CMD = g++ -g -O0 -std=gnu++17 
C_CMD = gcc -g -O0 -std=gnu11 
objects = Capture.o
headers = AlsaInterface.hpp

all: Capture

clean:
	rm -f Capture $(objects) capture playback

Capture: $(objects) $(headers)
	$(CXX_CMD) -o Capture $(objects) -lasound

# $@ - target of the rule
# $? - name of all pre-requisites that are newer than the target
Capture.o: Capture.cpp
	$(CXX_CMD) -o $@ -c $?

examples: TestCapture TestPlayback

TestCapture: testCapture.c
	$(C_CMD) -o $@ $? -lasound

TestPlayback: testPlayback.c
	$(C_CMD) -o $@ $? -lasound