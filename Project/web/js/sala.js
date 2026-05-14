const Config = {
  roomFile: sessionStorage.getItem("sala"),
  playerName: sessionStorage.getItem("jogador"),
  playerSymbol: sessionStorage.getItem("simbolo"),
};

if (!Config.roomFile || !Config.playerName || !Config.playerSymbol) {
  window.location.href = "/";
}

const UI = {
  roomLabel: document.getElementById("lblSala"),
  playerLabel: document.getElementById("lblJogador"),
  symbolLabel: document.getElementById("lblSimbolo"),
  statusBanner: document.getElementById("statusTurno"),
  cells: document.querySelectorAll(".cell"),
  tempoBanner: document.getElementById("statusTempo"),
  tempoLabel: document.getElementById("lblTempo"),
};

UI.roomLabel.textContent = Config.roomFile
  .replace("sala_", "")
  .replace(".json", "");
UI.playerLabel.textContent = Config.playerName;
UI.symbolLabel.textContent = Config.playerSymbol;

const State = {
  gameData: null,
  isUpdating: false,
};

const syncWithServer = async () => {
  State.isUpdating = true;
  await fetch(`/api/data/${Config.roomFile}`, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(State.gameData),
  });
  State.isUpdating = false;
};

const renderGameOver = () => {
  const { winner, players } = State.gameData;
  if (winner === "Empate") {
    UI.statusBanner.className = "status-banner empate";
    UI.statusBanner.textContent = "Deu Velha! (Empate)";
  } else if (winner === Config.playerSymbol) {
    UI.statusBanner.className = "status-banner vitoria";
    UI.statusBanner.textContent = `Vitória! Parabéns, ${Config.playerName}! 🎉`;
  } else {
    UI.statusBanner.className = "status-banner derrota";
    const winnerName = winner === "X" ? players.X : players.O;
    UI.statusBanner.textContent = `Derrota! Vitória de ${winnerName} 😢`;
  }
};

const updateInterface = () => {
  if (!State.gameData) return;

  UI.cells.forEach((cell, index) => {
    const value = State.gameData.board[index];
    cell.textContent = value;
    cell.className = `cell ${value}`;
  });

  if (State.gameData.winner) return renderGameOver();

  const isMyTurn = State.gameData.turn === Config.playerSymbol;
  UI.statusBanner.textContent = isMyTurn
    ? "Sua vez de jogar!"
    : `Aguardando oponente (${State.gameData.turn})...`;
  UI.statusBanner.className = `status-banner ${isMyTurn ? "vez-ativa" : "vez-espera"}`;
};

const atualizarCronometro = () => {
  if (!State.gameData) return;

  if (State.gameData.winner) {
    UI.tempoBanner.style.display = "none";
    return;
  }

  const agora = Date.now();
  const tempoDecorrido = Math.floor((agora - State.gameData.createdAt) / 1000);
  const tempoRestante = 240 - tempoDecorrido;

  if (tempoRestante <= 0) {
    UI.tempoLabel.textContent = "00:00";
    return;
  }

  const min = String(Math.floor(tempoRestante / 60)).padStart(2, "0");
  const sec = String(tempoRestante % 60).padStart(2, "0");
  UI.tempoLabel.textContent = `${min}:${sec}`;

  if (tempoRestante <= 60) {
    UI.tempoBanner.classList.add("urgente");
  }
};

const fetchServerState = async () => {
  if (State.isUpdating || State.gameData?.winner) return;

  try {
    const res = await fetch(`/api/data/${Config.roomFile}`);

    if (res.status === 404) {
      if (!State.gameData?.winner) {
        alert("⏰ Tempo Esgotado! A sala foi encerrada pelo servidor.");
        window.location.href = "/";
      }
      return;
    }

    const data = await res.json();
    if (State.isUpdating) return;
    State.gameData = data;

    if (Config.playerSymbol === "O" && !State.gameData.players.O) {
      State.gameData.players.O = Config.playerName;
      await syncWithServer();
    }

    updateInterface();
    atualizarCronometro();
  } catch (e) {
    console.error("Erro ao buscar estado:", e);
  }
};

const checkWinner = () => {
  const winningCombos = [
    [0, 1, 2],
    [3, 4, 5],
    [6, 7, 8],
    [0, 3, 6],
    [1, 4, 7],
    [2, 5, 8],
    [0, 4, 8],
    [2, 4, 6],
  ];

  for (const [a, b, c] of winningCombos) {
    const board = State.gameData.board;
    if (board[a] && board[a] === board[b] && board[a] === board[c]) {
      State.gameData.winner = board[a];
      return;
    }
  }
};

window.jogar = async (index) => {
  if (
    !State.gameData ||
    State.gameData.winner ||
    State.gameData.turn !== Config.playerSymbol ||
    State.gameData.board[index]
  )
    return;

  State.gameData.board[index] = Config.playerSymbol;
  checkWinner();

  if (!State.gameData.winner) {
    const isBoardFull = !State.gameData.board.includes("");
    if (isBoardFull) State.gameData.winner = "Empate";
    else State.gameData.turn = Config.playerSymbol === "X" ? "O" : "X";
  }

  updateInterface();
  await syncWithServer();
};

window.voltarParaLobby = () => {
  window.location.href = "/";
};

setInterval(atualizarCronometro, 1000);
setInterval(fetchServerState, 512);
fetchServerState();
