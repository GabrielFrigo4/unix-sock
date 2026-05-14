# ⚙️ **Automação e Setup**

## 🛠️ **Scripts Disponíveis**

Scripts desenvolvidos em **sh** (Shell padrão do FreeBSD) para automatizar a preparação do ambiente de desenvolvimento.

| Script                               | Local de Execução | Descrição                                                                                      |
| ------------------------------------ | ----------------- | ---------------------------------------------------------------------------------------------- |
| **[`download.sh`](./download.sh)**   | 🏠 _Host_         | Baixa a ISO mais recente do FreeBSD 15, verifica o Checksum (SHA256) e extrai o arquivo `.xz`. |
| **[`install.sh`](./install.sh)**     | 🏠 _Host_         | Cria a VM no KVM via `virt-install` (4 vCPUs, 8GB RAM, 32GB Disk, UEFI).                       |
| **[`connect.sh`](./connect.sh)**     | 🏠 _Host_         | Inicia a VM, abre o console gráfico e tenta conexão automática via SSH.                        |
| **[`setup.sh`](./setup.sh)**         | 📟 _Guest (VM)_   | **O coração do setup.** Configura drivers, Desktop (GNOME), Editores, Shell e Ferramentas.     |
| **[`uninstall.sh`](./uninstall.sh)** | 🏠 _Host_         | Remove completamente a VM e seus discos do sistema.                                            |

## 🚀 **Como Utilizar**

1.  Dê permissão de execução:

```sh
chmod +x *.sh
```

2.  Execute conforme a necessidade:

```sh
./downlaod.sh
./install.sh
./connect.sh
./setup.sh
./uninstall.sh
```

---

> ⚙️ **Nota de Engenharia:** Esta suíte implementa um modelo de **Setup Determinístico**, eliminando variações de ambiente entre implantações. Ao centralizar a configuração em scripts puramente **POSIX sh**, garantimos a compatibilidade nativa com o `sh` do FreeBSD, enquanto a arquitetura do `setup.sh` transforma um sistema base _vanilla_ em uma estação de trabalho de alto desempenho com suporte total a **C23**, **Rust-core tools** e **Gnome Wayland/X11**.
