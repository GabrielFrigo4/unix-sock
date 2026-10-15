.POSIX:
.SILENT:

MAKEFLAGS += --no-print-directory -s

# ----------------------------------------------------------------
# Makefile: Posix Socket Web Server (Root Orchestrator)
# License: MIT (c) 2026 GabrielFrigo
# ----------------------------------------------------------------

.PHONY: all help build run update clean erase clangd format clang-format prettier lint hooks ci

### ================================
### HELP & DOCUMENTATION
### ================================
help:
	_e=$$'\e'; \
	cmd() { printf "    $${_e}[36mmake %-22s$${_e}[0m %s\n" "$$1" "$$2"; }; \
	sec() { printf "\n  $${_e}[1;33m%s$${_e}[0m\n" "$$1"; }; \
	printf "\n  $${_e}[1;37mPosix Socket — Servidor HTTP Concorrente C23 / FreeBSD kqueue$${_e}[0m\n"; \
	printf "  ===============================================================\n"; \
	sec "Compilação & Execução:"; \
	cmd "build"          "Compila o servidor HTTP concorrente em Project/"; \
	cmd "run"            "Executa o servidor localmente (porta padrão/admin)"; \
	cmd "update"         "Dispara deploy e sincronização com o servidor Oracle"; \
	cmd "clean"          "Remove objetos e binários compilados em Project/"; \
	cmd "erase"          "Remove objetos, binários e a pasta de dados data/"; \
	cmd "clangd"         "Gera compile_commands.json via bear para o LSP clangd"; \
	sec "Qualidade & Governança:"; \
	cmd "format"         "Formata fontes C (clang-format) e Markdown (Prettier)"; \
	cmd "clang-format"   "Formata arquivos C com clang-format"; \
	cmd "prettier"       "Formata arquivos Markdown com Prettier"; \
	cmd "lint"           "Valida formatação com Prettier sem alterar arquivos"; \
	cmd "hooks"          "Configura e ativa os quality gates locais (.githooks)"; \
	cmd "ci"             "Executa pipeline local de validação e compilação"; \
	echo ""

all: help

### ================================
### BUILD & EXECUTION (Project/)
### ================================
build:
	echo "⚙️  Delegando compilação para Project/Makefile..."
	$(MAKE) -C Project build

run:
	echo "🟢 Iniciando servidor a partir de Project/..."
	$(MAKE) -C Project run

update:
	echo "🚀 Disparando atualização remota via Project/..."
	$(MAKE) -C Project update

clean:
	echo "🧹 Limpando artefatos de compilação em Project/..."
	$(MAKE) -C Project clean

erase:
	echo "🔥 Executando apagamento total em Project/..."
	$(MAKE) -C Project erase

clangd:
	echo "📝 Atualizando compile_commands.json em Project/..."
	$(MAKE) -C Project clangd

### ================================
### GOVERNANCE & QUALITY GATES
### ================================
format: clang-format prettier
	echo "✅ Formatação concluída!"

clang-format:
	echo "🎨 Formatando arquivos C/Header com clang-format..."
	if command -v clang-format > "/dev/null" 2>&1; then \
		find Project/src Project/inc -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} + 2>/dev/null || true; \
	fi

prettier:
	echo "🎨 Formatando arquivos com Prettier..."
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --write "**/*.{md,json,yml,yaml}" 2> "/dev/null" || true; \
	elif command -v npx > "/dev/null" 2>&1; then \
		npx --yes prettier --write "**/*.{md,json,yml,yaml}" 2> "/dev/null" || true; \
	fi

lint:
	echo "🔍 Validando formatação com Prettier..."
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --check "**/*.{md,json,yml,yaml}"; \
	elif command -v npx > "/dev/null" 2>&1; then \
		npx --yes prettier --check "**/*.{md,json,yml,yaml}"; \
	fi

hooks:
	echo "⚓ Configurando permissões e ativando .githooks..."
	chmod 0755 .githooks/* 2> "/dev/null" || true
	git config core.hooksPath .githooks 2> "/dev/null" || true
	echo "  ✅ Posix Socket: core.hooksPath -> .githooks"

ci: lint build
	echo "✅ Pipeline local de validação e build concluída com sucesso!"
