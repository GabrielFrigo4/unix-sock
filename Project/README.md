# 💻 **Implementação: Servidor HTTP/1.1**

## 🎯 **Objetivo Técnica**
 Implementação de um servidor web em **C** puro, utilizando a API de **Berkeley Sockets** no ambiente Unix (FreeBSD). O servidor gerencia manualmente alocação de memória e descritores de arquivo.

## ⚡ **Funcionalidades Suportadas**
 | Método | Status | Descrição |
 | :--- | :---: | :--- |
 | **GET** | ✅ | Recuperação de arquivos estáticos. |
 | **POST** | ✅ | Submissão de dados e criação de recursos. |
 | **PUT/PATCH** | 🚧 | Atualização de recursos (Controle de concorrência). |
 | **DELETE** | ⚠️ | Remoção de arquivos (Requer autenticação). |

## 🔨 **Compilação e Execução**
 Para compilar o projeto (certifique-se de ter o `clang` ou `gcc` instalado):

 ```sh
 make
 ./webserver
 ```

---

> **Requisito:** Este projeto foi otimizado para rodar sobre o Kernel do FreeBSD, aproveitando a estabilidade da pilha TCP/IP nativa.
