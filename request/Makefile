TARGET = http-request

CC = gcc
CFLAGS = -Wall -fsanitize=address -fno-omit-frame-pointer -g

LINKER = $(CC)
LFLAGS = -fsanitize=address

SRCDIR = src
OBJDIR = obj
BINDIR = bin
IDIR = $(SRCDIR)/include

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(IDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -rf

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	-@$(rm) $(BINDIR)/$(TARGET) $(OBJECTS) $(BINDIR) $(OBJDIR)
	@echo "Executable and objects removed!"