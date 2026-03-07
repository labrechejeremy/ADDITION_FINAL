const LOCAL_API = (() => {
    const queryApi = new URLSearchParams(window.location.search).get('api');
    if (queryApi) {
        const normalized = queryApi.trim().replace(/\/$/, '');
        localStorage.setItem('PORTAL_API_BASE', normalized);
        return normalized;
    }

    const stored = (localStorage.getItem('PORTAL_API_BASE') || '').trim();
    const host = window.location.hostname;

    if (stored) {
        const normalizedStored = stored.replace(/\/$/, '');
        // Ignore localhost override when browsing a public domain.
        if (!(host !== 'localhost' && host !== '127.0.0.1' && normalizedStored.includes('127.0.0.1'))) {
            return normalizedStored;
        }
    }

    if (host === 'localhost' || host === '127.0.0.1') return 'http://127.0.0.1:8080';
    if (host === 'dogecointoday.com' || host === 'www.dogecointoday.com') return 'https://api.dogecointoday.com';

    // Default production convention: api.<current-domain>
    const bareHost = host.replace(/^www\./, '');
    return `https://api.${bareHost}`;
})();

const TPS_HISTORY = [];
const TPS_HISTORY_MAX = 60;

let CHAIN_TOKEN_CATALOG = {};

const WALLET_STATE = {
    provider: null,
    account: null,
    web3: null,
    connectedBy: null
};

function byId(id) {
    return document.getElementById(id);
}

function setText(id, value) {
    const el = byId(id);
    if (el) el.textContent = value;
}

function setAssistant(msg) {
    setText('swap-assistant-msg', msg);
}

function showApiDebugStatus() {
    const box = byId('network-raw');
    if (box && !box.textContent) {
        box.textContent = `API base: ${LOCAL_API}`;
    }
    // Console hint for browser diagnostics
    console.info('[ADDITION Portal] API base =', LOCAL_API);
}

function getChainCfg(chainKey) {
    return CHAIN_TOKEN_CATALOG[chainKey] || {
        label: chainKey,
        chain_id_hex: '',
        rpc_urls: [],
        native_currency: { name: '', symbol: '', decimals: 18 },
        block_explorer_urls: [],
        tokens: []
    };
}

async function getJson(path) {
    try {
        const res = await fetch(LOCAL_API + '/api' + path);
        return await res.json();
    } catch (e) {
        return { ok: false, error: e.message || 'network error' };
    }
}

async function postJson(path, body) {
    try {
        const res = await fetch(LOCAL_API + path, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body || {})
        });
        return await res.json();
    } catch (e) {
        return { ok: false, error: e.message || 'network error' };
    }
}

function pushTpsPoint(v) {
    TPS_HISTORY.push(Number.isFinite(v) ? v : 0);
    if (TPS_HISTORY.length > TPS_HISTORY_MAX) TPS_HISTORY.shift();
}

function renderTpsChart() {
    const canvas = byId('tps-chart');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = '#0a0d1a';
    ctx.fillRect(0, 0, w, h);

    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 1;
    for (let i = 1; i < 4; i += 1) {
        const y = (h / 4) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }

    if (TPS_HISTORY.length < 2) return;
    const min = Math.min(...TPS_HISTORY);
    const max = Math.max(...TPS_HISTORY);
    const span = Math.max(1, max - min);

    ctx.strokeStyle = '#53d8ff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    TPS_HISTORY.forEach((v, i) => {
        const x = (i / (TPS_HISTORY.length - 1)) * (w - 20) + 10;
        const y = h - (((v - min) / span) * (h - 30) + 15);
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.stroke();

    ctx.fillStyle = '#edf1ff';
    ctx.font = '12px JetBrains Mono';
    ctx.fillText(`TPS now: ${TPS_HISTORY[TPS_HISTORY.length - 1].toFixed(2)}`, 12, 18);
}

function normalizeApiResponse(res) {
    if (!res) return { ok: false, error: 'empty response' };
    if (typeof res === 'object' && Object.prototype.hasOwnProperty.call(res, 'ok')) return res;
    if (typeof res === 'string') {
        try {
            const parsed = JSON.parse(res);
            if (parsed && typeof parsed === 'object') return parsed;
        } catch (_) {}
    }
    return { ok: false, error: 'invalid response format', raw: String(res) };
}

async function refresh() {
    const infoRes = normalizeApiResponse(await getJson('/getinfo'));
    const monRes = normalizeApiResponse(await getJson('/monetary'));

    const netRaw = byId('network-raw');
    if (infoRes.ok && infoRes.data) {
        const d = infoRes.data;
        setText('m-height', d.height || '0');
        setText('m-peers', d.peers || '0');
        setText('kpi-tps', d.last_tps || '0.00');
        setText('m-diff', d.difficulty_target || '-');

        pushTpsPoint(Number(d.last_tps || 0));
        renderTpsChart();

        const mineMs = Number(d.last_mine_ms || 0);
        const hash = mineMs > 0 ? (1000 / mineMs) : 0;
        setText('m-hash', hash > 1000 ? `${hash.toFixed(2)} MH/s` : `${hash.toFixed(2)} kH/s`);

        const hs = byId('health-status');
        if (hs) {
            hs.textContent = 'OPERATIONAL';
            hs.className = 'status-ok';
        }

        if (netRaw) netRaw.textContent = infoRes.raw || JSON.stringify(d, null, 2);
        setText('kpi-height', d.height || '0');
        setText('kpi-peers', d.peers || '0');
        const kpiHealth = byId('kpi-health');
        if (kpiHealth) {
            kpiHealth.textContent = 'OPERATIONAL';
            kpiHealth.className = 'status-ok';
        }
    } else {
        const hs = byId('health-status');
        if (hs) {
            hs.textContent = 'OFFLINE';
            hs.className = 'status-danger';
        }
        if (netRaw) netRaw.textContent = infoRes.error || infoRes.raw || 'RPC offline';
        const kpiHealth = byId('kpi-health');
        if (kpiHealth) {
            kpiHealth.textContent = 'OFFLINE';
            kpiHealth.className = 'status-danger';
        }
    }

    if (monRes.ok && monRes.data) {
        const m = monRes.data;
        setText('m-emitted', `${Number(m.emitted || 0).toLocaleString()} /ADD`);
        setText('tok-max', Number(m.max_supply || 0).toLocaleString());
        setText('tok-emitted', Number(m.emitted || 0).toLocaleString());
        const maxSupply = Number(m.max_supply || 0);
        const emitted = Number(m.emitted || 0);
        const remain = Math.max(0, maxSupply - emitted);
        const progress = maxSupply > 0 ? ((emitted / maxSupply) * 100) : 0;
        setText('tok-remaining', remain.toLocaleString());
        setText('tok-progress', `${progress.toFixed(4)}%`);
        const tokRaw = byId('tokenomics-raw');
        if (tokRaw) tokRaw.textContent = monRes.raw || JSON.stringify(m, null, 2);
    }
}

function populateChainSelect(selectId, selectedKey) {
    const el = byId(selectId);
    if (!el) return;
    el.innerHTML = '';
    Object.entries(CHAIN_TOKEN_CATALOG).forEach(([key, cfg]) => {
        const opt = document.createElement('option');
        opt.value = key;
        opt.textContent = cfg.label;
        if (key === selectedKey) opt.selected = true;
        el.appendChild(opt);
    });
}

function populateTokenSelect(chainKey, tokenSelectId, contractTextId, preferredSymbol) {
    const tokenEl = byId(tokenSelectId);
    if (!tokenEl) return;
    const cfg = getChainCfg(chainKey);
    tokenEl.innerHTML = '';
    cfg.tokens.forEach((t) => {
        const opt = document.createElement('option');
        opt.value = t.symbol;
        opt.dataset.contract = t.contract || '';
        opt.textContent = t.symbol;
        if (preferredSymbol && preferredSymbol === t.symbol) opt.selected = true;
        tokenEl.appendChild(opt);
    });

    const selected = tokenEl.options[tokenEl.selectedIndex];
    const c = selected ? (selected.dataset.contract || 'native') : '—';
    setText(contractTextId, c || 'native');
}

function refreshSwapContracts() {
    const chainIn = byId('swap-chain-in')?.value;
    const chainOut = byId('swap-chain-out')?.value;
    if (chainIn) populateTokenSelect(chainIn, 'swap-token-in', 'swap-contract-in', byId('swap-token-in')?.value);
    if (chainOut) populateTokenSelect(chainOut, 'swap-token-out', 'swap-contract-out', byId('swap-token-out')?.value);
    updateAssistantMessage();
}

function updateAssistantMessage() {
    const inChain = byId('swap-chain-in')?.value;
    const outChain = byId('swap-chain-out')?.value;
    const inToken = byId('swap-token-in')?.value;
    const outToken = byId('swap-token-out')?.value;
    if (!inChain || !outChain || !inToken || !outToken) return;

    if (inChain === outChain) {
        setAssistant(`Ready: ${inToken} -> ${outToken} on ${getChainCfg(inChain).label}. You can request best route now.`);
    } else {
        setAssistant(`Cross-chain mode: ${getChainCfg(inChain).label} -> ${getChainCfg(outChain).label}. Assistant will stage Bridge + DEX routing.`);
    }
}

async function connectMetaMask() {
    if (!window.ethereum) {
        setText('wallet-status', 'Wallet: MetaMask not found');
        return;
    }
    try {
        const accounts = await window.ethereum.request({ method: 'eth_requestAccounts' });
        WALLET_STATE.provider = window.ethereum;
        WALLET_STATE.account = accounts?.[0] || null;
        WALLET_STATE.web3 = new window.Web3(window.ethereum);
        WALLET_STATE.connectedBy = 'MetaMask';
        setText('wallet-status', `Wallet: ${WALLET_STATE.connectedBy} (${WALLET_STATE.account || 'connected'})`);
        setAssistant('Wallet connected. You can now add token/network or request guided swap routes.');
    } catch (e) {
        setText('wallet-status', `Wallet: connect failed (${e.message || 'unknown'})`);
    }
}

async function connectWalletConnect() {
    try {
        const WCProvider = window.WalletConnectProvider?.default;
        const Web3ModalCtor = window.Web3Modal?.default;
        if (!WCProvider || !Web3ModalCtor) {
            setText('wallet-status', 'Wallet: WalletConnect SDK not available');
            return;
        }
        const modal = new Web3ModalCtor({
            cacheProvider: false,
            providerOptions: {
                walletconnect: {
                    package: WCProvider,
                    options: { rpc: { 1: 'https://rpc.ankr.com/eth' } }
                }
            }
        });
        const provider = await modal.connect();
        const web3 = new window.Web3(provider);
        const accounts = await web3.eth.getAccounts();
        WALLET_STATE.provider = provider;
        WALLET_STATE.web3 = web3;
        WALLET_STATE.account = accounts?.[0] || null;
        WALLET_STATE.connectedBy = 'WalletConnect';
        setText('wallet-status', `Wallet: ${WALLET_STATE.connectedBy} (${WALLET_STATE.account || 'connected'})`);
        setAssistant('WalletConnect session active. Verify network before executing any transaction.');
    } catch (e) {
        setText('wallet-status', `Wallet: WalletConnect failed (${e.message || 'unknown'})`);
    }
}

async function disconnectWallet() {
    try {
        if (WALLET_STATE.provider && typeof WALLET_STATE.provider.disconnect === 'function') {
            await WALLET_STATE.provider.disconnect();
        }
    } catch (_) {}
    WALLET_STATE.provider = null;
    WALLET_STATE.web3 = null;
    WALLET_STATE.account = null;
    WALLET_STATE.connectedBy = null;
    setText('wallet-status', 'Wallet: disconnected');
    setAssistant('Wallet disconnected. Read-only mode is active.');
}

function switchSwap() {
    const inChain = byId('swap-chain-in');
    const outChain = byId('swap-chain-out');
    const inTok = byId('swap-token-in');
    const outTok = byId('swap-token-out');
    if (!inChain || !outChain || !inTok || !outTok) return;

    const tmpChain = inChain.value;
    const tmpToken = inTok.value;
    inChain.value = outChain.value;
    outChain.value = tmpChain;
    refreshSwapContracts();
    inTok.value = outTok.value;
    outTok.value = tmpToken;
    refreshSwapContracts();
}

async function executeBestRouteSwap() {
    const out = byId('swap-result');
    if (out) out.textContent = 'Analyzing best route...';

    const chainIn = byId('swap-chain-in')?.value || '';
    const chainOut = byId('swap-chain-out')?.value || '';
    const tokenIn = byId('swap-token-in')?.value || '';
    const tokenOut = byId('swap-token-out')?.value || '';
    const amountIn = Number(byId('swap-amount-in')?.value || 0);
    const rawEl = byId('swap-raw');

    if (!chainIn || !chainOut || !tokenIn || !tokenOut || amountIn <= 0) {
        if (out) out.textContent = 'Please select chains/tokens and enter amount.';
        return;
    }

    if (chainIn !== chainOut) {
        const res = await postJson('/api/swap/route-crosschain', {
            chain_in: chainIn,
            chain_out: chainOut,
            token_in: tokenIn,
            token_out: tokenOut,
            amount_in: amountIn
        });
        if (res.ok) {
            setText('swap-quote-out', 'bridge route prepared');
            setText('swap-route', `${getChainCfg(chainIn).label} -> BRIDGE -> ${getChainCfg(chainOut).label}`);
            if (out) out.textContent = 'Cross-chain route prepared. Follow listed execution steps.';
            if (rawEl) rawEl.textContent = JSON.stringify(res, null, 2);
            setAssistant('Cross-chain plan generated. Security tip: validate destination chain + contract before signing.');
        } else {
            if (out) out.textContent = `Error: ${res.error || 'route generation failed'}`;
            if (rawEl) rawEl.textContent = JSON.stringify(res, null, 2);
            setAssistant('Cross-chain planning failed. Verify chain/token selection and backend health.');
        }
        return;
    }

    const res = await postJson('/api/swap/quote', {
        token_in: tokenIn,
        token_out: tokenOut,
        amount_in: amountIn
    });

    if (res.ok && res.data) {
        setText('swap-quote-out', res.data.amount_out || '0.00');
        setText('swap-route', res.data.route || 'direct');
        if (out) out.textContent = `Best route ready: ${res.data.route || 'direct'}`;
        if (rawEl) rawEl.textContent = res.raw || JSON.stringify(res.data, null, 2);
        setAssistant('Quote generated. Verify slippage and wallet network before signing.');
    } else {
        if (out) out.textContent = `Error: ${res.error || res.raw || 'backend unavailable'}`;
        if (rawEl) rawEl.textContent = res.error || res.raw || 'No response';
        setAssistant('Quote failed. Check backend health endpoint and daemon connection.');
    }
}

async function ensureWalletChain(chainKey) {
    if (!window.ethereum) throw new Error('MetaMask not available');
    const cfg = getChainCfg(chainKey);
    if (!cfg || !cfg.chain_id_hex) throw new Error('Selected chain is non-EVM or unsupported for wallet add');

    try {
        await window.ethereum.request({
            method: 'wallet_switchEthereumChain',
            params: [{ chainId: cfg.chain_id_hex }]
        });
    } catch (err) {
        if (err?.code === 4902) {
            await window.ethereum.request({
                method: 'wallet_addEthereumChain',
                params: [{
                    chainId: cfg.chain_id_hex,
                    chainName: cfg.label,
                    rpcUrls: cfg.rpc_urls,
                    nativeCurrency: cfg.native_currency,
                    blockExplorerUrls: cfg.block_explorer_urls
                }]
            });
        } else {
            throw err;
        }
    }
}

async function addAddTokenToWallet() {
    const out = byId('wallet-add-result');
    if (!window.ethereum) {
        if (out) out.textContent = 'MetaMask required for token import.';
        return;
    }

    const chainKey = byId('wallet-token-chain')?.value || '';
    const tokenAddress = byId('wallet-token-contract')?.value?.trim() || '';
    const decimals = Number(byId('wallet-token-decimals')?.value || 18);

    try {
        await ensureWalletChain(chainKey);
        if (!tokenAddress || !/^0x[a-fA-F0-9]{40}$/.test(tokenAddress)) {
            throw new Error('Invalid EVM contract address for ADD token');
        }

        const wasAdded = await window.ethereum.request({
            method: 'wallet_watchAsset',
            params: {
                type: 'ERC20',
                options: {
                    address: tokenAddress,
                    symbol: 'ADD',
                    decimals,
                    image: `${window.location.origin}/assets/logo-transparent.png`
                }
            }
        });

        if (out) out.textContent = wasAdded ? 'ADD token added successfully.' : 'Token add request was rejected.';
    } catch (e) {
        if (out) out.textContent = `Failed: ${e.message || 'unknown error'}`;
    }
}

function initExchangeUI() {
    const chainIn = byId('swap-chain-in');
    const chainOut = byId('swap-chain-out');
    if (!chainIn || !chainOut) return;

    populateChainSelect('swap-chain-in', 'ethereum');
    populateChainSelect('swap-chain-out', 'addition');
    refreshSwapContracts();
    populateChainSelect('wallet-token-chain', 'ethereum');
}

async function loadCatalogFromBackend() {
    const res = await getJson('/swap/catalog');
    if (res.ok && res.catalog && typeof res.catalog === 'object') {
        CHAIN_TOKEN_CATALOG = res.catalog;
    }
}

function bindButtons() {
    byId('btn-swap-switch')?.addEventListener('click', switchSwap);
    byId('btn-swap-quote')?.addEventListener('click', executeBestRouteSwap);
    byId('btn-connect-metamask')?.addEventListener('click', connectMetaMask);
    byId('btn-connect-walletconnect')?.addEventListener('click', connectWalletConnect);
    byId('btn-disconnect-wallet')?.addEventListener('click', disconnectWallet);
    byId('btn-add-add-token')?.addEventListener('click', addAddTokenToWallet);

    byId('swap-chain-in')?.addEventListener('change', refreshSwapContracts);
    byId('swap-chain-out')?.addEventListener('change', refreshSwapContracts);
    byId('swap-token-in')?.addEventListener('change', refreshSwapContracts);
    byId('swap-token-out')?.addEventListener('change', refreshSwapContracts);
}

window.addEventListener('load', () => {
    (async () => {
        showApiDebugStatus();
        await loadCatalogFromBackend();
        initExchangeUI();
        bindButtons();
        setInterval(refresh, 5000);
        refresh();
    })();
});

window.addEventListener('error', (e) => {
    const hs = byId('health-status');
    if (hs) {
        hs.textContent = 'UI ERROR';
        hs.className = 'status-danger';
    }
    const kpiHealth = byId('kpi-health');
    if (kpiHealth) {
        kpiHealth.textContent = 'UI ERROR';
        kpiHealth.className = 'status-danger';
    }
    const raw = byId('network-raw');
    if (raw) {
        raw.textContent = `Frontend error: ${e.message || 'unknown'}`;
    }
});
