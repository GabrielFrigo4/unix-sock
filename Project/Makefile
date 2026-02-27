CC      := cc
CFLAGS  := -Wall -Wextra -std=c23 -I./inc
TARGET  := server

SRCDIR  := src
INCDIR  := inc
OBJDIR  := obj

SOURCES := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(INCDIR)/*.h)
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all
all: build

.PHONY: build
build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "✅ Executável '$(TARGET)' Gerado com Sucesso!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: build
	@echo "🚀 Iniciando Servidor..."
	./$(TARGET)

.PHONY: clear clean
clear clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "🧹 Ambiente Limpo (Objetos e Binários Removidos)."
