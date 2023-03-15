CFLAGS = -MMD -Wall -Wextra -Werror
LDFLAGS = -O
LDLIBS = -lpthread

PROGRAM = lab1
OBJECTS = lab1.o

$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $(PROGRAM)

DEPS := $(OBJECTS:.o=.d)

.PHONY: run clean

run: $(PROGRAM)
	@./$(PROGRAM)

clean:
	-rm -rf $(PROGRAM) $(OBJECTS) $(DEPS)

-include $(DEPS)