Para compreender a verdadeira engenharia de sistemas de altíssimo desempenho, é preciso abandonar as abstrações confortáveis. Quando operamos na escala de 400 Gbps, o Sistema Operacional tradicional deixa de ser um facilitador e passa a ser o principal obstáculo. O "Data Path" não é sobre código limpo ou arquiteturas de software; é sobre a física imperdoável da latência, a sinalização elétrica no silício e o gerenciamento brutal da memória.

Abaixo, dissecaremos a anatomia do tráfego de dados em um servidor bare-metal moderno. Esta é a análise definitiva sobre como os elétrons trafegam dos transistores NAND do seu NVMe até as portas ópticas QSFP-DD da sua placa de rede inteligente (SmartNIC), culminando na eliminação absoluta da CPU no processo.

---

### 1. O Paradoxo do Trabalhador e do Maestro (CPU vs. DMA)

#### A Anatomia do DMA (Direct Memory Access)

A Unidade Central de Processamento (CPU) é o maestro de uma orquestra complexa, equipada com unidades de execução superescalares, preditores de desvios e pipelines profundos projetados para computação matemática e lógica de estado. Utilizar esse silício caríssimo para mover bytes de um endereço de RAM para outro é um crime arquitetural. É aqui que entra o DMA, um subsistema de hardware dedicado que atua como a força bruta do sistema, assumindo a transferência de dados e liberando a CPU para continuar executando instruções reais.

Fisicamente, as placas PCIe (como NVMes e NICs) contêm seus próprios motores de DMA. A comunicação ocorre por meio de estruturas chamadas *Ring Buffers* (Filas de Submissão e Conclusão) alocadas na memória principal. A CPU escreve um descritor de I/O na fila de submissão — indicando o endereço físico, o tamanho do payload e a operação —, executa uma instrução de *Memory-Mapped I/O* (MMIO) para tocar a campainha (*Doorbell Register*) no barramento PCIe, e se retira. A partir desse momento, a placa PCIe emite pacotes TLP (*Transaction Layer Packets*) que navegam pelo Root Complex do PCIe diretamente para o Controlador de Memória.

Durante essa transferência autônoma, a CPU não está ociosa. Ela pode processar pacotes TCP, calcular rotas ou servir requisições de outros clientes. Quando o controlador de DMA finaliza a movimentação dos dados, ele escreve um evento de conclusão no *Completion Queue* e dispara uma interrupção de hardware (MSI-X) para avisar o núcleo da CPU de que o trabalho braçal foi finalizado. Essa delegação absoluta é a fundação que permite a um servidor lidar com milhões de IOPS simultâneos sem que os núcleos físicos travem esperando barramentos elétricos.

#### A Ilusão do Gargalo da RAM: Latência vs. Throughput

Um erro primário entre desenvolvedores de alto nível é confundir latência com largura de banda. A latência da memória DDR5 moderna para buscar um único byte ainda é restrita pelas leis da física e ciclos de CAS, oscilando em torno de 70 a 90 nanossegundos. Para a CPU, que opera em frequências acima de 3 GHz, 80 nanossegundos equivalem a centenas de ciclos de clock desperdiçados esperando que a voltagem das células capacitivas da RAM se estabilize. No entanto, o throughput (largura de banda) é colossal. Usando a equação de largura de banda teórica $B = f_{clk} \times W \times T$ (onde $f_{clk}$ é o clock do barramento, $W$ é a largura em bytes e $T$ é o número de transferências por clock), servidores com múltiplos canais de memória entregam facilmente mais de 200 GB/s.

Para mascarar a latência desastrosa, a engenharia de silício introduziu os Caches L1, L2 e L3. O prefetcher de hardware da CPU tenta adivinhar quais endereços de memória serão acessados em seguida, puxando blocos de 64 bytes (Cache Lines) da RAM principal para a SRAM ultrarrápida (L1) antes mesmo que a instrução os solicite. Enquanto a memória principal é inundada por múltiplos fluxos de DMA vindos de vários discos e placas de rede simultaneamente (saturando os canais de throughput), a CPU executa suas instruções de rede e manipulação de protocolos inteiramente dentro do escopo de baixa latência dos caches locais.

O desafio crítico aqui é a Coerência de Cache (protocolos como MESI). Quando um controlador de rede faz um DMA escrevendo diretamente na RAM, ele invalida passivamente as *Cache Lines* correspondentes que a CPU possa ter armazenado em L3. Se a CPU tentar ler esse pacote logo em seguida, sofrerá um *Cache Miss* brutal e terá que esperar os dolorosos 80 nanossegundos para buscar os dados atualizados na memória principal. Gerenciar o *Data Path* moderno significa alinhar estruturas de dados em múltiplos de 64 bytes para evitar que threads destruam as linhas de cache umas das outras (*False Sharing*).

#### O Plot Twist do DMA Interno (Intel DSA)

Historicamente, o DMA resolvia o tráfego entre periféricos e a RAM. Mas e quanto à movimentação interna de dados dentro da própria memória? Operações de sistema operacional como a chamada `memcpy()` em C ainda forçavam os núcleos da CPU a puxar dados da RAM para os registradores vetoriais e empurrá-los de volta para a RAM. Em um link de rede moderno, copiar um pacote na memória pode consumir de 20% a 30% do tempo de CPU de um servidor hyper-scale.

Para sanar esse gargalo, fabricantes introduziram motores de DMA embutidos diretamente no SoC (System on a Chip), como o Intel DSA (*Data Streaming Accelerator*). O DSA é um acelerador on-die cujo único propósito ético é otimizar cópias de memória intra-RAM, preenchimento de buffers (`memset`) e cálculos de redundância. Ele existe fora dos núcleos de execução tradicionais, mas está intimamente ligado ao barramento interno do processador e ao cache L3 compartilhando a mesma visão de memória.

Com o DSA habilitado via drivers especializados, quando o Kernel Linux ou o DPDK precisam reordenar pacotes na memória ou duplicar um buffer de rede, a thread utiliza a nova instrução de assembly `ENQCMD` para enviar o descritor de trabalho diretamente ao acelerador interno. A CPU instantaneamente retoma o processamento das regras de negócio, enquanto o silício do DSA move silenciosamente megabytes de dados nos bastidores. É a terceirização completa da burocracia do `memcpy()`.

---

### 2. O Pedágio do Sistema de Arquivos (UFS vs ZFS no Sendfile)

#### O Casamento do UFS com a Rede

Quando a Netflix otimiza seus *Open Connect Appliances* (OCAs) para empurrar múltiplos terabits por segundo de tráfego de vídeo usando FreeBSD, ela abraça o UFS (Unix File System). O UFS é considerado "primitivo" pelos puristas de storage, mas é exatamente sua simplicidade brutal que o torna ideal para hyper-scale. No UFS, há um mapeamento estático e previsível entre o Inode do arquivo e os blocos lógicos no disco. O Kernel carrega os setores do disco diretamente para o *Page Cache* da memória RAM física.

O pulo do gato ocorre na chamada de sistema `sendfile()`. Como o UFS delega o cache de leitura para o sistema de memória virtual unificado do Kernel, as páginas de memória que contêm o vídeo requisitado já estão residentes no *Page Cache*. Quando o Nginx ou outro servidor web invoca o `sendfile`, o Kernel simplesmente pega o endereço físico dessa página de memória, acopla-o a uma estrutura descritora de rede (como o `mbuf` no FreeBSD ou o `sk_buff` no Linux) e envia esse ponteiro para a placa de rede.

Não há cópia de dados. O motor de DMA da placa de rede consome os bytes lendo diretamente do *Page Cache* residente. A simplicidade do UFS garante que o Kernel não precise intervir na transação de dados; a CPU atua puramente como um despachante de ponteiros de memória, permitindo que a infraestrutura sature a capacidade máxima das portas ópticas mantendo a utilização da CPU abaixo dos 10%.

#### A Mágica e o Custo do ZFS

O ZFS é uma obra-prima da engenharia de dados, mas cobra um pedágio impagável para redes extremas. Ao contrário do UFS, que confia cegamente que o dado lido do disco está íntegro, o ZFS é um sistema copy-on-write transacional que garante integridade criptográfica bloco a bloco. O ZFS possui seu próprio cache altamente inteligente e segregado, o ARC (*Adaptive Replacement Cache*), que compete com — ou trabalha paralelamente a — o *Page Cache* padrão do sistema.

Aqui reside o problema: a verificação rigorosa do ZFS significa que os dados não podem simplesmente fluir do disco para o cache passivamente via DMA invisível. Para validar as *Checksums* (como Fletcher4 ou SHA-256), cada byte de cada bloco lido deve ser roteado pelas unidades lógico-aritméticas da CPU. O driver do ZFS emprega intensamente instruções vetoriais SIMD/AVX para calcular hashes em alta velocidade. Embora isso garanta a pureza dos dados contra *bit-rot* magnético, isso satura os canais de memória (buscando o dado e a checksum) e queima ciclos preciosos da CPU no Data Path.

Ainda que o ZFS moderno no FreeBSD tenha sido extensivamente remendado para suportar `sendfile()` de forma mais limpa, a abstração não é gratuita. O ato de puxar terabytes de tráfego do NVMe, desembalar compressão (LZ4/ZSTD), e verificar hashes via AVX-512 destrói a ilusão de "custo zero". Para um storage corporativo, é excelente. Para um CDN tentando empurrar 400 Gbps de dados estáticos diretamente para a internet, é um gargalo de processamento matemático insustentável.

#### Block Cloning (Copy-on-Write)

Existem situações onde não estamos despachando arquivos para a rede, mas manipulando-os no próprio disco (ex: duplicar um arquivo de log ou template de máquina virtual). No UFS tradicional, uma cópia local forçaria o Kernel a ler todos os blocos do arquivo original para o *Page Cache* e gravar os mesmos blocos de volta em novos setores no disco, dobrando o desgaste do SSD e o tráfego do barramento PCIe.

Sistemas como o ZFS (e btrfs no Linux) utilizam a arquitetura Copy-on-Write para implementar o *Block Cloning* através de chamadas como `copy_file_range()`. Devido à sua árvore-B (*B-Tree*) transacional e baseada em Merkle Trees, quando um aplicativo solicita a cópia de um arquivo gigante, o ZFS não move um único byte de payload. O sistema de arquivos simplesmente aloca um novo Inode, duplica os ponteiros de bloco (*Block Pointers*) na árvore de metadados e incrementa o contador de referências (*Reference Count*) dos blocos físicos originais no disco.

A cópia de um arquivo de 100 GB acontece em poucos microssegundos, pois apenas alguns kilobytes de metadados foram atualizados em memória. Se a cópia for modificada posteriormente, as gravações subsequentes serão alocadas em novos blocos vazios (o verdadeiro Copy-on-Write). Esta é uma das poucas vezes em que a complexidade do ZFS oferece um "Zero-Copy" mais poderoso e radical do que o UFS, poupando a vida útil do NVMe e o tráfego no barramento I/O local.

---

### 3. O Pesadelo Multi-Core: NUMA e a Arquitetura "Shared-Nothing"

#### Atravessando a Ponte: O Custo do Interconnect

Servidores modernos não são computadores singulares; são redes distribuídas comprimidas em um chassi 1U. A arquitetura NUMA (*Non-Uniform Memory Access*) dita que, em sistemas de múltiplos soquetes (Dual-Socket Xeon ou EPYC), cada processador possui seu próprio controlador de memória e gerencia a RAM e os slots PCIe fisicamente conectados a ele. Se a placa de rede estiver instalada no slot PCIe 1 (controlado pela CPU 0) e o dado estiver na RAM espetada na placa-mãe perto da CPU 1, o desastre arquitetural está montado.

Quando a NIC tenta realizar um DMA de envio, a transação não vai diretamente para a RAM. O controlador PCIe da CPU 0 nota que o endereço físico solicitado pertence à CPU 1. O dado deve então atravessar a interconexão de alta velocidade entre os processadores — UPI (*Ultra Path Interconnect*) na Intel ou Infinity Fabric na AMD. Essa viagem aumenta a latência de acesso à memória de cerca de 80ns (local) para mais de 130ns (remoto).

Pior do que a latência é o esgotamento da largura de banda. Em taxas extremas (centenas de Gbps), saturar o link UPI/Infinity Fabric significa bloquear o tráfego de coerência de cache e de sincronização entre os núcleos. O sistema entra em colapso; as CPUs passam mais tempo esperando pacotes cruzarem a ponte entre soquetes do que processando lógica de rede, resultando em quedas abissais de performance e instabilidade generalizada. A assimetria de hardware pune a ignorância de software com severidade.

#### Affinity & Pinning: O Hardware como Sistemas Isolados

Para domar a arquitetura NUMA no Kernel Linux ou FreeBSD, arquitetos tratam cada soquete físico — e às vezes cada *Die* dentro de um chip moderno — como um nó isolado (*Shared-Nothing*). A regra de ouro do alto desempenho é evitar que ponteiros e memória cruzem as fronteiras do NUMA. O sistema operacional utiliza mecanismos como o RSS (*Receive Side Scaling*) nos hardwares de rede para distribuir os pacotes de entrada baseados em hashes criptográficos, mas a configuração humana é crucial.

As threads da aplicação são fixadas (*Pinned*) em núcleos específicos de uma CPU (`taskset` ou `cpuset`). As interrupções (IRQs) geradas pela placa de rede e pelo NVMe são rigidamente roteadas para os mesmos núcleos lógicos onde as threads correspondentes residem (usando ferramentas como `irqbalance` desativado e roteamento manual no `smp_affinity`).

A política de alocação de memória virtual usa bibliotecas como o `numactl` para forçar que todo o *Page Cache*, buffers de rede e heaps da aplicação sejam alocados estritamente nos bancos de memória DDR locais daquele núcleo. O servidor de dois soquetes deixa de existir como um supercomputador unificado e passa a operar como dois computadores independentes compartilhando a mesma carcaça, dividindo a carga de tráfego externamente, mas mantendo a pureza absoluta do Data Path em suas fronteiras internas de silício.

---

### 4. A Escala Definitiva do Zero-Copy (Níveis 0 a 3)

#### Nível 0 (O Inferno do Context Switch)

O cenário padrão de desenvolvimento de aplicações (e a razão pela qual APIs em Node.js ou Java não servem para processamento de pacotes na escala de terabits) utiliza o paradigma `read()` e `write()`. Quando a aplicação pede para ler do disco e enviar para a rede, ela dispara interrupções de software chamadas *Context Switches*. O Kernel muda do User-Land para o Ring 0 privilegiado, esvaziando o pipeline de execução da CPU.

Os dados viajam do Disco via DMA para o Buffer do Kernel (*Page Cache*). Em seguida, a CPU deve fisicamente copiar os bytes desse buffer do Kernel para o *User Buffer* no espaço de memória da aplicação (`copy_to_user`). A aplicação invoca `write()`, gerando outro *Context Switch*. A CPU, novamente, copia os bytes do User Buffer de volta para o *Socket Buffer* do Kernel (`copy_from_user`), de onde finalmente a placa de rede faz o DMA de envio.

O saldo é aterrorizante: Duas travessias violentas de fronteiras de proteção (User/Kernel) destruindo predições e caches locais (*TLB flushes*). Três cópias brutas de dados congestionando o barramento da RAM DDR5 de forma desnecessária. É uma arquitetura focada na segurança do sistema multiusuário concebida nos anos 70, mas letalmente ineficiente para a infraestrutura contemporânea de internet.

#### Nível 1 (In-Kernel Copy)

O primeiro degrau na busca pela sanidade computacional é eliminar as passagens pelos anéis de privilégio de usuário e os buffers da aplicação. Utilizando as chamadas de sistema `splice()` no Linux ou o funcionamento rudimentar de `copy_file_range()` em sistemas de arquivos sem copy-on-write nativo (onde ocorre o fallback), a aplicação atua apenas para abrir os *File Descriptors* do arquivo e do socket de rede, instruindo o Kernel a realizar o trabalho internamente.

O disco lê para o *Page Cache* via DMA. Como o descritor de arquivo de origem e o socket de destino estão conectados em *Kernel Space*, o Kernel utiliza a CPU para rodar uma rotina de in-kernel copy (um `memcpy()` altamente otimizado), movendo os bytes do buffer de I/O do disco para o buffer de transmissão do protocolo TCP/IP, sem nunca expor esses dados para o *User-Land*.

O gargalo do *Context Switch* foi resolvido. A aplicação delegou a função perfeitamente. Contudo, o pecado da CPU trabalhando como operária de cargas persiste. A CPU ainda precisa ler e escrever gigabytes na RAM. Embora seja muito mais rápido do que o Nível 0, a utilização do processador para transferências ainda cria um teto artificial de throughput diretamente ligado ao clock da CPU e ao acesso local ao cache L3.

#### Nível 2 (Hardware Zero-Copy)

Aqui entramos no domínio do profissional. O verdadeiro `sendfile()` suportado pelo hardware das SmartNICs e dos SoCs modernos muda a filosofia do Data Path: a CPU não move pacotes de dados, apenas ponteiros de controle. O arquivo de vídeo ou banco de dados repousa imóvel no *Page Cache* da memória RAM após o DMA inicial realizado pelo NVMe.

A aplicação chama `sendfile()`, informando ao Kernel que o arquivo *X* deve ser enviado pelo socket *Y*. O Kernel constrói apenas os cabeçalhos TCP/IP (Headers) associados a esta transmissão e os enfileira. Mas no lugar do corpo do pacote (*Payload*), ele insere na estrutura do buffer (`sk_buff`/`mbuf`) um endereço físico apontando diretamente para o início da página de memória residente no *Page Cache*.

Quando a placa de rede inicia sua operação de envio por DMA via barramento PCIe, seu *Scatter-Gather engine* busca de forma inteligente os cabeçalhos TCP construídos pelo Kernel em uma área de memória, e emenda-os ao vivo com o enorme payload lido diretamente do *Page Cache*. Os dados viajam do Disco para a RAM, e da RAM para o link óptico QSFP. A CPU apenas preencheu formulários burocráticos (headers e ponteiros), mas nunca colocou a "mão" no pacote pesado propriamente dito. Zero cópias intermediárias na RAM.

#### Nível +2 (Bypass de File System / Raw Data)

A Netflix dominou esta técnica com seu offload TLS (kTLS) e roteamento raw. Mesmo no Nível 2, a existência de um *File System* e da burocracia do *Virtual File System* (VFS) com seus Inodes, diretórios e *locks* de controle de concorrência cria uma sobrecarga (*overhead*) inaceitável. O VFS tenta gerenciar os acessos concorrentes a arquivos com mecanismos de exclusão mútua e refcounting nas páginas em cache, o que causa degradação (*cache line bouncing*) quando 64 núcleos de CPU tentam ler do mesmo servidor web simultaneamente.

A solução na Escala Nível +2 é assassinar o Sistema de Arquivos da equação do I/O massivo. Os engenheiros mapeiam o dispositivo de bloco bruto (Raw Block Device) no espaço lógico do sistema. O banco de dados ou a aplicação de streaming conhece com exatidão a posição física de cada pedaço de dado traduzido em *Logical Block Addresses* (LBAs) da unidade NVMe. Eles pré-computam as estruturas e utilizam a arquitetura de blocos diretos para solicitar a I/O, escapando de toda a camada VFS.

O fluxo se torna cristalino: O motor da aplicação instrui o disco a preencher um buffer predefinido em RAM utilizando endereços físicos crus, e instrui o processador criptográfico offload da NIC a buscar esse bloco e encapsulá-lo em criptografia AES-GCM diretamente via hardware. Evitamos a serialização imposta pelos semáforos dos *Inodes* e conseguimos explorar o NVMe na máxima performance assíncrona tolerada por seus controladores Flash.

#### Nível 3 (Peer-to-Peer DMA / O Santo Graal)

O Santo Graal da arquitetura "Data Path". Neste nível avançado, até mesmo a memória RAM se torna um intermediário inútil que rouba ciclos de latência do sistema. Na tecnologia PCIe moderna (via NVMe CMB - *Controller Memory Buffer* ou P2PDMA do Linux), conseguimos forçar as pontas finais do hardware a conversar diretamente sem envolver o host.

O Barramento PCIe opera usando Base Address Registers (BARs), que mapeiam o espaço de endereçamento das placas para o sistema. No *Peer-to-Peer DMA*, a aplicação configura a memória física exposta no BAR da Placa de Rede para ser o alvo de gravação do NVMe. A CPU envia o comando de inicialização (submissão) para o NVMe solicitando a leitura física do bloco de filme no disco e estipulando que o endereço de destino (Destination Address) do DMA não é um módulo DDR5 na placa mãe, mas o endereço físico do BAR da NIC.

O dado viaja internamente na controladora do SSD, encapsula-se em TLPs de barramento, sobe para o Switch PCIe integrado (ou do processador) e é roteado imediatamente para o barramento físico da placa de rede de 400 Gbps, ou mesmo para a memória GDDR de uma GPU para computação via Vulkan ou CUDA. A memória principal não é tocada. A CPU não é acionada. O consumo elétrico na RAM DDR despenca e alcançamos a perfeição do roteamento puro no silício, o verdadeiro *Hardware Bypass*.

---

### 5. O Adeus ao Sistema Operacional (Kernel Bypass)

#### A Parede de Concreto do Sendfile

Por mais eficientes que as chamadas POSIX tradicionais pareçam, elas repousam sobre uma fundação apodrecida para fins de Hyper-Scale: a pilha genérica de rede TCP/IP do Kernel. Quando utilizamos `sendfile()` ou sockets nativos do Linux, submetemo-nos a especificações projetadas décadas atrás, que priorizavam confiabilidade em conexões instáveis e serialização estrita de streams de bytes.

O Kernel é, por definição, o árbitro global. Ele impõe travamentos globais (Locks), concorrência de interrupts, gerenciamento complexo da tabela de roteamento e *context switching* reativo. Em uma placa de rede despachando dezenas de milhões de pacotes por segundo em portas duplas de 100/400 Gbps, o custo das interrupções de hardware contínuas leva ao "Interrupt Storm", sufocando a CPU inteiramente com rotinas de *Interrupt Service Routines* (ISR) em vez de executar a aplicação de fato. A parede de concreto intransponível é que um Sistema Operacional multipropósito simplesmente carrega muito lixo na bagagem para acompanhar latências na casa dos nanossegundos contínuos.

A estrutura de rede POSIX falha em escalar linearmente porque a abstração oculta o controle direto das múltiplas filas de hardware (*Multi-Queue NICs*) que devem estar mapeadas deterministicamente para evitar colisões entre núcleos (Locks). O Kernel exige conformidade. Para esmagar a barreira da rede de nova geração, os desenvolvedores de baixo nível precisaram ignorar o Kernel.

#### SPDK, DPDK e Netmap

A solução arquitetural definitiva para contornar a letargia do Kernel é arrancá-lo violentamente do "Data Path" de execução usando frameworks como DPDK (*Data Plane Development Kit*) e SPDK (*Storage Performance Development Kit*). A mágica dessas tecnologias de user-space se apoia em separar a NIC e o NVMe do controle padrão dos drivers do Kernel através de subsistemas como UIO ou VFIO, e expor os BARs do PCIe e os *Ring Buffers* de I/O de acesso direto à memória alocada nos Hugepages (blocos contíguos gigantes de RAM de 2MB ou 1GB para eliminar os TLB Misses da paginação convencional de 4KB).

Dentro do ambiente da aplicação, os drivers em user-space operam em *Poll Mode Drivers* (PMD). Diferente do Kernel, que passivamente dorme esperando que a NIC envie uma interrupção (IRQ) para avisar da chegada de um pacote, o aplicativo de *Bypass* aloca núcleos físicos isolados de CPU para executar loops mortos de repetição infinita (`while(1)`). Estes núcleos interrogam (fazem *Polling* contínuo) dos registradores da placa de rede a cada ciclo de clock. Interrupções são banidas.

O resultado final é que o aplicativo consome 100% de CPU naquele núcleo mesmo em repouso, mas a latência de processamento em tráfego intenso desaba, pois o pacote que cai no buffer da rede é pescado e processado no exato microssegundo de sua existência, fluindo do PCIe para o aplicativo em estado puro. Construiu-se pequenos, microscópicos Sistemas Operacionais polidos dentro da aplicação, controlando as filas com exatidão cirúrgica.

---

A abstração do Sistema Operacional baseada em POSIX, VFS e a API de Sockets, que universalizou e impulsionou a construção da internet moderna, metamorfoseou-se na âncora monolítica que impede a saturação matemática dos cabos ópticos na infraestrutura de Hyper-Scale. Em busca das taxas assombrosas de throughput e zero latência impostas pelas nuvens globais, a engenharia de vanguarda foi encurralada em um paradoxo silencioso: somos obrigados a dedicar anos projetando frameworks brutais para ignorar, contornar e eviscerar ativamente o próprio Sistema Operacional que inicialmente carregamos no servidor.

---

> Gostaria que eu fornecesse um *snippet* prático em C detalhando a estruturação exata das filas de submissão do protocolo NVMe no SPDK, demonstrando fisicamente como a aplicação realiza o polling em Ring Buffers e ignora as interrupções do SO?