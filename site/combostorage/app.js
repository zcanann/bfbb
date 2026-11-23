const DATA = "data/";
const PAGE_SIZE = 150;
const sceneNames = {
  HB: "Bikini Bottom", JF: "Jellyfish Fields", BB: "Downtown Bikini Bottom",
  GL: "Goo Lagoon", B1: "Poseidome", RB: "Rock Bottom", BC: "Mermalair",
  SM: "Sand Mountain", B2: "Industrial Park", KF: "Kelp Forest",
  GY: "Flying Dutchman's Graveyard", DB: "SpongeBob's Dream",
  B3: "Chum Bucket Lab", PG: "Playground", MN: "Main Menu"
};
const effectLabels = {
  "wrong-warp": "Wrong warp",
  progression: "Progression changed",
  reward: "Reward / collectible",
  "gameplay-corruption": "Gameplay corruption",
  visual: "Visual glitch",
  crash: "Crash / softlock",
  "no-effect": "No visible effect",
  unresolved: "Effect not tested"
};
const state = {
  rows: [], effect: "all", platform: "all", source: "all",
  destination: "all", query: "", result: "", shown: PAGE_SIZE
};
const $ = selector => document.querySelector(selector);
const esc = value => String(value ?? "").replace(/[&<>'"]/g, character => ({
  "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", '"': "&quot;"
})[character]);
const words = value => String(value ?? "").replaceAll("_", " ").replace(/\b\w/g, letter => letter.toUpperCase());
const platformName = value => ({ gamecube: "GameCube", ps2: "PS2", xbox: "Xbox" })[value] || value || "Unknown";
const sceneName = code => {
  if (!code) return "Unknown";
  const prefix = Object.keys(sceneNames).find(key => code.startsWith(key));
  return prefix ? `${sceneNames[prefix]} (${code})` : code;
};
const effect = item => item.effect_category;
const setupText = item => item.setup_description;
const effectText = item => item.effect_description;
const notesText = item => item.notes_description;

function filteredRows() {
  const query = state.query.trim().toLowerCase();
  return state.rows.filter(item => {
    if (state.result) {
      const ids = [item.public_id, item.id, ...(item.legacy_public_ids || [])];
      return ids.includes(state.result);
    }
    if (state.platform !== "all" && item.platform !== state.platform) return false;
    if (state.source !== "all" && (item.source_scene || "unknown") !== state.source) return false;
    if (state.destination !== "all" && (item.destination_scene || "unknown") !== state.destination) return false;
    if (state.effect === "changed" && ["no-effect", "unresolved"].includes(effect(item))) return false;
    if (state.effect !== "all" && state.effect !== "changed" && effect(item) !== state.effect) return false;
    if (!query) return true;
    return [
      item.source_scene, item.destination_scene, item.owner_name, item.member,
      setupText(item), effectText(item), effectLabels[effect(item)]
    ].join(" ").toLowerCase().includes(query);
  });
}

function technicalDetails(item) {
  const references = item.evidence_refs || (item.evidence_path ? [item.evidence_path] : []);
  const refs = Array.isArray(references)
    ? references
    : String(references).split(/\s*\|\s*|;/).filter(Boolean);
  const targets = item.overlap_targets?.length
    ? item.overlap_targets.join("; ")
    : (item.writes?.[0]?.owner_name || item.owner_name);
  const entries = [
    ["Observed effect", item.effect_label],
    ["Evidence", words(item.evidence_kind)],
    ["Technical classification", item.classification],
    ["Recorded results", item.recorded_results?.join(", ") || item.result],
    ["Targets", targets],
    ["Address", item.holder_hex],
    ["Timing", words(item.timing_phase)]
  ].filter(([, value]) => value != null && value !== "");
  return `<details><summary>Technical record</summary>
    <dl class="technical">${entries.map(([name, value]) => `<div><dt>${esc(name)}</dt><dd>${esc(value)}</dd></div>`).join("")}</dl>
    ${refs.length ? `<p class="evidence"><strong>Evidence:</strong> ${refs.map(esc).join(" · ")}</p>` : ""}
  </details>`;
}

function renderRow(item) {
  const kind = effect(item);
  return `<article class="result-row" id="${esc(item.public_id)}">
    <div class="route"><span>${esc(platformName(item.platform))}</span><strong>${esc(sceneName(item.source_scene))} &rarr; ${esc(sceneName(item.destination_scene))}</strong></div>
    <p>${esc(setupText(item))}</p>
    <p>${esc(effectText(item))}</p>
    <p class="remaining">${esc(notesText(item))}</p>
    <span class="badge ${esc(kind)}">${esc(item.effect_label || effectLabels[kind])}</span>
    ${technicalDetails(item)}
    <button class="copy-result" type="button" data-result="${esc(item.public_id)}">Copy this result</button>
  </article>`;
}

function render() {
  const rows = filteredRows();
  const shown = rows.slice(0, state.shown);
  $("#result-count").textContent = `Showing ${shown.length.toLocaleString()} of ${rows.length.toLocaleString()} matching results · ${state.rows.length.toLocaleString()} input/output results total`;
  $("#results").innerHTML = shown.length ? shown.map(renderRow).join("") : '<p class="empty">No result matches these filters.</p>';
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
  for (const key of ["effect", "platform", "source", "destination"]) {
    if (params.has(key)) state[key] = params.get(key);
  }
  state.query = params.get("q") || "";
  state.result = params.get("result") || "";
}

function stateToParams() {
  const params = new URLSearchParams();
  if (state.result) {
    params.set("result", state.result);
    history.replaceState({}, "", `${location.pathname}?${params}`);
    return;
  }
  for (const key of ["effect", "platform", "source", "destination"]) {
    const defaultValue = "all";
    if (state[key] !== defaultValue) params.set(key, state[key]);
  }
  if (state.query) params.set("q", state.query);
  history.replaceState({}, "", `${location.pathname}${params.size ? `?${params}` : ""}`);
}

function bindSelect(id, key) {
  $(id).value = state[key];
  $(id).addEventListener("change", event => {
    state.result = "";
    state[key] = event.target.value;
    state.shown = PAGE_SIZE;
    stateToParams();
    render();
  });
}

async function boot() {
  paramsToState();
  const [manifest, index] = await Promise.all([
    fetch(`${DATA}manifest.json`).then(response => response.json()),
    fetch(`${DATA}results-index.json`).then(response => response.json())
  ]);
  const chunks = await Promise.all(
    index.chunks.map(chunk => fetch(`${DATA}${chunk.file}`).then(response => response.json()))
  );
  state.rows = chunks.flat();
  addOptions("#effect-filter", Object.keys(effectLabels), value => effectLabels[value]);
  addOptions("#platform-filter", state.rows.map(item => item.platform), platformName);
  addOptions("#source-filter", state.rows.map(item => item.source_scene || "unknown"), sceneName);
  addOptions("#destination-filter", state.rows.map(item => item.destination_scene || "unknown"), sceneName);
  bindSelect("#effect-filter", "effect");
  bindSelect("#platform-filter", "platform");
  bindSelect("#source-filter", "source");
  bindSelect("#destination-filter", "destination");
  $("#search-input").value = state.query;
  $("#search-input").addEventListener("input", event => {
    state.result = "";
    state.query = event.target.value;
    state.shown = PAGE_SIZE;
    stateToParams();
    render();
  });
  $("#show-more").addEventListener("click", () => { state.shown += PAGE_SIZE; render(); });
  $("#results").addEventListener("click", async event => {
    const button = event.target.closest(".copy-result");
    if (!button) return;
    const url = new URL(location.href);
    url.search = "";
    url.searchParams.set("result", button.dataset.result);
    await navigator.clipboard.writeText(url.href);
    button.textContent = "Link copied";
    setTimeout(() => button.textContent = "Copy this result", 1200);
  });
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

if (typeof document !== "undefined") {
  boot().catch(error => {
    console.error(error);
    $("#results").innerHTML = `<p class="empty">The catalog could not load: ${esc(error.message)}</p>`;
  });
}

export { effect, setupText };
