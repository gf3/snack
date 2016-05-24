include config.mk

.PHONY: default all clean cloc

# default: $(TARGET)
default: all
all: options tags $(TARGET)

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%, $(OBJDIR)/%, $(patsubst %.c, %.o, $(SOURCES)))
HEADERS = $(wildcard $(SRCDIR)/*.h)
DEPENDS = $(OBJECTS:.o=.d)

-include $(DEPENDS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	rm -rf tags
	rm -rf $(DEPENDS)
	rm -rf $(OBJECTS)
	rm -rf $(TARGET)

cloc:
	cloc --not-match-f=tags --not-match-f=ycm --not-match-d=build .

options:
	@echo build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

tags: src/*
	ctags --recurse --fields=+l --langmap=c:.c.h ./src/*
