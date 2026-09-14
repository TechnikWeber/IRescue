const $ = (id) => document.getElementById(id);
const post = (url, data) => {
  const body = new URLSearchParams(data || {});
  return fetch(url, { method: "POST", body }).then((r) => r.json().catch(() => ({})));
};

let learned = [];
let cands = [];     // loaded candidate list for the stepper
let stepIdx = 0;

// ---- WebSocket live status ----
function connect() {
  const ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen  = () => setConn(true);
  ws.onclose = () => { setConn(false); setTimeout(connect, 1500); };
  ws.onmessage = (ev) => {
    const m = JSON.parse(ev.data);
    if (m.event === "log") return addLog(m.line);
    if (m.event === "learned") return onLearned(m);
    render(m);
  };
}
function setConn(ok) {
  const el = $("conn");
  el.textContent = ok ? "verbunden" : "getrennt";
  el.className = "pill " + (ok ? "on" : "off");
}

function addLog(line) {
  const el = $("log");
  const div = document.createElement("div");
  div.textContent = line;
  if (line.includes("TREFFER")) div.className = "hit";
  el.appendChild(div);
  while (el.childNodes.length > 200) el.removeChild(el.firstChild);
  el.scrollTop = el.scrollHeight;
}

function render(s) {
  const max = Math.max(s.baseline * 1.2, 4095);
  $("bar").style.width = Math.min(100, (s.sensor / max) * 100) + "%";
  $("sensorVal").textContent = s.sensor;
  $("baseVal").textContent = "Grundlinie: " + Math.round(s.baseline || 0);

  const pct = s.total ? Math.floor((s.index / s.total) * 100) : 0;
  $("prog").style.width = pct + "%";
  $("progLbl").textContent = s.running
    ? `${s.index} / ${s.total} (${pct}%)${s.paused ? " — pausiert" : ""}${s.feedback ? "" : " · blind"}`
    : (s.total ? `gestoppt bei ${s.index} / ${s.total}` : "bereit");

  if (s.cur) {
    $("nowCode").textContent = s.cur.proto === "NEC"
      ? `NEC  addr ${s.cur.addr}  cmd ${s.cur.cmd}`
      : `${s.cur.proto}  ${s.cur.value}/${s.cur.bits}b`;
    $("nowName").textContent = s.cur.name || "";
  }
  $("learnState").textContent = s.learning ? "läuft — Taste drücken" : "aus";
  renderHits(s.hits || []);
}

function renderHits(hits) {
  const tb = $("hits").querySelector("tbody");
  tb.innerHTML = "";
  hits.forEach((h) => {
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${h.index}</td><td class=mono>${h.address}</td><td class=mono>${h.command}</td><td>${h.dip}</td>`;
    const td = document.createElement("td");
    td.append(
      btn("Senden", () => post("/api/send", { address: h.address, command: h.command })),
      btn("Speichern", () => savePack(h.address, h.command))
    );
    tr.append(td);
    tb.append(tr);
  });
}

function savePack(address, command) {
  const pack = $("packName").value || prompt("Pack-Name (Gerät)?", "Lampe") || "Lampe";
  const button = prompt("Tastenname?", "Power") || "Taste";
  post("/api/pack/save", { pack, button, address, command }).then(loadPacks);
}

function onLearned(m) {
  learned.push(m);
  const li = document.createElement("li");
  li.innerHTML = `<span class=mono>${m.protocol} ${m.value}</span> `;
  if (m.protocol === "NEC") li.append(btn("→ Pack", () => savePack(m.address, m.command)));
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

// ---- candidate list (manual stepping) ----
function loadCands() {
  fetch("/api/candidates?limit=1000").then((r) => r.json()).then((d) => {
    cands = d.items || [];
    stepIdx = 0;
    renderCandList();
    showStep();
  });
}
function renderCandList() {
  const el = $("candlist");
  el.innerHTML = "";
  cands.forEach((c, i) => {
    const row = document.createElement("div");
    row.className = "candrow";
    const label = c.address !== undefined
      ? `${c.proto} ${c.address}/${c.command}` : `${c.proto} ${c.value}`;
    row.innerHTML = `<span class=mono>${label}</span> <span class=cn>${c.name || ""}</span>`;
    row.append(btn("Senden", () => { stepIdx = i; showStep(); sendCand(c); }));
    el.append(row);
  });
}
function showStep() {
  if (!cands.length) { $("stepInfo").textContent = "–"; return; }
  const c = cands[stepIdx];
  const label = c.address !== undefined ? `${c.proto} ${c.address}/${c.command}` : `${c.proto} ${c.value}`;
  $("stepInfo").textContent = `${stepIdx + 1}/${cands.length}  ${label} ${c.name || ""}`;
}
function sendCand(c) {
  if (c.address !== undefined) post("/api/send", { address: c.address, command: c.command });
  else post("/api/send", { protocol: c.proto, value: c.value, bits: c.bits });
}

// ---- controls ----
$("calBtn").onclick    = () => post("/api/calibrate");
$("startBtn").onclick  = () => post("/api/start", {
  mode: document.querySelector("input[name=mode]:checked").value,
  address: $("addr").value,
  feedback: $("feedback").checked ? 1 : 0,
});
$("pauseBtn").onclick  = () => post("/api/pause");
$("resumeBtn").onclick = () => post("/api/resume");
$("stopBtn").onclick   = () => post("/api/stop");

$("mSendNec").onclick  = () => post("/api/send", { address: $("mAddr").value, command: $("mCmd").value });
$("mSendRaw").onclick  = () => post("/api/send", { protocol: $("mProto").value, value: $("mVal").value, bits: $("mBits").value });

$("listLoad").onclick  = loadCands;
$("stepPrev").onclick  = () => { if (stepIdx > 0) { stepIdx--; showStep(); } };
$("stepNext").onclick  = () => { if (stepIdx < cands.length - 1) { stepIdx++; showStep(); } };
$("stepSend").onclick  = () => { if (cands[stepIdx]) sendCand(cands[stepIdx]); };

$("learnBtn").onclick  = () => {
  const on = $("learnState").textContent.startsWith("läuft");
  post(on ? "/api/learn/stop" : "/api/learn/start");
};
$("sens").oninput  = (e) => { $("sensLbl").textContent = Math.round(e.target.value * 100) + "%"; };
$("sens").onchange = (e) => post("/api/config", { sens: e.target.value });
$("gap").oninput   = (e) => { $("gapLbl").textContent = e.target.value + " ms"; };
$("gap").onchange  = (e) => post("/api/config", { gap: e.target.value });

connect();
loadPacks();
