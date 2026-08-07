import { useRef, useState } from 'react'
import { PARAMS } from './params'
import { useSynthSocket } from './useSynthSocket'
import { AdsrGraph, FilterGraph, LfoGraph, OscWaveGraph, SubNoiseGraph, DistortionGraph, DelayGraph, VoicingGraph, ModMatrixGraph } from './Graphs'
import { Keyboard } from './Keyboard'
import { Oscilloscope } from './Oscilloscope'
import './App.css'

// Sweep range for the knob indicator, matching typical hardware synth knobs
// (270 degrees, centered, leaving a 90 degree gap at the bottom).
const SWEEP_DEG = 270
const START_DEG = -135
const DRAG_PIXELS_FOR_FULL_SWEEP = 160

function Knob({ control, value, onChange }) {
  const dragState = useRef(null)
  const range = control.max - control.min
  const t = range === 0 ? 0 : Math.min(1, Math.max(0, (value - control.min) / range))
  const angle = START_DEG + t * SWEEP_DEG
  const decimals = control.step < 0.01 ? 3 : control.step < 1 ? 2 : 0

  function clampToStep(raw) {
    const stepped = Math.round(raw / control.step) * control.step
    return Math.min(control.max, Math.max(control.min, stepped))
  }

  function handlePointerDown(e) {
    e.preventDefault()
    e.currentTarget.setPointerCapture(e.pointerId)
    dragState.current = { startY: e.clientY, startValue: value }
  }

  function handlePointerMove(e) {
    if (!dragState.current) return
    const deltaY = dragState.current.startY - e.clientY
    const deltaValue = (deltaY / DRAG_PIXELS_FOR_FULL_SWEEP) * range
    onChange(clampToStep(dragState.current.startValue + deltaValue))
  }

  function handlePointerUp(e) {
    dragState.current = null
    e.currentTarget.releasePointerCapture?.(e.pointerId)
  }

  function handleWheel(e) {
    e.preventDefault()
    const direction = e.deltaY > 0 ? -1 : 1
    onChange(clampToStep(value + direction * control.step))
  }

  return (
    <div className="knob">
      <div
        className="knob-dial-wrap"
        role="slider"
        tabIndex={0}
        aria-label={control.label}
        aria-valuemin={control.min}
        aria-valuemax={control.max}
        aria-valuenow={value}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onWheel={handleWheel}
        onDoubleClick={() => onChange(control.default)}
      >
        <div className="knob-ring" style={{ '--fill-deg': `${t * SWEEP_DEG}deg` }} />
        <div className="knob-dial">
          <div className="knob-pointer" style={{ transform: `rotate(${angle}deg)` }} />
        </div>
      </div>
      <div className="knob-readout">
        {value.toFixed(decimals)}
        {control.unit}
      </div>
      <div className="knob-caption">
        <span className="knob-label">{control.label}</span>
      </div>
    </div>
  )
}

// A row of pill-style buttons for choosing a discrete waveform, in place of
// a knob -- turning a continuous dial to pick between 4 fixed shapes reads
// poorly, whereas clear labeled options match how real synths do it.
function WaveformSelect({ control, value, onChange }) {
  return (
    <div className="wave-select">
      <span className="knob-label">{control.label}</span>
      <div className="wave-select-options">
        {control.options.map((opt) => (
          <button
            key={opt.value}
            type="button"
            className={`wave-select-btn${value === opt.value ? ' active' : ''}`}
            onClick={() => onChange(opt.value)}
          >
            {opt.label}
          </button>
        ))}
      </div>
    </div>
  )
}

// A compact native <select> dropdown, used where a control has many options
// (e.g. mod matrix source/destination) and pill buttons would wrap onto
// several lines and blow out the panel's height.
function DropdownSelect({ control, value, onChange }) {
  return (
    <div className="dropdown-select">
      <span className="knob-label">{control.label}</span>
      <select
        className="dropdown-select-input"
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
      >
        {control.options.map((opt) => (
          <option key={opt.value} value={opt.value}>
            {opt.label}
          </option>
        ))}
      </select>
    </div>
  )
}

export default function App() {
  const { connected, sendParam, sendNoteOn, sendNoteOff, onWaveform, onNoteEvent } = useSynthSocket()
  const [values, setValues] = useState(() => {
    const initial = {}
    for (const section of PARAMS) {
      for (const control of section.controls) initial[control.id] = control.default
    }
    return initial
  })

  function handleChange(id, value) {
    setValues((prev) => ({ ...prev, [id]: value }))
    sendParam(id, value)
  }

  // Each panel gets a small live graph reflecting its own knob values, keyed
  // off the section title from params.js.
  function renderVisual(sectionTitle) {
    switch (sectionTitle) {
      case 'Voicing':
        return <VoicingGraph mode={values[60]} glideSeconds={values[61]} />
      case 'Oscillator A':
        return <OscWaveGraph waveform={values[24]} unisonVoices={values[21]} level={values[23]} />
      case 'Oscillator B':
        return <OscWaveGraph waveform={values[34]} unisonVoices={values[31]} level={values[33]} />
      case 'Sub & Noise':
        return <SubNoiseGraph subWaveform={values[41]} subLevel={values[40]} noiseLevel={values[42]} />
      case 'Distortion':
        return <DistortionGraph drive={values[50]} mix={values[51]} />
      case 'Delay':
        return <DelayGraph timeMs={values[52]} feedback={values[53]} mix={values[54]} />
      case 'Filter':
        return <FilterGraph cutoffOffset={values[0]} resonance={values[1]} />
      case 'Amp envelope':
        return <AdsrGraph attack={values[2]} decay={values[3]} sustain={values[4]} release={values[5]} />
      case 'Filter envelope':
        return <AdsrGraph attack={values[6]} decay={values[7]} sustain={values[8]} release={values[9]} />
      case 'Mod matrix':
        return (
          <ModMatrixGraph
            slot1Source={values[70]} slot1Dest={values[71]} slot1Amount={values[72]}
            slot2Source={values[80]} slot2Dest={values[81]} slot2Amount={values[82]}
          />
        )
      case 'LFO':
        return <LfoGraph rate={values[10]} toCutoff={values[11]} toPitch={values[12]} />
      default:
        return null
    }
  }

  return (
    <div className="app">
      <div className="chassis">
        <header>
          <div className="brand">
            <span className="brand-mark">PS</span>
            <div className="brand-text">
              <h1>Pi Synth</h1>
              <span className="brand-sub">v1 &middot; polysynth engine</span>
            </div>
          </div>
          <div className={`status ${connected ? 'connected' : 'disconnected'}`}>
            <span className="status-led" />
            {connected ? 'Online' : 'Offline'}
          </div>
        </header>

        <Oscilloscope onWaveform={onWaveform} connected={connected} />

        <div className="panels-row">
          {PARAMS.map((section) => (
            <section className="panel" key={section.section}>
              <h2>{section.section}</h2>
              <div className="panel-visual">{renderVisual(section.section)}</div>
              <div className={`knob-grid${section.section === 'Mod matrix' ? ' mod-matrix-grid' : ''}`}>
                {section.controls.map((control) => {
                  if (control.type === 'select') {
                    return (
                      <WaveformSelect
                        key={control.id}
                        control={control}
                        value={values[control.id]}
                        onChange={(v) => handleChange(control.id, v)}
                      />
                    )
                  }
                  if (control.type === 'dropdown') {
                    return (
                      <DropdownSelect
                        key={control.id}
                        control={control}
                        value={values[control.id]}
                        onChange={(v) => handleChange(control.id, v)}
                      />
                    )
                  }
                  return (
                    <Knob
                      key={control.id}
                      control={control}
                      value={values[control.id]}
                      onChange={(v) => handleChange(control.id, v)}
                    />
                  )
                })}
              </div>
            </section>
          ))}
        </div>

        <Keyboard onNoteOn={sendNoteOn} onNoteOff={sendNoteOff} onNoteEvent={onNoteEvent} />
      </div>
    </div>
  )
}
