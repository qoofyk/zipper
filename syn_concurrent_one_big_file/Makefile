########################################
#
#  IUPUI CSCI 503 -- Operating Systems
#
########################################

## Compiler, tools and options
CC      =  cc
FC      =  ftn
LINK    =  ftn
# CC      = mpicc
# FC      =  mpifort  #ftn
# LINK    =  mpifort  #ftn
OPT     =  -O3

ADDCFLAGS = -Wall
CCFLAGS = $(OPT)
LDFLAGS = $(OPT)

## Libraries
LIBS = -lpthread -lm
INC  = -I.

## FILES
OBJECTS = main.o do_thread.o generator.o sender.o writer.o receiver.o reader.o consumer.o
TARGET  = concurrent


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
