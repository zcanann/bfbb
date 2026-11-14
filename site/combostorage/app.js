const DATA = "data/";
const sceneNames = {
  HB: "Bikini Bottom", JF: "Jellyfish Fields", BB: "Downtown Bikini Bottom",
  GL: "Goo Lagoon", B1: "Poseidome", RB: "Rock Bottom", BC: "Mermalair",
  SM: "Sand Mountain", B2: "Industrial Park", KF: "Kelp Forest",
  GY: "Flying Dutchman's Graveyard", DB: "SpongeBob's Dream",
  B3: "Chum Bucket Lab", PG: "Playground", MN: "Main Menu"
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
  if (/progression_award|pickup_reward_increased|runtime_verified_payoff/.test(text)) {
    const lab = /diagnostic|synthetic|artificial/.test(`${item.naturality} ${item.feasibility}`.toLowerCase());
    return {key:"payoff", label:lab ? "Lab oracle" : "Verified payoff", tone:"good"};
  }
  if (/consumer_verified|wrong_warp_consumer_verified/.test(text)) return {key:"consumer", label:"Consumer verified", tone:"warn"};
  if (/load_blocker|stalled|crash|emulator_artifact/.test(text)) return {key:"blocked", label:/crash/.test(text) ? "Crash" : "Load blocker", tone:"bad"};
  if (/no_reward|no_progression|no_advantage|negative_closure/.test(text)) return {key:"no-payoff", label:"Verified no payoff", tone:"neutral"};
  if (/visible_effect|state_corruption|controlled_confirmation/.test(text)) return {key:"effect", label:"Verified effect", tone:"warn"};
  if (/verified/.test(text)) return {key:"verified", label:"Runtime verified", tone:"neutral"};
  return {key:"candidate", label:"Candidate", tone:"neutral"};
}

function routeability(item) {
  const text = `${item.naturality || ""} ${item.feasibility || ""} ${item.classification || ""}`.toLowerCase();
  if (/consumer_only|missing_producer/.test(text)) return {
    label: "Producer missing", tone: "warn",
    explanation: "The useful end state works, but no gameplay action is known to write it."
  };
  if (/diagnostic|synthetic|artificial|layout_alignment_only/.test(text)) return {
    label: "Lab-only setup", tone: "warn",
    explanation: "The harness forced combo/HUD state or scene transport. This verifies the result, not a controller-ready route."
  };
  if (/blocked|invalid_followup|load_blocker/.test(text)) return {
    label: "Not recoverable", tone: "bad",
    explanation: "The tested sequence does not return to usable gameplay."
  };
  if (/natural_source_available|natural/.test(text)) return {
    label: "Natural source", tone: "good",
    explanation: "The source action exists in normal gameplay; timing and route practicality may still need work."
  };
  return {
    label: "Route unknown", tone: "neutral",
    explanation: "The catalog has not established a controller-ready setup for this result."
  };
}

function recipeFor(item) {
  const producer = item.producers?.[0] || {};
  const outer = producer.recipe || {};
  return outer.recipe && typeof outer.recipe === "object" ? outer.recipe : outer;
}

function isPlayerResult(item) {
  const producers = item.producers || [];
  return producers.some(producer => {
    const text = `${producer.feasibility || ""} ${producer.naturality || ""}`.toLowerCase();
    const positive = /controller_actions|controller_reproduced|natural_source_available|natural_route_verified|runner_feasible/.test(text);
    const disqualifier = /diagnostic|synthetic|artificial|missing|not_routeable|layout_only/.test(text);
    return positive && !disqualifier && Boolean(producer.action_summary);
  });
}

function playerInput(item) {
  const producer = (item.producers || []).find(producer => {
    const text = `${producer.feasibility || ""} ${producer.naturality || ""}`.toLowerCase();
    return /controller_actions|controller_reproduced|natural_source_available|natural_route_verified|runner_feasible/.test(text)
      && !/diagnostic|synthetic|artificial|missing|not_routeable|layout_only/.test(text);
  });
  return producer?.action_summary || "No controller-ready input recorded.";
}

function plainSetup(item) {
  const status = routeability(item);
  const recipe = recipeFor(item);
  const source = sceneName(item.source_scene);
  const destination = sceneName(item.destination_scene);
  const target = item.writes?.[0]?.owner_name || item.consumers?.[0]?.kind || "the affected object";
  const warmups = Number(recipe.warmups ?? recipe.source_combo_warmups ?? 0);
  const hud = recipe.hud_precondition || recipe.hud_action;
  const delay = Number(recipe.hud_precondition_delay ?? recipe.hud_delay ?? 0);
  const isDiagnostic = status.label === "Lab-only setup";

  if (/wrong_warp_consumer_verified/.test(item.result || "")) return {
    status,
    playable: "No — the wrong warp works only after the lab supplies the task value.",
    scope: "This verifies the game's response to a corrupted task counter. It does not verify a combo-storage setup that creates that counter.",
    missing: "A gameplay action that writes a positive value to Chum Bucket Lab task 1 without breaking the menu.",
    summary: "TAKE BUS enters the Robo-SpongeBob phase when Chum Bucket Lab task 1 is positive, but no natural write producer is known.",
    steps: [
      "Use the lab to set Chum Bucket Lab task 1 to a positive value.",
      "Open the spatula screen and select TAKE BUS for that task.",
      "The game loads B302, the Robo-SpongeBob phase."
    ]
  };

  if (isPlayerResult(item)) {
    const consumer = item.consumers?.[0];
    return {
      status: {label: "Player input known", tone: "good", explanation: "The catalog records a concrete controller sequence for this result."},
      playable: "Yes — the required input is made from ordinary controller actions.",
      scope: "The input and resulting corruption are both recorded. This does not imply that the effect saves time.",
      missing: /no access|no .*skip|no payoff/i.test(plainOutcome(item))
        ? "A useful consequence; the observed effect currently has no demonstrated speedrun payoff."
        : "Route timing and category legality still need separate evaluation.",
      summary: `${playerInput(item)} Result: ${plainOutcome(item)}`,
      steps: [
        playerInput(item),
        consumer?.action || `Enter ${destination} and perform the recorded interaction.`,
        consumer?.result || plainOutcome(item),
      ],
    };
  }

  if (item.transition === "title_round_trip" && item.primitive === "stale_motive_allocator") {
    const steps = [`Load ${source}.`];
    if (warmups) steps.push(isDiagnostic
      ? `The harness creates ${warmups} completed five-hit combo cycle${warmups === 1 ? "" : "s"} to prepare the recorded allocation history.`
      : `Complete ${warmups} five-hit combo cycle${warmups === 1 ? "" : "s"}.`);
    if (hud) steps.push(isDiagnostic
      ? `The harness starts the ${titleCase(hud)} HUD animation${delay ? ` and advances ${delay} game frame${delay === 1 ? "" : "s"}` : ""}.`
      : `Trigger the ${titleCase(hud)} HUD animation${delay ? `, then wait ${delay} game frame${delay === 1 ? "" : "s"}` : ""}.`);
    steps.push(isDiagnostic
      ? "The harness starts another five-hit combo reward and moves to the title screen while its HUD timer is pending, causing the delayed reward animation to allocate there."
      : "Start another five-hit combo reward, pause and quit while its HUD timer is pending, and let the delayed reward animation begin on the title screen.");
    steps.push(isDiagnostic
      ? `The harness loads ${destination}; the test then approaches ${target}.`
      : `Load ${destination}, then approach ${target}.`);
    return {
      status,
      playable: isDiagnostic
        ? "No — there is no known controller setup for this address."
        : "Yes — the source action exists in normal play, although its route may still be impractical.",
      scope: isDiagnostic
        ? "The harness directly prepared the combo and HUD allocation history, then used lab scene transport. A normal stored combo does not select this address."
        : "This row uses the ordinary pause, quit, title-screen, and file-load path.",
      missing: isDiagnostic
        ? "A natural way to make the title-screen allocator retain this exact address. Frame-perfect timing on an ordinary combo is not currently enough."
        : "A practical route and timing demonstration on controller input.",
      summary: isDiagnostic
        ? "Forced-address oracle: the harness selected this address and observed the effect. This is not a player input sequence."
        : `Carry a pending combo reward through the title screen, load ${destination}, then interact with ${target}.`,
      steps
    };
  }

  const consumer = item.consumers?.[0];
  return {
    status,
    playable: status.label === "Natural source"
      ? "Partly — the source action is available in normal gameplay, but no useful route is proved here."
      : "No complete controller setup is known for this result.",
    scope: status.explanation,
    missing: status.label === "Not recoverable"
      ? "A continuation that reaches controllable gameplay after the write."
      : "A controller-reproducible producer and a surviving transition.",
    summary: consumer?.action || "The lab delivered the recorded write, then checked the affected object or transition.",
    steps: [consumer?.action || "Reproduce the recorded source and transition timing.", consumer?.result || "Observe the recorded result."]
  };
}

function plainOutcome(item) {
  const outcome = item.outcomes?.[0] || {};
  if (outcome.category === "progression_award" && outcome.shiny_delta) {
    return `The corrupted pickup produced ${outcome.shiny_delta > 0 ? "+" : ""}${outcome.shiny_delta} shinies versus the matched clean run, and the total survived the follow-up transition.`;
  }
  if (outcome.category === "progression_award" && outcome.spatula_delta) {
    return `The run gained ${outcome.spatula_delta} spatula${Math.abs(outcome.spatula_delta) === 1 ? "" : "s"}; the collected state survived reload.`;
  }
  return outcome.description || item.summary || titleCase(item.result);
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
  const sourceScenes = Array.isArray(item.source_scenes) ? item.source_scenes : [];
  const sceneTerms = [item.source_scene, item.destination_scene, item.destination, ...sourceScenes]
    .filter(Boolean)
    .map(sceneName)
    .join(" ");
  return `${sceneTerms} ${JSON.stringify(item)}`.toLowerCase();
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
  const setup = plainSetup(item);
  const headline = plainOutcome(item);
  return `<button class="result-card" data-id="${esc(item.id)}">
    <span class="route">${esc(platformName(item.platform))}<strong>${esc(sceneName(item.source_scene))} → ${esc(sceneName(item.destination_scene))}</strong></span>
    <span><h3>${esc(headline)}</h3><p>${esc(setup.summary)}${item.reruns > 1 ? ` · reproduced in ${item.reruns} catalog runs` : ""}</p></span>
    <span class="badge-stack"><span class="badge ${status.tone}">${status.label}</span><span class="badge ${setup.status.tone}">${setup.status.label}</span></span>
  </button>`;
}

function renderFindings() {
  const items = filtered(dedupeFindings(state.findings)).filter(isPlayerResult);
  const counts = new Map();
  items.forEach(item => { const s = classify(item); counts.set(s.label, (counts.get(s.label) || 0) + 1); });
  $("#finding-summary").innerHTML = [...counts].map(([label, count]) => `<span class="summary-chip"><strong>${count}</strong> ${esc(label.toLowerCase())}</span>`).join("");
  $("#findings-list").innerHTML = items.length ? items.map(item => {
    const status = classify(item);
    return `<button class="player-result-row" data-id="${esc(item.id)}">
      <span class="route">${esc(platformName(item.platform))}<strong>${esc(sceneName(item.destination_scene))}</strong></span>
      <p>${esc(playerInput(item))}</p>
      <p>${esc(plainOutcome(item))}</p>
      <span class="badge ${status.tone}">${esc(status.label)}</span>
    </button>`;
  }).join("") : `<div class="empty">No controller-complete results match these filters. Lab-only address tests are listed under Research leads.</div>`;
  $$("#findings-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
}

function renderLeads() {
  const items = filtered(dedupeFindings(state.findings)).filter(item => !isPlayerResult(item));
  const counts = new Map();
  items.forEach(item => { const s = classify(item); counts.set(s.label, (counts.get(s.label) || 0) + 1); });
  $("#lead-summary").innerHTML = [...counts].map(([label, count]) => `<span class="summary-chip"><strong>${count}</strong> ${esc(label.toLowerCase())}</span>`).join("");
  $("#leads-list").innerHTML = items.length ? items.map(renderFindingCard).join("") : `<div class="empty">No promoted research leads match these filters.</div>`;
  $$("#leads-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
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
  const setup = plainSetup(item);
  const outcomes = item.outcomes || [];
  const consumers = item.consumers || [];
  const evidence = item.evidence_refs || item.evidence || (item.evidence_path ? [item.evidence_path] : []);
  $("#detail-content").innerHTML = `<div class="detail-header">
    <span class="badge ${status.tone}">${status.label}</span> <span class="badge ${setup.status.tone}">${setup.status.label}</span>
    <h2>${esc(sceneName(item.source_scene))} → ${esc(sceneName(item.destination_scene))}</h2>
    <p>${esc(setup.summary)}</p>
  </div>
  <section class="playability-panel">
    <h3>Can a player do this now?</h3>
    <p><strong>${esc(setup.playable)}</strong></p>
    <p>${esc(setup.scope)}</p>
    <p><span>Still missing:</span> ${esc(setup.missing)}</p>
  </section>
  <section class="detail-section action-section"><h3>Exact test that produced this result</h3><ol>${setup.steps.map(step => `<li>${esc(step)}</li>`).join("")}</ol></section>
  <section class="detail-section"><h3>What happened</h3><p>${esc(plainOutcome(item))}</p>${outcomes.slice(1).map(o => `<p>${esc(o.description || titleCase(o.category))}</p>`).join("")}</section>
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
      <span>${esc(sceneName(item.source_scene))} → ${esc(sceneName(item.destination_scene))}</span>
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
  $("#queue-list").innerHTML = items.length ? items.map(item => `<button class="compact-row" data-id="${esc(item.write_id)}"><strong>#${item.rank}</strong><span>${esc(sceneName(item.source_scene))} → ${esc(sceneName(item.destination_scene))}</span><span>${esc(item.owner_name || "Unknown target")}</span><span class="technical">${esc(item.suggested_action || item.summary)}</span><span class="badge warn">${esc(titleCase(item.status))}</span></button>`).join("") : `<div class="empty">No queued tests match these filters.</div>`;
  $$("#queue-list [data-id]").forEach(el => el.addEventListener("click", () => openDetail(findById(el.dataset.id))));
}

async function switchView(name, push = true) {
  state.view = name;
  $$(".tab").forEach(x => x.classList.toggle("is-active", x.dataset.view === name));
  $$(".view").forEach(x => x.classList.toggle("is-active", x.id === `view-${name}`));
  if (name === "browse") { await ensureResults(); renderBrowse(); }
  if (name === "leads") renderLeads();
  if (name === "queue") { await ensureQueue(); renderQueue(); }
  if (name === "coverage") renderCoverage();
  if (push) setParams(true);
}

function renderCurrent() {
  renderFindings();
  renderLeads();
  if (state.view === "browse") renderBrowse();
  if (state.view === "coverage") renderCoverage();
  if (state.view === "queue") renderQueue();
}

async function boot() {
  const params = new URLSearchParams(location.search);
  state.view = ["findings","leads","browse","coverage","queue","about"].includes(params.get("view")) ? params.get("view") : "findings";
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
  renderLeads();
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
