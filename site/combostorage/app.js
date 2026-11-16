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
  rows: [], effect: "changed", evidence: "all", platform: "all", source: "all",
  destination: "all", query: "", shown: PAGE_SIZE
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
function itemText(item) {
  return `${item.classification || ""} ${item.feasibility || ""} ${item.naturality || ""} ${item.result || ""} ${item.outcome_category || ""}`.toLowerCase();
}
function producer(item) { return item.producers?.[0] || {}; }
function evidence(item) {
  const text = itemText(item);
  if (/community_observed|community_reported|community_observation/.test(text)) return "community";
  if (/reported_controller|needs_lab_replay/.test(text)) return "reported";
  return "replayed";
}
function effect(item) {
  if (item.result === "towers_of_power_unlock_broken") return "gameplay-corruption";
  const descriptions = item.outcomes?.map(entry => entry.description || "").join(" ") || "";
  const text = `${itemText(item)} ${item.summary || ""} ${item.outcome_description || ""} ${descriptions}`.toLowerCase();
  const terminal = `${item.result || ""} ${item.outcome_category || ""} ${item.classification || ""}`.toLowerCase();
  // The terminal result wins: corrupting a useful field and then crashing is a crash.
  if (/memory_faults_during_play|faults_while_play_continues/.test(terminal)) return "gameplay-corruption";
  if (/(^|[_ ])(?:crash|stalled|guest_stall|runtime_stall|load_blocker|scene_reset_fault|consumer_crash|conditional_crash|timeout)(?:$|[_ ])/.test(terminal)) return "crash";
  if (/fodder_model_initialization_erases_overlap|damaged_background_plant_remains_live_near_player/.test(terminal)) return "no-effect";
  if (/wrong[_ -]?warp|loads? (?:a )?(?:different|wrong) (?:scene|level)|take bus.*loads/.test(text)) return "wrong-warp";
  if (/spatula|sock|shiny|reward|collectible|ability|cruise bubble|bubble bowl/.test(text)
      && /award|grant|increase|collect|receive|unlock/.test(text)) return "reward";
  if (/task|progression|unlock|counter/.test(text) && /change|award|grant|unlock|persist|saved/.test(text)) return "progression";
  if (/visual|render|geometry|transform|matrix basis|relocat|shrink|skew|invisible/.test(text)) return "visual";
  if (/consumer_no_difference|no_observed_difference|negative_result|runtime_action_observation|pickup_no_observed_difference|consumer_probe_no_effect|write_erased_before_play|stable_nearby_observation/.test(text)) return "no-effect";
  if (/no visible|no observed|no effect|benign|write erased|no difference/.test(text)) return "no-effect";
  return "gameplay-corruption";
}
const effectLabels = {
  "wrong-warp": "Wrong warp",
  progression: "Progression changed",
  reward: "Reward / collectible",
  "gameplay-corruption": "Gameplay corruption",
  visual: "Visual glitch",
  crash: "Crash / softlock",
  "no-effect": "No visible effect"
};
function isPlayerInput(item) {
  return true;
}
function setupText(item) {
  if (item.result === "towers_of_power_unlock_broken") {
    return "Build a combo, pause and quit on the frame before its reward appears, let the delayed reward appear on the title screen, then load a save in Mermalair 2.";
  }
  const action = producer(item).action_summary || "";
  if (action && !/create the measured|alignment|warmups|diagnostic|artificial|forced|menu motive|created menu/i.test(action)) return action;
  const evidenceText = `${item.evidence_refs || ""} ${item.evidence_path || ""}`;
  const phaseText = `${action} ${evidenceText}`;
  const source = sceneName(item.source_scene);
  const destination = sceneName(item.destination_scene);
  if (/after_show_expiry|let (?:its |the )?(?:menu motive|delayed combo) expire/i.test(phaseText)) {
    return `Build a combo in ${source}, pause and quit before its reward appears, let the delayed combo appear and expire on the title screen, then load ${destination}.`;
  }
  if (/after_show|before expiry|before it expires/i.test(phaseText)) {
    return `Build a combo in ${source}, pause and quit before its reward appears, let the delayed combo appear on the title screen, then load ${destination} before it expires.`;
  }
  return `Build a combo in ${source}, pause and quit before its reward appears, let the delayed combo fire on the title screen, then load ${destination}.`;
}
function effectText(item) {
  const first = item.outcomes?.[0];
  if (item.result === "towers_of_power_unlock_broken") {
    return "Towers of Power can no longer be completed, so its spatula cannot be earned.";
  }
  if (item.result === "benign_resource_buffer_write") {
    return "The destination loads and remains playable. No visible gameplay change has been found.";
  }
  if (item.result === "missing_summoned_enemy") {
    const reset = item.consumers?.find(consumer => /reset|die|death/i.test(`${consumer.kind || ""} ${consumer.action || ""}`));
    return `${item.summary || "King Jellyfish summons one fewer enemy than normal."}${reset?.result ? ` ${reset.result}` : ""}`;
  }
  if (/consumer_no_difference|no_observed_difference|negative_result|pickup_no_observed_difference|consumer_probe_no_effect|write_erased_before_play/.test(item.outcome_category || "")) {
    return "The tested action completes and play continues; no visible gameplay difference was found.";
  }
  if (item.outcome_category === "runtime_action_observation") {
    return "The destination remains playable through the tested object action; no distinct visible effect was confirmed.";
  }
  if (item.outcome_category === "loaded_resource_corruption") {
    const target = item.owner_name || "scene memory";
    return `The destination reaches playable state after the combo write corrupts ${target}. No later gameplay effect was tested.`;
  }
  if (effect(item) === "crash") {
    const stage = first?.crash_stage || item.crash_stage;
    return stage ? `The game crashes during ${words(stage).toLowerCase()}.` : "The game crashes before play can continue.";
  }
  if (first?.description) return first.description;
  if (item.outcome_description) {
    return item.outcome_description.length <= 280
      ? item.outcome_description
      : `${item.outcome_description.slice(0, 277).trimEnd()}...`;
  }
  return item.summary || words(item.result || "No effect description recorded");
}
function notesText(item) {
  const repeats = item.repeat_count > 1 ? ` Reproduced in ${item.repeat_count} recorded runs.` : "";
  if (effect(item) === "crash") return `The run ends here.${repeats}`;
  if (item.result === "towers_of_power_unlock_broken") return "The failure persists for the observed visit; reset and save behavior are not recorded.";
  if (item.result === "benign_resource_buffer_write") return "No later consequence has been observed.";
  if (/synthetic_combo|patched_destination|controller.*pending|pre_expiry_window/.test(item.naturality || "")) {
    return `The effect was reproduced; exact controller routing for the combo/timing is not yet recorded.${repeats}`;
  }
  const persistence = item.outcomes?.[0]?.persistence || item.persistence;
  return persistence && persistence !== "not_observed"
    ? `Persistence: ${words(persistence)}.${repeats}`
    : `No additional effect has been established.${repeats}`;
}
function normalizeFinding(item) { return { ...item, key: item.id }; }
function normalizeResult(item) { return { ...item, key: item.experiment_id }; }
function filteredRows() {
  const query = state.query.trim().toLowerCase();
  return state.rows.filter(item => {
    if (state.platform !== "all" && item.platform !== state.platform) return false;
    if (state.source !== "all" && (item.source_scene || "unknown") !== state.source) return false;
    if (state.destination !== "all" && (item.destination_scene || "unknown") !== state.destination) return false;
    if (state.effect === "changed" && effect(item) === "no-effect") return false;
    if (state.effect !== "all" && state.effect !== "changed" && effect(item) !== state.effect) return false;
    if (state.evidence !== "all" && evidence(item) !== state.evidence) return false;
    if (!query) return true;
    return [item.source_scene, item.destination_scene, item.owner_name, item.member, setupText(item), effectText(item), effectLabels[effect(item)]]
      .join(" ").toLowerCase().includes(query);
  });
}
function technicalDetails(item) {
  const references = item.evidence_refs || (item.evidence_path ? [item.evidence_path] : []);
  const refs = Array.isArray(references) ? references : String(references).split(";").filter(Boolean);
  const entries = [
    ["Observed effect", effectLabels[effect(item)]], ["Evidence", words(evidence(item))],
    ["Technical classification", item.classification], ["Recorded result", item.result],
    ["Target", item.writes?.[0]?.owner_name || item.owner_name],
    ["Member", item.writes?.[0]?.member || item.member], ["Address", item.holder_hex]
  ].filter(([, value]) => value != null && value !== "");
  return `<details><summary>Technical record</summary>
    <dl class="technical">${entries.map(([name, value]) => `<div><dt>${esc(name)}</dt><dd>${esc(value)}</dd></div>`).join("")}</dl>
    ${refs.length ? `<p class="evidence"><strong>Evidence:</strong> ${refs.map(esc).join(" · ")}</p>` : ""}
  </details>`;
}
function renderRow(item) {
  const kind = effect(item);
  return `<article class="result-row">
    <div class="route"><span>${esc(platformName(item.platform))}</span><strong>${esc(sceneName(item.source_scene))} &rarr; ${esc(sceneName(item.destination_scene))}</strong></div>
    <p>${esc(setupText(item))}</p><p>${esc(effectText(item))}</p>
    <p class="remaining">${esc(notesText(item))}</p>
    <span class="badge ${esc(kind)}">${esc(effectLabels[kind])}</span>
    ${technicalDetails(item)}
  </article>`;
}
function render() {
  const rows = filteredRows();
  const shown = rows.slice(0, state.shown);
  $("#result-count").textContent = `Showing ${shown.length.toLocaleString()} of ${rows.length.toLocaleString()} matching results · ${state.rows.length.toLocaleString()} player-input results total`;
  $("#results").innerHTML = shown.length ? shown.map(renderRow).join("") : '<p class="empty">No player-performable result matches these filters.</p>';
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
  for (const key of ["effect", "evidence", "platform", "source", "destination"]) {
    if (params.has(key)) state[key] = params.get(key);
  }
  state.query = params.get("q") || "";
}
function stateToParams() {
  const params = new URLSearchParams();
  for (const key of ["effect", "evidence", "platform", "source", "destination"]) {
    const defaultValue = key === "effect" ? "changed" : "all";
    if (state[key] !== defaultValue) params.set(key, state[key]);
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
  ].filter(isPlayerInput);
  addOptions("#effect-filter", Object.keys(effectLabels), value => effectLabels[value]);
  addOptions("#platform-filter", state.rows.map(item => item.platform), platformName);
  addOptions("#source-filter", state.rows.map(item => item.source_scene || "unknown"), sceneName);
  addOptions("#destination-filter", state.rows.map(item => item.destination_scene || "unknown"), sceneName);
  bindSelect("#effect-filter", "effect");
  bindSelect("#evidence-filter", "evidence");
  bindSelect("#platform-filter", "platform");
  bindSelect("#source-filter", "source");
  bindSelect("#destination-filter", "destination");
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
if (typeof document !== "undefined") {
  boot().catch(error => {
    console.error(error);
    $("#results").innerHTML = `<p class="empty">The catalog could not load: ${esc(error.message)}</p>`;
  });
}

export { effect, effectLabels, evidence, setupText };
