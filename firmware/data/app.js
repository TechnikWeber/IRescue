const $ = (id) => document.getElementById(id);
const post = (url, data) => {
  const body = new URLSearchParams(data || {});
  return fetch(url, { method: "POST", body }).then((r) => r.json().catch(() => ({})));
};

let learned = [];   // codes captured in the learn step

// ---- WebSocket live status ----
function connect() {
  const ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen  = () => setConn(true);
  ws.onclose = () => { setConn(false); setTimeout(connect, 1500); };
  ws.onmessage = (ev) => {
    const m = JSON.parse(ev.data);
    if (m.event === "learned") return onLearned(m);
    render(m);
  };
}
function setConn(ok) {
  const el = $("conn");
  el.textContent = ok ? "verbunden" : "getrennt";
  el.className = "pill " + (ok ? "on" : "off");
}

function render(s) {
  const max = Math.max(s.baseline * 1.2, 4095);
  $("bar").style.width = Math.min(100, (s.sensor / max) * 100) + "%";
  $("sensorVal").textContent = s.sensor;
  $("baseVal").textContent = "Grundlinie: " + Math.round(s.baseline || 0);
  const pct = s.total ? Math.floor((s.index / s.total) * 100) : 0;
  $("prog").style.width = pct + "%";
  $("progLbl").textContent = s.running
    ? `${s.index} / ${s.total} (${pct}%)${s.paused ? " — pausiert" : ""}`
    : (s.total ? `gestoppt bei ${s.index} / ${s.total}` : "bereit");
  $("learnState").textContent = s.learning ? "läuft — Taste drücken" : "aus";
  renderHits(s.hits || []);
}

function renderHits(hits) {
  const tb = $("hits").querySelector("tbody");
  tb.innerHTML = "";
  hits.forEach((h) => {
    const tr = document.createElement("tr");
    tr.innerHTML =
      `<td>${h.index}</td><td class=mono>${h.address}</td><td class=mono>${h.command}</td><td>${h.dip}</td>`;
    const td = document.createElement("td");
    const test = btn("Senden", () => post("/api/test", { address: h.address, command: h.command }));
    const save = btn("Speichern", () => {
      const pack = $("packName").value || prompt("Pack-Name (Gerät)?", "Lampe") || "Lampe";
      const button = prompt("Tastenname?", "Power") || "Taste";
      post("/api/pack/save", { pack, button, address: h.address, command: h.command }).then(loadPacks);
    });
    td.append(test, save);
    tr.append(td);
    tb.append(tr);
  });
}

function onLearned(m) {
  learned.push(m);
  const li = document.createElement("li");
  li.innerHTML = `<span class=mono>${m.protocol} ${m.value}</span> `;
  if (m.protocol === "NEC") {
    const save = btn("→ Pack", () => {
      const pack = $("packName").value || prompt("Pack-Name (Gerät)?", "Laufband-Remote") || "Remote";
      const button = prompt("Tastenname?", "Power") || "Taste";
      post("/api/pack/save", { pack, button, address: m.address, command: m.command }).then(loadPacks);
    });
    li.append(save);
  }
  $("learned").append(li);
}

function btn(label, fn) {
  const b = document.createElement("button");
  b.textContent = label; b.onclick = fn; b.style.marginLeft = "6px";
  return b;
}

function loadPacks() {
  fetch("/api/packs").then((r) => r.json()).then((d) => {
    $("packs").innerHTML = "";
    (d.packs || []).forEach((p) => {
      const li = document.createElement("li");
      li.innerHTML = `<span class=mono>${p.name}</span> — ${p.size} B`;
      $("packs").append(li);
    });
  });
}

// ---- controls ----
$("calBtn").onclick    = () => post("/api/calibrate");
$("startBtn").onclick  = () => {
  const mode = document.querySelector("input[name=mode]:checked").value;
  post("/api/start", { mode, address: $("addr").value });
};
$("pauseBtn").onclick  = () => post("/api/pause");
$("resumeBtn").onclick = () => post("/api/resume");
$("stopBtn").onclick   = () => post("/api/stop");
$("learnBtn").onclick  = () => {
  const on = $("learnState").textContent.startsWith("läuft");
  post(on ? "/api/learn/stop" : "/api/learn/start");
};
$("sens").oninput = (e) => { $("sensLbl").textContent = Math.round(e.target.value * 100) + "%"; };
$("sens").onchange = (e) => post("/api/config", { sens: e.target.value });
$("gap").oninput  = (e) => { $("gapLbl").textContent = e.target.value + " ms"; };
$("gap").onchange = (e) => post("/api/config", { gap: e.target.value });

connect();
loadPacks();
