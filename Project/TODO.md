# 💻 **TODO do Projeto Servidor HTTP/1.1 RESTful em C (POSIX / FreeBSD & Linux)**

## 📋 TO-DO List Versão 1

- [x] Refatorar levemente o código C
- [x] Corrigir sincronia do Timer e expulsão de salas
- [x] Acoplar regras do jogo no `api.c` (Backend Autoritário)
- [x] Blindar endpoints e auditar acessos à API
- [x] Configurar certificado TLS/HTTPS (Caddy)
- [x] Fazer o Deploy na Oracle Cloud
- [x] Configurar domínio customizado (Registro.br)

## 📋 TO-DO List Versão 2 (Arquitetura Assíncrona)

- [ ] Adicionar um delay para cada tentativa falha de login
- [ ] Substituir `fork()` por Sockets Não-Bloqueantes (`fcntl`)
- [ ] Implementar Event Loop em O(1) (`epoll`/`kqueue`)
- [ ] Criar Máquina de Estados para leitura/escrita simultânea
- [ ] Implementar Server-Sent Events (SSE) para tempo real
- [ ] Adicionar Thread Pool para processar JSON/Lógica do jogo
- [ ] Otimizar entrega de estáticos com Zero-Copy (`sendfile()`)
