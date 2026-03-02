# 🤖 **Estudos de Prompts (Sistemas Operacionais, Kernel & Redes)**
 Aqui ficam registradas as sessões de exploração teórica focadas no comportamento de sistemas operacionais, arquiteturas de baixo nível, chamadas de sistema (`syscalls`), estruturas reais da linguagem C e na física da infraestrutura de telecomunicações.

## 🗂️ **Índice de Arquivos**
 | Arquivo | Tópico Abordado |
 | :--- | :--- |
 | **[`Redes.md`](./Redes.md)** | 🌐 **Arquitetura de Redes & Telecomunicações.** <br>Ensaio acadêmico e rigoroso dissecando o Modelo OSI (da física pura à semântica), topologia, física da transmissão (FDM vs. TDM) e a matemática do atraso nodal em redes de comutação por pacotes versus circuitos. |
 | **[`História.md`](./História.md)** | 📜 **A Saga dos Sistemas Operacionais.** <br>Uma narrativa épica e geopolítica que traduz a evolução desde os Mainframes e o projeto Multics, passando pela fundação do Unix, a glória e queda do Império Solaris, a fragmentação das Cidades-Estado BSD, até a horda assimiladora do GNU/Linux e os reinos modernos do Windows e MacOS. |
 | **[`Unix.md`](./Unix.md)** | 🐧 **Kernel & Arquitetura de SO.** <br>Análise comparativa profunda em nível de engenharia (Linux, FreeBSD, Solaris/Illumos, Multics e Plan 9). Explora a camada VFS utilizando termos reais em C (como `struct vnode`, `struct file`, `pointers`, `syscalls`) e destaca o pragmatismo inteligente do FreeBSD em absorver nativamente as tecnologias do Solaris, como o ZFS e o DTrace. |
 | **[`Zero-Copy.md`](./Zero-Copy.md)** | ⚡ **Arquitetura de Hardware & Data Path.** <br>Análise técnica e física dissecando o caminho dos dados em um servidor moderno. Explora o uso de DMA, os impactos da arquitetura NUMA no acesso à memória, a sobrecarga de sistemas de arquivos (UFS vs ZFS) e detalha a Escala Definitiva do Zero-Copy (Níveis 0 a 3), culminando no Kernel Bypass através de SPDK e DPDK para saturação de links Hyper-Scale. |
