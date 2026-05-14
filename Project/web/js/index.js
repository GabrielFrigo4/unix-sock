const UI = {
  playerName: () =>
    document.getElementById("playerName").value.trim().substring(0, 32),
  roomList: document.getElementById("roomList"),
};

const generateRoomId = () =>
  Math.random().toString(36).substring(2, 8).toUpperCase();

const fetchRooms = async () => {
  const res = await fetch("/api/files");
  const files = await res.json();
  const rooms = files.filter(
    (file) => file.startsWith("sala_") && file.endsWith(".json"),
  );

  if (rooms.length === 0) {
    UI.roomList.innerHTML =
      '<div style="text-align: center; color: #94a3b8;">Nenhuma sala aberta. Crie uma!</div>';
    return;
  }

  UI.roomList.innerHTML = rooms
    .map((room) => {
      const formattedId = room.replace("sala_", "").replace(".json", "");
      return `
            <div class="room-card">
                <div class="room-info">Sala: ${formattedId}</div>
                <button class="btn-join" onclick="joinRoom('${room}')">Entrar 🎮</button>
            </div>
        `;
    })
    .join("");
};

window.criarSala = async () => {
  const player = UI.playerName();
  if (!player) return alert("Digite seu Nickname antes de criar uma sala!");

  const roomId = generateRoomId();
  const roomFile = `sala_${roomId}.json`;

  await fetch(`/api/data/${roomFile}`, {
    method: "POST",
    body: player,
  });

  sessionStorage.setItem("sala", roomFile);
  sessionStorage.setItem("jogador", player);
  sessionStorage.setItem("simbolo", "X");
  window.location.href = "sala";
};

window.joinRoom = async (roomFile) => {
  const player = UI.playerName();
  if (!player) return alert("Digite seu Nickname antes de entrar!");

  const res = await fetch(`/api/data/${roomFile}`);
  const gameState = await res.json();

  const symbol =
    player === gameState.players.X
      ? "X"
      : player === gameState.players.O || !gameState.players.O
        ? "O"
        : null;

  if (!symbol) return alert("Sala cheia!");

  sessionStorage.setItem("sala", roomFile);
  sessionStorage.setItem("jogador", player);
  sessionStorage.setItem("simbolo", symbol);
  window.location.href = "sala";
};

document.addEventListener("DOMContentLoaded", fetchRooms);
setInterval(fetchRooms, 1024);
