CC      := clang
CFLAGS  := -Wall -Wextra -Werror -std=c23 -I./inc -O2
TARGET  := server

SRCDIR  := src
INCDIR  := inc
OBJDIR  := obj

SOURCES := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(INCDIR)/*.h)
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

all:
	@$(MAKE) clear
	@$(MAKE) build
	@$(MAKE) run

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "✅ Compilação do executável '$(TARGET)' concluída!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: build
	@echo "🚀 Executando..."
	./$(TARGET)

clear clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "🧹 Arquivos limpos!"

.PHONY: all build run clear clean
