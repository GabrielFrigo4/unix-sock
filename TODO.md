# 🗺️ Roadmap & Backlog

> Planejamento estratégico, status operacional e visão de futuro para o Servidor Web HTTP Concorrente em C puro (POSIX / FreeBSD & Linux).

---

## 📊 Status do Projeto

| Área                            |     Status     | Cobertura / Estado                                                 |
| :------------------------------ | :------------: | :----------------------------------------------------------------- |
| **Servidor HTTP/1.1 (v1)**      |   🟢 Estável   | Socket multiplexing base, regras de negócio e deploy em produção   |
| **Arquitetura Assíncrona (v2)** | 🟡 Em Evolução | Event loop O(1) via `kqueue`/`epoll`, non-blocking I/O e zero-copy |
| **Segurança & Sandbox**         |   🟢 Estável   | Capsicum Capability Mode no FreeBSD e isolamento de descritores    |
| **Observabilidade & Build**     |   🟢 Estável   | Makefile POSIX silencioso e instrumentação DTrace                  |

---

## 🎯 Grandes Épicos & Backlog

### 1. 📋 Versão 1: Fundação & Deploy

- [x] Refatorar levemente o código C
- [x] Corrigir sincronia do Timer e expulsão de salas
- [x] Acoplar regras do jogo no `api.c` (Backend Autoritário)
- [x] Blindar endpoints e auditar acessos à API
- [x] Configurar certificado TLS/HTTPS (Caddy)
- [x] Fazer o Deploy na Oracle Cloud
- [x] Configurar domínio customizado (Registro.br)

### 2. ⚡ Versão 2: Arquitetura Assíncrona

- [ ] Adicionar um delay para cada tentativa falha de login
- [ ] Substituir `fork()` por Sockets Não-Bloqueantes (`fcntl`)
- [ ] Implementar Event Loop em O(1) (`epoll`/`kqueue`)
- [ ] Criar Máquina de Estados para leitura/escrita simultânea
- [ ] Implementar Server-Sent Events (SSE) para tempo real
- [ ] Adicionar Thread Pool para processar JSON/Lógica do jogo
- [ ] Otimizar entrega de estáticos com Zero-Copy (`sendfile()`)

---

> [!TIP]
> Para detalhes sobre convenções de código e diretrizes de engenharia, consulte o [PRINCIPLES.md](PRINCIPLES.md).
