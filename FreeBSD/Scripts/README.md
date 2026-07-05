# ⚙️ **Automação e Setup**

## 🛠️ **Scripts Disponíveis**

Scripts desenvolvidos em **sh** (Shell padrão do FreeBSD) para automatizar a preparação do ambiente de desenvolvimento.

| Script                                   | Local de Execução | Descrição                                                                                                                                |
| ---------------------------------------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| **[`download.sh`](./kvm/download.sh)**   | 🏠 _Host_         | Baixa a ISO mais recente do FreeBSD, valida a integridade (SHA256) e extrai o arquivo `.xz`.                                             |
| **[`openzfs.sh`](./kvm/openzfs.sh)**     | 📟 _Guest (VM)_   | Configura o particionamento do disco alvo (GPT/EFI), inicializa o pool ZFS (`zroot`), cria os datasets base e prepara o bootloader UEFI. |
| **[`install.sh`](./kvm/install.sh)**     | 🏠 _Host_         | Provisiona a VM no KVM via `virt-install` (4 vCPUs, 4GB RAM, 32GB Disk, UEFI).                                                           |
| **[`connect.sh`](./kvm/connect.sh)**     | 🏠 _Host_         | Inicializa a VM, abre o console gráfico e estabelece a conexão automática via SSH.                                                       |
| **[`reboot.sh`](./kvm/reboot.sh)**       | 🏠 _Host_         | Reinicia os recursos e o daemon de gerenciamento da VM.                                                                                  |
| **[`setup.sh`](./setup.sh)**             | 📟 _Guest (VM)_   | **O coração do setup.** Automatiza o pós-instalação: drivers, GNOME, editores, shell e utilitários.                                      |
| **[`uninstall.sh`](./kvm/uninstall.sh)** | 🏠 _Host_         | Remove completamente a VM e desaloca todos os seus discos do sistema.                                                                    |

## 🚀 **Como Utilizar**

1.  Dê permissão de execução:

```sh
chmod +x *.sh
```

2.  Execute conforme a necessidade:

```sh
./downlaod.sh
./openzfs.sh
./install.sh
./connect.sh
./reboot.sh
./setup.sh
./uninstall.sh
```

---

> ⚙️ **Nota de Engenharia:** Esta suíte implementa um modelo de **Setup Determinístico**, eliminando variações de ambiente entre implantações. Ao centralizar a configuração em scripts puramente **POSIX sh**, garantimos a compatibilidade nativa com o `sh` do FreeBSD, enquanto a arquitetura do `setup.sh` transforma um sistema base _vanilla_ em uma estação de trabalho de alto desempenho com suporte total a **C23**, **Rust-core tools** e **Gnome Wayland/X11**.
