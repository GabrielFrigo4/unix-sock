# **Projeto: Implementação de um Web Server HTTP (C/Unix)**

## **O Projeto**
 Este projeto acadêmico foca na construção de um servidor web robusto e em conformidade com as especificações do **HTTP/1.1**. Diferente de implementações básicas, este servidor não apenas serve arquivos, mas gerencia o ciclo de vida completo de recursos através da implementação de múltiplos métodos de requisição:

 * **GET**: Recuperação de recursos estáticos e dinâmicos.
 * **POST**: Criação de novos recursos e submissão de formulários/dados via `stdin`.
 * **PUT & PATCH**: Atualização total e parcial de recursos no sistema de arquivos do servidor, exigindo lógica de controle de concorrência e permissões.
 * **DELETE**: Remoção segura de recursos.

 A implementação utiliza a **API de Sockets de Berkeley**, exigindo uma gestão manual e rigorosa de memória (malloc/free), manipulação de strings (parsing de headers) e controle de descritores de arquivos para garantir que o servidor seja performático e resiliente a *memory leaks*.

---

# **Por que FreeBSD?**

## **A Origem Histórica: O Berço dos Sockets**
 A escolha do FreeBSD não é meramente estética ou por "dificuldade". Historicamente, o **BSD (Berkeley Software Distribution)** foi o berço da pilha de protocolos TCP/IP moderna.

 A **API de Sockets** que usamos hoje em quase todos os sistemas operacionais (incluindo Linux e Windows) foi introduzida originalmente no **4.2BSD** em 1983. Desenvolver no FreeBSD é trabalhar no ambiente "nativo" onde a comunicação em rede via Unix foi concebida e refinada.

 > **Referência Oficial:** [FreeBSD Developers Handbook - Sockets Programming](https://docs.freebsd.org/en/books/developers-handbook/sockets/)

## **Vantagens Técnicas e Arquiteturais**
 * **A Pilha de Rede (Network Stack):** O FreeBSD é amplamente reconhecido por ter uma das pilhas TCP/IP mais limpas, estáveis e performáticas do mundo, sendo a base para infraestruturas de gigantes como Netflix e WhatsApp.
 * **kqueue (Event Notification):** Enquanto o Linux utiliza o `epoll`, o FreeBSD oferece o **`kqueue`**. É uma interface de notificação de eventos escalável e extremamente eficiente que permite ao servidor monitorar milhares de conexões simultâneas com baixo overhead de CPU.
 * **Jails e Isolamento:** Para um servidor HTTP, o FreeBSD oferece o conceito de `Jails`, permitindo rodar o processo do servidor em um ambiente de virtualização a nível de sistema operacional, aumentando drasticamente a segurança contra exploits de rede.
 * **Documentação (Man Pages):** A documentação técnica do FreeBSD (`man sockets`, `man 2 bind`) é frequentemente citada como superior e mais precisa que a de suas contrapartes, facilitando o desenvolvimento de software de sistema de baixo nível.

---

## **Destaques da Implementação Técnica**
 Para garantir alta performance e conformidade com os padrões de sistemas Unix-like, o servidor foi construído sobre três pilares fundamentais:

### ⚡ **Gerenciamento de Concorrência de Baixo Nível**
 * **Modelo orientado a eventos (kqueue):** Diferente do modelo *thread-per-connection*, utilizamos a interface `kqueue(2)` e `kevent(2)` nativa do FreeBSD para monitorar múltiplos descritores de arquivos. Isso permite uma escalabilidade eficiente com consumo mínimo de memória.
 * **Non-blocking I/O:** Implementação de sockets em modo não-bloqueante, garantindo que o servidor nunca fique ocioso aguardando uma operação de rede lenta.

### 🧩 **Parsing de Protocolo via Máquina de Estados (FSM)**
 * **Reconstrução de Fluxo:** Implementação de uma **Máquina de Estados Finitos** para processar o fluxo de bytes bruto vindo do socket. Isso permite tratar requisições fragmentadas ou ataques de *Slowloris* de forma resiliente.
 * **Análise de Headers:** Parsing manual de cabeçalhos HTTP/1.1 (como `Content-Length`, `Transfer-Encoding` e `Connection: keep-alive`), evitando o overhead de bibliotecas de alto nível e garantindo controle total sobre a memória.

### 💾 **Persistência e Manipulação de I/O**
 * **Gestão de Recursos:** Lógica robusta para os métodos de escrita (**PUT**, **PATCH** e **POST**), incluindo o tratamento de permissões de sistema de arquivos Unix e concorrência na escrita de arquivos.
 * **Zero-Copy:** Uso potencial de `sendfile(2)` para otimizar a entrega de arquivos estáticos, movendo dados diretamente do cache do kernel para o socket, sem passar pelo espaço do usuário.

---

# **Recursos e Documentação Oficial**
 Para garantir a integridade do desenvolvimento, utilizamos a documentação oficial do FreeBSD como **Single Source of Truth (SSoT)**.

## 🌐 **Consulta Online (Web)**
 *Melhor para busca indexada e navegação rápida entre capítulos.*

 * **[FreeBSD Books](https://docs.freebsd.org/en/books/)**: O hub central para livros e artigos técnicos.
 * **[FreeBSD Handbook](https://docs.freebsd.org/en/books/handbook/)**: O "guia definitivo" para instalação e administração.
 * **[FreeBSD Developers Handbook](https://docs.freebsd.org/en/books/developers-handbook/)**: Essencial para **programação de sockets**, chamadas de sistema e arquitetura do kernel.
 * **[FreeBSD FAQ](https://docs.freebsd.org/en/books/faq/)**: Respostas para as dúvidas mais comuns sobre o SO.
 * **[FreeBSD Manual Pages](https://man.freebsd.org/)**: Referência direta de comandos e funções da biblioteca C.

## 📄 **Download Offline (PDF)**
 *Ideal para ambientes isolados (air-gapped) ou leitura focada sem distrações.*

 | Recurso | Link de Download | Comando Rápido (curl) |
 | --- | --- | --- |
 | **FreeBSD Handbook** | [PDF](https://download.freebsd.org/doc/en/books/handbook/handbook_en.pdf) | `curl -O https://download.freebsd.org/doc/en/books/handbook/handbook_en.pdf` |
 | **FreeBSD Developers Handbook** | [PDF](https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.pdf) | `curl -O https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.pdf` |
 | **FreeBSD FAQ** | [PDF](https://download.freebsd.org/doc/en/books/faq/faq_en.pdf) | `curl -O https://download.freebsd.org/doc/en/books/faq/faq_en.pdf` |
