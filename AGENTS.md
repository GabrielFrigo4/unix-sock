# 🤖 Posix Socket — AI Agent Briefing

> Servidor HTTP concorrente de alto desempenho construído do zero em C puro sobre primitivas POSIX e multiplexação de I/O via FreeBSD `kqueue`.

---

## 🧭 Identidade e Papel

O **Posix Socket** é um servidor HTTP concorrente de alto desempenho construído do zero em C puro sobre as primitivas de rede do POSIX e o subsistema de multiplexação de I/O `kqueue` do FreeBSD.

- **Stack Primária:** C23 estrito (`-std=c23`), POSIX.1-2024, FreeBSD `kqueue`, Capsicum Sandbox, OpenSSL/libcrypto
- **Foco:** Baixa latência, concorrência orientada a eventos, isolamento em sandbox e desconstrução de protocolos

---

## 📁 Estrutura Canônica de Arquivos

```text
Posix Socket/
├── Project/                   # Código-fonte em C23 do servidor HTTP
├── FreeBSD/                   # Scripts de suporte e infraestrutura FreeBSD
├── Learn/                     # Notas conceituais de sockets e kqueue
├── .agents/                   # Governança e runbooks locais de IA
├── AGENTS.md                  # Constituição e briefing para agentes de IA
├── LICENSE                    # Licença MIT
├── Makefile                   # Orquestrador POSIX silencioso
├── PRINCIPLES.md              # 22 Princípios de Engenharia aplicados
├── README.md                  # Documentação do servidor HTTP concorrente
└── TODO.md                    # Roadmap de desenvolvimento
```

---

## ⚠️ Regras Críticas para Agentes de IA

1. **Padrão C23 & POSIX Estrito:** Todo código deve compilar sem warnings com `-Wall -Wextra -Wpedantic -Wconversion -Werror`.
2. **Invariante Hermetismo de Produção:** O servidor nunca depende de `.agents/`. A deleção de `.agents/` deixa o repositório 100% funcional.
3. **Invariante Out-of-the-Box:** Modos octais no Git Index canônicos (`0755` para scripts/hooks, `0644` para fontes e documentação).
4. **Makefile Mestre:** Operações de compilação, execução e linter são orquestradas pelo `Makefile` raiz (`make build`, `make run`, `make format`, `make lint`, `make ci`).
5. **Zero Cruft:** Proibição de arquivos de banco temporários ou dados staged.
6. **Commits Semânticos:** Mensagens padronizadas no formato `<type>(<scope>): <descrição>`.

---

## 🛡️ Regra da Proatividade e Correção Contínua (Boy Scout Rule)

O agente de IA atua de forma proativa na manutenção e aplicação dos padrões canônicos deste repositório.

Ao identificar linhas ou artefatos fora dos padrões estabelecidos:

1. **Notificar concisamente** o usuário sobre o ajuste realizado.
2. **Corrigir a inconformidade**, aplicando o padrão correspondente:
    - **Comentários Narrativos:** Eliminar comentários óbvios que apenas narram código executável.
    - **Banners Estruturais:** Ajustar réguas para exatamente 64 hífens no topo ou 32 caracteres com `### ` no corpo.
    - **Portabilidade POSIX:** Substituir bashismos (`[[ ]]`, `&>`, arrays, `source`) por sintaxe estrita POSIX `/bin/sh`.
    - **Shebang Universal:** Garantir sempre `#!/usr/bin/env sh` ou `#!/usr/bin/env python3`.
    - **Sequências ANSI & Escapes:** Não use octais (`\033`, `\001`) para caracteres ou escapes. Use `[ -t 1 ] && echo -n $'\e...'` ou notação hexadecimal (`\x01`, `\x1b`). Octal é exclusivo para permissões POSIX (`chmod 0755`, `chmod 0644`, `umask`).
    - **Redirecionamento Seguro:** Envolver destinos em aspas duplas (ex: `> "/dev/null" 2>&1`).
    - **Makefiles:** Assegurar cabeçalho `.POSIX: .SILENT:`, `MAKEFLAGS += --no-print-directory -s`, alinhamento estético de variáveis e zero `@` redundante.
    - **Permissões Canônicas:** Aplicar 4 dígitos octais (`chmod 0755`, `chmod 0644`, `chmod 0700`, `chmod 0600`).
    - **Invariante Out-of-the-Box:** Garantir modos octais corretos no Git Index e auto-cura em tempo de execução sem requerer intervenção manual pós-clone.
    - **Emissão Semântica de UI:** Substituir `echo` avulsos com emojis pelas rotinas canônicas `_ui_*`.
    - **Antifragilidade & Resiliência:** Resolução ativa de caminhos em cascata, auto-cura de permissões (0600 em chaves/segredos) e validação defensiva de pré-requisitos.
    - **Curadoria Cognitiva:** Capturar decisões estruturais e regras tácitas em skills locais compactas (`.agents/skills/`), mantendo-as atualizadas e expurgando ruído obsoleto.
    - **Refatoração Sem Legado:** Expurgar aliases obsoletos, variáveis mortas e shims de compatibilidade em renomeações.
    - **Elevação Antifrágil & Shift Seguro:** Usar o bloco explícito `if [ "$(id -u)" -ne 0 ]` para resolução de `ELEVATE` (evitando término fatal sob `set -e` como root) e proteger todo comando `shift` com validação prévia de parâmetros (`[ "$#" -ge N ] && shift N`).
    - **Simetria de Documentação:** Mover regras ou invariantes universais documentadas erroneamente em sub-membros específicos para o documento de arquitetura do grupo correspondente, eliminando falsas assimetrias.

---

## 🚀 Comandos de Verificação Rápida (Quality Gates)

| Comando             | Finalidade Operacional                              |
| :------------------ | :-------------------------------------------------- |
| `make help`         | Exibe o menu interativo com alvos disponíveis       |
| `make build`        | Compila o servidor HTTP concorrente em `Project/`   |
| `make run`          | Executa o servidor localmente                       |
| `make update`       | Dispara sincronização com o servidor de deploy      |
| `make format`       | Formata fontes C (clang-format) e Markdown Prettier |
| `make clang-format` | Formata apenas o código C com clang-format          |
| `make prettier`     | Formata arquivos Markdown com Prettier              |
| `make lint`         | Valida formatação sem alterar arquivos              |
| `make hooks`        | Ativa os githooks locais com permissões `0755`      |
| `make ci`           | Executa pipeline local de validação de qualidade    |
| `make clean`        | Remove artefatos de compilação em `Project/`        |

---

## 📖 Referências Obrigatórias

Antes de qualquer modificação neste ecossistema, consulte:

- [README.md](README.md) — Documentação do servidor HTTP concorrente
- [PRINCIPLES.md](PRINCIPLES.md) — Os 22 Princípios de Engenharia Soberanos
- [TODO.md](TODO.md) — Roadmap de desenvolvimento
- [Project/Makefile](Project/Makefile) — Makefile de compilação do binário
- [LICENSE](LICENSE) — Licença MIT
- [.agents/rules/principles.md](.agents/rules/principles.md) — Regra de Princípios
- [.agents/skills/posix-socket-kqueue/SKILL.md](.agents/skills/posix-socket-kqueue/SKILL.md) — Runbook de Sockets POSIX e kqueue
