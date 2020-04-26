# From https://www.linuxjournal.com/article/6735

C_CMD = gcc -g -O2 -std=gnu11 

all: examples

clean:
	rm -f  $(objects) TestCapture TestPlayback

examples: TestCapture TestPlayback

# $@ - target of the rule
# $? - name of all pre-requisites that are newer than the target
TestCapture: testCapture.c
	$(C_CMD) -o $@ $? -lasound

TestPlayback: testPlayback.c
	$(C_CMD) -o $@ $? -lasound