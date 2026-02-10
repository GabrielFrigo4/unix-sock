#!/bin/sh

### ################################################################################################################################

### ################################
### Setup Groups
### ################################

pw groupmod wheel -m gabriel
pw groupmod video -m gabriel

### ################################
### Setup Packages
### ################################

pkg bootstrap --yes
pkg update
pkg upgrade --yes

### ################################
### Setup Terminal
### ################################

sysrc allscreens_flags="-f spleen-16x32"

### ################################
### Setup Super User
### ################################

pkg install --yes sudo
echo '%wheel ALL=(ALL:ALL) ALL' > "/usr/local/etc/sudoers.d/wheel"
chmod 0440 "/usr/local/etc/sudoers.d/wheel"

### ################################################################################################################################

### ################################
### Setup Desktop Environment
### ################################

sudo pkg install --yes desktop-installer
desktop-installer

### ################################################################################################################################

### ################################
### Setup Shell
### ################################

cat << 'EOF' | tee -a "$HOME/.shrc" | sudo tee -a "/root/.shrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="\033[1;31m"
else
	usr_color="\033[1;32m"
fi
export PS1="
\033[0;33m\033[1;31m \033[1;35m${os_version}\033[0;33m─\033[1;34m \033[1;35m${sh_name}\033[0;33m
\033[0;33m┌──❮ \033[1;33m \033[1;36m\w\033[0;33m ❯─ ❮\033[1;34m ${usr_color}\u\033[0;33m❯
\033[0;33m└─\033[1;34m\033[0m "

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Setup Bash
### ################################

sudo pkg install --yes bash

cat << 'EOF' | tee -a "$HOME/.bashrc" | sudo tee -a "/root/.bashrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮\[\e[1;31m\]󰊢 \[\e[1;35m\]${branch}\[\e[0;33m\]❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="\[\e[1;31m\]"
else
	usr_color="\[\e[1;32m\]"
fi
update_prompt() {
	PS1="\n\[\e[0;33m\]\[\e[1;31m\] \[\e[1;35m\]${os_version}\[\e[0;33m\]─\[\e[1;34m\] \[\e[1;35m\]${sh_name}\[\e[0;33m\]"
	PS1+="\n\[\e[0;33m\]┌──❮ \[\e[1;32m\] \t\[\e[0;33m\] ❯─❮ \[\e[1;32m\] \D{%d/%m/%y}\[\e[0;33m\] ❯─❮ \[\e[1;33m\] \[\e[1;36m\]\W\[\e[0;33m\] ❯─ ❮\[\e[1;34m\] ${usr_color}\u\[\e[0;33m\]❯ $(show_git_branch)"
	PS1+="\n\[\e[0;33m\]└─\[\e[1;34m\]\[\e[0m\] "
}
PROMPT_COMMAND=update_prompt

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Setup Zsh
### ################################

sudo pkg install --yes zsh
zsh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended
sudo zsh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

cat << 'EOF' | tee -a "$HOME/.zshrc" | sudo tee -a "/root/.zshrc" > "/dev/null"
### ################################
### SHELL OPTIONS SETUP
### ################################

# History OPTIONS
setopt SHARE_HISTORY
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_REDUCE_BLANKS

# Globbing & Expansion OPTIONS
setopt EXTENDED_GLOB
setopt GLOB_DOTS
setopt PROMPT_SUBST

# Interaction OPTIONS
setopt CORRECT
setopt INTERACTIVE_COMMENTS
unsetopt BEEP

# Navigation OPTIONS
setopt AUTO_CD

### ################################
### SHELL ENVIRONMENT
### ################################

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮%B%F{red}󰊢 %F{magenta}${branch}%b%F{yellow}❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="%B%F{red}"
else
	usr_color="%B%F{green}"
fi
export PROMPT=$'
%b%F{yellow}%B%F{red} %F{magenta}${os_version}%b%F{yellow}─%B%F{blue} %F{magenta}${sh_name}%b%F{yellow}
%b%F{yellow}┌──❮ %B%F{green} %*%b%F{yellow} ❯─❮ %B%F{green} %D{%d/%m/%y}%b%F{yellow} ❯─❮ %B%F{yellow} %B%F{cyan}%c%b%F{yellow} ❯─ ❮%B%F{blue} ${usr_color}%n%b%F{yellow}❯ \$(show_git_branch)
%b%F{yellow}└─%B%F{blue}%f%b '

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################################################################################################################
