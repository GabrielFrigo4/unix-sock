# 💻 **Implementação: Servidor HTTP/1.1 de Alta Concorrência em C**

## 🎯 **Arquitetura e Objetivo Técnico**
 Implementação de um servidor web robusto em **C puro**, operando diretamente sobre a API de **Berkeley Sockets**. O projeto foi otimizado para o ambiente Unix, com foco no Kernel do **FreeBSD** para aproveitar a estabilidade e performance de sua pilha TCP/IP nativa.

 Diferente de servidores iterativos básicos, esta implementação utiliza uma **Arquitetura Concorrente Multiprocesso**. 

### ⚙️ **Modelo de Concorrência: Fork-per-Request**
 Para garantir que múltiplas requisições de rede sejam atendidas simultaneamente sem bloqueio de I/O, o servidor adota a system call `fork()`. 
 * **Isolamento de Falhas:** Cada conexão de cliente é delegada a um processo filho isolado em seu próprio espaço de endereçamento.
 * **Escalabilidade:** O processo daemon principal dedica-se exclusivamente ao loop de `accept()`, enquanto o escalonador do SO (FreeBSD Scheduler) distribui o processamento dos filhos entre os núcleos de CPU disponíveis.
 * **Gestão de Recursos:** Implementação rigorosa de handlers para `SIGCHLD` para prevenir o acúmulo de processos zumbis (Defunct) e garantir o recolhimento de memória.

## ⚡ Definição dos Métodos Suportados
 | Método | Comportamento no Servidor | Finalidade Técnica |
 | :--- | :--- | :--- |
 | **GET** | Leitura de arquivos via `fread` ou `sendfile` | Recuperação de recursos estáticos do diretório raiz. |
 | **POST** | Processamento de buffers de entrada (STDIN-like) | Submissão de dados para criação de novos estados ou recursos. |
 | **PUT** | Escrita integral de arquivos (Sobrescrita) | Atualização completa de um recurso em uma URI específica. |
 | **PATCH** | Modificação atômica parcial | Atualização segmentada de recursos existentes. |
 | **DELETE** | Remoção via syscall `unlink` | Exclusão definitiva de um recurso no sistema de arquivos. |

## 🔨 **Compilação e Deploy**
 O gerenciamento de build é feito via Makefile. Requer compilador compatível com C23 (`clang` recomendado no ecossistema **FreeBSD** ou **MacOS**, ou `gcc` no **Linux**).

 ```sh
 make clear
 make build
 make run
 ```
