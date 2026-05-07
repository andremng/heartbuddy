// ── FLAT ECG (mini) ──────────────────────────────────────────

const ecgCanvas = document.getElementById('ecgCanvas');
const ecgCtx = ecgCanvas.getContext('2d');

function drawFlatECG() {
  const w = ecgCanvas.width;
  const h = ecgCanvas.height;

  ecgCtx.clearRect(0, 0, w, h);

  // Grid background
  ecgCtx.fillStyle = '#fff5f5';
  ecgCtx.fillRect(0, 0, w, h);

  ecgCtx.strokeStyle = '#fecaca';
  ecgCtx.lineWidth = 0.5;

  for (let x = 0; x < w; x += 20) {
    ecgCtx.beginPath();
    ecgCtx.moveTo(x, 0);
    ecgCtx.lineTo(x, h);
    ecgCtx.stroke();
  }

  for (let y = 0; y < h; y += 14) {
    ecgCtx.beginPath();
    ecgCtx.moveTo(0, y);
    ecgCtx.lineTo(w, y);
    ecgCtx.stroke();
  }

  // Flat line at vertical center
  const midY = h / 2;
  ecgCtx.beginPath();
  ecgCtx.moveTo(0, midY);
  ecgCtx.lineTo(w, midY);
  ecgCtx.strokeStyle = '#ef4444';
  ecgCtx.lineWidth = 2;
  ecgCtx.stroke();
}

drawFlatECG();


// ── MAIN CHART ───────────────────────────────────────────────

const mainCanvas = document.getElementById('mainChart');
const mainCtx = mainCanvas.getContext('2d');

function resizeCanvas() {
  mainCanvas.width = mainCanvas.parentElement.offsetWidth;
  mainCanvas.height = 180;
  drawMainChart();
}

function drawMainChart() {
  const w = mainCanvas.width;
  const h = mainCanvas.height;
  const pad = { top: 20, right: 20, bottom: 36, left: 40 };
  const cw = w - pad.left - pad.right;
  const ch = h - pad.top - pad.bottom;

  mainCtx.clearRect(0, 0, w, h);

  // Y axis labels & grid lines
  const yLabels = [120, 100, 80, 60, 40];
  mainCtx.font = '11px DM Sans';
  mainCtx.fillStyle = '#94a3b8';
  mainCtx.textAlign = 'right';

  yLabels.forEach(val => {
    const y = pad.top + ch - ((val - 40) / (120 - 40)) * ch;
    mainCtx.fillText(val, pad.left - 8, y + 4);
    mainCtx.beginPath();
    mainCtx.strokeStyle = '#e2e8f0';
    mainCtx.lineWidth = 1;
    mainCtx.moveTo(pad.left, y);
    mainCtx.lineTo(pad.left + cw, y);
    mainCtx.stroke();
  });

  // X axis labels
  const xLabels = ['10:00', '10:10', '10:20', '10:30', '10:40', '10:50', '11:00'];
  mainCtx.textAlign = 'center';
  mainCtx.fillStyle = '#94a3b8';

  xLabels.forEach((lbl, i) => {
    const x = pad.left + (i / (xLabels.length - 1)) * cw;
    mainCtx.fillText(lbl, x, h - 6);
  });

  // Flat line at y=0 (bottom of chart area)
  const flatY = pad.top + ch;

  // Shaded area (gradient below flat line — minimal)
  const grad = mainCtx.createLinearGradient(0, flatY, 0, pad.top + ch);
  grad.addColorStop(0, 'rgba(59,130,246,0.12)');
  grad.addColorStop(1, 'rgba(59,130,246,0)');
  mainCtx.beginPath();
  mainCtx.moveTo(pad.left, flatY);
  mainCtx.lineTo(pad.left + cw, flatY);
  mainCtx.lineTo(pad.left + cw, flatY);
  mainCtx.lineTo(pad.left, flatY);
  mainCtx.closePath();
  mainCtx.fillStyle = grad;
  mainCtx.fill();

  // Flat line
  mainCtx.beginPath();
  mainCtx.moveTo(pad.left, flatY);
  mainCtx.lineTo(pad.left + cw, flatY);
  mainCtx.strokeStyle = '#3b82f6';
  mainCtx.lineWidth = 2.5;
  mainCtx.lineJoin = 'round';
  mainCtx.stroke();

  // BPM label
  mainCtx.fillStyle = '#64748b';
  mainCtx.font = '11px DM Sans';
  mainCtx.textAlign = 'left';
  mainCtx.fillText('BPM', 4, pad.top - 6);
}

window.addEventListener('resize', resizeCanvas);
resizeCanvas();


// ── TAB SWITCHING ────────────────────────────────────────────

document.querySelectorAll('.tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    tab.classList.add('active');
  });
});
