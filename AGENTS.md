# 🤖 AGENTS.md — Diretrizes para Agentes de IA no Posix Socket

Bem-vindo ao repositório **Posix Socket** (`Personal/Systems/Posix Socket`). Este documento é a constituição primária e instrução mandatória para agentes de Inteligência Artificial operando nesta base de código.

---

## 1. Identidade e Papel

O **Posix Socket** é um servidor HTTP concorrente de alto desempenho construído do zero em C puro sobre as primitivas de rede do POSIX e o subsistema de multiplexação de I/O `kqueue` do FreeBSD.

- **Stack:** C23 estrito (`-std=c23`), POSIX.1-2024, FreeBSD `kqueue`, Capsicum Sandbox, OpenSSL/libcrypto.
- **Foco:** Baixa latência, concorrência orientada a eventos, isolamento em sandbox e desconstrução de protocolos.

---

## 2. Regras Críticas Soberanas

1. **Padrão C23 & POSIX Estrito:** Todo código deve compilar sem warnings com `-Wall -Wextra -Wpedantic -Wconversion -Werror`.
2. **Invariante Hermetismo de Produção:** O servidor nunca depende de `.agents/`. A deleção de `.agents/` deixa o repositório 100% funcional.
3. **Invariante Out-of-the-Box:** Modos octais no Git Index canônicos (`0755` para scripts/hooks, `0644` para fontes e documentação).
4. **Makefile Mestre:** Operações de compilação, execução e linter são orquestradas pelo `Makefile` raiz (`make build`, `make run`, `make format`, `make lint`, `make ci`).
5. **Zero Cruft:** Proibição de arquivos de banco temporários ou dados staged.
6. **Commits Semânticos:** Mensagens padronizadas no formato `<type>(<scope>): <descrição>`.

---

## 3. Boy Scout Rule

Sempre deixe o acampamento mais limpo do que encontrou:

- [ ] Valide que nenhuma credencial ou arquivo em `data/` seja comitado.
- [ ] Mantenha títulos de documentação com badges vetoriais sem excesso de emojis.
- [ ] Valide formatação com `make lint` e `make format`.

---

## 4. Comandos de Verificação Rápidos

| Comando       | Descrição                                       |
| :------------ | :---------------------------------------------- |
| `make help`   | Exibe o menu interativo com alvos disponíveis   |
| `make build`  | Compila o servidor HTTP concorrente em Project/ |
| `make run`    | Executa o servidor localmente                   |
| `make clean`  | Remove artefatos de compilação em Project/      |
| `make format` | Formata arquivos Markdown com Prettier          |
| `make lint`   | Valida formatação sem alterar arquivos          |
| `make hooks`  | Ativa os githooks locais com permissões 0755    |
| `make ci`     | Executa pipeline de validação de qualidade      |

---

## 5. Referências Obrigatórias

- [Documentação do Projeto](README.md)
- [Regras de Agentes](.agents/rules/principles.md)
- [Project Makefile](Project/Makefile)
