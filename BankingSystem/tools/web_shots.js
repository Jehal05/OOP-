// Drives the real web UI in headless Chrome and saves screenshots to docs/img/web/.
// Usage: node tools/web_shots.js [baseUrl]   (server must be running with fresh demo data)
const path = require('path');
const root = require('child_process').execSync('npm root -g').toString().trim();
const { chromium } = require(path.join(root, '@playwright/mcp/node_modules/playwright-core'));
const BASE = process.argv[2] || 'http://localhost:8099';
const OUT = path.join(__dirname, '..', 'docs', 'img', 'web');
const errors = [];

async function session(browser, fn) {
  const ctx = await browser.newContext({ viewport: { width: 1280, height: 800 }, deviceScaleFactor: 1, colorScheme: 'light' });
  const page = await ctx.newPage();
  page.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', e => errors.push('PAGEERROR ' + e.message));
  await page.goto(BASE);
  await fn(page);
  await ctx.close();
}
const shot = async (page, name, full = false) => { await page.waitForTimeout(700); await page.screenshot({ path: path.join(OUT, name + '.png'), fullPage: full }); console.log('shot', name); };
const login = async (page, id, pin, admin) => {
  if (admin) await page.click('button[data-r=admin]');
  await page.fill('input[aria-label=' + (admin ? '"Admin ID"' : '"Account number"') + '], input[autocomplete=username]', id);
  await page.fill('input[type=password]', pin);
  await page.click('button[type=submit]');
  await page.waitForSelector('.shell');
};
const go = async (page, route) => { await page.evaluate(r => { location.hash = '#/' + r; }, route); await page.waitForTimeout(500); };

(async () => {
  const browser = await chromium.launch({ executablePath: '/usr/bin/google-chrome', args: ['--no-sandbox', '--disable-gpu'] });

  await session(browser, async page => {
    await page.waitForSelector('.login-card');
    await shot(page, '01_login');
    // wrong PIN message
    await page.fill('input[autocomplete=username]', 'SB1001'); await page.fill('input[type=password]', '0000'); await page.click('button[type=submit]');
    await page.waitForSelector('.banner.err:not(.hidden)'); await shot(page, '02_login_error');
    await login(page, 'SB1001', '1234');
    await shot(page, '03_dashboard');
    await go(page, 'pay');
    await page.fill('input[aria-label=Amount]', '2500'); await page.click('text=Deposit >> nth=-1');
    await page.waitForSelector('.receipt'); await shot(page, '04_receipt');
    await page.click('text=Done');
    await page.click('role=tab[name=Transfer]'); await page.waitForTimeout(400);
    await page.fill('input[placeholder="e.g. CA2001"]', 'CA2001'); await page.click('input[aria-label=Amount]');
    await page.fill('input[aria-label=Amount]', '1200'); await page.selectOption('select >> nth=1', 'Bills');
    await page.click('text=Review transfer'); await page.waitForSelector('.modal'); await shot(page, '05_transfer_confirm');
    await page.click('text=Send money >> nth=-1'); await page.waitForSelector('.receipt'); await page.click('text=Done');
    await go(page, 'statement');
    await page.selectOption('select >> nth=0', 'Withdraw'); await page.selectOption('select >> nth=2', 'amount');
    await page.click('text=Apply filters'); await page.waitForTimeout(600); await shot(page, '06_statement');
    await go(page, 'spending'); await shot(page, '07_spending');
    await go(page, 'goals'); await shot(page, '08_goals');
    await go(page, 'loans'); await shot(page, '09_loans_emi');
    await page.click('text=View amortization table'); await page.waitForSelector('.modal.wide'); await shot(page, '10_amortization'); await page.keyboard.press('Escape');
    await go(page, 'payees'); await shot(page, '11_payees');
    await go(page, 'simulator'); await shot(page, '12_simulator');
  });

  await session(browser, async page => {                 // fraud alert + lockout
    await login(page, 'CA2001', '2345');
    await go(page, 'pay'); await page.click('role=tab[name=Withdraw]');
    await page.fill('input[aria-label=Amount]', '60000'); await page.click('text=Review withdrawal');
    await page.waitForSelector('.modal'); await page.click('text=Withdraw >> nth=-1'); await page.waitForSelector('.receipt'); await page.click('text=Done');
    await page.waitForTimeout(500); await shot(page, '13_fraud_toast');
    await go(page, 'security'); await shot(page, '14_security_alerts');
  });
  await session(browser, async page => {
    await page.waitForSelector('.login-card');
    for (let i = 0; i < 3; i++) { await page.fill('input[autocomplete=username]', 'ST1001'); await page.fill('input[type=password]', '9999'); await page.click('button[type=submit]'); await page.waitForSelector('.banner.err:not(.hidden)'); await page.waitForTimeout(150); }
    await shot(page, '15_locked');
  });
  await session(browser, async page => {                 // FD penalty confirm
    await login(page, 'FD3001', '3456');
    await go(page, 'pay'); await page.click('role=tab[name=Withdraw]'); await page.fill('input[aria-label=Amount]', '20000'); await page.click('text=Review withdrawal');
    await page.waitForSelector('.modal'); await shot(page, '16_fd_penalty');
  });
  await session(browser, async page => {
    await login(page, 'admin', 'admin123', true);
    await shot(page, '17_admin_overview');
    await go(page, 'accounts'); await shot(page, '18_admin_accounts');
    await go(page, 'aloans'); await shot(page, '19_admin_loans');
    await go(page, 'afraud'); await shot(page, '20_admin_fraud');
    await go(page, 'alocked'); await shot(page, '21_admin_locked');
    await go(page, 'reports'); await shot(page, '22_admin_reports', true);
    await go(page, 'audit'); await shot(page, '23_admin_audit');
  });
  await browser.close();
  console.log(errors.length ? 'CONSOLE ERRORS:\n' + errors.join('\n') : 'no console errors');
})().catch(e => { console.error('FAILED', e.message); process.exit(1); });
