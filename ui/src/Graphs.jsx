import { MOD_SOURCES, MOD_DESTINATIONS } from './params'

// Small SVG visualizations that mirror what each panel's knobs are doing,
// so the parameters aren't just numbers -- you can see the envelope shape,
// filter response, and LFO waveform update live as knobs are turned.

const VIEW_W = 280
const VIEW_H = 84

// Compresses a wide time range (ms..seconds) into a readable visual width;
// linear time would make fast attacks invisible next to long releases.
function timeToVisualWidth(seconds) {
  return Math.sqrt(Math.max(seconds, 0.0001))
}

export function AdsrGraph({ attack, decay, sustain, release }) {
  const aVis = timeToVisualWidth(attack)
  const dVis = timeToVisualWidth(decay)
  const rVis = timeToVisualWidth(release)
  const totalVis = aVis + dVis + rVis || 1

  const sustainHoldW = VIEW_W * 0.22
  const envelopeW = VIEW_W - sustainHoldW
  const aW = (aVis / totalVis) * envelopeW
  const dW = (dVis / totalVis) * envelopeW
  const rW = (rVis / totalVis) * envelopeW

  const top = 6
  const bottom = VIEW_H - 6
  const sustainY = bottom - sustain * (bottom - top)

  const x0 = 0
  const x1 = aW
  const x2 = aW + dW
  const x3 = x2 + sustainHoldW
  const x4 = x3 + rW

  const path = `M ${x0} ${bottom} L ${x1} ${top} L ${x2} ${sustainY} L ${x3} ${sustainY} L ${x4} ${bottom}`
  const fillPath = `${path} L ${x4} ${bottom} Z`

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={bottom} x2={VIEW_W} y2={bottom} className="graph-baseline" />
      <path d={fillPath} className="graph-fill" />
      <path d={path} className="graph-line" />
    </svg>
  )
}

export function FilterGraph({ cutoffOffset, resonance }) {
  const baseCutoff = 1200 // approximate mid-velocity base cutoff from Voice.cpp
  const cutoff = Math.min(20000, Math.max(20, baseCutoff + cutoffOffset))
  const minHz = 20
  const maxHz = 20000
  const logMin = Math.log10(minHz)
  const logMax = Math.log10(maxHz)
  const logCutoff = Math.log10(cutoff)

  const resonanceNorm = Math.min(1, Math.max(0, (resonance - 0.5) / (8 - 0.5)))
  const bumpHeight = resonanceNorm * 1.1
  const bumpWidth = 0.12 // in decades, narrower = sharper resonance peak

  const points = []
  const samples = 64
  for (let i = 0; i <= samples; i++) {
    const logF = logMin + (i / samples) * (logMax - logMin)
    const octavesPastCutoff = Math.max(0, logF - logCutoff)
    const rolloff = 1 / Math.sqrt(1 + Math.pow(octavesPastCutoff * 6, 2))
    const bump = bumpHeight * Math.exp(-Math.pow((logF - logCutoff) / bumpWidth, 2))
    const magnitude = Math.min(1.5, rolloff + bump)

    const x = ((logF - logMin) / (logMax - logMin)) * VIEW_W
    const y = VIEW_H - 6 - (magnitude / 1.5) * (VIEW_H - 12)
    points.push([x, y])
  }

  const path = 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  const fillPath = `${path} L ${VIEW_W} ${VIEW_H} L 0 ${VIEW_H} Z`

  const cutoffX = ((logCutoff - logMin) / (logMax - logMin)) * VIEW_W

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={VIEW_H - 6} x2={VIEW_W} y2={VIEW_H - 6} className="graph-baseline" />
      <line x1={cutoffX} y1="0" x2={cutoffX} y2={VIEW_H} className="graph-marker" />
      <path d={fillPath} className="graph-fill" />
      <path d={path} className="graph-line" />
    </svg>
  )
}

function BipolarBar({ label, value, min, max }) {
  const norm = Math.max(-1, Math.min(1, value / Math.max(Math.abs(min), Math.abs(max))))
  const fromCenter = Math.abs(norm) * 50
  const left = norm >= 0 ? 50 : 50 - fromCenter
  return (
    <div className="bipolar-bar">
      <span className="bipolar-label">{label}</span>
      <div className="bipolar-track">
        <div className="bipolar-center" />
        <div className="bipolar-fill" style={{ left: `${left}%`, width: `${fromCenter}%` }} />
      </div>
    </div>
  )
}

function waveformSample(waveform, phase) {
  // phase in [0, 1). Mirrors engine::Oscillator's shapes (non-band-limited
  // here, since this is just a visual preview, not audio).
  switch (waveform) {
    case 0: // Sine
      return Math.sin(2 * Math.PI * phase)
    case 2: // Square
      return phase < 0.5 ? 1 : -1
    case 3: // Triangle
      return phase < 0.5 ? 4 * phase - 1 : 3 - 4 * phase
    case 1: // Saw
    default:
      return 2 * phase - 1
  }
}

export function OscWaveGraph({ waveform, unisonVoices, level }) {
  const amplitude = ((VIEW_H - 16) / 2) * Math.max(0.15, level)
  const midY = VIEW_H / 2
  const cycles = 2

  function buildPath(phaseOffset) {
    const points = []
    const samples = 120
    for (let i = 0; i <= samples; i++) {
      const t = i / samples
      const x = t * VIEW_W
      let phase = (t * cycles + phaseOffset) % 1
      if (phase < 0) phase += 1
      const y = midY - waveformSample(waveform, phase) * amplitude
      points.push([x, y])
    }
    return 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  }

  // Draw a faint extra trace per additional unison voice, slightly phase-
  // shifted, so more unison voices visibly look "thicker" on the graph.
  const extraTraces = Math.min(3, Math.max(0, unisonVoices - 1))

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={midY} x2={VIEW_W} y2={midY} className="graph-baseline" />
      {Array.from({ length: extraTraces }, (_, i) => (
        <path
          key={i}
          d={buildPath((i + 1) * 0.03)}
          className="graph-line graph-line-ghost"
        />
      ))}
      <path d={buildPath(0)} className="graph-line" />
    </svg>
  )
}

// The sub oscillator sits an octave below Osc A/B, so drawn at half the
// visual frequency (1 cycle instead of 2) to suggest that relationship;
// noise is rendered as a jagged random trace layered underneath, both
// scaled by their respective levels.
export function SubNoiseGraph({ subWaveform, subLevel, noiseLevel }) {
  const midY = VIEW_H / 2
  const subAmplitude = ((VIEW_H - 16) / 2) * Math.max(0.1, subLevel)
  const noiseAmplitude = ((VIEW_H - 16) / 2) * Math.max(0, noiseLevel)

  function buildSubPath() {
    const points = []
    const samples = 120
    for (let i = 0; i <= samples; i++) {
      const t = i / samples
      const x = t * VIEW_W
      const y = midY - waveformSample(subWaveform, t) * subAmplitude
      points.push([x, y])
    }
    return 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  }

  function buildNoisePath() {
    const points = []
    const samples = 60
    // Deterministic pseudo-random so the graph doesn't jitter every render.
    let seed = 42
    const rand = () => {
      seed = (seed * 1103515245 + 12345) & 0x7fffffff
      return (seed / 0x7fffffff) * 2 - 1
    }
    for (let i = 0; i <= samples; i++) {
      const x = (i / samples) * VIEW_W
      const y = midY - rand() * noiseAmplitude
      points.push([x, y])
    }
    return 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  }

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={midY} x2={VIEW_W} y2={midY} className="graph-baseline" />
      {noiseLevel > 0 && <path d={buildNoisePath()} className="graph-line graph-line-ghost" />}
      <path d={buildSubPath()} className="graph-line" />
    </svg>
  )
}

// Draws the distortion's tanh() transfer curve (input -> output), so the
// user can see the waveshaping curve get harder as drive increases.
export function DistortionGraph({ drive, mix }) {
  const midY = VIEW_H / 2
  const midX = VIEW_W / 2
  const amplitude = (VIEW_H - 16) / 2
  const halfWidth = (VIEW_W - 16) / 2
  const safeDrive = Math.max(0.01, drive)

  function buildPath() {
    const points = []
    const samples = 100
    for (let i = 0; i <= samples; i++) {
      const x = (i / samples) * 2 - 1 // -1..1
      const driven = Math.tanh(x * safeDrive) / Math.tanh(safeDrive)
      const shaped = x * (1 - mix) + driven * mix
      points.push([midX + x * halfWidth, midY - shaped * amplitude])
    }
    return 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  }

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={midY} x2={VIEW_W} y2={midY} className="graph-baseline" />
      <line x1={midX} y1="0" x2={midX} y2={VIEW_H} className="graph-baseline" />
      <path d={buildPath()} className="graph-line" />
    </svg>
  )
}

// Draws a decaying series of echo "taps" as vertical bars, spaced by the
// delay time and shrinking by the feedback amount each repeat -- a quick
// visual of how many audible echoes you'll get and how fast they decay.
export function DelayGraph({ timeMs, feedback, mix }) {
  const baseY = VIEW_H - 8
  const maxHeight = VIEW_H - 16
  const tapWidth = 6
  const spacingPx = Math.max(8, (timeMs / 2000) * (VIEW_W - 16))
  const taps = []
  let amplitude = mix
  let x = 8
  let i = 0
  while (x < VIEW_W - tapWidth && i < 12 && amplitude > 0.02) {
    taps.push({ x, height: amplitude * maxHeight })
    amplitude *= feedback
    x += spacingPx
    i++
  }

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={baseY} x2={VIEW_W} y2={baseY} className="graph-baseline" />
      {taps.map((tap, idx) => (
        <rect
          key={idx}
          x={tap.x}
          y={baseY - tap.height}
          width={tapWidth}
          height={tap.height}
          className="graph-tap"
        />
      ))}
    </svg>
  )
}

// Draws an exponential glide curve from one note to another over the
// configured glide time, illustrating how fast pitch slides in mono mode
// (flat line at 0s = instant retune, no glide).
export function VoicingGraph({ mode, glideSeconds }) {
  const midY = VIEW_H / 2
  const topY = 8
  const bottomY = VIEW_H - 8

  function buildPath() {
    const points = []
    const samples = 80
    const totalTime = Math.max(0.05, glideSeconds * 1.4)
    const tau = Math.max(0.001, glideSeconds)
    for (let i = 0; i <= samples; i++) {
      const t = (i / samples) * totalTime
      const x = (i / samples) * VIEW_W
      const progress = glideSeconds <= 0 ? 1 : 1 - Math.exp(-t / tau)
      const y = bottomY - progress * (bottomY - topY)
      points.push([x, y])
    }
    return 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')
  }

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      <line x1="0" y1={midY} x2={VIEW_W} y2={midY} className="graph-baseline" />
      <path d={buildPath()} className="graph-line" />
      {mode === 0 && <text x="6" y={VIEW_H - 4} className="graph-label">POLY</text>}
      {mode === 1 && <text x="6" y={VIEW_H - 4} className="graph-label">MONO</text>}
    </svg>
  )
}

// Draws each of the 2 mod-matrix slots as a labeled source -> destination
// row, with a horizontal bar showing the routing amount/depth and its
// polarity (bar grows right for positive, left for negative, from center).
export function ModMatrixGraph({ slot1Source, slot1Dest, slot1Amount, slot2Source, slot2Dest, slot2Amount }) {
  const label = (list, value) => list.find((o) => o.value === value)?.label ?? '?'
  const rowH = VIEW_H / 2
  const barMidX = VIEW_W * 0.62
  const barHalfWidth = VIEW_W * 0.3

  function row(y, source, dest, amount) {
    const barWidth = Math.abs(amount) * barHalfWidth
    const barX = amount >= 0 ? barMidX : barMidX - barWidth
    return (
      <g key={y}>
        <text x="6" y={y + 4} className="graph-label mod-row-label">
          {label(MOD_SOURCES, source)} &rarr; {label(MOD_DESTINATIONS, dest)}
        </text>
        <line x1={barMidX} y1={y + 8} x2={barMidX} y2={y + 8} className="graph-baseline" />
        <rect x={barX} y={y + 6} width={Math.max(1, barWidth)} height="6" className="graph-tap" />
      </g>
    )
  }

  return (
    <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
      {row(rowH * 0.4, slot1Source, slot1Dest, slot1Amount)}
      {row(rowH * 1.4, slot2Source, slot2Dest, slot2Amount)}
    </svg>
  )
}

export function LfoGraph({ rate, toCutoff, toPitch }) {
  const cycles = Math.min(10, Math.max(1, rate / 2))
  const amplitude = (VIEW_H - 16) / 2
  const midY = VIEW_H / 2

  const points = []
  const samples = 100
  for (let i = 0; i <= samples; i++) {
    const x = (i / samples) * VIEW_W
    const phase = (i / samples) * cycles * 2 * Math.PI
    const y = midY - Math.sin(phase) * amplitude
    points.push([x, y])
  }
  const path = 'M ' + points.map(([x, y]) => `${x.toFixed(1)} ${y.toFixed(1)}`).join(' L ')

  return (
    <div className="lfo-graph-wrap">
      <svg className="param-graph" viewBox={`0 0 ${VIEW_W} ${VIEW_H}`} preserveAspectRatio="none">
        <line x1="0" y1={midY} x2={VIEW_W} y2={midY} className="graph-baseline" />
        <path d={path} className="graph-line" />
      </svg>
      <div className="bipolar-row">
        <BipolarBar label="→ Cutoff" value={toCutoff} min={-1000} max={1000} />
        <BipolarBar label="→ Pitch" value={toPitch} min={-2} max={2} />
      </div>
    </div>
  )
}
