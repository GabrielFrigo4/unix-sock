# ⚙️ **Automação e Setup**

## 🛠️ **Scripts Disponíveis**
 Scripts desenvolvidos em **sh** (Shell padrão do FreeBSD) para automatizar a preparação do ambiente de desenvolvimento.

| Script | Local de Execução | Descrição |
| --- | --- | --- |
| **[`download.sh`](https://www.google.com/search?q=./download.sh)** | Host | Baixa a ISO mais recente do FreeBSD 15, verifica o Checksum (SHA256) e extrai o arquivo `.xz`. |
| **[`install.sh`](https://www.google.com/search?q=./install.sh)** | Host | Cria a VM no KVM via `virt-install` (4 vCPUs, 8GB RAM, 32GB Disk, UEFI). |
| **[`connect.sh`](https://www.google.com/search?q=./connect.sh)** | Host | Inicia a VM, abre o console gráfico e tenta conexão automática via SSH. |
| **[`setup.sh`](https://www.google.com/search?q=./setup.sh)** | Guest (VM) | **O coração do setup.** Configura drivers, Desktop (GNOME), Editores, Shell e Ferramentas. |
| **[`uninstall.sh`](https://www.google.com/search?q=./uninstall.sh)** | Host | Remove completamente a VM e seus discos do sistema. |


## 🚀 **Como Utilizar**
 1. Dê permissão de execução:
 ```sh
 chmod +x *.sh
 ```

 2. Execute conforme a necessidade:
 ```sh
 ./downlaod.sh
 ./install.sh
 ./connect.sh
 ./setup.sh
 ./uninstall.sh
 ```
