CC = gcc
LD = gcc

SRCPATH  = ./source/
INCPATH  = ./include/
OUTPATH  = ./out/
TESTPATH = ./test/

CFLAGS = $(CFLAG)
CFLAGS += -I$(INCPATH)
LFLAGS = -lpthread

TARGET = MediaServer
TEST = testMediaServer
TARGET_SRC = $(wildcard $(SRCPATH)*.c)
TARGET_DPS = $(patsubst $(SRCPATH)%c, $(OUTPATH)%o, $(TARGET_SRC))
#TEST_SRC = $(wildcard $(TESTPATH)/*.c)
#TEST_DPS = $(patsubst %.c, %o, $(TEST_SRC))

all : $(TARGET) 

$(TARGET): $(TARGET_DPS)
	$(LD) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET_DPS): $(TARGET_SRC)
	$(CC) $(CFLAGS) -c $^ 
	mv *.o $(OUTPATH)
	
.PHONY:all clean test

clean : 
	rm $(TARGET_DPS) $(TARGET) $(TEST) 