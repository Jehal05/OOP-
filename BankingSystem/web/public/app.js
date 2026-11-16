/* Nova Bank - browser UI. Plain JavaScript, no libraries. All text from the server is inserted with
   textContent (via el()), never innerHTML, so data cannot inject markup. */
'use strict';

// ------------------------------------------------------------------ helpers
const inr = new Intl.NumberFormat('en-IN', { style: 'currency', currency: 'INR', minimumFractionDigits: 2 });
const money = v => inr.format(Number(v));
const num = v => new Intl.NumberFormat('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 }).format(Number(v));

function el(tag, props, ...kids) {
  const n = document.createElement(tag);
  for (const [k, v] of Object.entries(props || {})) {
    if (v == null || v === false) continue;
    if (k === 'class') n.className = v;
    else if (k === 'style') Object.assign(n.style, v);
    else if (k.startsWith('on')) n[k] = v;
    else if (k === 'text') n.textContent = v;
    else n.setAttribute(k, v === true ? '' : v);
  }
  const add = k => {
    if (k == null || k === false) return;
    if (Array.isArray(k)) k.forEach(add);
    else n.append(k instanceof Node ? k : document.createTextNode(String(k)));
  };
  kids.forEach(add);
  return n;
}
// replaceChildren() would stringify an array, so flatten first
const put = (node, ...kids) => node.replaceChildren(...kids.flat(Infinity).filter(k => k != null && k !== false));
const svgEl = (tag, attrs) => {
  const n = document.createElementNS('http://www.w3.org/2000/svg', tag);
  for (const [k, v] of Object.entries(attrs || {})) n.setAttribute(k, v);
  return n;
};

const state = { user: null, unseen: 0, route: '', prefill: null };

async function api(path, opts = {}) {
  const init = { method: opts.method || 'GET', headers: {}, credentials: 'same-origin' };
  if (opts.body) { init.method = 'POST'; init.headers['Content-Type'] = 'application/json'; init.body = JSON.stringify(opts.body); }
  else if (init.method === 'POST') { init.headers['Content-Type'] = 'application/json'; init.body = '{}'; }
  let res;
  try { res = await fetch(path, init); } catch (e) { throw new Error('Cannot reach the bank server. Is it running?'); }
  let data = {};
  try { data = await res.json(); } catch (e) { /* non-JSON body */ }
  if (res.status === 401 && state.user && path !== '/api/login') { state.user = null; showLogin('Your session ended. Please log in again.'); throw new Error('session'); }
  if (!res.ok) { const err = new Error(data.error || 'Request failed'); err.status = res.status; throw err; }
  return data;
}
const post = (p, body) => api(p, { method: 'POST', body: body || {} });

function toast(msg, kind = '') {
  const t = el('div', { class: 'toast ' + kind, role: 'status' }, (kind === 'ok' ? '✔ ' : kind === 'err' ? '✖ ' : kind === 'warn' ? '⚠ ' : '') + msg);
  document.getElementById('toasts').append(t);
  setTimeout(() => t.remove(), kind === 'err' ? 6500 : 4200);
}
const fail = e => { if (e.message !== 'session') toast(e.message, 'err'); };

function modal(title, body, opts = {}) {
  const close = () => { ov.remove(); document.removeEventListener('keydown', onKey); };
  const onKey = e => { if (e.key === 'Escape') close(); };
  const ov = el('div', { class: 'overlay', onclick: e => { if (e.target === ov) close(); } },
    el('div', { class: 'modal' + (opts.wide ? ' wide' : ''), role: 'dialog', 'aria-modal': 'true', 'aria-label': title },
      el('h2', {}, title), body));
  document.addEventListener('keydown', onKey);
  document.body.append(ov);
  return { close, ov };
}
function confirmBox(title, lines, okText = 'Confirm') {      // Y/N guard, like the console app
  return new Promise(res => {
    let m;
    const done = v => { m.close(); res(v); };
    m = modal(title, el('div', {},
      el('dl', { class: 'kv' }, lines.map(([k, v]) => [el('dt', {}, k), el('dd', {}, v)])),
      el('div', { class: 'actions', style: { marginTop: '20px', justifyContent: 'flex-end' } },
        el('button', { class: 'btn ghost', onclick: () => done(false) }, 'Cancel'),
        el('button', { class: 'btn teal', onclick: () => done(true) }, okText))));
  });
}
function askAmount(title, label, hint) {
  return new Promise(res => {
    const inp = el('input', { inputmode: 'decimal', placeholder: '0.00', 'aria-label': label });
    let m;
    const go = () => { m.close(); res(inp.value.trim()); };
    m = modal(title, el('div', { class: 'form' }, el('label', { class: 'f' }, label, inp), hint && el('div', { class: 'hint' }, hint),
      el('div', { class: 'actions', style: { justifyContent: 'flex-end' } },
        el('button', { class: 'btn ghost', onclick: () => { m.close(); res(null); } }, 'Cancel'),
        el('button', { class: 'btn teal', onclick: go }, 'Continue'))));
    inp.onkeydown = e => { if (e.key === 'Enter') go(); };
    setTimeout(() => inp.focus(), 30);
  });
}
function receipt(tx, balance, heading = 'Transaction successful') {
  const body = el('div', {},
    el('div', { class: 'banner ok' }, '✔ ' + heading),
    el('div', { class: 'receipt' },
      el('div', { class: 'muted small' }, tx.type + (tx.category !== '-' ? ' · ' + tx.category : '')),
      el('div', { class: 'amount ' + (tx.credit ? 'pos' : 'neg') }, (tx.credit ? '+ ' : '− ') + money(tx.amount)),
      el('dl', { class: 'kv', style: { marginTop: '12px' } },
        el('dt', {}, 'Reference'), el('dd', { class: 'ref' }, tx.ref),
        el('dt', {}, 'Date'), el('dd', {}, tx.time),
        el('dt', {}, 'Details'), el('dd', {}, tx.note),
        el('dt', {}, 'Balance after'), el('dd', {}, money(tx.balanceAfter)))),
    el('div', { class: 'actions no-print', style: { justifyContent: 'flex-end' } },
      el('button', { class: 'btn ghost', onclick: () => window.print() }, 'Print'),
      el('button', { class: 'btn', onclick: () => m.close() }, 'Done')));
  const m = modal('Receipt', body);
}
const badge = (text, kind = '') => el('span', { class: 'badge ' + kind }, text);
const progress = (frac, done) => {
  const bar = el('span', {});
  requestAnimationFrame(() => { bar.style.width = Math.max(0, Math.min(1, frac)) * 100 + '%'; });
  bar.style.width = '0%';
  return el('div', { class: 'progress' + (done ? ' done' : ''), role: 'progressbar', 'aria-valuenow': Math.round(frac * 100), 'aria-valuemin': 0, 'aria-valuemax': 100 }, bar);
};
function table(heads, rows, aligns = [], rowClass) {
  return el('div', { class: 'tbl-wrap' }, el('table', {},
    el('thead', {}, el('tr', {}, heads.map((h, i) => el('th', { class: aligns[i] === 'r' ? 'r' : '' }, h)))),
    el('tbody', {}, rows.map((r, ri) => el('tr', { class: rowClass ? rowClass(ri) : '' },
      r.map((c, i) => el('td', { class: aligns[i] === 'r' ? 'r' : '' }, c)))))));
}
const stat = (label, value, cls = '') => el('div', { class: 'card stat' }, el('div', { class: 'label' }, label), el('div', { class: 'value num ' + cls }, value));
const empty = msg => el('div', { class: 'empty' }, msg);
function field(label, input, hint) { return el('label', { class: 'f' }, label, input, hint && el('span', { class: 'hint' }, hint)); }
const CATS = ['Food', 'Bills', 'Travel', 'Shopping', 'Rent', 'Health', 'Education', 'Other'];
const TYPES = ['Deposit', 'Withdraw', 'Transfer Out', 'Transfer In', 'Interest', 'Loan Credit', 'EMI Payment', 'Goal Saving', 'Fee'];
const select = (opts, val, props) => el('select', props || {}, opts.map(o => { const [v, t] = Array.isArray(o) ? o : [o, o]; return el('option', { value: v, selected: v === val }, t); }));
const txCell = t => el('span', { class: t.credit ? 'pos' : 'neg' }, (t.credit ? '+ ' : '− ') + num(t.amount));

async function refreshBadge() {
  if (!state.user || state.user.role !== 'CUSTOMER') return;
  try { const d = await api('/api/dashboard'); state.unseen = Number(d.unseenAlerts); } catch (e) { return; }
  const c = document.querySelector('.nav .count');
  if (c) { c.textContent = state.unseen; c.classList.toggle('hidden', !state.unseen); }
}
async function afterMoney() {           // surface fraud alerts raised by the operation that just happened
  const before = state.unseen;
  await refreshBadge();
  if (state.unseen > before) toast('A security alert was raised on this transaction. Review it under Security.', 'warn');
}

// ------------------------------------------------------------------ login
function showLogin(notice) {
  state.user = null;
  const app = document.getElementById('app');
  let role = 'customer';
  const err = el('div', { class: 'banner err hidden', role: 'alert' });
  const idIn = el('input', { autocomplete: 'username', placeholder: 'e.g. SB1001', required: true, 'aria-label': 'Account number' });
  const pinIn = el('input', { type: 'password', autocomplete: 'current-password', placeholder: 'Enter your PIN', required: true, 'aria-label': 'PIN' });
  const idLab = el('span', {}, 'Account number'), pinLab = el('span', {}, 'PIN');
  const tabs = el('div', { class: 'tabs', role: 'tablist' });
  const setRole = r => {
    role = r;
    tabs.querySelectorAll('button').forEach(b => b.setAttribute('aria-selected', String(b.dataset.r === r)));
    idLab.textContent = r === 'admin' ? 'Admin ID' : 'Account number';
    pinLab.textContent = r === 'admin' ? 'Password' : 'PIN';
    idIn.placeholder = r === 'admin' ? 'admin' : 'e.g. SB1001';
    pinIn.placeholder = r === 'admin' ? 'Enter your password' : 'Enter your PIN';
    demo.replaceChildren(...demoChips(r));
  };
  const demoChips = r => (r === 'admin' ? [['admin', 'admin123']] : [['SB1001', '1234'], ['CA2001', '2345'], ['FD3001', '3456'], ['ST1001', '4567']])
    .map(([i, p]) => el('button', { type: 'button', class: 'chip', onclick: () => { idIn.value = i; pinIn.value = p; } }, i + ' / ' + p));
  const demo = el('div', { class: 'chips' });
  ['customer', 'admin'].forEach(r => tabs.append(el('button', { type: 'button', role: 'tab', 'data-r': r, onclick: () => setRole(r) }, r === 'admin' ? 'Admin / Staff' : 'Customer')));
  const form = el('form', { class: 'form', onsubmit: async e => {
    e.preventDefault(); err.classList.add('hidden');
    try {
      const u = await post('/api/login', { role, id: idIn.value, pin: pinIn.value });
      state.user = u; location.hash = u.role === 'ADMIN' ? '#/admin' : '#/dashboard'; render();
    } catch (ex) { err.textContent = '✖ ' + ex.message; err.classList.remove('hidden'); pinIn.value = ''; pinIn.focus(); }
  } }, field(idLab, idIn), field(pinLab, pinIn), err, el('button', { class: 'btn teal', type: 'submit' }, 'Log in securely'));
  app.replaceChildren(el('div', { class: 'login-wrap' },
    el('section', { class: 'login-hero' },
      el('div', { class: 'brand' }, el('span', { class: 'brand-mark' }, '◆'), 'Nova Bank'),
      el('div', {}, el('h1', {}, 'Banking that watches your back.'),
        el('p', {}, 'Transfers, goals, loans and live fraud screening, running on the same C++ engine as the console app.'),
        el('div', { class: 'hero-points' }, el('div', {}, '✔ 5 polymorphic fraud rules on every transaction'), el('div', {}, '✔ 3-attempt PIN lockout and an append-only audit log'),
          el('div', {}, '✔ EMI loans with full amortization tables'))),
      el('div', { class: 'small', style: { opacity: .7 } }, 'OOP mini project · C++17 · localhost only')),
    el('section', { class: 'login-panel' }, el('div', { class: 'login-card' },
      el('h2', {}, 'Welcome back'), el('div', { class: 'muted small' }, 'Log in to continue'),
      notice && el('div', { class: 'banner warn', style: { marginTop: '12px' } }, '⚠ ' + notice), tabs, form,
      el('div', { class: 'demo' }, el('div', { class: 'small muted' }, 'Demo logins (click to fill)'), demo)))));
  setRole('customer');
}

// ------------------------------------------------------------------ shell
const CUSTOMER_NAV = [['dashboard', '▦', 'Dashboard'], ['pay', '⇄', 'Payments'], ['statement', '☰', 'Statement'], ['spending', '%', 'Spending'],
  ['goals', '◎', 'Savings goals'], ['loans', '₹', 'Loans & EMI'], ['payees', '☺', 'Payees'], ['simulator', '↗', 'Interest simulator'], ['security', '⛨', 'Security']];
const ADMIN_NAV = [['admin', '▦', 'Overview'], ['accounts', '☰', 'Accounts'], ['aloans', '₹', 'Loan approvals'], ['afraud', '⚠', 'Fraud alerts'],
  ['alocked', '⛨', 'Locked users'], ['reports', '%', 'Reports'], ['audit', '✎', 'Audit log']];

function shell(title, crumb, content) {
  const nav = state.user.role === 'ADMIN' ? ADMIN_NAV : CUSTOMER_NAV;
  const cur = state.route;
  return el('div', { class: 'shell' },
    el('aside', { class: 'side' },
      el('div', { class: 'brand' }, el('span', { class: 'brand-mark' }, '◆'), 'Nova Bank'),
      el('nav', { class: 'nav', 'aria-label': 'Main' }, nav.map(([r, ic, t]) =>
        el('a', { href: '#/' + r, 'aria-current': r === cur ? 'page' : null }, el('span', { class: 'ico' }, ic), t,
          r === 'security' && el('span', { class: 'count' + (state.unseen ? '' : ' hidden') }, state.unseen)))),
      el('div', { class: 'user' }, el('b', {}, state.user.name), el('span', { class: 'small' }, state.user.role === 'ADMIN' ? 'Administrator' : 'Account ' + state.user.accNo),
        el('button', { onclick: async () => { try { await post('/api/logout'); } catch (e) {} showLogin(); } }, 'Log out'))),
    el('main', { class: 'main' }, el('div', { class: 'page-head' }, el('div', {}, el('div', { class: 'crumb' }, 'Home › ' + crumb), el('h1', {}, title))), content));
}
function mount(title, crumb, content) { document.getElementById('app').replaceChildren(shell(title, crumb, content)); window.scrollTo(0, 0); }
const loading = () => el('div', { class: 'spinner' }, 'Loading…');

// ------------------------------------------------------------------ customer pages
function sparkline(vals) {
  const s = svgEl('svg', { class: 'spark', viewBox: '0 0 220 70', 'aria-hidden': 'true' });
  const v = vals.map(Number);
  if (v.length < 2) return s;
  const mn = Math.min(...v), mx = Math.max(...v), rng = mx - mn || 1;
  const pts = v.map((y, i) => [i * 220 / (v.length - 1), 62 - ((y - mn) / rng) * 54]);
  s.append(svgEl('polyline', { points: pts.map(p => p.join(',')).join(' '), fill: 'none', stroke: '#7ff0da', 'stroke-width': 2.5, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' }));
  const last = pts[pts.length - 1];
  s.append(svgEl('circle', { cx: last[0], cy: last[1], r: 4, fill: '#fff' }));
  return s;
}
function txTable(rows) {
  if (!rows.length) return empty('No transactions to show.');
  return table(['Date', 'Reference', 'Details', 'Category', 'Amount', 'Balance'],
    rows.map(t => [el('span', { style: { whiteSpace: 'nowrap' } }, t.time), el('span', { class: 'small', style: { fontFamily: 'ui-monospace,monospace' } }, t.ref), t.note, t.type + (t.category !== '-' ? ' · ' + t.category : ''), txCell(t), num(t.balanceAfter)]),
    ['', '', '', '', 'r', 'r']);
}

async function pageDashboard() {
  mount('Dashboard', 'Customer', loading());
  const d = await api('/api/dashboard');
  state.unseen = Number(d.unseenAlerts);
  const a = d.account;
  mount('Hello, ' + d.name.split(' ')[0], 'Customer › Dashboard', el('div', { class: 'stack' },
    state.unseen > 0 && el('div', { class: 'banner warn' }, '⚠ You have ' + state.unseen + ' unread security alert(s). ', el('a', { href: '#/security' }, 'Review now')),
    el('div', { class: 'card hero-card' },
      el('div', { class: 'label' }, a.type + ' account · ' + a.accNo), el('div', { class: 'balance num' }, money(a.balance)),
      el('div', { class: 'meta' }, el('span', {}, 'Available: ' + money(a.spendable)), el('span', {}, 'Opened ' + a.opened), el('span', {}, a.rules)), sparkline(d.trend)),
    el('div', { class: 'grid g4' }, stat('Money in (this month)', money(d.monthIn), 'pos'), stat('Money out (this month)', money(d.monthOut), 'neg'),
      stat('Savings goals', d.goals), stat('Active loans', d.activeLoans)),
    el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, 'Recent activity'), el('a', { class: 'btn ghost sm', href: '#/statement' }, 'Full statement')), txTable(d.recent)),
    el('div', { class: 'actions' }, el('a', { class: 'btn teal', href: '#/pay' }, 'Send money'), el('a', { class: 'btn ghost', href: '#/goals' }, 'Savings goals'), el('a', { class: 'btn ghost', href: '#/loans' }, 'Check EMI'))));
}

async function pageSecurity() {
  mount('Security & alerts', 'Customer › Security', loading());
  const alerts = await api('/api/alerts');
  const old = state.unseen;
  const oldPin = el('input', { type: 'password', inputmode: 'numeric', maxlength: 4, autocomplete: 'current-password' });
  const newPin = el('input', { type: 'password', inputmode: 'numeric', maxlength: 4, autocomplete: 'new-password' });
  const confirmPin = el('input', { type: 'password', inputmode: 'numeric', maxlength: 4, autocomplete: 'new-password' });
  const list = alerts.length ? alerts.map(a => el('div', { class: 'banner ' + (a.seen ? 'info' : 'warn') },
    el('div', {}, el('b', {}, a.rule + ' '), !a.seen && badge('NEW', 'amber'), el('div', {}, a.message), el('div', { class: 'small', style: { opacity: .8 } }, a.time + (a.ref ? ' · ' + a.ref : '') + (a.reviewed ? ' · reviewed by bank' : ''))))) :
    [el('div', { class: 'banner ok' }, '✔ No alerts. Your account looks healthy.')];
  mount('Security & alerts', 'Customer › Security', el('div', { class: 'grid g-main' },
    el('div', { class: 'card' }, el('h2', {}, 'Fraud alerts'), list,
      el('div', { class: 'small muted' }, 'Rules checked on every transaction: large amount, rapid transactions, new payee high value, odd hours, repeated failures.')),
    el('div', { class: 'card' }, el('h2', {}, 'Change PIN'), el('form', { class: 'form', onsubmit: async e => {
      e.preventDefault();
      if (newPin.value !== confirmPin.value) return toast('The new PINs do not match.', 'err');
      try { await post('/api/pin', { oldPin: oldPin.value, newPin: newPin.value }); toast('PIN changed.', 'ok'); [oldPin, newPin, confirmPin].forEach(i => i.value = ''); } catch (ex) { fail(ex); }
    } }, field('Current PIN', oldPin), field('New PIN (4 digits)', newPin), field('Confirm new PIN', confirmPin), el('button', { class: 'btn', type: 'submit' }, 'Update PIN')),
      el('p', { class: 'small muted' }, 'Three wrong PINs at login lock the account. Only an administrator can unlock it.'))));
  if (old > 0) { try { await post('/api/alerts/seen'); } catch (e) {} state.unseen = 0; const c = document.querySelector('.nav .count'); if (c) c.classList.add('hidden'); }
}

function pagePay() {
  let mode = (state.prefill && state.prefill.payee) ? 'transfer' : 'deposit';
  const holder = el('div', {});
  const draw = async () => {
    const tabs = el('div', { class: 'seg', role: 'tablist' }, [['deposit', 'Deposit'], ['withdraw', 'Withdraw'], ['transfer', 'Transfer']].map(([m, t]) =>
      el('button', { role: 'tab', 'aria-selected': String(m === mode), onclick: () => { mode = m; draw(); } }, t)));
    let body;
    if (mode === 'deposit') body = depositForm(); else if (mode === 'withdraw') body = withdrawForm(); else body = await transferForm();
    put(holder, tabs, el('div', { style: { marginTop: '20px' } }, body));
  };
  mount('Payments', 'Customer › Payments', el('div', { class: 'card', style: { maxWidth: '640px' } }, holder));
  draw();
}
const amountInput = () => el('input', { inputmode: 'decimal', placeholder: '0.00', required: true, 'aria-label': 'Amount' });
function depositForm() {
  const amt = amountInput(), note = el('input', { maxlength: 60, placeholder: 'Cash deposit' });
  return el('form', { class: 'form', onsubmit: async e => {
    e.preventDefault();
    try { const r = await post('/api/deposit', { amount: amt.value, note: note.value }); receipt(r.tx, r.balance, 'Deposit successful'); amt.value = ''; await afterMoney(); } catch (ex) { fail(ex); }
  } }, field('Amount (₹)', amt), field('Note (optional)', note), el('button', { class: 'btn teal', type: 'submit' }, 'Deposit'));
}
function withdrawForm() {
  const amt = amountInput(), cat = select(CATS, 'Other'), note = el('input', { maxlength: 60, placeholder: 'Optional' });
  return el('form', { class: 'form', onsubmit: async e => {
    e.preventDefault();
    try {
      const fee = Number((await api('/api/fdfee?amount=' + encodeURIComponent(amt.value))).fee);
      const lines = [['Amount', money(amt.value)], ['Category', cat.value]];
      if (fee > 0) lines.push(['Early-withdrawal penalty', money(fee)], ['Total debit', money(Number(amt.value) + fee)]);
      if (!await confirmBox('Confirm withdrawal', lines, 'Withdraw')) return;
      const r = await post('/api/withdraw', { amount: amt.value, category: cat.value, note: note.value });
      receipt(r.tx, r.balance, 'Withdrawal successful'); amt.value = ''; await afterMoney();
    } catch (ex) { fail(ex); }
  } }, el('div', { class: 'row' }, field('Amount (₹)', amt), field('Category', cat)), field('Note (optional)', note),
    el('div', { class: 'hint' }, 'Fixed-deposit accounts show any early-withdrawal penalty before the money moves.'), el('button', { class: 'btn teal', type: 'submit' }, 'Review withdrawal'));
}
async function transferForm() {
  const payees = await api('/api/payees');
  const pre = state.prefill && state.prefill.payee; state.prefill = null;
  const sel = select([['', 'New recipient (enter account number)'], ...payees.map(p => [p.nickname, p.nickname + ' · ' + p.name + ' (' + p.acc + ')'])], pre || '');
  const acc = el('input', { placeholder: 'e.g. CA2001', autocomplete: 'off' }), who = el('div', { class: 'hint' });
  const amt = amountInput(), cat = select(CATS, 'Other'), note = el('input', { maxlength: 60, placeholder: 'Optional' });
  const sync = () => { acc.disabled = !!sel.value; if (sel.value) { acc.value = ''; who.textContent = ''; } };
  sel.onchange = sync; sync();
  acc.onblur = async () => {
    if (!acc.value.trim()) { who.textContent = ''; return; }
    try { const r = await api('/api/lookup?acc=' + encodeURIComponent(acc.value)); who.textContent = '✔ ' + r.holder + ' (' + r.masked + ')'; who.style.color = 'var(--green)'; }
    catch (e) { who.textContent = '✖ ' + e.message; who.style.color = 'var(--red)'; }
  };
  return el('form', { class: 'form', onsubmit: async e => {
    e.preventDefault();
    try {
      const body = { amount: amt.value, category: cat.value, note: note.value };
      let label;
      if (sel.value) { body.payee = sel.value; label = sel.options[sel.selectedIndex].text; }
      else { const r = await api('/api/lookup?acc=' + encodeURIComponent(acc.value)); body.to = r.accNo; label = r.holder + ' (' + r.masked + ')'; }
      if (!await confirmBox('Confirm transfer', [['To', label], ['Amount', money(amt.value)], ['Category', cat.value], ['Note', note.value || '-']], 'Send money')) return;
      const r = await post('/api/transfer', body);
      receipt(r.tx, r.balance, 'Transfer successful'); amt.value = ''; await afterMoney();
    } catch (ex) { fail(ex); }
  } }, field('Send to', sel), field('Account number', acc), who, el('div', { class: 'row' }, field('Amount (₹)', amt), field('Category', cat)),
    field('Note (optional)', note), el('button', { class: 'btn teal', type: 'submit' }, 'Review transfer'));
}

async function pageStatement() {
  const f = { from: '', to: '', type: '', category: '', min: '', max: '', sort: 'date', dir: 'desc' };
  const out = el('div', {});
  const qs = () => Object.entries(f).filter(([, v]) => v !== '').map(([k, v]) => k + '=' + encodeURIComponent(v)).join('&');
  const inp = (k, props) => { const i = el('input', Object.assign({ value: f[k], onchange: () => { f[k] = i.value; } }, props)); return i; };
  const sel = (k, opts) => { const s = select(opts, f[k], { onchange: () => { f[k] = s.value; } }); return s; };
  const load = async () => {
    put(out, loading());
    try {
      const r = await api('/api/statement?' + qs());
      put(out, 
        el('div', { class: 'grid g4', style: { marginBottom: '18px' } }, stat('Credits', money(r.summary.credits), 'pos'), stat('Debits', money(r.summary.debits), 'neg'),
          stat('Net', money(r.summary.net), Number(r.summary.net) >= 0 ? 'pos' : 'neg'), stat('Largest', money(r.summary.largest))),
        el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, r.count + ' transaction(s)'), el('span', { class: 'small muted' }, 'Filters: ' + r.filters)),
          txTable(r.rows), Number(r.count) > 500 && el('div', { class: 'small muted' }, 'Showing the first 500 rows.')));
    } catch (ex) { put(out, el('div', { class: 'banner err' }, '✖ ' + ex.message)); }
  };
  mount('Smart statement', 'Customer › Statement', el('div', { class: 'stack' },
    el('div', { class: 'card' }, el('form', { class: 'form', onsubmit: e => { e.preventDefault(); load(); } },
      el('div', { class: 'row' }, field('From', inp('from', { type: 'date' })), field('To', inp('to', { type: 'date' })),
        field('Type', sel('type', [['', 'All types'], ...TYPES])), field('Category', sel('category', [['', 'All categories'], ...CATS]))),
      el('div', { class: 'row' }, field('Min amount', inp('min', { inputmode: 'decimal' })), field('Max amount', inp('max', { inputmode: 'decimal' })),
        field('Sort by', sel('sort', [['date', 'Date'], ['amount', 'Amount']])), field('Order', sel('dir', [['desc', 'Newest / largest first'], ['asc', 'Oldest / smallest first']]))),
      el('div', { class: 'actions' }, el('button', { class: 'btn teal', type: 'submit' }, 'Apply filters'),
        el('button', { class: 'btn ghost', type: 'button', onclick: () => location.reload() }, 'Reset'),
        el('a', { class: 'btn ghost', href: '#', onclick: e => { e.preventDefault(); window.location = '/api/statement.txt?' + qs(); } }, '⬇ Export .txt')))), out));
  load();
}

async function pageSpending() {
  const months = [['', 'All time']]; const d = new Date();
  for (let i = 0; i < 6; i++) { const m = new Date(d.getFullYear(), d.getMonth() - i, 1); const k = m.getFullYear() + '-' + String(m.getMonth() + 1).padStart(2, '0'); months.push([k, m.toLocaleString('en', { month: 'long', year: 'numeric' })]); }
  const out = el('div', {});
  const load = async m => {
    const r = await api('/api/spending?month=' + m);
    if (!r.slices.length) return put(out, empty('No spending in this period.'));
    put(out, el('div', { class: 'muted', style: { marginBottom: '8px' } }, 'Total spent: ', el('b', { class: 'num' }, money(r.total))),
      r.slices.map((s, i) => el('div', { class: 'bar-row' + (i === 0 ? ' top' : '') }, el('div', {}, (i === 0 ? '★ ' : '') + s.category), progress(Number(s.percent) / 100),
        el('div', { class: 'amt' }, money(s.total) + '  ' + Number(s.percent).toFixed(1) + '%'))),
      el('div', { class: 'small muted', style: { marginTop: '8px' } }, '★ Highest spending category'));
  };
  const sel = select(months, '', { onchange: () => load(sel.value) });
  mount('Spending categories', 'Customer › Spending', el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, 'Where your money goes'), sel), out));
  load('');
}

async function pageGoals() {
  const out = el('div', {});
  const load = async () => {
    const goals = await api('/api/goals');
    put(out, goals.length ? el('div', { class: 'grid g2' }, goals.map(g => {
      const done = g.status === 'Achieved';
      return el('div', { class: 'card goal' }, el('h3', {}, g.name, badge(g.status, done ? 'green' : g.status === 'Overdue' ? 'red' : '')),
        progress(Number(g.fraction), done),
        el('div', { class: 'small', style: { margin: '8px 0 12px' } }, el('b', { class: 'num' }, money(g.saved)), ' of ', money(g.target), '  ·  ' + Math.round(g.fraction * 100) + '%'),
        el('dl', { class: 'kv' }, el('dt', {}, 'Deadline'), el('dd', {}, g.deadline), !done && [el('dt', {}, 'Save per month'), el('dd', {}, money(g.monthly))]),
        !done && el('div', { class: 'actions', style: { marginTop: '14px' } }, el('button', { class: 'btn teal sm', onclick: async () => {
          const v = await askAmount('Add to “' + g.name + '”', 'Amount (₹)', 'Moves money from your account into this goal.');
          if (!v) return;
          try { const r = await post('/api/goals/contribute', { id: g.id, amount: v }); toast('Added ' + money(v) + ' to ' + g.name, 'ok'); receipt(r.tx, r.balance, 'Goal contribution'); load(); await afterMoney(); } catch (ex) { fail(ex); }
        } }, 'Add money')));
    })) : empty('No goals yet. Create your first one below.'));
  };
  const name = el('input', { maxlength: 24, placeholder: 'e.g. Laptop', required: true });
  const target = amountInput();
  const dl = el('input', { type: 'date', required: true });
  mount('Savings goals', 'Customer › Goals', el('div', { class: 'stack' }, out, el('div', { class: 'card' }, el('h2', {}, 'New goal'),
    el('form', { class: 'form', onsubmit: async e => {
      e.preventDefault();
      try { await post('/api/goals', { name: name.value, target: target.value, deadline: dl.value }); toast('Goal created.', 'ok'); name.value = target.value = dl.value = ''; load(); } catch (ex) { fail(ex); }
    } }, el('div', { class: 'row' }, field('Goal name', name), field('Target (₹)', target), field('Deadline', dl)), el('button', { class: 'btn teal', type: 'submit', style: { justifySelf: 'start' } }, 'Create goal')))));
  load();
}

async function pageLoans() {
  const LT = [['HOME', 'Home loan (8.5%, up to ₹50 L, 240 m)'], ['EDU', 'Education loan (9.5%, up to ₹15 L, 120 m)'], ['PER', 'Personal loan (12.5%, up to ₹5 L, 60 m)']];
  const type = select(LT, 'PER'), prin = el('input', { inputmode: 'decimal', value: '100000' }), months = el('input', { inputmode: 'numeric', value: '24' });
  const calc = el('div', {}), mine = el('div', {});
  let timer;
  const compute = async () => {
    try {
      const r = await api('/api/emi?type=' + type.value + '&principal=' + encodeURIComponent(prin.value) + '&months=' + encodeURIComponent(months.value));
      put(calc, el('div', { class: 'grid fit', style: { marginBottom: '14px' } }, stat('Monthly EMI', money(r.emi)), stat('Total interest', money(r.totalInterest)), stat('Total payable', money(r.totalPayable))),
        el('div', { class: 'actions' }, el('button', { class: 'btn teal', onclick: async () => {
          if (!await confirmBox('Apply for loan', [['Type', r.type], ['Amount', money(prin.value)], ['Tenure', months.value + ' months'], ['Rate', r.rate + '% p.a.'], ['EMI', money(r.emi)]], 'Submit application')) return;
          try { const l = await post('/api/loans/apply', { type: type.value, principal: prin.value, months: months.value }); toast('Application ' + l.id + ' submitted for approval.', 'ok'); loadMine(); } catch (ex) { fail(ex); }
        } }, 'Apply for this loan'), el('button', { class: 'btn ghost', onclick: () => scheduleModal(r.schedule, r.type + ' · ' + money(prin.value)) }, 'View amortization table')));
    } catch (ex) { put(calc, el('div', { class: 'banner info' }, ex.message)); }
  };
  const again = () => { clearTimeout(timer); timer = setTimeout(compute, 300); };
  [type, prin, months].forEach(i => { i.oninput = again; i.onchange = again; });
  const loadMine = async () => {
    const ls = await api('/api/loans');
    put(mine, ls.length ? ls.map(l => {
      const kind = { Active: 'green', Pending: 'amber', Rejected: 'red', Closed: '' }[l.status];
      return el('div', { class: 'loan', style: { padding: '14px 0', borderBottom: '1px solid var(--line)' } },
        el('div', { class: 'card-head', style: { marginBottom: '4px' } }, el('b', {}, l.id + ' · ' + l.type), badge(l.status, kind)),
        el('div', { class: 'small muted' }, money(l.principal) + ' · ' + l.months + ' months @ ' + l.rate + '% · EMI ' + money(l.emi)),
        l.status === 'Active' && [progress(l.paid / l.months), el('div', { class: 'small' }, l.paid + ' of ' + l.months + ' paid · outstanding ' + money(l.outstanding))],
        l.note && el('div', { class: 'small', style: { marginTop: '4px' } }, 'Note: ' + l.note),
        el('div', { class: 'actions', style: { marginTop: '10px' } }, el('button', { class: 'btn ghost sm', onclick: async () => {
          try { const r = await api('/api/loans/schedule?id=' + l.id); scheduleModal(r.schedule, l.id + ' · ' + l.type); } catch (ex) { fail(ex); }
        } }, 'Schedule'),
        l.status === 'Active' && el('button', { class: 'btn teal sm', onclick: async () => {
          if (!await confirmBox('Pay EMI', [['Loan', l.id], ['Installment', money(l.next)]], 'Pay now')) return;
          try { const r = await post('/api/loans/pay', { id: l.id }); receipt(r.tx, r.balance, 'EMI paid'); loadMine(); await afterMoney(); } catch (ex) { fail(ex); }
        } }, 'Pay EMI'),
        l.status === 'Active' && el('button', { class: 'btn ghost sm', onclick: async () => {
          if (!await confirmBox('Prepay and close loan', [['Loan', l.id], ['Outstanding principal', money(l.outstanding)]], 'Prepay')) return;
          try { const r = await post('/api/loans/prepay', { id: l.id }); receipt(r.tx, r.balance, 'Loan closed'); loadMine(); await afterMoney(); } catch (ex) { fail(ex); }
        } }, 'Prepay')));
    }) : empty('You have no loans yet.'));
  };
  mount('Loans & EMI', 'Customer › Loans', el('div', { class: 'grid g2' },
    el('div', { class: 'card' }, el('h2', {}, 'EMI calculator'), el('div', { class: 'form', style: { marginBottom: '16px' } }, field('Loan type', type),
      el('div', { class: 'row' }, field('Amount (₹)', prin), field('Tenure (months)', months))), calc),
    el('div', { class: 'card' }, el('h2', {}, 'My loans'), mine)));
  compute(); loadMine();
}
function scheduleModal(rows, title) {
  const m = modal('Amortization · ' + title, el('div', {}, table(['Month', 'EMI', 'Interest', 'Principal', 'Balance'],
    rows.map(r => [r.month, num(r.emi), num(r.interest), num(r.principal), num(r.balance)]), ['', 'r', 'r', 'r', 'r'], i => rows[i].paid ? 'paid' : ''),
    el('div', { class: 'actions', style: { justifyContent: 'flex-end', marginTop: '14px' } }, el('button', { class: 'btn', onclick: () => m.close() }, 'Close'))), { wide: true });
}

async function pagePayees() {
  const out = el('div', {});
  const load = async () => {
    const ps = await api('/api/payees');
    put(out, ps.length ? table(['Nickname', 'Name', 'Account', 'Added', ''],
      ps.map(p => [el('b', {}, p.nickname), p.name, p.acc, p.added + (p.isNew ? ' (new · monitored 24h)' : ''), el('span', { class: 'actions' },
        el('button', { class: 'btn teal sm', onclick: () => { state.prefill = { payee: p.nickname }; location.hash = '#/pay'; } }, 'Send'),
        el('button', { class: 'btn ghost sm', onclick: async () => { if (!await confirmBox('Remove payee', [['Payee', p.nickname + ' (' + p.name + ')']], 'Remove')) return; try { await post('/api/payees/remove', { nickname: p.nickname }); load(); } catch (ex) { fail(ex); } } }, 'Remove'))])) : empty('No saved payees yet.'));
  };
  const nick = el('input', { maxlength: 20, required: true, placeholder: 'e.g. Mom' }), acc = el('input', { required: true, placeholder: 'e.g. CA2001' });
  mount('Payees', 'Customer › Payees', el('div', { class: 'stack' }, el('div', { class: 'card' }, el('h2', {}, 'Saved payees'), out),
    el('div', { class: 'card' }, el('h2', {}, 'Add payee'), el('form', { class: 'form', onsubmit: async e => {
      e.preventDefault();
      try { const p = await post('/api/payees', { nickname: nick.value, account: acc.value }); toast('Saved ' + p.nickname + ' (' + p.name + ')', 'ok'); nick.value = acc.value = ''; load(); } catch (ex) { fail(ex); }
    } }, el('div', { class: 'row' }, field('Nickname', nick), field('Account number', acc)), el('div', { class: 'hint' }, 'High-value transfers to a payee added in the last 24 hours trigger a fraud check.'),
      el('button', { class: 'btn teal', type: 'submit', style: { justifySelf: 'start' } }, 'Save payee')))));
  load();
}

function lineChart(rows, series) {
  const W = 900, H = 300, L = 62, B = 30, T = 14, R = 14;
  const svg = svgEl('svg', { class: 'chart', viewBox: `0 0 ${W} ${H}`, role: 'img', 'aria-label': 'Projected balance by year' });
  const all = rows.flatMap(r => series.map(s => Number(r[s.key])));
  const mn = Math.min(...all) * 0.98, mx = Math.max(...all) * 1.02;
  const x = i => L + i * (W - L - R) / Math.max(1, rows.length - 1), y = v => T + (H - T - B) * (1 - (v - mn) / ((mx - mn) || 1));
  for (let i = 0; i <= 4; i++) {
    const v = mn + (mx - mn) * i / 4, yy = y(v);
    svg.append(svgEl('line', { x1: L, x2: W - R, y1: yy, y2: yy, stroke: 'var(--line)' }));
    const t = svgEl('text', { x: L - 8, y: yy + 4, 'text-anchor': 'end', 'font-size': 11, fill: 'var(--muted)' }); t.textContent = '₹' + new Intl.NumberFormat('en-IN', { notation: 'compact', maximumFractionDigits: 1 }).format(v); svg.append(t);
  }
  rows.forEach((r, i) => { const t = svgEl('text', { x: x(i), y: H - 8, 'text-anchor': 'middle', 'font-size': 11, fill: 'var(--muted)' }); t.textContent = 'Y' + r.year; svg.append(t); });
  series.forEach(s => {
    svg.append(svgEl('polyline', { points: rows.map((r, i) => x(i) + ',' + y(Number(r[s.key]))).join(' '), fill: 'none', stroke: s.color, 'stroke-width': 2.6, 'stroke-linejoin': 'round' }));
    rows.forEach((r, i) => svg.append(svgEl('circle', { cx: x(i), cy: y(Number(r[s.key])), r: 3, fill: s.color })));
  });
  return svg;
}
async function pageSimulator() {
  const prin = el('input', { inputmode: 'decimal', value: '100000' }), years = el('input', { inputmode: 'numeric', value: '5' }), out = el('div', {});
  const SER = [{ key: 'fd', name: 'Fixed deposit 7% (compound)', color: '#14a58c' }, { key: 'student', name: 'Student 4.5%', color: '#e08a1e' }, { key: 'savings', name: 'Savings 4%', color: '#2f6fd0' }, { key: 'current', name: 'Current 0%', color: '#8896a3' }];
  const run = async () => {
    try {
      const r = await api('/api/simulate?principal=' + encodeURIComponent(prin.value) + '&years=' + encodeURIComponent(years.value));
      const last = r.rows[r.rows.length - 1];
      put(out, el('div', { class: 'grid g4', style: { margin: '16px 0' } }, SER.map(s => stat(s.name.split(' (')[0], money(last[s.key]), s.key === 'fd' ? 'pos' : ''))),
        el('div', { class: 'card', style: { marginBottom: '18px' } }, lineChart(r.rows, SER), el('div', { class: 'legend' }, SER.map(s => el('span', {}, el('i', { style: { background: s.color } }), s.name)))),
        el('div', { class: 'card' }, table(['Year', 'Savings 4%', 'Student 4.5%', 'FD 7% comp.', 'Current 0%'], r.rows.map(x => [x.year, money(x.savings), money(x.student), money(x.fd), money(x.current)]), ['', 'r', 'r', 'r', 'r']),
          el('div', { class: 'small muted' }, 'Savings and Student use simple interest. FD compounds yearly. Current pays none.')));
    } catch (ex) { put(out, el('div', { class: 'banner err' }, '✖ ' + ex.message)); }
  };
  mount('Interest & FD simulator', 'Customer › Simulator', el('div', {}, el('div', { class: 'card' }, el('form', { class: 'form', onsubmit: e => { e.preventDefault(); run(); } },
    el('div', { class: 'row' }, field('Amount to invest (₹)', prin), field('Years (1-20)', years)), el('button', { class: 'btn teal', type: 'submit', style: { justifySelf: 'start' } }, 'Compare'))), out));
  run();
}

// ------------------------------------------------------------------ admin pages
async function pageAdmin() {
  mount('Overview', 'Admin › Overview', loading());
  const o = await api('/api/admin/overview');
  mount('Admin overview', 'Admin › Overview', el('div', { class: 'stack' },
    el('div', { class: 'grid g4' }, stat('Accounts', o.accounts), stat('Total balance', money(o.totalBalance)), stat('Pending loans', o.pendingLoans, Number(o.pendingLoans) ? 'neg' : ''), stat('Alerts to review', o.alerts, Number(o.alerts) ? 'neg' : '')),
    el('div', { class: 'grid g2' }, el('div', { class: 'card' }, el('h2', {}, 'Needs attention'), el('div', { class: 'stack' },
      el('a', { href: '#/aloans' }, '₹  ' + o.pendingLoans + ' loan application(s) awaiting a decision'), el('a', { href: '#/afraud' }, '⚠  ' + o.alerts + ' fraud alert(s) to review'),
      el('a', { href: '#/alocked' }, '⛨  ' + o.locked + ' locked user(s)'))),
      el('div', { class: 'card' }, el('h2', {}, 'System'), el('dl', { class: 'kv' }, el('dt', {}, 'Fraud rules active'), el('dd', {}, o.rules), el('dt', {}, 'Logged in as'), el('dd', {}, o.name),
        el('dt', {}, 'Monthly interest'), el('dd', {}, el('button', { class: 'btn ghost sm', onclick: async () => {
          if (!await confirmBox('Apply monthly interest', [['Applies to', 'Savings and Student accounts'], ['Once per month', 'Already-paid accounts are skipped']], 'Apply')) return;
          try { const r = await post('/api/admin/interest'); toast('Interest paid: ' + money(r.total), 'ok'); pageAdmin(); } catch (ex) { fail(ex); }
        } }, 'Run now')))))));
}
async function pageAccounts() {
  const out = el('div', {}), qin = el('input', { placeholder: 'Search by account number or name', 'aria-label': 'Search accounts' });
  const load = async () => {
    const as = await api('/api/admin/accounts?q=' + encodeURIComponent(qin.value));
    put(out, as.length ? table(['Account', 'Holder', 'Type', 'Balance', 'Status', ''], as.map(a => [el('b', {}, a.accNo), a.holder, a.type, money(a.balance), badge(a.active ? 'Active' : 'Closed', a.active ? 'green' : 'red'),
      el('span', { class: 'actions' }, el('button', { class: 'btn ghost sm', onclick: () => history(a) }, 'History'),
        a.active && el('button', { class: 'btn ghost sm', onclick: () => rename(a) }, 'Rename'),
        a.active && el('button', { class: 'btn ghost sm', onclick: () => close(a) }, 'Close'))]), ['', '', '', 'r', '', '']) : empty('No accounts match.'));
  };
  const history = async a => {
    try { const r = await api('/api/admin/history?acc=' + a.accNo); const m = modal(a.holder + ' · ' + a.accNo, el('div', {}, txTable(r.rows), el('div', { class: 'actions', style: { justifyContent: 'flex-end', marginTop: '12px' } }, el('button', { class: 'btn', onclick: () => m.close() }, 'Close'))), { wide: true }); } catch (ex) { fail(ex); }
  };
  const rename = a => {
    const n = el('input', { value: a.holder, maxlength: 40 });
    const m = modal('Rename holder', el('form', { class: 'form', onsubmit: async e => { e.preventDefault(); try { await post('/api/admin/accounts/update', { acc: a.accNo, holder: n.value }); m.close(); toast('Updated.', 'ok'); load(); } catch (ex) { fail(ex); } } },
      field('New holder name', n), el('button', { class: 'btn teal', type: 'submit' }, 'Save')));
  };
  const close = async a => {
    if (!await confirmBox('Close account', [['Account', a.accNo + ' · ' + a.holder], ['Balance paid out', money(a.balance)], ['Effect', 'Customer login is disabled']], 'Close account')) return;
    try { const r = await post('/api/admin/accounts/close', { acc: a.accNo }); toast('Closed. Payout ' + money(r.payout), 'ok'); load(); } catch (ex) { fail(ex); }
  };
  const t = select([['SAV', 'Savings (4%, min ₹1,000)'], ['STU', 'Student savings (4.5%)'], ['CUR', 'Current (₹25,000 overdraft)'], ['FD', 'Fixed deposit (7%, 12 months)']], 'SAV');
  const hn = el('input', { required: true, maxlength: 40 }), dep = el('input', { inputmode: 'decimal', value: '1000' }), pin = el('input', { type: 'password', inputmode: 'numeric', maxlength: 4, required: true, autocomplete: 'new-password' });
  qin.oninput = () => { clearTimeout(qin._t); qin._t = setTimeout(load, 250); };
  mount('Accounts', 'Admin › Accounts', el('div', { class: 'stack' }, el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, 'All accounts'), qin), out),
    el('div', { class: 'card' }, el('h2', {}, 'Open a new account'), el('form', { class: 'form', onsubmit: async e => {
      e.preventDefault();
      try { const a = await post('/api/admin/accounts', { type: t.value, holder: hn.value, deposit: dep.value, pin: pin.value }); toast('Opened ' + a.accNo + ' for ' + a.holder + '. Login: ' + a.accNo, 'ok'); hn.value = pin.value = ''; load(); } catch (ex) { fail(ex); }
    } }, el('div', { class: 'row' }, field('Account type', t), field('Holder name', hn), field('Opening deposit (₹)', dep), field('4-digit PIN', pin)),
      el('div', { class: 'hint' }, 'The customer logs in with the new account number and this PIN.'), el('button', { class: 'btn teal', type: 'submit', style: { justifySelf: 'start' } }, 'Open account')))));
  load();
}
async function pageAdminLoans() {
  mount('Loan approvals', 'Admin › Loans', loading());
  const ls = await api('/api/admin/loans');
  mount('Loan approvals', 'Admin › Loans', el('div', { class: 'card' }, ls.length ? table(['Loan', 'Customer', 'Type', 'Amount', 'Tenure', 'EMI', 'Applied', ''],
    ls.map(l => [el('b', {}, l.id), l.holder + ' (' + l.acc + ')', l.type, money(l.principal), l.months + ' m @ ' + l.rate + '%', money(l.emi), l.applied, el('span', { class: 'actions' },
      el('button', { class: 'btn teal sm', onclick: async () => { try { await post('/api/admin/loans/approve', { id: l.id }); toast(l.id + ' approved and credited.', 'ok'); pageAdminLoans(); } catch (ex) { fail(ex); } } }, 'Approve'),
      el('button', { class: 'btn danger sm', onclick: async () => {
        const r = el('input', { placeholder: 'Reason', maxlength: 80 });
        const m = modal('Reject ' + l.id, el('form', { class: 'form', onsubmit: async e => { e.preventDefault(); try { await post('/api/admin/loans/reject', { id: l.id, reason: r.value }); m.close(); toast(l.id + ' rejected.', 'ok'); pageAdminLoans(); } catch (ex) { fail(ex); } } },
          field('Reason (shown to customer)', r), el('button', { class: 'btn danger', type: 'submit' }, 'Reject')));
      } }, 'Reject'))]), ['', '', '', 'r', '', 'r', '', '']) : el('div', { class: 'banner ok' }, '✔ No pending applications.')));
}
async function pageAdminFraud() {
  mount('Fraud alerts', 'Admin › Fraud', loading());
  const as = await api('/api/admin/alerts');
  mount('Fraud alerts', 'Admin › Fraud', el('div', { class: 'stack' }, as.length ? as.map(a => el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, '⚠ ' + a.rule), badge(a.id, 'amber')),
    el('div', {}, a.message), el('div', { class: 'small muted', style: { margin: '6px 0 12px' } }, a.holder + ' (' + a.acc + ') · ' + a.time + ' · ref ' + a.ref),
    el('button', { class: 'btn teal sm', onclick: async () => { try { await post('/api/admin/alerts/review', { id: a.id }); toast('Marked reviewed.', 'ok'); pageAdminFraud(); } catch (ex) { fail(ex); } } }, 'Mark reviewed'))) :
    el('div', { class: 'banner ok' }, '✔ Nothing waiting for review.')));
}
async function pageLocked() {
  mount('Locked users', 'Admin › Security', loading());
  const us = await api('/api/admin/locked');
  mount('Locked users', 'Admin › Security', el('div', { class: 'card' }, us.length ? table(['User', 'Name', 'Role', ''], us.map(u => [el('b', {}, u.id), u.name, u.role,
    el('button', { class: 'btn teal sm', onclick: async () => { try { await post('/api/admin/unlock', { id: u.id }); toast(u.id + ' unlocked.', 'ok'); pageLocked(); } catch (ex) { fail(ex); } } }, 'Unlock')])) :
    el('div', { class: 'banner ok' }, '✔ No locked users. (Three wrong PINs at login lock a user.)')));
}
async function pageReports() {
  mount('Reports', 'Admin › Reports', loading());
  const r = await api('/api/admin/reports');
  const maxBal = Math.max(1, ...r.richest.map(a => Number(a.balance)));
  const maxM = Math.max(1, ...r.months.flatMap(m => [Number(m.credits), Number(m.debits)]));
  mount('Reports', 'Admin › Reports', el('div', { class: 'stack' }, el('div', { class: 'grid g2' },
    el('div', { class: 'card' }, el('h2', {}, 'Richest accounts'), r.richest.map((a, i) => el('div', { class: 'bar-row' + (i === 0 ? ' top' : '') }, el('div', {}, a.accNo), progress(Number(a.balance) / maxBal), el('div', { class: 'amt' }, money(a.balance))))),
    el('div', { class: 'card' }, el('h2', {}, 'Dormant accounts (30+ days)'), r.dormant.length ? table(['Account', 'Holder', 'Last activity'], r.dormant.map(d => [d.acc, d.holder, d.last])) : empty('None.'))),
    el('div', { class: 'card' }, el('h2', {}, 'Bank-wide credits and debits by month'), r.months.length ? r.months.map(m => el('div', {}, el('div', { class: 'small muted', style: { marginTop: '10px' } }, m.month),
      el('div', { class: 'bar-row' }, el('div', { class: 'pos small' }, '+ credits'), progress(Number(m.credits) / maxM, true), el('div', { class: 'amt' }, money(m.credits))),
      el('div', { class: 'bar-row' }, el('div', { class: 'neg small' }, '− debits'), progress(Number(m.debits) / maxM), el('div', { class: 'amt' }, money(m.debits))))) : empty('No data.'))));
}
async function pageAudit() {
  const out = el('div', { class: 'log' }), q = el('input', { placeholder: 'Filter (e.g. LOGIN_FAILED, SB1001)', 'aria-label': 'Filter audit log' });
  const load = async () => {
    const r = await api('/api/admin/audit?q=' + encodeURIComponent(q.value));
    put(out, ...r.lines.map(l => el('div', { class: /FAIL|LOCK|FRAUD|BLOCK/.test(l) ? 'bad' : /APPROVED|UNLOCK|SUCCESS/.test(l) ? 'good' : '' }, l)));
    if (!r.lines.length) put(out, empty('No entries.'));
  };
  q.oninput = () => { clearTimeout(q._t); q._t = setTimeout(load, 250); };
  mount('Audit log', 'Admin › Audit', el('div', { class: 'card' }, el('div', { class: 'card-head' }, el('h2', {}, 'Append-only audit trail (latest first)'), q), out, el('div', { class: 'small muted', style: { marginTop: '8px' } }, 'The log file is only ever opened in append mode; the app has no way to edit past entries.')));
  load();
}

// ------------------------------------------------------------------ router
const ROUTES = { dashboard: pageDashboard, pay: pagePay, statement: pageStatement, spending: pageSpending, goals: pageGoals, loans: pageLoans, payees: pagePayees,
  simulator: pageSimulator, security: pageSecurity, admin: pageAdmin, accounts: pageAccounts, aloans: pageAdminLoans, afraud: pageAdminFraud, alocked: pageLocked, reports: pageReports, audit: pageAudit };
let rendering = false, rerun = false;      // serialize renders so a slow page can never overwrite a newer one
async function render() {
  if (rendering) { rerun = true; return; }
  rendering = true;
  do { rerun = false; await renderOnce(); } while (rerun);
  rendering = false;
}
async function renderOnce() {
  if (!state.user) { try { const me = await api('/api/me'); if (me.loggedIn) state.user = me; } catch (e) { return; } }
  if (!state.user) return showLogin();
  const isAdmin = state.user.role === 'ADMIN';
  let r = location.hash.replace('#/', '') || (isAdmin ? 'admin' : 'dashboard');
  const allowed = (isAdmin ? ADMIN_NAV : CUSTOMER_NAV).map(n => n[0]);
  if (!allowed.includes(r)) r = allowed[0];
  state.route = r;
  if (!isAdmin && r !== 'security') await refreshBadge();
  try { await ROUTES[r](); } catch (e) { fail(e); }
}
window.addEventListener('hashchange', () => { if (state.user) render(); });
render();
