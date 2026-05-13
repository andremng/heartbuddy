const API_BASE = 'http://localhost:5259/api';
const POLL_INTERVAL = 2000; // ms

// ── STATE ─────────────────────────────────────────────────────

const MAX_HISTORY = 30; // points kept in the chart
const chartHistory = []; // { bpm, time }
let lastTimestamp = null;
let activeTab = '1H';

// Summary tracking
let todayReadings = [];

// ── DOM REFS ──────────────────────────────────────────────────

const hrValueEl      = document.querySelector('.hr-value');
const hrDescEl       = document.querySelector('.hr-desc');
const hrStatusEl     = document.querySelector('.hr-card .status-dot');
const spo2ValueEl    = document.querySelector('.big-val');
const spo2StatusEl   = document.querySelector('.card:has(.big-val) .status-dot');
const spo2DescEl     = document.querySelector('.card:has(.big-val) .hr-desc');
const movementValEl  = document.querySelector('.movement-val');
const movementDescEl = document.querySelector('.movement-val + .hr-desc');
const timeValEl      = document.querySelector('.time-val');
const liveIndicator  = document.querySelector('.live-indicator');
const avgBpmEl       = document.querySelectorAll('.summary-row .val')[0];
const maxBpmEl       = document.querySelectorAll('.summary-row .val')[1];
const minBpmEl       = document.querySelectorAll('.summary-row .val')[2];

// ── ECG CANVAS ────────────────────────────────────────────────

const ecgCanvas = document.getElementById('ecgCanvas');
const ecgCtx    = ecgCanvas.getContext('2d');

function drawFlatECG() {
  const w = ecgCanvas.width;
  const h = ecgCanvas.height;

  ecgCtx.clearRect(0, 0, w, h);
  ecgCtx.fillStyle = '#fff5f5';
  ecgCtx.fillRect(0, 0, w, h);

  ecgCtx.strokeStyle = '#fecaca';
  ecgCtx.lineWidth = 0.5;
  for (let x = 0; x < w; x += 20) {
    ecgCtx.beginPath(); ecgCtx.moveTo(x, 0); ecgCtx.lineTo(x, h); ecgCtx.stroke();
  }
  for (let y = 0; y < h; y += 14) {
    ecgCtx.beginPath(); ecgCtx.moveTo(0, y); ecgCtx.lineTo(w, y); ecgCtx.stroke();
  }

  const midY = h / 2;
  ecgCtx.beginPath();
  ecgCtx.moveTo(0, midY);
  ecgCtx.lineTo(w, midY);
  ecgCtx.strokeStyle = '#ef4444';
  ecgCtx.lineWidth = 2;
  ecgCtx.stroke();
}

drawFlatECG();

// ── MAIN CHART ────────────────────────────────────────────────

const mainCanvas = document.getElementById('mainChart');
const mainCtx    = mainCanvas.getContext('2d');

function resizeCanvas() {
  mainCanvas.width  = mainCanvas.parentElement.offsetWidth;
  mainCanvas.height = 180;
  drawMainChart();
}

function getFilteredHistory() {
  const now = Date.now();
  const windows = { '1H': 3600, '6H': 21600, '24H': 86400, '7D': 604800, '30D': 2592000 };
  const seconds = windows[activeTab] || 3600;
  return chartHistory.filter(p => (now - p.time) / 1000 <= seconds);
}

function drawMainChart() {
  const w   = mainCanvas.width;
  const h   = mainCanvas.height;
  const pad = { top: 20, right: 20, bottom: 36, left: 40 };
  const cw  = w - pad.left - pad.right;
  const ch  = h - pad.top - pad.bottom;

  mainCtx.clearRect(0, 0, w, h);

  const data = getFilteredHistory();

  // Y range
  const allBpm = data.map(p => p.bpm);
  const yMin = allBpm.length ? Math.max(0,  Math.min(...allBpm) - 10) : 40;
  const yMax = allBpm.length ? Math.min(220, Math.max(...allBpm) + 10) : 120;
  const yRange = yMax - yMin || 1;

  // Y grid + labels
  const yStep  = Math.ceil(yRange / 4 / 10) * 10;
  const yStart = Math.floor(yMin / 10) * 10;
  const yLabels = [];
  for (let v = yStart; v <= yMax + yStep; v += yStep) yLabels.push(v);

  mainCtx.font      = '11px DM Sans';
  mainCtx.fillStyle = '#94a3b8';
  mainCtx.textAlign = 'right';
  yLabels.forEach(val => {
    const y = pad.top + ch - ((val - yMin) / yRange) * ch;
    if (y < pad.top || y > pad.top + ch) return;
    mainCtx.fillText(val, pad.left - 8, y + 4);
    mainCtx.beginPath();
    mainCtx.strokeStyle = '#e2e8f0';
    mainCtx.lineWidth   = 1;
    mainCtx.moveTo(pad.left, y);
    mainCtx.lineTo(pad.left + cw, y);
    mainCtx.stroke();
  });

  // X labels
  mainCtx.textAlign = 'center';
  mainCtx.fillStyle = '#94a3b8';
  if (data.length >= 2) {
    const tMin = data[0].time;
    const tMax = data[data.length - 1].time;
    const tRange = tMax - tMin || 1;
    const numLabels = Math.min(7, data.length);
    for (let i = 0; i < numLabels; i++) {
      const t   = tMin + (i / (numLabels - 1)) * tRange;
      const x   = pad.left + ((t - tMin) / tRange) * cw;
      const d   = new Date(t);
      const lbl = d.getHours().toString().padStart(2, '0') + ':' + d.getMinutes().toString().padStart(2, '0');
      mainCtx.fillText(lbl, x, h - 6);
    }
  }

  // BPM label
  mainCtx.fillStyle = '#64748b';
  mainCtx.font      = '11px DM Sans';
  mainCtx.textAlign = 'left';
  mainCtx.fillText('BPM', 4, pad.top - 6);

  if (data.length < 2) return;

  const tMin  = data[0].time;
  const tMax  = data[data.length - 1].time;
  const tRange = tMax - tMin || 1;

  const toX = t   => pad.left + ((t   - tMin)  / tRange)  * cw;
  const toY = bpm => pad.top  + ch - ((bpm - yMin) / yRange) * ch;

  // Gradient fill
  const grad = mainCtx.createLinearGradient(0, pad.top, 0, pad.top + ch);
  grad.addColorStop(0, 'rgba(59,130,246,0.18)');
  grad.addColorStop(1, 'rgba(59,130,246,0)');

  mainCtx.beginPath();
  mainCtx.moveTo(toX(data[0].time), pad.top + ch);
  data.forEach(p => mainCtx.lineTo(toX(p.time), toY(p.bpm)));
  mainCtx.lineTo(toX(data[data.length - 1].time), pad.top + ch);
  mainCtx.closePath();
  mainCtx.fillStyle = grad;
  mainCtx.fill();

  // Line
  mainCtx.beginPath();
  data.forEach((p, i) => {
    i === 0 ? mainCtx.moveTo(toX(p.time), toY(p.bpm))
            : mainCtx.lineTo(toX(p.time), toY(p.bpm));
  });
  mainCtx.strokeStyle = '#3b82f6';
  mainCtx.lineWidth   = 2.5;
  mainCtx.lineJoin    = 'round';
  mainCtx.stroke();

  // Latest dot
  const last = data[data.length - 1];
  mainCtx.beginPath();
  mainCtx.arc(toX(last.time), toY(last.bpm), 4, 0, Math.PI * 2);
  mainCtx.fillStyle = '#3b82f6';
  mainCtx.fill();
}

window.addEventListener('resize', resizeCanvas);
resizeCanvas();

// ── TAB SWITCHING ─────────────────────────────────────────────

document.querySelectorAll('.tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    tab.classList.add('active');
    activeTab = tab.textContent.trim();
    drawMainChart();
  });
});

// ── HELPERS ───────────────────────────────────────────────────

function getHrStatus(bpm) {
  if (bpm === 0)        return { label: 'No data',  color: '#94a3b8' };
  if (bpm < 40)         return { label: 'Too low',  color: '#ef4444' };
  if (bpm <= 100)       return { label: 'Normal',   color: '#22c55e' };
  if (bpm <= 130)       return { label: 'Elevated', color: '#f59e0b' };
  return                       { label: 'High',     color: '#ef4444' };
}

function getSpo2Status(spo2) {
  if (spo2 === 0)  return { label: 'No data', color: '#94a3b8' };
  if (spo2 >= 95)  return { label: 'Normal',  color: '#22c55e' };
  if (spo2 >= 90)  return { label: 'Low',     color: '#f59e0b' };
  return                  { label: 'Critical', color: '#ef4444' };
}

function timeAgo(isoString) {
  const diff = Math.floor((Date.now() - new Date(isoString).getTime()) / 1000);
  if (diff < 5)  return 'Just now';
  if (diff < 60) return `${diff} sec ago`;
  return `${Math.floor(diff / 60)} min ago`;
}

function setStatusEl(el, label, color) {
  if (!el) return;
  el.textContent = label;
  el.style.color = color;
  if (el.classList.contains('status-dot')) {
    el.style.setProperty('--dot-color', color);
  }
}

// ── TODAY'S SUMMARY ───────────────────────────────────────────

function updateSummary() {
  const midnight = new Date(); midnight.setHours(0, 0, 0, 0);
  const todayBpm = todayReadings
    .filter(r => new Date(r.timestamp) >= midnight)
    .map(r => r.heartRate)
    .filter(b => b > 0);

  if (!todayBpm.length) return;
  const avg = Math.round(todayBpm.reduce((a, b) => a + b, 0) / todayBpm.length);
  if (avgBpmEl) avgBpmEl.textContent = avg + ' BPM';
  if (maxBpmEl) maxBpmEl.textContent = Math.max(...todayBpm) + ' BPM';
  if (minBpmEl) minBpmEl.textContent = Math.min(...todayBpm) + ' BPM';
}

// ── API POLLING ───────────────────────────────────────────────

async function fetchLatest() {
  try {
    const res = await fetch(`${API_BASE}/readings/latest`);

    if (!res.ok) throw new Error(`HTTP ${res.status}`);

    const data = await res.json();

    if (!data) {
      setOffline('No readings yet');
      return;
    }

    // Avoid duplicate points
    if (data.timestamp === lastTimestamp) return;
    lastTimestamp = data.timestamp;

    const bpm    = data.heartRate  ?? 0;
    const spo2   = data.sp02       ?? 0; // note: matches the typo in the model
    const move   = data.movement   ?? 0;

    // ── Heart Rate ──
    const hrNum = hrValueEl.querySelector('span');
    if (hrNum) {
      hrValueEl.childNodes[0].textContent = bpm;
    } else {
      hrValueEl.textContent = bpm;
      hrValueEl.insertAdjacentHTML('beforeend', '<span>BPM</span>');
    }
    const hrStatus = getHrStatus(bpm);
    setStatusEl(hrStatusEl, hrStatus.label, hrStatus.color);
    if (hrDescEl) hrDescEl.textContent = bpm === 0
      ? 'Waiting for sensor data.'
      : `Your heart rate is ${hrStatus.label.toLowerCase()}.`;

    // ── SpO2 ──
    const spo2Num = spo2ValueEl.querySelector('.unit');
    if (spo2Num) {
      spo2ValueEl.childNodes[0].textContent = spo2;
    } else {
      spo2ValueEl.textContent = spo2;
      spo2ValueEl.insertAdjacentHTML('beforeend', '<span class="unit">%</span>');
    }
    const spo2Status = getSpo2Status(spo2);
    setStatusEl(spo2StatusEl, spo2Status.label, spo2Status.color);
    if (spo2DescEl) spo2DescEl.textContent = spo2 === 0
      ? 'Waiting for sensor data.'
      : `Blood oxygen is ${spo2Status.label.toLowerCase()}.`;

    // ── Movement ──
    if (movementValEl) {
      if (move === 0) {
        movementValEl.textContent = 'No Movement';
        movementValEl.style.color = '#22c55e';
        if (movementDescEl) movementDescEl.textContent = 'No significant movement detected.';
      } else {
        movementValEl.textContent = 'Moving';
        movementValEl.style.color = '#f59e0b';
        if (movementDescEl) movementDescEl.textContent = 'Movement detected.';
      }
    }

    // ── Last updated ──
    if (timeValEl) timeValEl.textContent = timeAgo(data.timestamp);

    // ── Live indicator ──
    if (liveIndicator) {
      liveIndicator.style.color = '#22c55e';
      liveIndicator.childNodes[liveIndicator.childNodes.length - 1].textContent = ' Live monitoring connected';
    }

    // ── Chart history ──
    if (bpm > 0) {
      chartHistory.push({ bpm, time: new Date(data.timestamp).getTime() });
      if (chartHistory.length > MAX_HISTORY) chartHistory.shift();
      drawMainChart();
    }

    // ── Today's summary ──
    todayReadings.push(data);
    if (todayReadings.length > 1000) todayReadings = todayReadings.slice(-1000);
    updateSummary();

  } catch (err) {
    setOffline('API unreachable');
    console.error('HeartBuddy fetch error:', err);
  }
}

function setOffline(reason) {
  if (liveIndicator) {
    liveIndicator.style.color = '#94a3b8';
    liveIndicator.childNodes[liveIndicator.childNodes.length - 1].textContent = ` ${reason}`;
  }
  if (timeValEl) timeValEl.textContent = '—';
}

// Kick off
fetchLatest();
setInterval(fetchLatest, POLL_INTERVAL);