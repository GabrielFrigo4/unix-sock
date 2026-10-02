---
name: posix-socket-kqueue
description: >-
    Runbook cognitivo e guia de programação de sistemas para o servidor Posix Socket.
    Use ao implementar ou depurar a arquitetura de rede em C23, multiplexação de eventos com kqueue,
    concorrência assíncrona, sandboxing Capsicum no FreeBSD e parsing seguro de HTTP.
---

# Posix Socket & kqueue Engineering Runbook

Este runbook instrui agentes de IA sobre as práticas de programação de sistemas POSIX e FreeBSD no repositório **Posix Socket** (`Personal/Systems/Posix Socket`).

---

## 1. Multiplexação Orientada a Eventos: FreeBSD `kqueue`

O servidor utiliza `kqueue(2)` e `kevent(2)` como motor de despacho de I/O de baixa latência:

```c
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int kq = kqueue();
struct kevent change;
EV_SET(&change, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
kevent(kq, &change, 1, NULL, 0, NULL);
```

### Regras de I/O Não-Bloqueante

- Todos os descritores de cliente aceitos via `accept()` devem ser configurados com `O_NONBLOCK` via `fcntl()`.
- O loop nunca deve bloquear em um único cliente; se `read()` retornar `EAGAIN` ou `EWOULDBLOCK`, o evento deve ser delegado de volta ao `kqueue`.

---

## 2. Sandboxing com Capsicum no FreeBSD

Para segurança extrema contra exploração remota:

- Após abrir portas de escuta e arquivos de configuração iniciais, entre em modo de contenção com `cap_enter()`.
- Restrinja as permissões de descritores com `cap_rights_limit(fd, &rights)` garantindo que nenhum cliente consiga invocar `open()` ou abrir sockets adicionais fora dos direitos pré-concedidos.

---

## 3. Segurança no Parsing HTTP

- **Imunidade a Buffer Overflow:** Nunca utilize `strcpy`, `strcat` ou `sprintf`. Utilize `snprintf` ou primitivas com tamanho fixo.
- **Header Parsing:** Limite o tamanho máximo dos cabeçalhos HTTP recebidos para evitar ataques de esgotamento de memória (Slowloris / DoS).
