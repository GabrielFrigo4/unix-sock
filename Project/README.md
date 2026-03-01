# 💻 **Implementação: Servidor HTTP/1.1 Concorrente em C (POSIX)**

## 🎯 **Arquitetura e Objetivo Técnico (V1)**
 Implementação de um servidor web robusto em **C puro**, operando diretamente sobre a API padrão de **Berkeley Sockets**. Este projeto foi projetado com foco em **portabilidade UNIX**, garantindo plena compatibilidade tanto com ambientes **FreeBSD** quanto **Linux**.

 O objetivo primário desta versão é a exploração aprofundada da pilha TCP/IP, protocolos da Camada de Aplicação (HTTP/1.1) e a interface padrão POSIX, sem depender de chamadas de sistema proprietárias ou específicas de um único Kernel.

### ⚙️ **Modelo de Concorrência: POSIX Fork-per-Request**
 Para garantir que múltiplas requisições de rede sejam atendidas simultaneamente sem bloqueio de I/O, o servidor adota a system call UNIX padrão `fork()`.
 * **Isolamento de Falhas e Segurança:** Cada conexão de cliente é delegada a um processo filho isolado em seu próprio espaço de endereçamento de memória.
 * **Portabilidade de Escalonamento:** O processo daemon principal dedica-se exclusivamente ao loop de `accept()`. O escalonamento dos processos filhos é delegado ao *Scheduler* genérico do sistema operacional hospedeiro (seja o ULE no FreeBSD ou o CFS no Linux).
 * **Gestão de Recursos:** Implementação rigorosa de handlers genéricos para `SIGCHLD` para prevenir o acúmulo de processos zumbis (Defunct) e garantir o recolhimento adequado da memória.

### ⏱️ Ciclo de Vida da Conexão e Syscalls POSIX
 O diagrama abaixo ilustra a segregação de responsabilidades entre o Daemon Principal, o Kernel e o Processo Filho recém-criado, demonstrando a gestão dos *File Descriptors* de rede.

 ```mermaid
 sequenceDiagram
     participant C as HTTP Client (Browser)
     participant M as Master Daemon
     participant OS as OS Kernel (Scheduler)
     participant W as Child Process (Fork)
    
     M->>M: socket(), bind(), listen()
     loop Main Event Loop
         M->>OS: Blocked on accept()
         C->>OS: TCP 3-Way Handshake
         OS-->>M: accept() returns client_fd
         M->>OS: syscall: fork()
         OS-->>M: returns Child PID (> 0)
         OS-->>W: returns 0 (is Child)
        
         Note over M: Processo Pai
         M->>M: close(client_fd)
        
         Note over W: Processo Filho Isolado
         W->>W: close(server_fd)
         C->>W: HTTP Request payload
         W->>W: Parse Headers (FSM)
         W->>W: File I/O (fread)
         W->>C: HTTP/1.1 200 OK + Body
         W->>W: close(client_fd)
         W->>OS: syscall: exit(0)
        
         OS->>M: SIGNAL: SIGCHLD
         M->>OS: waitpid(WNOHANG)
         Note over M: Zombie Reaped! Memória liberada.
     end
 ```

## ⚡ Definição dos Métodos Suportados
 | Método | Comportamento no Servidor | Finalidade Técnica |
 | --- | --- | --- |
 | **GET** | Leitura via I/O padrão (`fread`) | Recuperação de recursos estáticos do diretório raiz. |
 | **POST** | Processamento de buffers de entrada | Submissão de dados para criação de novos estados ou recursos. |
 | **PUT** | Escrita integral de arquivos | Atualização completa de um recurso em uma URI específica. |
 | **PATCH** | Modificação atômica parcial | Atualização segmentada de recursos existentes. |
 | **DELETE** | Remoção via syscall `unlink` | Exclusão definitiva de um recurso no sistema de arquivos. |

## 🔨 **Compilação e Deploy**
 O gerenciamento de build é feito via Makefile, configurado para o compilador **GCC**, garantindo padronização entre os ambientes FreeBSD e Linux. Requer suporte ao padrão C23.

 ```sh
 make clear
 make build
 make run
 ```

---

## 🚀 **Roadmap e Evolução Arquitetural (Versão 2)**
 Embora a V1 foque em portabilidade e nos fundamentos de redes (Sockets e POSIX), a arquitetura foi desenhada para permitir uma futura evolução visando **Alta Performance Absoluta (High Concurrency)**. A Versão 2 focará na exploração aprofundada das otimizações nativas do Kernel do **FreeBSD**:

 1. **I/O Assíncrono Orientado a Eventos:** Migração do modelo `fork()` para multiplexação de I/O de alta performance utilizando a API `kqueue`/`kevent` do FreeBSD (escalando conexões simultâneas em complexidade $O(1)$ ativo).
 2. **Zero-Copy I/O:** Substituição do `fread` pela system call `sendfile()`, permitindo a transferência direta de dados do cache de página do Kernel para o buffer do socket de rede, contornando o overhead do *User Space*.
 3. **Multithreading:** Implementação de um *Thread Pool* (pthreads) aliado ao `kqueue` para processamento escalável em arquiteturas multicore, eliminando o custo de *context switch* de processos completos.
