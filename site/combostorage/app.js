const DATA = "data/";
const PAGE_SIZE = 150;
const sceneNames = {
  HB: "Bikini Bottom", JF: "Jellyfish Fields", BB: "Downtown Bikini Bottom",
  GL: "Goo Lagoon", B1: "Poseidome", RB: "Rock Bottom", BC: "Mermalair",
  SM: "Sand Mountain", B2: "Industrial Park", KF: "Kelp Forest",
  GY: "Flying Dutchman's Graveyard", DB: "SpongeBob's Dream",
  B3: "Chum Bucket Lab", PG: "Playground", MN: "Main Menu"
};

const state = {
  rows: [], confidence: "player", platform: "all", source: "all",
  destination: "all", outcome: "all", query: "", shown: PAGE_SIZE
};

const $ = selector => document.querySelector(selector);
const esc = value => String(value ?? "").replace(/[&<>'"]/g, character => ({
  "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", '"': "&quot;"
})[character]);
const words = value => String(value ?? "").replaceAll("_", " ").replace(/\b\w/g, letter => letter.toUpperCase());
const platformName = value => ({ gamecube: "GameCube", ps2: "PS2", xbox: "Xbox" })[value] || value || "Unknown";
const sceneName = code => {
  if (!code) return "Unknown source";
  const prefix = Object.keys(sceneNames).find(key => code.startsWith(key));
  return prefix ? `${sceneNames[prefix]} (${code})` : code;
};

function itemText(item) {
  return `${item.classification || ""} ${item.feasibility || ""} ${item.naturality || ""} ${item.result || ""} ${item.outcome_category || ""}`.toLowerCase();
}

function confidence(item) {
  const text = itemText(item);
  if (/natural_end_to_end_verified|player_replay_verified|community_observed|community_reported|reported_controller/.test(text)
      && !/diagnostic|synthetic|artificial|not_routeable|layout_only/.test(text)) return "player";
  if (/planned_candidate|layout_candidate|unproduced_candidate|source_address_recon_only/.test(text)) return "candidate";
  if (/runtime|verified|observed|controlled_confirmation|stable|stalled|crash|fault/.test(text)) return "lab";
  return "candidate";
}

function outcome(item) {
  const text = itemText(item);
  if (/progression_award|pickup_reward_increased|saved_progression_wrong_warp/.test(text)) return "progression";
  if (/crash|stall|load_blocker|memory_fault/.test(text)) return "crash";
  if (/no_effect|no_observed|no_reward|no_progression|negative_closure|write_erased|benign/.test(text)) return "no-effect";
  if (/planned_candidate|layout_candidate|unproduced_candidate|unknown/.test(text)) return "candidate";
  return "gameplay-effect";
}

const outcomeLabels = {
  progression: "Progression / reward", "gameplay-effect": "Gameplay effect",
  crash: "Crash / blocker", "no-effect": "No effect found", candidate: "Untested candidate"
};

function producer(item) {
  return item.producers?.[0] || {};
}

function setupText(item) {
  const action = producer(item).action_summary;
  if (action) return action;
  if (confidence(item) === "candidate") {
    return `No playable setup is known. This is a predicted ${sceneName(item.source_scene)} → ${sceneName(item.destination_scene)} overlap.`;
  }
  if (confidence(item) === "lab") {
    return `Lab-assisted replay from ${sceneName(item.source_scene)} into ${sceneName(item.destination_scene)}; no player-ready setup is established.`;
  }
  return `Perform the recorded combo-storage transition into ${sceneName(item.destination_scene)}.`;
}

function effectText(item) {
  const first = item.outcomes?.[0];
  if (item.result === "towers_of_power_unlock_broken") {
    return "Towers of Power no longer completes, so its spatula never becomes available. The pickup itself is not what breaks.";
  }
  if (item.result === "benign_resource_buffer_write") {
    return "The destination loads and remains playable. No visible gameplay change has been found.";
  }
  if (outcome(item) === "candidate") {
    const target = item.owner_name || item.writes?.[0]?.owner_name || "unidentified memory";
    const member = item.member || item.writes?.[0]?.member;
    return `Predicted to overwrite ${target}${member ? ` (${member})` : ""}; the runtime effect has not been tested.`;
  }
  if (first?.description) return first.description;
  if (item.outcome_description) {
    if (item.outcome_description.length <= 260) return item.outcome_description;
    if (outcome(item) === "crash") return `The test ended as ${words(item.result)} before a longer gameplay check could complete.`;
    if (outcome(item) === "no-effect") return "The tested write produced no observed gameplay difference from the control.";
    return `${item.outcome_description.slice(0, 257).trimEnd()}…`;
  }
  return words(item.result || "No effect description recorded");
}

function remainingText(item) {
  if (item.result === "towers_of_power_unlock_broken") {
    return "Whether death, scene reset, saving, or reloading repairs it has not been checked.";
  }
  if (item.result === "benign_resource_buffer_write") {
    return "No later action using the damaged scene data is known.";
  }
  const kind = confidence(item);
  if (kind === "candidate") return "Needs a reachable setup and a runtime test.";
  if (kind === "lab") return "Needs the same result from a player-performable setup.";
  const persistence = item.outcomes?.[0]?.persistence || item.persistence;
  return persistence && persistence !== "not_observed"
    ? `Persistence: ${words(persistence)}.`
    : "No additional consequence has been established.";
}

function status(item) {
  const kind = confidence(item);
  if (kind === "player") return { label: /community/.test(itemText(item)) ? "Community observed" : "Player replay", tone: "player" };
  if (kind === "candidate") return { label: "Untested", tone: "candidate" };
  return { label: outcome(item) === "crash" ? "Lab: crash" : "Lab-assisted", tone: "lab" };
}

function normalizeFinding(item) {
  return { ...item, key: item.id, curated: true };
}

function normalizeResult(item) {
  return { ...item, key: item.experiment_id, curated: false };
}

function matchesConfidence(item) {
  const kind = confidence(item);
  if (state.confidence === "all") return true;
  if (state.confidence === "observed") return kind !== "candidate";
  return kind === state.confidence;
}

function filteredRows() {
  const query = state.query.trim().toLowerCase();
  return state.rows.filter(item => {
    if (!matchesConfidence(item)) return false;
    if (state.platform !== "all" && item.platform !== state.platform) return false;
    if (state.source !== "all" && (item.source_scene || "unknown") !== state.source) return false;
    if (state.destination !== "all" && (item.destination_scene || "unknown") !== state.destination) return false;
    if (state.outcome !== "all" && outcome(item) !== state.outcome) return false;
    if (!query) return true;
    return [item.source_scene, item.destination_scene, item.owner_name, item.member, setupText(item), effectText(item)]
      .join(" ").toLowerCase().includes(query);
  });
}

function technicalDetails(item) {
  const references = item.evidence_refs || (item.evidence_path ? [item.evidence_path] : []);
  const refs = Array.isArray(references) ? references : String(references).split(";").filter(Boolean);
  const entries = [
    ["Classification", item.classification], ["Result", item.result],
    ["Target", item.writes?.[0]?.owner_name || item.owner_name],
    ["Member", item.writes?.[0]?.member || item.member],
    ["Holder", item.holder_hex], ["Continuation", item.writes?.[0]?.continuation_class || item.continuation_class]
  ].filter(([, value]) => value != null && value !== "");
  return `<details><summary>Technical record</summary>
    <dl class="technical">${entries.map(([name, value]) => `<div><dt>${esc(name)}</dt><dd>${esc(value)}</dd></div>`).join("")}</dl>
    ${refs.length ? `<p class="evidence"><strong>Evidence:</strong> ${refs.map(esc).join(" · ")}</p>` : ""}
  </details>`;
}

function renderRow(item) {
  const badge = status(item);
  return `<article class="result-row">
    <div class="route"><span>${esc(platformName(item.platform))}</span><strong>${esc(sceneName(item.source_scene))} → ${esc(sceneName(item.destination_scene))}</strong></div>
    <p>${esc(setupText(item))}</p>
    <p>${esc(effectText(item))}</p>
    <p class="remaining">${esc(remainingText(item))}</p>
    <span class="badge ${badge.tone}">${esc(badge.label)}</span>
    ${technicalDetails(item)}
  </article>`;
}

function render() {
  const rows = filteredRows();
  const shown = rows.slice(0, state.shown);
  $("#result-count").textContent = `Showing ${shown.length.toLocaleString()} of ${rows.length.toLocaleString()} matching results · ${state.rows.length.toLocaleString()} total`;
  $("#results").innerHTML = shown.length ? shown.map(renderRow).join("") : `<p class="empty">No results match these filters.</p>`;
  $("#show-more").hidden = shown.length >= rows.length;
}

function addOptions(selector, values, label) {
  const select = $(selector);
  for (const value of [...new Set(values.filter(Boolean))].sort()) {
    const option = document.createElement("option");
    option.value = value;
    option.textContent = label(value);
    select.append(option);
  }
}

function paramsToState() {
  const params = new URLSearchParams(location.search);
  for (const key of ["confidence", "platform", "source", "destination", "outcome"]) {
    if (params.has(key)) state[key] = params.get(key);
  }
  state.query = params.get("q") || "";
}

function stateToParams() {
  const params = new URLSearchParams();
  for (const key of ["confidence", "platform", "source", "destination", "outcome"]) {
    const value = state[key];
    if (value !== (key === "confidence" ? "player" : "all")) params.set(key, value);
  }
  if (state.query) params.set("q", state.query);
  history.replaceState({}, "", `${location.pathname}${params.size ? `?${params}` : ""}`);
}

function bindSelect(id, key) {
  $(id).value = state[key];
  $(id).addEventListener("change", event => {
    state[key] = event.target.value;
    state.shown = PAGE_SIZE;
    stateToParams();
    render();
  });
}

async function boot() {
  paramsToState();
  const [manifest, findings, index] = await Promise.all([
    fetch(`${DATA}manifest.json`).then(response => response.json()),
    fetch(`${DATA}findings.json`).then(response => response.json()),
    fetch(`${DATA}results-index.json`).then(response => response.json())
  ]);
  const chunks = await Promise.all(index.chunks.map(chunk => fetch(`${DATA}${chunk.file}`).then(response => response.json())));
  const curatedIds = new Set(findings.map(item => item.id));
  state.rows = [
    ...findings.map(normalizeFinding),
    ...chunks.flat().filter(item => !curatedIds.has(item.experiment_id)).map(normalizeResult)
  ];

  addOptions("#platform-filter", state.rows.map(item => item.platform), platformName);
  addOptions("#source-filter", state.rows.map(item => item.source_scene || "unknown"), sceneName);
  addOptions("#destination-filter", state.rows.map(item => item.destination_scene || "unknown"), sceneName);
  addOptions("#outcome-filter", Object.keys(outcomeLabels), value => outcomeLabels[value]);

  bindSelect("#confidence-filter", "confidence");
  bindSelect("#platform-filter", "platform");
  bindSelect("#source-filter", "source");
  bindSelect("#destination-filter", "destination");
  bindSelect("#outcome-filter", "outcome");
  $("#search-input").value = state.query;
  $("#search-input").addEventListener("input", event => {
    state.query = event.target.value;
    state.shown = PAGE_SIZE;
    stateToParams();
    render();
  });
  $("#show-more").addEventListener("click", () => { state.shown += PAGE_SIZE; render(); });
  $("#reset-filters").addEventListener("click", () => { location.href = location.pathname; });
  $("#copy-view").addEventListener("click", async () => {
    stateToParams();
    await navigator.clipboard.writeText(location.href);
    $("#copy-view").textContent = "Link copied";
    setTimeout(() => $("#copy-view").textContent = "Copy filtered link", 1200);
  });
  $("#updated-at").textContent = `Catalog updated ${new Date(manifest.generated_at).toLocaleString()}.`;
  render();
}

boot().catch(error => {
  console.error(error);
  $("#results").innerHTML = `<p class="empty">The catalog could not load: ${esc(error.message)}</p>`;
});
