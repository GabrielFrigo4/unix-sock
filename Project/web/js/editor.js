const UI = {
    fileList: document.getElementById('fileList'),
    editor: document.getElementById('meuEditor'),
    currentFileSpan: document.getElementById('currentFile'),
    btnSave: document.getElementById('btnSalvar'),
    btnDelete: document.getElementById('btnExcluir'),
    btnNew: document.getElementById('btnNovo'),
    inputNew: document.getElementById('newFileName'),
    statusMsg: document.getElementById('statusMensagem')
};

const State = {
    currentFile: null
};

const refreshFileList = async () => {
    const res = await fetch('/api/files');
    const files = await res.json();

    if (files.length === 0) {
        UI.fileList.innerHTML = '<li>Nenhum arquivo encontrado</li>';
        return;
    }

    UI.fileList.innerHTML = files.map(file => {
        const activeClass = file === State.currentFile ? 'class="ativo"' : '';
        return `<li ${activeClass} onclick="openFile('${file}')">${file}</li>`;
    }).join('');
};

const showStatus = (message, type) => {
    UI.statusMsg.textContent = message;
    UI.statusMsg.className = `status-visivel ${type}`;
    setTimeout(() => { UI.statusMsg.className = 'status-oculto'; }, 3000);
};

window.openFile = async (fileName) => {
    State.currentFile = fileName;
    UI.currentFileSpan.textContent = fileName;
    UI.editor.disabled = false;
    UI.btnSave.disabled = false;
    UI.btnDelete.disabled = false;
    UI.editor.placeholder = "Carregando...";

    const res = await fetch(`/api/data/${fileName}`);
    const text = res.ok ? await res.text() : "";
    
    UI.editor.value = text;
    UI.editor.placeholder = "Digite aqui...";
    refreshFileList();
};

const saveCurrentFile = async () => {
    if (!State.currentFile || UI.btnSave.disabled) return;

    UI.btnSave.textContent = 'Salvando...';
    UI.btnSave.disabled = true;

    const res = await fetch(`/api/data/${State.currentFile}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'text/plain' },
        body: UI.editor.value
    });

    const success = res.ok;
    showStatus(success ? "Salvo com sucesso!" : "Erro ao salvar.", success ? "sucesso" : "erro");
    
    UI.btnSave.textContent = 'Salvar Arquivo';
    UI.btnSave.disabled = false;
};

UI.btnSave.onclick = saveCurrentFile;

UI.btnNew.onclick = async () => {
    const rawName = UI.inputNew.value.trim();
    if (!rawName) return;
    
    const fileName = rawName.includes('.') ? rawName : `${rawName}.txt`;

    await fetch(`/api/data/${fileName}`, { method: 'POST', body: "" });
    UI.inputNew.value = '';
    window.openFile(fileName);
};

UI.btnDelete.onclick = async () => {
    if (!State.currentFile || !confirm(`Tem certeza que deseja excluir ${State.currentFile}?`)) return;

    UI.btnDelete.disabled = true;
    const res = await fetch(`/api/data/${State.currentFile}`, { method: 'DELETE' });

    if (res.ok) {
        State.currentFile = null;
        UI.currentFileSpan.textContent = "Nenhum arquivo selecionado";
        UI.editor.value = "";
        UI.editor.disabled = true;
        UI.btnSave.disabled = true;
        UI.btnDelete.disabled = true;
        showStatus("Excluído com sucesso!", "sucesso");
        refreshFileList();
    } else {
        showStatus("Erro ao excluir.", "erro");
        UI.btnDelete.disabled = false;
    }
};

window.addEventListener('keydown', (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key === 's') {
        event.preventDefault();
        saveCurrentFile();
    }
});

window.onload = refreshFileList;
setInterval(refreshFileList, 2048);
