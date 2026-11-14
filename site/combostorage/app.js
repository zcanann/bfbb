const DATA = "data/";
const sceneNames = {
  HB: "Bikini Bottom", JF: "Jellyfish Fields", BB: "Downtown Bikini Bottom",
  GL: "Goo Lagoon", BC: "Mermalair", RB: "Rock Bottom", SM: "Sand Mountain",
  KF: "Kelp Forest", GY: "Flying Dutchman's Graveyard", DB: "SpongeBob's Dream",
  B1: "Robo-Sandy", B2: "Robo-Patrick", B3: "Robo-SpongeBob", MNU: "Main menu"
};

const state = {
  view: "findings", platform: "all", query: "", selected: null,
  manifest: null, findings: [], coverage: [], results: null, queue: null, automation: null
};

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];
const esc = (value) => String(value ?? "").replace(/[&<>'"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;","'":"&#39;",'"':"&quot;"})[c]);
const titleCase = (value) => String(value ?? "").replaceAll("_", " ").replace(/\b\w/g, c => c.toUpperCase());
const platformName = (value) => ({gamecube:"GameCube", ps2:"PS2", xbox:"Xbox"})[value] || value || "Unknown";
const sceneName = (code) => {
  if (!code) return "Any source";
  const prefix = Object.keys(sceneNames).find(key => code.startsWith(key));
  return prefix ? `${sceneNames[prefix]} (${code})` : code;
};

function classify(item) {
  const categories = (item.outcomes || []).map(x => x.category);
  const text = [item.classification, item.result, item.feasibility, ...categories].join(" ").toLowerCase();
  if (/progression_award|pickup_reward_increased|runtime_verified_payoff/.test(text)) return {key:"payoff", label:"Verified payoff", tone:"good"};
  if (/consumer_verified|wrong_warp_consumer_verified/.test(text)) return {key:"consumer", label:"Consumer verified", tone:"warn"};
  if (/load_blocker|stalled|crash|emulator_artifact/.test(text)) return {key:"blocked", label:/crash/.test(text) ? "Crash" : "Load blocker", tone:"bad"};
  if (/no_reward|no_progression|no_advantage|negative_closure/.test(text)) return {key:"no-payoff", label:"Verified no payoff", tone:"neutral"};
  if (/visible_effect|state_corruption|controlled_confirmation/.test(text)) return {key:"effect", label:"Verified effect", tone:"warn"};
  if (/verified/.test(text)) return {key:"verified", label:"Runtime verified", tone:"neutral"};
  return {key:"candidate", label:"Candidate", tone:"neutral"};
}

function outcomeKey(item) {
  const outcome = item.outcomes?.[0] || {};
  const write = item.writes?.[0] || {};
  return [item.platform, item.source_scene, item.destination_scene, item.transition, write.owner_name, write.member, outcome.category, outcome.progression_delta, outcome.shiny_delta, outcome.spatula_delta].join("|");
}

function dedupeFindings(items) {
  const map = new Map();
  for (const item of items) {
    const key = outcomeKey(item);
    if (!map.has(key)) map.set(key, {...item, reruns: 1});
    else {
      const current = map.get(key);
      current.reruns += 1;
      current.evidence_refs = [...new Set([...(current.evidence_refs || []), ...(item.evidence_refs || [])])];
    }
  }
  return [...map.values()];
}

function searchable(item) {
  return JSON.stringify(item).toLowerCase();
}

function filtered(items) {
  const q = state.query.trim().toLowerCase();
  return items.filter(item => (state.platform === "all" || item.platform === state.platform) && (!q || searchable(item).includes(q)));
}

function setParams(push = false) {
  const params = new URLSearchParams();
  if (state.view !== "findings") params.set("view", state.view);
  if (state.platform !== "all") params.set("platform", state.platform);
  if (state.query) params.set("q", state.query);
  if (state.selected) params.set("result", state.selected);
  const url = `${location.pathname}${params.size ? `?${params}` : ""}`;
  history[push ? "pushState" : "replaceState"]({}, "", url);
}

function renderStats() {
  const counts = state.manifest.stats.counts;
  const platforms = state.manifest.stats.platforms;
  $("#catalog-stats").innerHTML = [
    [counts.experiments.toLocaleString(), "experiments"],
    [counts.outcomes.toLocaleString(), "outcomes"],
    [counts.evidence_files.toLocaleString(), "evidence files"],
    [platforms.length, "versions"]
  ].map(([n, label]) => `<div class="stat"><strong>${n}</strong><span>${label}</span></div>`).join("");
  $("#generated-at").textContent = `Updated ${new Date(state.manifest.generated_at).toLocaleString()}`;
}

function renderFindingCard(item) {
  const status = classify(item);
  const outcome = item.outcomes?.[0] || {};
  const headline = outcome.description || item.summary || titleCase(item.result);
  return `<button class="result-card" data-id="${esc(item.id)}">
    <span class="route">${esc(platformName(item.platform))}<strong>${esc(item.source_scene || "—")} → ${esc(item.destination_scene || "—")}</strong></span>
    <span><h3>${esc(headline)}</h3><p>${esc(item.summary || titleCase(item.classification))}${item.reruns > 1 ? ` · reproduced in ${item.reruns} catalog runs` : ""}</p></span>
    <span class="badge ${status.tone}">${status.label}</span>
  </button>`;
}

function renderFindings() {
  const items = filtered(dedupeFindings(state.findings));
  const counts = new Map();
  items.forEach(item => { const s = classify(item); counts.set(s.label, (counts.get(s.label) || 0) + 1); });
  $("#finding-summary").innerHTML = [...counts].map(([label, count]) => `<span class="summary-chip"><strong>${count}</strong> ${esc(label.toLowerCase())}</span>`).join("");
  $("#findings-list").innerHTML = items.length ? items.map(renderFindingCard).join("") : `<div class="empty">No known results match these filters.</div>`;
  $$("#findings-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
}

function findById(id) {
  return state.findings.find(x => x.id === id) || state.results?.find(x => x.experiment_id === id) || state.queue?.actionable?.find(x => x.write_id === id);
}

function detailRows(item) {
  const write = item.writes?.[0] || item.write_raw || item;
  const producer = item.producers?.[0] || {};
  return [
    ["Version", platformName(item.platform)], ["Source", sceneName(item.source_scene)],
    ["Destination", sceneName(item.destination_scene)], ["Transition", titleCase(item.transition)],
    ["Primitive", titleCase(item.primitive)], ["Feasibility", titleCase(item.feasibility)],
    ["Producer", producer.action_summary || producer.recipe || "Not recorded"],
    ["Retained address", producer.holder_hex || item.holder_hex || write.holder_hex || "Not recorded"],
    ["Destination owner", write.owner_name || item.owner_name || "Not identified"],
    ["C++ member", write.member || item.member || "Not narrowed to one member"],
    ["Continuation", write.continuation_class || item.continuation_class || "Not recorded"]
  ];
}

function openDetail(item) {
  if (!item) return;
  state.selected = item.id || item.experiment_id || item.write_id;
  setParams();
  const status = classify(item);
  const outcomes = item.outcomes || [];
  const consumers = item.consumers || [];
  const evidence = item.evidence_refs || item.evidence || (item.evidence_path ? [item.evidence_path] : []);
  $("#detail-content").innerHTML = `<div class="detail-header">
    <span class="badge ${status.tone}">${status.label}</span>
    <h2>${esc(item.source_scene || "—")} → ${esc(item.destination_scene || "—")}</h2>
    <p>${esc(item.summary || item.outcome_description || titleCase(item.result || item.classification))}</p>
  </div>
  <section class="detail-section"><h3>What happened</h3>${outcomes.length ? outcomes.map(o => `<p>${esc(o.description || titleCase(o.category))}</p>`).join("") : `<p>${esc(item.outcome_description || "No runtime outcome has been recorded yet.")}</p>`}</section>
  ${consumers.length ? `<section class="detail-section"><h3>How the result is consumed</h3>${consumers.map(c => `<p><strong>${esc(c.action)}</strong><br>${esc(c.result)}</p>`).join("")}</section>` : ""}
  <section class="detail-section"><h3>Technical record</h3><dl class="detail-grid">${detailRows(item).map(([k,v]) => `<dt>${esc(k)}</dt><dd>${esc(v)}</dd>`).join("")}</dl></section>
  <section class="detail-section"><h3>Evidence</h3>${evidence.length ? `<ul class="evidence-list">${evidence.map(e => `<li>${esc(e)}</li>`).join("")}</ul>` : `<p>No evidence path exported.</p>`}</section>`;
  $("#detail-dialog").showModal();
}

async function ensureResults() {
  if (state.results) return;
  $("#browse-loading").textContent = "Loading the full result index…";
  const index = await fetch(`${DATA}results-index.json`).then(r => r.json());
  const chunks = await Promise.all(index.chunks.map(chunk => fetch(`${DATA}${chunk.file}`).then(r => r.json())));
  state.results = chunks.flat();
  $("#browse-loading").remove();
}

function genericStatus(item) {
  const text = [item.classification, item.outcome_category, item.result].join(" ").toLowerCase();
  if (/progression|award|increased/.test(text) && !/no_|candidate/.test(text)) return {label:"Observed payoff", tone:"good"};
  if (/crash|stalled|blocker/.test(text)) return {label:"Blocked", tone:"bad"};
  if (/no_reward|no_advantage|no_progression/.test(text)) return {label:"No payoff", tone:"neutral"};
  if (/planned|candidate|layout/.test(text)) return {label:"Candidate", tone:"warn"};
  return {label:"Observed", tone:"neutral"};
}

function renderBrowse() {
  if (!state.results) return;
  const items = filtered(state.results).slice(0, 300);
  $("#browse-list").innerHTML = items.length ? items.map(item => {
    const s = genericStatus(item);
    return `<button class="compact-row" data-id="${esc(item.experiment_id)}">
      <strong>${esc(platformName(item.platform))}</strong>
      <span>${esc(item.source_scene || "—")} → ${esc(item.destination_scene || "—")}</span>
      <span>${esc(item.owner_name || "Unidentified destination")}</span>
      <span class="technical">${esc(item.member || item.outcome_description || item.result)}</span>
      <span class="badge ${s.tone}">${s.label}</span>
    </button>`;
  }).join("") : `<div class="empty">No catalog rows match these filters.</div>`;
  $$("#browse-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
}

function renderCoverage() {
  const q = state.query.toLowerCase();
  const grouped = new Map();
  for (const row of state.coverage) {
    if (state.platform !== "all" && row.platform !== state.platform) continue;
    if (q && !searchable(row).includes(q)) continue;
    const key = `${row.platform}|${row.destination_scene || "unknown"}`;
    const value = grouped.get(key) || {platform:row.platform, scene:row.destination_scene || "Unknown", experiments:0, holders:0, failures:0, routes:0};
    value.experiments += Number(row.experiments || 0); value.holders += Number(row.holders || 0); value.failures += Number(row.failures || 0); value.routes += 1;
    grouped.set(key, value);
  }
  const rows = [...grouped.values()].sort((a,b) => b.experiments - a.experiments);
  $("#coverage-list").innerHTML = rows.length ? rows.map(x => `<article class="coverage-card"><p class="eyebrow">${esc(platformName(x.platform))}</p><h3>${esc(sceneName(x.scene))}</h3><dl><dt>Experiments</dt><dd>${x.experiments}</dd><dt>Distinct holders</dt><dd>${x.holders}</dd><dt>Source routes</dt><dd>${x.routes}</dd><dt>Recorded failures</dt><dd>${x.failures}</dd></dl></article>`).join("") : `<div class="empty">No coverage rows match these filters.</div>`;
}

async function ensureQueue() {
  if (state.queue) return;
  [state.queue, state.automation] = await Promise.all([
    fetch(`${DATA}experiment-queue-lite.json`).then(r => r.json()),
    fetch(`${DATA}automation-status.json`).then(r => r.json())
  ]);
}

function renderQueue() {
  if (!state.queue) return;
  const sum = state.queue.summary;
  $("#queue-summary").innerHTML = [[sum.actionable,"actionable"],[sum.verified,"verified"],[sum.closed,"closed"],[sum.backlog,"backlog"]].map(([n,l]) => `<span class="summary-chip"><strong>${Number(n).toLocaleString()}</strong> ${l}</span>`).join("");
  const auto = state.automation || {};
  const before = auto.before?.runnable_gamecube;
  const after = auto.after?.runnable_gamecube;
  $("#automation-status").innerHTML = auto.status === "not_run"
    ? "No automated catalog cycle has been recorded yet."
    : `<strong>Last automation cycle: ${esc(titleCase(auto.status))}</strong>${auto.finished_at ? ` · ${esc(new Date(auto.finished_at).toLocaleString())}` : ""}${Number.isFinite(before) && Number.isFinite(after) ? ` · runnable GameCube recipes ${before} → ${after}` : ""}`;
  const items = filtered(state.queue.actionable || []).slice(0, 200);
  $("#queue-list").innerHTML = items.length ? items.map(item => `<button class="compact-row" data-id="${esc(item.write_id)}"><strong>#${item.rank}</strong><span>${esc(item.source_scene || "—")} → ${esc(item.destination_scene || "—")}</span><span>${esc(item.owner_name || "Unknown target")}</span><span class="technical">${esc(item.suggested_action || item.summary)}</span><span class="badge warn">${esc(titleCase(item.status))}</span></button>`).join("") : `<div class="empty">No queued tests match these filters.</div>`;
  $$("#queue-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
}

async function switchView(name, push = true) {
  state.view = name;
  $$(".tab").forEach(x => x.classList.toggle("is-active", x.dataset.view === name));
  $$(".view").forEach(x => x.classList.toggle("is-active", x.id === `view-${name}`));
  if (name === "browse") { await ensureResults(); renderBrowse(); }
  if (name === "queue") { await ensureQueue(); renderQueue(); }
  if (name === "coverage") renderCoverage();
  if (push) setParams(true);
}

function renderCurrent() {
  renderFindings();
  if (state.view === "browse") renderBrowse();
  if (state.view === "coverage") renderCoverage();
  if (state.view === "queue") renderQueue();
}

async function boot() {
  const params = new URLSearchParams(location.search);
  state.view = ["findings","browse","coverage","queue","about"].includes(params.get("view")) ? params.get("view") : "findings";
  state.platform = ["gamecube","ps2","xbox"].includes(params.get("platform")) ? params.get("platform") : "all";
  state.query = params.get("q") || "";
  state.selected = params.get("result");
  [state.manifest, state.findings, state.coverage] = await Promise.all([
    fetch(`${DATA}manifest.json`).then(r => r.json()),
    fetch(`${DATA}findings.json`).then(r => r.json()),
    fetch(`${DATA}coverage.json`).then(r => r.json())
  ]);
  renderStats();
  $("#search-input").value = state.query;
  $$("#platform-filter button").forEach(x => x.classList.toggle("is-active", x.dataset.platform === state.platform));
  renderFindings();
  await switchView(state.view, false);
  if (state.selected) {
    if (!findById(state.selected)) await ensureResults();
    if (!findById(state.selected)) await ensureQueue();
    openDetail(findById(state.selected));
  }
}

$$(".tab").forEach(x => x.addEventListener("click", () => switchView(x.dataset.view)));
$$("#platform-filter button").forEach(x => x.addEventListener("click", () => {
  state.platform = x.dataset.platform;
  $$("#platform-filter button").forEach(y => y.classList.toggle("is-active", y === x));
  renderCurrent(); setParams();
}));
let searchTimer;
$("#search-input").addEventListener("input", event => {
  clearTimeout(searchTimer);
  searchTimer = setTimeout(() => { state.query = event.target.value; renderCurrent(); setParams(); }, 120);
});
$("#copy-view").addEventListener("click", async () => {
  setParams(); await navigator.clipboard.writeText(location.href);
  $("#copy-view").textContent = "Link copied";
  setTimeout(() => $("#copy-view").textContent = "Copy this view", 1400);
});
$("#close-detail").addEventListener("click", () => $("#detail-dialog").close());
$("#detail-dialog").addEventListener("close", () => { state.selected = null; setParams(); });
window.addEventListener("popstate", () => location.reload());

boot().catch(error => {
  console.error(error);
  document.querySelector("main").innerHTML = `<div class="notice">The catalog data could not be loaded. ${esc(error.message)}</div>`;
});
