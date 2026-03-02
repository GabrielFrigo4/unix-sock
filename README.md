# 🌐 **Projeto: Implementação de um Web Server HTTP Concorrente (C/POSIX)**

## 🧠 **A Filosofia e o Desafio Arquitetural**
 No ecossistema de desenvolvimento moderno, a comunicação de rede é frequentemente ofuscada por pesadas abstrações (frameworks web, reverse proxies, runtimes gerenciados). O objetivo primário deste projeto não é competir com o Nginx ou Apache, mas sim **desconstruir a abstração**. 

 Desenvolver um servidor HTTP/1.1 em **C puro** exige confrontar diretamente a dura realidade da engenharia de sistemas: a rede é não-confiável, a memória é finita e a concorrência gera condições de corrida. Este projeto é um estudo rigoroso sobre:
 
 1. **O Modelo OSI na Prática:** A transição do fluxo de bytes brutos da Camada 4 (TCP) para o protocolo semântico da Camada 7 (HTTP), lidando com fragmentação de pacotes, reconstrução de *streams* e latência.
 2. **Gerenciamento de Estado sem Garbage Collector:** Como alocar, rastrear e liberar memória (buffers de requisição e resposta) em um ambiente de alta concorrência sem introduzir *Memory Leaks* ou *Use-After-Free*.
 3. **A Evolução da Concorrência:** O projeto está estruturado em uma jornada arquitetural clara. Começamos estabelecendo uma base sólida com as regras universais do UNIX (Versão 1 - POSIX Multiprocesso) para, em seguida, quebrarmos essas regras em busca de performance extrema utilizando APIs específicas de Kernel (Versão 2 - Event-Driven/kqueue).

---

# 🎯 **O Projeto (Versão 1 - Portabilidade e Fundamentos)**
 Esta primeira iteração foca na construção de um servidor web robusto, determinístico e portável. A implementação utiliza a **API de Sockets de Berkeley** genérica (POSIX) e operações de I/O padrão, garantindo que o código seja compilável via GCC/Gmake e rode nativamente tanto em FreeBSD quanto em distribuições Linux.

 Diferente de implementações iterativas básicas, este servidor gerencia o ciclo de vida completo de recursos através de uma Máquina de Estados Finitos (FSM) própria, suportando os seguintes métodos de requisição:

## ⚡ Definição dos Métodos Suportados
 | Método | Comportamento no Servidor | Finalidade Técnica |
 | :--- | :--- | :--- |
 | **GET** | Leitura via I/O padrão (`fread`) | Recuperação de recursos estáticos do diretório raiz. |
 | **POST** | Processamento de buffers de entrada | Submissão de dados para criação de novos estados ou recursos. |
 | **PUT** | Escrita integral de arquivos | Atualização completa de um recurso em uma URI específica. |
 | **PATCH** | Modificação atômica parcial | Atualização segmentada de recursos existentes. |
 | **DELETE** | Remoção via syscall `unlink` | Exclusão definitiva de um recurso no sistema de arquivos. |

---

# 😈 **O Ambiente de Desenvolvimento: Por que FreeBSD?**
 Embora o servidor na Versão 1 seja multiplataforma, o **FreeBSD** foi escolhido como o ecossistema primário de pesquisa, arquitetura e validação. Para engenharia de software de baixo nível (Systems Programming), o FreeBSD oferece vantagens estruturais e ferramentas analíticas que superam alternativas tradicionais.

## 📜 **1. A Origem Histórica: O Berço dos Sockets**
 O **BSD (Berkeley Software Distribution)** foi o laboratório onde a pilha TCP/IP moderna foi forjada. A **API de Sockets**, adotada universalmente hoje, foi introduzida no **4.2BSD** em 1983. Desenvolver sobre o FreeBSD é trabalhar na implementação "de referência" das redes UNIX.
 > **Referência Oficial:** [FreeBSD Developers Handbook - Sockets Programming](https://docs.freebsd.org/en/books/developers-handbook/sockets/)

## 🏗️ **2. O "Base System" Coeso**
 Diferente do Linux, que é apenas um Kernel combinado com utilitários GNU de terceiros, o FreeBSD desenvolve o Kernel, a biblioteca padrão C (`libc`) e as ferramentas de *User Space* como um **único repositório coeso (Base System)**. Isso significa que o comportamento das *syscalls*, a implementação da memória e a documentação estão sempre em perfeita sincronia, eliminando a ambiguidade comum ao se debugar problemas profundos de integração.

## 🔬 **3. Observabilidade Absoluta com DTrace**
 Para construir servidores de alta performance, "achismo" não funciona. O FreeBSD integra nativamente o **DTrace (Dynamic Tracing Framework)**. Ele permite instrumentar e rastrear o servidor em tempo real e em produção, mapeando exatamente quantos milissegundos o Kernel gasta alocando buffers de rede (mbufs), realizando trocas de contexto (*context switches*) no `fork()`, ou travando em operações de I/O de disco, sem precisar alterar uma linha de código C ou recompilar o servidor.

## 🛡️ **4. Paradigmas Superiores de Arquitetura (Roadmap V2)**
 O FreeBSD expõe primitivas de Kernel consideradas o estado da arte para escalabilidade e segurança de rede:
 * **kqueue vs epoll:** O `kqueue` do FreeBSD não monitora apenas Sockets de rede, mas unifica o monitoramento de processos (`SIGCHLD`), timers, I/O assíncrono e eventos de sistema de arquivos (vnodes) em uma única API elegante.
 * **Segurança Ofensiva/Defensiva (Capsicum):** Enquanto containers dependem de namespaces complexos, o FreeBSD permite que um servidor drope seus próprios privilégios e entre em um "Capability Mode" (`Capsicum`). Se um atacante explorar um *Buffer Overflow* na função de *parsing* HTTP deste servidor, o *Capsicum* bloqueará fisicamente no Kernel qualquer tentativa de abrir novos arquivos ou sockets maliciosos.

---

# 🚀 **Destaques da Implementação Técnica (V1)**
 Para garantir um aprendizado sólido dos fundamentos de sistemas *Unix-like*, o servidor foi construído sobre as seguintes decisões arquiteturais:

## ⚡ **Gerenciamento de Concorrência: Fork-per-Request**
 * **Modelo Multiprocesso (POSIX):** O servidor utiliza a system call `fork(2)` para delegar cada nova conexão a um processo filho isolado. O Kernel hospedeiro gerencia o escalonamento nos múltiplos núcleos da CPU.
 * **Isolamento de Memória:** Falhas de segmentação (*Segfaults*) durante o processamento de uma requisição HTTP não derrubam o daemon principal.
 * **Gestão de Zumbis:** Implementação rigorosa de handlers para o sinal `SIGCHLD`, garantindo o recolhimento (*reap*) correto de processos finalizados e evitando o esgotamento da tabela de processos do SO.

### 🗺️ Arquitetura de Concorrência do Sistema
 ```mermaid
 graph TD
     subgraph Internet Layer
         C1[Client 1]
         C2[Client 2]
         CN[Client N]
     end

     subgraph OS Kernel / User Space
         subgraph Master Daemon Process
             L[Socket Bind/Listen]
             A[Accept Loop block]
             S[SIGCHLD Reaper]
         end

         subgraph Isolated Child Processes
             W1[Child PID: 1001\nHTTP FSM]
             W2[Child PID: 1002\nHTTP FSM]
             WN[Child PID: 100N\nHTTP FSM]
         end

         subgraph Storage
             FS[(File System\nStatic Assets)]
         end
     end

     C1 -->|TCP SYN| L
     C2 -->|TCP SYN| L
     CN -->|TCP SYN| L

     L --> A
     A -->|Syscall: fork| W1
     A -->|Syscall: fork| W2
     A -->|Syscall: fork| WN

     W1 <-->|fread/fwrite| FS
     W2 <-->|fread/fwrite| FS
     WN <-->|unlink| FS

     W1 -.->|exit_success| S
     W2 -.->|exit_success| S
     WN -.->|exit_error| S
     S -.->|waitpid| A
 ```

## 🧩 **Parsing de Protocolo via Máquina de Estados (FSM)**
 * **Reconstrução de Fluxo:** Implementação de uma Máquina de Estados Finitos para processar o fluxo de bytes bruto do socket, permitindo tratar requisições fragmentadas ou ataques de *Slowloris* de forma resiliente.
 * **Análise de Headers:** Parsing manual de cabeçalhos HTTP/1.1 (como `Content-Length`), garantindo controle total sobre o layout de memória sem overhead de bibliotecas de terceiros.

---

# 🔮 **Roadmap e Evolução Arquitetural (Versão 2)**
 Se a Versão 1 foca em **Fundamentos e Portabilidade POSIX**, a futura **Versão 2** terá como foco **Alta Performance Absoluta (C10K Problem)** e **Segurança Ofensiva**, acoplando-se especificamente às otimizações avançadas do Kernel do FreeBSD:

 1. **I/O Multiplexing (kqueue/kevent):** Substituição do modelo `fork()` por um event-loop assíncrono utilizando `kqueue`, reduzindo drasticamente o overhead de *context switch* e permitindo milhares de conexões em uma única thread.
 2. **Zero-Copy I/O (`sendfile`):** Otimização da entrega de recursos utilizando a syscall `sendfile(2)` do FreeBSD, transferindo dados diretamente do Page Cache do Kernel para o buffer do socket TCP.
 3. **Sandboxing de Primitivas (Jails & Capsicum):** Execução do daemon isolado dentro de um `FreeBSD Jail` com o framework **Capsicum**, restringindo severamente a superfície de ataque em caso de exploração de vulnerabilidades.

---

# 📖 **Recursos e Documentação Oficial**
 Para garantir a integridade do desenvolvimento, utilizamos a documentação oficial do FreeBSD como **Single Source of Truth (SSoT)**.

## 🔎 **Consulta Online (Web)**
 *Melhor para busca indexada e navegação rápida entre capítulos.*

 * **[FreeBSD Books](https://docs.freebsd.org/en/books/)**: O hub central para livros e artigos técnicos.
 * **[FreeBSD Handbook](https://docs.freebsd.org/en/books/handbook/)**: O "guia definitivo" para instalação e administração.
 * **[FreeBSD Developers Handbook](https://docs.freebsd.org/en/books/developers-handbook/)**: Essencial para **programação de sockets**, chamadas de sistema e arquitetura do kernel.
 * **[FreeBSD FAQ](https://docs.freebsd.org/en/books/faq/)**: Respostas para as dúvidas mais comuns sobre o SO.
 * **[FreeBSD Manual Pages](https://man.freebsd.org/)**: Referência direta de comandos e funções da biblioteca C.

## 📥 **Download Offline (PDF)**
 *Ideal para ambientes isolados (air-gapped) ou leitura focada sem distrações.*

 | Recurso | Download PDF | Download Página |
 | --- | --- | --- |
 | **FreeBSD Handbook** | [📄 **Visualizar PDF](https://download.freebsd.org/doc/en/books/handbook/handbook_en.pdf)** | [📥 **Baixar Página](https://download.freebsd.org/doc/en/books/handbook/handbook_en.tar.gz)** |
 | **Developers Handbook** | [📄 **Visualizar PDF](https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.pdf)** | [📥 **Baixar Página](https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.tar.gz)** |
 | **FreeBSD FAQ** | [📄 **Visualizar PDF](https://download.freebsd.org/doc/en/books/faq/faq_en.pdf)** | [📥 **Baixar Página](https://download.freebsd.org/doc/en/books/faq/faq_en.tar.gz)** |

---

# 🛠️ **Acesso Rápido: Arquivos do Repositório**
 Além dos links oficiais, este repositório contém cópias locais da documentação e scripts de automação para facilitar o desenvolvimento no ambiente FreeBSD e Linux.

## 📚 **Livros (PDF Offline)**
 Estes arquivos estão localizados na pasta [`FreeBSD/Books/`](https://www.google.com/search?q=./FreeBSD/Books/).

 | 📄 Documento | 🔗 Link Local | 📝 Descrição |
 | --- | --- | --- |
 | **FreeBSD Handbook** | **[`FreeBSD Handbook.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Handbook.pdf)** | O guia definitivo de instalação, administração e uso geral do sistema. |
 | **FreeBSD Developers' Handbook** | **[`FreeBSD Developers' Handbook.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Developers%27%2520Handbook.pdf)** | Guia avançado focado em programação de Kernel, Sockets e IPC. |
 | **FreeBSD Architecture Handbook** | **[`FreeBSD Architecture Handbook.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Architecture%2520Handbook.pdf)** | Detalhes profundos sobre a estrutura e os subsistemas do kernel. |
 | **Design and Implementation of 4.4BSD** | **[`The Design and Implementation of the 4.4BSD Operating System.pdf`](https://www.google.com/search?q=./FreeBSD/Books/The%2520Design%2520and%2520Implementation%2520of%2520the%25204.4BSD%2520Operating%2520System.pdf)** | Livro clássico sobre a base de design do 4.4BSD (origem do FreeBSD). |
 | **FreeBSD Porter’s Handbook** | **[`FreeBSD Porter’s Handbook.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Porter%E2%80%99s%2520Handbook.pdf)** | Guia oficial para criar "ports" e empacotar softwares de terceiros. |
 | **FreeBSD FAQ** | **[`Frequently Asked Questions for FreeBSD.pdf`](https://www.google.com/search?q=./FreeBSD/Books/Frequently%2520Asked%2520Questions%2520for%2520FreeBSD.pdf)** | Perguntas frequentes e soluções rápidas de problemas comuns. |
 | **FreeBSD Documentation Primer** | **[`FreeBSD Documentation Project Primer for New Contributors.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Documentation%2520Project%2520Primer%2520for%2520New%2520Contributors.pdf)** | Manual para novos contribuidores do projeto de documentação oficial. |
 | **FreeBSD Project Model** | **[`A project model for the FreeBSD Project.pdf`](https://www.google.com/search?q=./FreeBSD/Books/A%2520project%2520model%2520for%2520the%2520FreeBSD%2520Project.pdf)** | Estudo sobre a estrutura organizacional e governança do projeto. |
 | **FreeBSD Accessibility Handbook** | **[`FreeBSD Accessibility Handbook.pdf`](https://www.google.com/search?q=./FreeBSD/Books/FreeBSD%2520Accessibility%2520Handbook.pdf)** | Guia de utilização e configuração de recursos de acessibilidade. |

## ⚙️ **Scripts de Configuração**
 Scripts utilitários localizados na pasta [`FreeBSD/Scripts/`](https://www.google.com/search?q=./FreeBSD/Scripts/) para auxiliar na preparação do ambiente.

 * **[`install.sh`](https://www.google.com/search?q=./FreeBSD/Scripts/install.sh)**: Script para instalação das dependências (GCC, Gmake) e compilação do projeto.
 * **[`setup.sh`](https://www.google.com/search?q=./FreeBSD/Scripts/setup.sh)**: Script para configuração inicial do ambiente (variáveis, permissões de diretórios web, etc).
