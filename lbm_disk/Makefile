########################################
#
#  IUPUI CSCI 503 -- Operating Systems
#
########################################

## Compiler, tools and options
CC      =  cc
FC      =  ftn
LINK    =  ftn
# OPT     = 	-g -O3
OPT     = 	-g

ADDCFLAGS = -Wall
CCFLAGS = $(OPT)
LDFLAGS = $(OPT)

## Libraries
LIBS = -lpthread -lm
INC  = -I.

## FILES
OBJECTS = main.o do_thread.o generator.o producer.o prefetch.o consumer.o
TARGET  = lbm_IObox


## Implicit rules
.SUFFIXES: .c
.c.o:
	$(CC) -c $(CCFLAGS) $(ADDCFLAGS) $(INC) $<

## Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f *~ core
