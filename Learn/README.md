# 🧠 **Base Teórica e Estudos**
 > *"Aqueles que não entendem o UNIX estão condenados a reinventá-lo, de forma pobre."* — Henry Spencer

---

## 📝 **Anotações de Estudo**
 Bem-vindo à área de documentação e arqueologia de software do projeto. Aqui residem os resumos, pesquisas teóricas e experimentações necessárias para compreender a implementação de baixo nível do servidor HTTP e a verdadeira arquitetura dos sistemas operacionais.

 Para manter o rigor e a organização, os estudos foram divididos em duas categorias principais, refletindo o fluxo de criação do conteúdo:

 | Diretório | Descrição |
 | :--- | :--- |
 | **[`/Prompts`](./Prompts/)** | 🤖 **Engenharia de Prompts.** <br>Contém os textos, questionamentos e interações originais formuladas. Estas são as entradas brutas (os "prompts") utilizadas para estruturar e gerar as documentações teóricas profundas, incluindo as investigações rigorosas sobre os gargalos do Data Path e a eliminação da CPU no tráfego de rede. |
 | **[`/Markdown`](./Markdown/)** | 📝 **Ensaios Teóricos Gerados.** <br>Contém os arquivos finais renderizados a partir dos prompts (`Unix.md`, `História.md`, `Redes.md` e `Zero-Copy.md`). Engloba a narrativa geodigital dos sistemas, a física das redes de telecomunicações e a exploração da arquitetura de kernel (detalhando a camada VFS com `struct vnode`, `struct file` e o pragmatismo do FreeBSD com ZFS e DTrace). O acervo explora também a dissecação da jornada dos dados em bare-metal, dissecando o uso de motores DMA, a assimetria da arquitetura NUMA e a escala absoluta do Zero-Copy até o Kernel Bypass (SPDK/DPDK) para infraestruturas hyper-scale. |

---

> *"A simplicidade é o pré-requisito para a confiabilidade."* — Edsger W. Dijkstra
