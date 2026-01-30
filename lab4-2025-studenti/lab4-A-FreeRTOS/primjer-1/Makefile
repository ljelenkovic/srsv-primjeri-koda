TARGET = lab4

DIST_DIR = ./FreeRTOS
KERNEL_DIR = $(DIST_DIR)/FreeRTOS/Source
PORT_DIR = $(KERNEL_DIR)/portable/ThirdParty/GCC/Posix

CC = gcc
CFLAGS = -Wall -g -pthread -I. 

INCLUDES = -I$(KERNEL_DIR)/include \
           -I$(PORT_DIR) \
           -I$(PORT_DIR)/utils


SOURCES = main.c
SOURCES += $(KERNEL_DIR)/tasks.c \
           $(KERNEL_DIR)/queue.c \
           $(KERNEL_DIR)/list.c \
           $(KERNEL_DIR)/timers.c \
           $(KERNEL_DIR)/event_groups.c \
           $(KERNEL_DIR)/stream_buffer.c

SOURCES += $(KERNEL_DIR)/portable/MemMang/heap_3.c

SOURCES += $(PORT_DIR)/port.c \
           $(PORT_DIR)/utils/wait_for_event.c

OBJS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)