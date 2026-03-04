const apiFromQuery = new URLSearchParams(window.location.search).get('api');
if (apiFromQuery) {
  localStorage.setItem('ADD_API_BASE', apiFromQuery);
}
const apiFromStorage = localStorage.getItem('ADD_API_BASE');
const isLocalHost = window.location.hostname === '127.0.0.1' || window.location.hostname === 'localhost';
const API = apiFromQuery || apiFromStorage || (isLocalHost ? 'http://127.0.0.1:8080' : '');

async function getJson(path) {
  const res = await fetch(`${API}${path}`);
  return res.json();
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

function shortError(err) {
  const msg = String(err || 'unknown error');
  return msg.length > 160 ? `${msg.slice(0, 157)}...` : msg;
}

function animateNumber(id, target, decimals = 0, suffix = '') {
  const el = document.getElementById(id);
  if (!el) return;
  const start = Number((el.dataset.value || '0'));
  const end = Number(target || 0);
  const steps = 16;
  let i = 0;
  const delta = (end - start) / steps;
  const t = setInterval(() => {
    i += 1;
    const v = (i >= steps) ? end : (start + delta * i);
    el.textContent = `${v.toFixed(decimals)}${suffix}`;
    el.dataset.value = String(v);
    if (i >= steps) clearInterval(t);
  }, 30);
}

async function refresh() {
  try {
    const info = await getJson('/api/getinfo');
    const mon = await getJson('/api/monetary');
    const health = await getJson('/api/health');
    const dash = await getJson('/api/dashboard');

    const i = info.data || {};
    const m = mon.data || {};

    setText('m-height', i.height || '-');
    setText('m-mempool', i.mempool || '-');
    setText('m-peers', i.peers || '-');
    setText('m-diff', i.difficulty_target || '-');
    setText('m-reward', i.next_reward || '-');
    setText('m-fees', i.fees_last_block || '-');

    setText('m-emitted', m.emitted || '-');
    setText('m-remaining', m.remaining || '-');
    setText('m-halving', m.next_halving_height || '-');

    if (dash && dash.ok) {
      animateNumber('kpi-tps', Number(dash.tps_est || 0), 2);
      animateNumber('kpi-bpd', Number(dash.blocks_per_day || 0), 0);
      animateNumber('kpi-circ', Number(dash.circulating_pct || 0), 4, '%');
    }

    setText('health', health.ok ? `Node: ONLINE (${health.raw})` : `Node: OFFLINE (${health.error || 'unreachable'})`);
  } catch (e) {
    setText('health', `Node: OFFLINE (${shortError(e)})`);
  }
}

refresh();
setInterval(refresh, 4000);

window.openFullscreen = function (fileName) {
  const dlg = document.getElementById('videoDialog');
  const v = document.getElementById('dialogVideo');
  if (!dlg || !v) return;
  v.src = `./assets/${fileName}`;
  dlg.showModal();
  v.play().catch(() => {});
};

window.closeFullscreenDialog = function () {
  const dlg = document.getElementById('videoDialog');
  const v = document.getElementById('dialogVideo');
  if (!dlg || !v) return;
  v.pause();
  v.src = '';
  dlg.close();
};

window.quoteSwap = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const result = document.getElementById('swap-result');
  try {
    const res = await getJson(`/api/swap_quote?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&amount_in=${encodeURIComponent(amountIn)}`);
    if (!res.ok) {
      result.textContent = `Swap quote error: ${res.error || 'unknown'}`;
      return;
    }
    result.textContent = `Quote: ${amountIn} ${tokenIn} -> ${res.amount_out} ${tokenOut}`;
  } catch (e) {
    result.textContent = `Swap quote error: ${shortError(e)}`;
  }
};

window.executeSwap = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const trader = (document.getElementById('swap-trader')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const minOut = (document.getElementById('swap-min-out')?.value || '0').trim();
  const result = document.getElementById('swap-result');
  try {
    const res = await getJson(`/api/swap_exact_in?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&trader=${encodeURIComponent(trader)}&amount_in=${encodeURIComponent(amountIn)}&min_out=${encodeURIComponent(minOut)}`);
    if (!res.ok) {
      result.textContent = `Swap execution error: ${res.error || 'unknown'}`;
      return;
    }
    result.textContent = `Swap executed: ${res.raw}`;
  } catch (e) {
    result.textContent = `Swap execution error: ${shortError(e)}`;
  }
};

window.loadPoolInfo = async function () {
  const tokenA = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenB = (document.getElementById('swap-token-out')?.value || '').trim();
  const out = document.getElementById('swap-pool');
  try {
    const res = await getJson(`/api/swap_pool_info?token_a=${encodeURIComponent(tokenA)}&token_b=${encodeURIComponent(tokenB)}`);
    if (!res.ok) {
      out.textContent = `Pool error: ${res.error || 'unknown'}`;
      return;
    }
    const d = res.data || {};
    out.textContent = `Pool ${tokenA}/${tokenB} | reserve_${tokenA}=${d[`reserve_${tokenA}`] || '-'} reserve_${tokenB}=${d[`reserve_${tokenB}`] || '-'} fee_bps=${d.fee_bps || '-'} lp_total_supply=${d.lp_total_supply || '-'}`;
  } catch (e) {
    out.textContent = `Pool error: ${shortError(e)}`;
  }
};

window.quoteBestRouteSwap = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const maxHops = (document.getElementById('swap-max-hops')?.value || '3').trim();
  const result = document.getElementById('swap-result');
  try {
    const res = await getJson(`/api/swap_best_route?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&amount_in=${encodeURIComponent(amountIn)}&max_hops=${encodeURIComponent(maxHops)}`);
    if (!res.ok) {
      result.textContent = `Best-route quote error: ${res.error || 'unknown'}`;
      return;
    }
    const d = res.data || {};
    result.textContent = `Best route quote: ${amountIn} ${tokenIn} -> ${d.amount_out || '-'} ${tokenOut} | route=${d.route || '-'}`;
  } catch (e) {
    result.textContent = `Best-route quote error: ${shortError(e)}`;
  }
};

window.executeBestRouteSwap = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const trader = (document.getElementById('swap-trader')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const minOut = (document.getElementById('swap-min-out')?.value || '0').trim();
  const maxHops = (document.getElementById('swap-max-hops')?.value || '3').trim();
  const deadlineSec = Number((document.getElementById('swap-deadline-sec')?.value || '60').trim());
  const deadlineUnix = Math.floor(Date.now() / 1000) + (Number.isFinite(deadlineSec) ? deadlineSec : 60);
  const result = document.getElementById('swap-result');

  try {
    const res = await getJson(`/api/swap_best_route_exact_in?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&trader=${encodeURIComponent(trader)}&amount_in=${encodeURIComponent(amountIn)}&min_out=${encodeURIComponent(minOut)}&deadline_unix=${encodeURIComponent(deadlineUnix)}&max_hops=${encodeURIComponent(maxHops)}`);
    if (!res.ok) {
      result.textContent = `Best-route execution error: ${res.error || 'unknown'}`;
      return;
    }
    const d = res.data || {};
    result.textContent = `Best-route executed: amount_out=${d.amount_out || '-'} route=${d.route || '-'}`;
  } catch (e) {
    result.textContent = `Best-route execution error: ${shortError(e)}`;
  }
};

window.getBestRouteSignPayload = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const trader = (document.getElementById('swap-trader')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const minOut = (document.getElementById('swap-min-out')?.value || '0').trim();
  const maxHops = (document.getElementById('swap-max-hops')?.value || '3').trim();
  const deadlineSec = Number((document.getElementById('swap-deadline-sec')?.value || '60').trim());
  const deadlineUnix = Math.floor(Date.now() / 1000) + (Number.isFinite(deadlineSec) ? deadlineSec : 60);
  const result = document.getElementById('swap-result');
  try {
    const res = await getJson(`/api/swap_best_route_sign_payload?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&trader=${encodeURIComponent(trader)}&amount_in=${encodeURIComponent(amountIn)}&min_out=${encodeURIComponent(minOut)}&deadline_unix=${encodeURIComponent(deadlineUnix)}&max_hops=${encodeURIComponent(maxHops)}`);
    if (!res.ok) {
      result.textContent = `Sign payload error: ${res.error || 'unknown'}`;
      return;
    }
    result.textContent = `Sign payload ready. Copy and sign with wallet: ${res.payload}`;
  } catch (e) {
    result.textContent = `Sign payload error: ${shortError(e)}`;
  }
};

window.executeBestRouteSwapSigned = async function () {
  const tokenIn = (document.getElementById('swap-token-in')?.value || '').trim();
  const tokenOut = (document.getElementById('swap-token-out')?.value || '').trim();
  const trader = (document.getElementById('swap-trader')?.value || '').trim();
  const amountIn = (document.getElementById('swap-amount-in')?.value || '').trim();
  const minOut = (document.getElementById('swap-min-out')?.value || '0').trim();
  const maxHops = (document.getElementById('swap-max-hops')?.value || '3').trim();
  const deadlineSec = Number((document.getElementById('swap-deadline-sec')?.value || '60').trim());
  const deadlineUnix = Math.floor(Date.now() / 1000) + (Number.isFinite(deadlineSec) ? deadlineSec : 60);
  const traderPubkey = (document.getElementById('swap-trader-pubkey')?.value || '').trim();
  const traderSig = (document.getElementById('swap-trader-signature')?.value || '').trim();
  const result = document.getElementById('swap-result');

  try {
    const res = await getJson(`/api/swap_best_route_exact_in_signed?token_in=${encodeURIComponent(tokenIn)}&token_out=${encodeURIComponent(tokenOut)}&trader=${encodeURIComponent(trader)}&amount_in=${encodeURIComponent(amountIn)}&min_out=${encodeURIComponent(minOut)}&deadline_unix=${encodeURIComponent(deadlineUnix)}&max_hops=${encodeURIComponent(maxHops)}&trader_pubkey=${encodeURIComponent(traderPubkey)}&trader_sig=${encodeURIComponent(traderSig)}`);
    if (!res.ok) {
      result.textContent = `Signed execution error: ${res.error || 'unknown'}`;
      return;
    }
    const d = res.data || {};
    result.textContent = `Signed best-route executed: amount_out=${d.amount_out || '-'} route=${d.route || '-'}`;
  } catch (e) {
    result.textContent = `Signed execution error: ${shortError(e)}`;
  }
};

function tkv(id, d = '') {
  return (document.getElementById(id)?.value || d).trim();
}

function tokenResult(msg) {
  const el = document.getElementById('token-admin-result');
  if (el) el.textContent = msg;
}

window.tokenCreateEx = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&name=${encodeURIComponent(tkv('tk-name'))}&owner=${encodeURIComponent(tkv('tk-owner'))}&max_supply=${encodeURIComponent(tkv('tk-max-supply'))}&initial_mint=${encodeURIComponent(tkv('tk-initial-mint'))}&decimals=${encodeURIComponent(tkv('tk-decimals', '18'))}&burnable=${encodeURIComponent(tkv('tk-burnable', '0'))}&dev_wallet=${encodeURIComponent(tkv('tk-dev-wallet', '-'))}&dev_allocation=${encodeURIComponent(tkv('tk-dev-allocation', '0'))}`;
    const r = await getJson(`/api/token_create_ex?${q}`);
    tokenResult(r.ok ? `CreateEx: ${r.raw}` : `CreateEx error: ${r.error}`);
  } catch (e) { tokenResult(`CreateEx error: ${shortError(e)}`); }
};

window.tokenSetPolicy = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&caller=${encodeURIComponent(tkv('tk-owner'))}&treasury_wallet=${encodeURIComponent(tkv('tk-treasury', '-'))}&transfer_fee_bps=${encodeURIComponent(tkv('tk-fee', '0'))}&burn_fee_bps=${encodeURIComponent(tkv('tk-burn-fee', '0'))}&paused=${encodeURIComponent(tkv('tk-paused', '0'))}`;
    const r = await getJson(`/api/token_set_policy?${q}`);
    tokenResult(r.ok ? `SetPolicy: ${r.raw}` : `SetPolicy error: ${r.error}`);
  } catch (e) { tokenResult(`SetPolicy error: ${shortError(e)}`); }
};

window.tokenSetBlacklist = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&caller=${encodeURIComponent(tkv('tk-owner'))}&wallet=${encodeURIComponent(tkv('tk-black-wallet'))}&blocked=${encodeURIComponent(tkv('tk-black-flag', '1'))}`;
    const r = await getJson(`/api/token_blacklist?${q}`);
    tokenResult(r.ok ? `Blacklist: ${r.raw}` : `Blacklist error: ${r.error}`);
  } catch (e) { tokenResult(`Blacklist error: ${shortError(e)}`); }
};

window.tokenSetFeeExempt = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&caller=${encodeURIComponent(tkv('tk-owner'))}&wallet=${encodeURIComponent(tkv('tk-ex-wallet'))}&exempt=${encodeURIComponent(tkv('tk-ex-flag', '1'))}`;
    const r = await getJson(`/api/token_fee_exempt?${q}`);
    tokenResult(r.ok ? `FeeExempt: ${r.raw}` : `FeeExempt error: ${r.error}`);
  } catch (e) { tokenResult(`FeeExempt error: ${shortError(e)}`); }
};

window.tokenSetLimits = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&caller=${encodeURIComponent(tkv('tk-owner'))}&max_tx_amount=${encodeURIComponent(tkv('tk-max-tx', '0'))}&max_wallet_amount=${encodeURIComponent(tkv('tk-max-wallet', '0'))}`;
    const r = await getJson(`/api/token_set_limits?${q}`);
    tokenResult(r.ok ? `SetLimits: ${r.raw}` : `SetLimits error: ${r.error}`);
  } catch (e) { tokenResult(`SetLimits error: ${shortError(e)}`); }
};

window.tokenBurn = async function () {
  try {
    const q = `symbol=${encodeURIComponent(tkv('tk-symbol'))}&from=${encodeURIComponent(tkv('tk-burn-from'))}&amount=${encodeURIComponent(tkv('tk-burn-amount', '0'))}`;
    const r = await getJson(`/api/token_burn?${q}`);
    tokenResult(r.ok ? `Burn: ${r.raw}` : `Burn error: ${r.error}`);
  } catch (e) { tokenResult(`Burn error: ${shortError(e)}`); }
};

window.tokenInfo = async function () {
  try {
    const r = await getJson(`/api/token_info?symbol=${encodeURIComponent(tkv('tk-symbol'))}`);
    if (!r.ok) {
      tokenResult(`TokenInfo error: ${r.error}`);
      return;
    }
    tokenResult(`TokenInfo: ${r.raw}`);
  } catch (e) { tokenResult(`TokenInfo error: ${shortError(e)}`); }
};
